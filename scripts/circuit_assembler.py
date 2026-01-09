import ast
import sys

class CircuitRenamer(ast.NodeTransformer):
    def __init__(self, prefix, global_funcs):
        self.prefix = prefix
        self.global_funcs = global_funcs
        self.local_scopes = []

    def visit_FunctionDef(self, node):
        # We need to rename the name of the function definition if it is global
        if node.name in self.global_funcs:
            node.name = self.prefix + node.name
            
        # Enter new scope
        self.local_scopes.append(set())
        
        # Visit children (args, body, decorators)
        self.generic_visit(node)
        
        # Exit scope
        self.local_scopes.pop()
        return node

    def visit_arg(self, node):
        if self.local_scopes:
            self.local_scopes[-1].add(node.arg)
        return node

    def visit_Name(self, node):
        # If we are in a scope and the name is defined locally, don't rename.
        is_local = False
        if self.local_scopes:
            for scope in reversed(self.local_scopes):
                if node.id in scope:
                    is_local = True
                    break
        
        if not is_local and node.id in self.global_funcs:
            node.id = self.prefix + node.id
            
        # If it's a store (assignment), and we are in a local scope, add to local scope
        if isinstance(node.ctx, ast.Store) and self.local_scopes:
            self.local_scopes[-1].add(node.id)
            
        return node

def assemble(files, output_path, unique_index=0):
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
            renamer = CircuitRenamer(prefix, file_global_funcs)
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
            
            if "main" in file_global_funcs:
                main_funcs.append(prefix + "main")
                
        except Exception as e:
            print(f"Error processing {file_path}: {e}")
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
        for m_name in main_funcs:
            main_body.append(
                ast.Expr(
                    value=ast.Call(
                        func=ast.Name(id=m_name, ctx=ast.Load()),
                        args=[],
                        keywords=[]
                    )
                )
            )
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
    print(f"Assembled {len(files)} files into {output_path}")

if __name__ == "__main__":
    import glob
    import random
    import os
    import subprocess

    # Configuration
    INPUT_DIR = "local_saved_circuits/gemini_gemini-3-pro-preview/generated"
    OUTPUT_DIR = "local_saved_circuits/combined_circuits"
    N_GENERATIONS = 10
    MIN_FILES = 2
    MAX_FILES = 5

    # Ensure output directory exists
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Get all python files
    input_files = glob.glob(os.path.join(INPUT_DIR, "output*.py"))
    
    if not input_files:
        print(f"No input files found in {INPUT_DIR}")
        sys.exit(1)

    print(f"Found {len(input_files)} input files. Starting generation...")

    seen_combinations = set()
    generated_count = 0

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
        
        print(f"[{generated_count+1}/{N_GENERATIONS}] Assembling {k} files into {output_file}...")
        assemble(selected_files, output_file, generated_count)
        
        # Execute the generated script
        print(f"Running {output_file}...")
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
                
            subprocess.run([sys.executable, output_file], check=True, env=env)
            print(f"Successfully executed {output_file}")
        except subprocess.CalledProcessError as e:
            print(f"Error executing {output_file}: {e}")
        except Exception as e:
            print(f"Unexpected error executing {output_file}: {e}")
        
        generated_count += 1

