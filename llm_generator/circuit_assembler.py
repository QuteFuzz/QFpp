import ast
import sys
import logging
import glob
import random
import os
import subprocess
import argparse
from tqdm import tqdm

# Add project root to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Import from local library
from lib.ast_ops import is_qubit_guppy, get_array_size_guppy, GuppyCircuitRenamer, QiskitMainTransformer

def assemble_qiskit(files, output_path, unique_index=0):
    all_imports = []
    renamed_bodies = []
    main_calls = []

    # Add required import for diff testing
    diff_test_import = ast.ImportFrom(
        module='diff_testing.lib',
        names=[ast.alias(name='qiskitTesting', asname=None)],
        level=0
    )
    all_imports.append(diff_test_import)
    
    # Global tracking for max resources
    global_max_qubits = 1
    global_max_clbits = 1

    # Track seen imports to avoid duplication
    seen_imports = set()

    for i, file_path in enumerate(files):
        try:
            with open(file_path, "r") as f:
                source = f.read()
            
            tree = ast.parse(source)
            prefix = f"c{i}_"
            file_global_funcs = set()
            
            # Transform main function to use shared resources
            transformer = QiskitMainTransformer()
            tree = transformer.visit(tree)
            
            # Update global max requirements
            global_max_qubits = max(global_max_qubits, transformer.max_qubits)
            global_max_clbits = max(global_max_clbits, transformer.max_clbits)
            
            # First pass: collect global function names
            for node in tree.body:
                if isinstance(node, ast.FunctionDef):
                    file_global_funcs.add(node.name)
            
            top_level_nodes = []
            
            # Second pass: process nodes and imports
            for node in tree.body:
                if isinstance(node, (ast.Import, ast.ImportFrom)):
                    code_str = ast.unparse(node)
                    if code_str not in seen_imports:
                        seen_imports.add(code_str)
                        all_imports.append(node)
                    continue
                
                # Verify passed code doesn't have if __name__ == "__main__" blocks that might run
                if isinstance(node, ast.If) and isinstance(node.test, ast.Compare):
                     if isinstance(node.test.left, ast.Name) and node.test.left.id == "__name__":
                         continue

                # Filter out top-level calls to main() since we will invoke it manually with appropriate arguments
                if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
                     if isinstance(node.value.func, ast.Name) and node.value.func.id == "main":
                         continue

                top_level_nodes.append(node)
                
            # Rename
            renamer = GuppyCircuitRenamer(prefix, file_global_funcs)
            new_nodes = []
            for node in top_level_nodes:
                new_node = renamer.visit(node)
                new_nodes.append(new_node)
                
            renamed_bodies.extend(new_nodes)
            
            # Store the call to the renamed main function
            main_calls.append(ast.Expr(
                value=ast.Call(
                    func=ast.Name(id=f"{prefix}main", ctx=ast.Load()),
                    args=[
                        ast.Name(id='qc', ctx=ast.Load()),
                        ast.Name(id='qr', ctx=ast.Load()),
                        ast.Name(id='cr', ctx=ast.Load())
                    ],
                    keywords=[]
                )
            ))
            
        except Exception as e:
            logging.error(f"Error processing {file_path}: {e}")
            continue

    # Construct output module
    new_module = ast.Module(body=[], type_ignores=[])
    
    # Add collected imports
    new_module.body.extend(all_imports)
    
    # Add all renamed bodies
    new_module.body.extend(renamed_bodies)
    
    # Create master main function
    
    # Setup global resources
    # qr = QuantumRegister(global_max_qubits, 'q')
    qr_init = ast.Assign(
        targets=[ast.Name(id='qr', ctx=ast.Store())],
        value=ast.Call(
            func=ast.Name(id='QuantumRegister', ctx=ast.Load()),
            args=[
                ast.Constant(value=global_max_qubits),
                ast.Constant(value='q')
            ],
            keywords=[]
        )
    )
    
    # cr = ClassicalRegister(global_max_clbits, 'c')
    cr_init = ast.Assign(
        targets=[ast.Name(id='cr', ctx=ast.Store())],
        value=ast.Call(
            func=ast.Name(id='ClassicalRegister', ctx=ast.Load()),
            args=[
                ast.Constant(value=global_max_clbits),
                ast.Constant(value='c')
            ],
            keywords=[]
        )
    )
    
    # qc = QuantumCircuit(qr, cr)
    qc_init = ast.Assign(
        targets=[ast.Name(id='qc', ctx=ast.Store())],
        value=ast.Call(
            func=ast.Name(id='QuantumCircuit', ctx=ast.Load()),
            args=[
                ast.Name(id='qr', ctx=ast.Load()),
                ast.Name(id='cr', ctx=ast.Load())
            ],
            keywords=[]
        )
    )

    # Return qc at the end
    return_qc = ast.Return(value=ast.Name(id='qc', ctx=ast.Load()))

    master_body = [qr_init, cr_init, qc_init]
    
    if main_calls:
        master_body.extend(main_calls)
    else:
        master_body.append(ast.Pass())

    master_body.append(return_qc)

    master_main = ast.FunctionDef(
        name='main',
        args=ast.arguments(
            posonlyargs=[], args=[], kwonlyargs=[], kw_defaults=[], defaults=[]
        ),
        body=master_body,
        decorator_list=[]
    )
    
    new_module.body.append(master_main)

    
    # Add __name__ == "__main__" block
    if_main = ast.If(
        test=ast.Compare(
            left=ast.Name(id='__name__', ctx=ast.Load()),
            ops=[ast.Eq()],
            comparators=[ast.Constant(value='__main__')]
        ),
        body=[
            # qt = qiskitTesting()
            ast.Assign(
                targets=[ast.Name(id='qt', ctx=ast.Store())],
                value=ast.Call(
                    func=ast.Name(id='qiskitTesting', ctx=ast.Load()),
                    args=[],
                    keywords=[]
                )
            ),
            # qt.ks_diff_test(main(), unique_index)
            ast.Expr(
                value=ast.Call(
                    func=ast.Attribute(
                        value=ast.Name(id='qt', ctx=ast.Load()),
                        attr='ks_diff_test',
                        ctx=ast.Load()
                    ),
                    args=[
                        ast.Call(func=ast.Name(id='main', ctx=ast.Load()), args=[], keywords=[]),
                        ast.Constant(value=unique_index)
                    ],
                    keywords=[]
                )
            )
        ],
        orelse=[]
    )
    new_module.body.append(if_main)
    
    # Write to file
    try:
        ast.fix_missing_locations(new_module)
        with open(output_path, "w") as f:
            f.write(ast.unparse(new_module))
        return True
    except Exception as e:
        logging.error(f"Error writing output to {output_path}: {e}")
        return False

def assemble_guppy(files, output_path, unique_index=0):
    all_imports = []
    renamed_bodies = []
    main_funcs = []

    # Track seen imports to avoid duplication
    # We store the unparsed string as key
    seen_imports = set(["from diff_testing.lib import guppyTesting"]) # Pre-add our required import

    # Add required import for diff testing
    diff_test_import = ast.ImportFrom(
        module='diff_testing.lib',
        names=[ast.alias(name='guppyTesting', asname=None)],
        level=0
    )
    all_imports.append(diff_test_import)

    for i, file_path in enumerate(files):
        try:
            with open(file_path, "r") as f:
                source = f.read()
            
            tree = ast.parse(source)
            
            prefix = f"c{i}_"
            file_global_funcs = set()
            
            # First pass: collect global function names
            for node in tree.body:
                if isinstance(node, ast.FunctionDef):
                    file_global_funcs.add(node.name)
            
            top_level_nodes = []
            
            # Second pass: process nodes
            for node in tree.body:
                if isinstance(node, (ast.Import, ast.ImportFrom)):
                    code_str = ast.unparse(node)
                    if code_str not in seen_imports:
                        seen_imports.add(code_str)
                        all_imports.append(node)
                    continue
                    
                top_level_nodes.append(node)
                
            # Rename
            renamer = GuppyCircuitRenamer(prefix, file_global_funcs)
            new_nodes = []
            for node in top_level_nodes:
                # Filter out top-level executions like 'main.compile()' 
                # or 'enable_experimental_features()' which we handle globally
                if isinstance(node, ast.Expr):
                    code_str = ast.unparse(node)
                    if "compile" in code_str or "enable_experimental_features" in code_str:
                        continue
                
                new_node = renamer.visit(node)
                new_nodes.append(new_node)
                
            renamed_bodies.extend(new_nodes)
            
            # Find the renamed main function definition to extract arguments
            for node in new_nodes:
                if isinstance(node, ast.FunctionDef) and node.name == prefix + "main":
                    main_funcs.append(node)
                    break
                
        except Exception as e:
            logging.error(f"Error processing {file_path}: {e}")
            continue

    # Construct output module
    new_module = ast.Module(body=[], type_ignores=[])
    
    # Add collected imports
    new_module.body.extend(all_imports)
    
    # Add guppylang.enable_experimental_features() if likely needed
    # (We check if 'guppylang' is in imports)
    has_guppy = any("guppylang" in imp for imp in seen_imports)
    if has_guppy:
        enable_expr = ast.Expr(
            value=ast.Call(
                func=ast.Attribute(
                    value=ast.Name(id='guppylang', ctx=ast.Load()),
                    attr='enable_experimental_features',
                    ctx=ast.Load()
                ),
                args=[],
                keywords=[]
            )
        )
        new_module.body.append(enable_expr)
        
    # Add all renamed bodies
    new_module.body.extend(renamed_bodies)
    
    # Create master main
    main_body = []
    
    if main_funcs:
        for main_node in main_funcs:
            m_name = main_node.name
            call_args = []
            result_stmts = []
            
            for arg in main_node.args.args:
                arg_name = arg.arg
                ann = arg.annotation
                if not ann:
                    continue
                
                # Create unique local variable name
                local_var = f"{m_name}_{arg_name}"
                
                setup_expr = None
                measure_call = None
                
                if is_qubit_guppy(ann):
                    # var = qubit()
                    setup_expr = ast.Call(
                        func=ast.Name(id='qubit', ctx=ast.Load()),
                        args=[], keywords=[]
                    )
                    # measure(var)
                    measure_call = ast.Call(
                        func=ast.Name(id='measure', ctx=ast.Load()),
                        args=[ast.Name(id=local_var, ctx=ast.Load())],
                        keywords=[]
                    )
                    
                else:
                    arr_size = get_array_size_guppy(ann)
                    if arr_size is not None:
                        # var = array(qubit() for _ in range(size))
                        range_call = ast.Call(
                            func=ast.Name(id='range', ctx=ast.Load()),
                            args=[ast.Constant(value=arr_size)],
                            keywords=[]
                        )
                        
                        qubit_call = ast.Call(
                            func=ast.Name(id='qubit', ctx=ast.Load()),
                            args=[], keywords=[]
                        )
                        
                        gen_exp = ast.GeneratorExp(
                            elt=qubit_call,
                            generators=[
                                ast.comprehension(
                                    target=ast.Name(id='_', ctx=ast.Store()),
                                    iter=range_call,
                                    ifs=[],
                                    is_async=0
                                )
                            ]
                        )
                        
                        setup_expr = ast.Call(
                            func=ast.Name(id='array', ctx=ast.Load()),
                            args=[gen_exp],
                            keywords=[]
                        )
                        
                        # measure_array(var)
                        measure_call = ast.Call(
                            func=ast.Name(id='measure_array', ctx=ast.Load()),
                            args=[ast.Name(id=local_var, ctx=ast.Load())],
                            keywords=[]
                        )

                if setup_expr:
                    # Assign setup
                    main_body.append(ast.Assign(
                        targets=[ast.Name(id=local_var, ctx=ast.Store())],
                        value=setup_expr
                    ))
                    
                    call_args.append(ast.Name(id=local_var, ctx=ast.Load()))
                    
                    if measure_call:
                        # result("m_name.arg_name", measure_call)
                        result_key = f"{m_name}.{arg_name}"
                        result_stmts.append(ast.Expr(
                            value=ast.Call(
                                func=ast.Name(id='result', ctx=ast.Load()),
                                args=[
                                    ast.Constant(value=result_key),
                                    measure_call
                                ],
                                keywords=[]
                            )
                        ))

            # Call the main function: m_name(args...)
            main_body.append(
                ast.Expr(
                    value=ast.Call(
                        func=ast.Name(id=m_name, ctx=ast.Load()),
                        args=call_args,
                        keywords=[]
                    )
                )
            )
            
            # Append measurement results AFTER the function call
            main_body.extend(result_stmts)

    else:
        main_body.append(ast.Pass())


    main_def = ast.FunctionDef(
        name='main',
        args=ast.arguments(
            posonlyargs=[], args=[], kwonlyargs=[], kw_defaults=[], defaults=[]
        ),
        body=main_body,
        decorator_list=[ast.Name(id='guppy', ctx=ast.Load())],
        returns=ast.Constant(value=None, kind=None), 
        lineno=0
    )
    
    new_module.body.append(main_def)
    
    # Add diff test execution code
    # gt = guppyTesting()
    # gt.ks_diff_test(main_circuit, unique_index)
    
    gt_assign = ast.Assign(
        targets=[ast.Name(id='gt', ctx=ast.Store())],
        value=ast.Call(
            func=ast.Name(id='guppyTesting', ctx=ast.Load()),
            args=[],
            keywords=[]
        )
    )
    new_module.body.append(gt_assign)
    
    diff_test_call = ast.Expr(
        value=ast.Call(
            func=ast.Attribute(
                value=ast.Name(id='gt', ctx=ast.Load()),
                attr='ks_diff_test',
                ctx=ast.Load()
            ),
            args=[
                ast.Name(id='main', ctx=ast.Load()),
                ast.Constant(value=unique_index)
            ],
            keywords=[]
        )
    )
    new_module.body.append(diff_test_call)
    
    ast.fix_missing_locations(new_module)
    output_code = ast.unparse(new_module)
    
    with open(output_path, "w") as f:
        f.write(output_code)
    # print(f"Assembled {len(files)} files into {output_path}")

def assemble(files, output_path, unique_index=0, language='guppy'):
    if language == 'qiskit':
        return assemble_qiskit(files, output_path, unique_index)
    
    return assemble_guppy(files, output_path, unique_index)

if __name__ == "__main__":

    # Configuration via argparse
    parser = argparse.ArgumentParser(description="Assemble and test quantum circuits.")
    parser.add_argument("input_dir", nargs='?', default="local_saved_circuits/Correct_format/", help="Directory containing input circuit files")
    parser.add_argument("--output-dir", default="local_saved_circuits/Correct_format/combined_circuits", help="Directory to save assembled circuits")
    parser.add_argument("--n-generations", type=int, default=1, help="Number of circuits to generate")
    parser.add_argument("--min-files", type=int, default=2, help="Minimum number of files per assembly")
    parser.add_argument("--max-files", type=int, default=5, help="Maximum number of files per assembly")
    parser.add_argument("--log-file", default="local_saved_circuits/Correct_format/assembler.log", help="Path to log file")
    parser.add_argument("--language", default="guppy", choices=["guppy", "qiskit"], help="Language of circuits (guppy or qiskit)")
    
    args = parser.parse_args()

    INPUT_DIR = args.input_dir
    OUTPUT_DIR = args.output_dir
    N_GENERATIONS = args.n_generations
    MIN_FILES = args.min_files
    MAX_FILES = args.max_files

    # Setup logging to both file and stdout
    # Remove existing handlers to avoid duplication if run multiple times in same session or if configured elsewhere
    for handler in logging.root.handlers[:]:
        logging.root.removeHandler(handler)
        
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(args.log_file),
            logging.StreamHandler(sys.stdout)
        ]
    )

    logging.info(f"Starting assembler with configurations: Input={INPUT_DIR}, Output={OUTPUT_DIR}, N={N_GENERATIONS}, Language={args.language}")

    # Ensure output directory exists
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Get all python files
    input_files = glob.glob(os.path.join(INPUT_DIR, "*.py"))
    
    if not input_files:
        msg = f"No input files found in {INPUT_DIR}"
        logging.error(msg)
        print(msg)
        sys.exit(1)

    logging.info(f"Found {len(input_files)} input files. Starting generation...")
    print(f"Found {len(input_files)} input files. Starting generation...")

    seen_combinations = set()
    generated_count = 0
    assembled_files = []

    # Phase 1: Assembly
    logging.info("Phase 1: Assembling circuits...")
    print("Phase 1: Assembling circuits...")
    
    with tqdm(total=N_GENERATIONS, desc="Assembling") as pbar:
        while generated_count < N_GENERATIONS:
            # Determine size of this combination
            k = random.randint(MIN_FILES, min(MAX_FILES, len(input_files)))
            
            # Sample k files
            selected_files = random.sample(input_files, k)
            
            # Create a representation to check for duplicates
            # Do not sort, as order of assembly matters for circuit execution
            combo_key = tuple(selected_files)
            
            if combo_key in seen_combinations:
                continue
                
            seen_combinations.add(combo_key)
            
            # Output path
            output_file = os.path.join(OUTPUT_DIR, f"assembled_circuit_{generated_count}.py")
            
            try:
                assemble(selected_files, output_file, generated_count, language=args.language)
                assembled_files.append(output_file)
                generated_count += 1
                pbar.update(1)
            except Exception as e:
                logging.error(f"Failed to assemble combination {selected_files}: {e}")

    # Phase 2: Execution
    logging.info("Phase 2: Running circuits...")
    print("Phase 2: Running circuits...")
    
    success_count = 0
    with tqdm(total=len(assembled_files), desc="Running") as pbar:
        for output_file in assembled_files:
            try:
                # Add project root to PYTHONPATH so diff_testing can be imported
                script_dir = os.path.dirname(os.path.abspath(__file__))
                project_root = os.path.dirname(script_dir)
                
                env = os.environ.copy()
                pythonpath = env.get("PYTHONPATH", "")
                if pythonpath:
                    env["PYTHONPATH"] = f"{project_root}{os.pathsep}{pythonpath}"
                else:
                    env["PYTHONPATH"] = project_root
                
                logging.info(f"Running {output_file}...")
                subprocess.run(
                    [sys.executable, output_file], 
                    check=True, 
                    env=env,
                    capture_output=True,
                    text=True
                )
                success_count += 1
                logging.info(f"Successfully ran {output_file}")
                
            except subprocess.CalledProcessError as e:
                logging.error(f"Error executing {output_file}: {e}\nStdout: {e.stdout}\nStderr: {e.stderr}")
            except Exception as e:
                logging.error(f"Unexpected error executing {output_file}: {e}")
            
            pbar.update(1)
    
    logging.info(f"Finished. {success_count}/{len(assembled_files)} assembled circuits ran successfully.")
    print(f"Finished. {success_count}/{len(assembled_files)} assembled circuits ran successfully.")
