import ast
import logging

def is_qubit_guppy(node):
    return isinstance(node, ast.Name) and node.id == 'qubit'

def get_array_size_guppy(node):
    if not isinstance(node, ast.Subscript):
        return None
    if not (isinstance(node.value, ast.Name) and node.value.id == 'array'):
        return None
    
    slice_content = node.slice
    # Handle Python < 3.9 ast.Index wrapper
    if isinstance(slice_content, ast.Index):
            slice_content = slice_content.value
    
    if isinstance(slice_content, ast.Tuple):
        elts = slice_content.elts
        if len(elts) == 2 and is_qubit_guppy(elts[0]):
            size_node = elts[1]
            if isinstance(size_node, ast.Constant):
                return size_node.value
            elif isinstance(size_node, ast.Num):
                return size_node.n
    return None

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

def add_main_wrapper_guppy(code: str) -> str:
    """
    Parses the code to find the main function, creates a wrapper function
    that initializes arguments and calls main, and adds a compilation check.
    """
    try:
        tree = ast.parse(code)
    except SyntaxError:
        return code

    main_func = None
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name == 'main':
            main_func = node
            break
            
    if not main_func:
        return code

    setup_code = []
    result_code = []
    call_args = []
    
    for arg in main_func.args.args:
        arg_name = arg.arg
        ann = arg.annotation
        
        if not ann:
            continue
            
        # We use the module-level helper functions here
        
        if is_qubit_guppy(ann):
            setup_code.append(f"    {arg_name} = qubit()")
            result_code.append(f'    result("{arg_name}", measure({arg_name}))')
            call_args.append(arg_name)
        else:
            arr_size = get_array_size_guppy(ann)
            if arr_size is not None:
                setup_code.append(f"    {arg_name} = array(qubit() for _ in range({arr_size}))")
                result_code.append(f'    result("{arg_name}", measure_array({arg_name}))')
                call_args.append(arg_name)
    
    wrapper_code = "\n\n@guppy\ndef main_wrapper() -> None:\n"
    if setup_code:
        wrapper_code += "\n".join(setup_code) + "\n"
    
    wrapper_code += f"    main({', '.join(call_args)})\n"
    
    if result_code:
        wrapper_code += "\n".join(result_code) + "\n"
    
    wrapper_code += "\nmain_wrapper.compile()\n"
    
    return code + wrapper_code
