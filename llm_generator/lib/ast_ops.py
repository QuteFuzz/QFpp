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

class GuppyCircuitRenamer(ast.NodeTransformer):
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


def _generate_guppy_wrapper_body(code: str) -> tuple[str, str]:
    """
    Helper to generate the wrapper function for a Guppy main function.
    Returns (wrapper_name, wrapper_code_string).
    """
    try:
        tree = ast.parse(code)
    except SyntaxError:
        return None, ""

    main_func = None
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name == 'main':
            main_func = node
            break
            
    if not main_func:
        return None, ""

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

    # If no args, we do not need a wrapper.
    if not call_args and not setup_code:
        return 'main', ""

    wrapper_code = "\n\n@guppy\ndef main_wrapper() -> None:\n"
    if setup_code:
        wrapper_code += "\n".join(setup_code) + "\n"
    
    wrapper_code += f"    main({', '.join(call_args)})\n"
    
    if result_code:
        wrapper_code += "\n".join(result_code) + "\n"
    
    return 'main_wrapper', wrapper_code

def wrap_for_compilation_guppy(code: str) -> str:
    wrapper_name, wrapper_code = _generate_guppy_wrapper_body(code)
    if wrapper_name is None: return code
    
    if wrapper_code:
        return code + wrapper_code + f"\n{wrapper_name}.compile()\n"
    else:
        # If no wrapper needed (e.g. main has no args), just compile main
        return code + f"\n{wrapper_name}.compile()\n"

def wrap_for_testing_guppy(code: str) -> str:
    wrapper_name, wrapper_code = _generate_guppy_wrapper_body(code)
    if wrapper_name is None: return code
    
    import_stmt = "from diff_testing.lib import guppyTesting\n"
    
    # Need to verify if imports already exist in code to avoid dupes? 
    # Python doesn't crash on duplicate imports, so prepending is fine.
    
    test_harness = f"\n\ngt = guppyTesting()\ngt.ks_diff_test({wrapper_name}, 0)\n"
    
    return import_stmt + code + wrapper_code + test_harness


def wrap_for_compilation_qiskit(code: str) -> str:
    """
    Parses the code to find the main function and adds a main block to execute it.
    """
    try:
        tree = ast.parse(code)
    except SyntaxError:
        return code

    has_main = False
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name == 'main':
            has_main = True
            break
            
    if not has_main:
        return code

    # Check if a main block already exists
    has_main_block = False
    for node in tree.body:
         if isinstance(node, ast.If) and isinstance(node.test, ast.Compare):
             if isinstance(node.test.left, ast.Name) and node.test.left.id == "__name__":
                 has_main_block = True
                 break

    if has_main_block:
        return code

    wrapper_code =  "\nif __name__ == '__main__':\n\t main()\n"
    return code + wrapper_code

def wrap_for_testing_qiskit(code: str) -> str:
    # Basic check for main
    try:
        tree = ast.parse(code)
        has_main = False
        for node in tree.body:
            if isinstance(node, ast.FunctionDef) and node.name == 'main':
                has_main = True
                break
        if not has_main: return code
    except: return code

    import_stmt = "from diff_testing.lib import qiskitTesting\n"
    
    # We want to replace existing main block or append new one
    # For simplicity, we just append. If there's an existing `if __name__ == "__main__": main()`,
    # execution might fall through or run twice if we aren't careful.
    # But usually generated code doesn't have the block yet.
    
    wrapper_code = """
if __name__ == '__main__':
    qt = qiskitTesting()
    qt.ks_diff_test(main(), 0)
"""
    return import_stmt + code + wrapper_code


class QiskitMainTransformer(ast.NodeTransformer):
    def __init__(self):
        self.max_qubits = 1
        self.max_clbits = 1
        self.in_main = False

    def visit_FunctionDef(self, node):
        # Only transform the main function
        if node.name == 'main':
            self.in_main = True
            
            # Add args: qc, qr, cr
            new_args = [
                ast.arg(arg='qc', annotation=None),
                ast.arg(arg='qr', annotation=None),
                ast.arg(arg='cr', annotation=None)
            ]
            node.args.args.extend(new_args)
            
            # Process body
            self.generic_visit(node)
            
            self.in_main = False
            return node
        return node

    def visit_Assign(self, node):
        if not self.in_main:
            return node
            
        if isinstance(node.value, ast.Call):
            func_name = self._get_func_name(node.value.func)
            
            # Check for QuantumRegister
            if 'QuantumRegister' in func_name:
                size = self._get_size_from_args(node.value)
                if size > self.max_qubits:
                    self.max_qubits = size
                # Replace with: target = qr
                return ast.Assign(targets=node.targets, value=ast.Name(id='qr', ctx=ast.Load()))
            
            # Check for ClassicalRegister
            if 'ClassicalRegister' in func_name:
                size = self._get_size_from_args(node.value)
                if size > self.max_clbits:
                    self.max_clbits = size
                # Replace with: target = cr
                return ast.Assign(targets=node.targets, value=ast.Name(id='cr', ctx=ast.Load()))
                
            # Check for QuantumCircuit
            if 'QuantumCircuit' in func_name:
                # Replace with: target = qc
                return ast.Assign(targets=node.targets, value=ast.Name(id='qc', ctx=ast.Load()))
        
        return node

    def _get_func_name(self, func_node):
        if isinstance(func_node, ast.Name):
            return func_node.id
        elif isinstance(func_node, ast.Attribute):
            return func_node.attr
        return ""

    def _get_size_from_args(self, call_node):
        if call_node.args:
            arg0 = call_node.args[0]
            if isinstance(arg0, ast.Constant): # python 3.8+
                return arg0.value
            elif isinstance(arg0, ast.Num): # python <3.8
                return arg0.n
        return 1


