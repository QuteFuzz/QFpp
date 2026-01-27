import ast
import logging
from .ast_ops import is_qubit_guppy, get_array_size_guppy, GuppyCircuitRenamer, QiskitMainTransformer

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
    
    ast.fix_missing_locations(new_module)
    output_code = ast.unparse(new_module)
    
    with open(output_path, "w") as f:
        f.write(output_code)
    # print(f"Assembled {len(files)} files into {output_path}")

def assemble(files, output_path, unique_index=0, language='guppy'):
    if language == 'qiskit':
        return assemble_qiskit(files, output_path, unique_index)
    
    return assemble_guppy(files, output_path, unique_index)
