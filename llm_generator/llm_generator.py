from litellm import completion
import dotenv
import os
from jinja2 import Template
import subprocess
import tempfile
import sys
import time
import shutil
from tqdm import tqdm


# Setup your keys (You can also set these in your OS environment variables)
os.environ["GEMINI_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "GEMINI_API_KEY")
os.environ["OPENAI_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "OPENAI_API_KEY")
os.environ["DEEPSEEK_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "DEEPSEEK_API_KEY")
os.environ["ANTHROPIC_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "ANTHROPIC_API_KEY")

# For the template, replace any variables given in kgwargs
def get_dynamic_prompt(template_path, **kwargs) -> str:
    with open(template_path, 'r') as f:
        template_str = f.read()
    template = Template(template_str)
    return template.render(**kwargs)

def save_text_to_file(text, file_path):
    """
    Saves the given text content to a file at the specified path.
    Creates directories if they don't exist.
    """
    text = strip_markdown_syntax(text) # Strip markdown syntax before saving

    directory = os.path.dirname(file_path)
    if directory and not os.path.exists(directory):
        os.makedirs(directory)
    
    with open(file_path, "w") as f:
        f.write(text)
    tqdm.write(f"Saved content to {file_path}")

def ask_any_model(model_name, prompt):
    tqdm.write(f"Sending to {model_name}...")
    
    max_retries = 3
    retry_count = 0
    wait_per_minute = 40
    
    while retry_count < max_retries:
        try:
            # distinct "completion" function works for ALL providers
            response = completion(
                model=model_name, 
                messages=[{ "content": prompt, "role": "user" }],
                reasoning_effort="high",
                drop_params=True
            )
            
            answer = response['choices'][0]['message']['content']
            return answer
            
        except Exception as e:
            error_str = str(e)
            
            # Check for rate limit or quota exceeded errors
            if "rate_limit" in error_str.lower() or "quota" in error_str.lower() or "429" in error_str or "503" in error_str:
                retry_count += 1
                if retry_count >= max_retries:
                    tqdm.write(f"Max retries ({max_retries}) reached. Error: {e}")
                    return None
                
                # Wait 60 seconds for per-minute rate limits
                tqdm.write(f"Rate limit hit (per-minute). Waiting {wait_per_minute} seconds... (Attempt {retry_count}/{max_retries})")
                time.sleep(wait_per_minute)
            else:
                tqdm.write(f"Error: {e}")
                return None

def strip_markdown_syntax(code: str) -> str:
    """
    Remove markdown code block syntax from generated code.
    Strips leading ```python or ``` and trailing ```.
    
    Args:
        code: The code string potentially wrapped in markdown syntax
        
    Returns:
        Clean code without markdown syntax
    """
    code = code.strip()
    
    # Remove leading markdown syntax (```python, ```py, ``` etc.)
    if code.startswith("```"):
        # Remove the line with opening ```
        lines = code.split('\n')
        lines = lines[1:]  # Skip first line with ```
        code = '\n'.join(lines)
    
    # Remove trailing markdown syntax (```)
    if code.endswith("```"):
        lines = code.split('\n')
        lines = lines[:-1]  # Skip last line with ```
        code = '\n'.join(lines)
    
    return code.strip()

def run_generated_program(program_code: str) -> str:
    """
    Execute generated Python program and capture error message.
    
    Args:
        program_code: The Python code to execute
        
    Returns:
        Error message string (stdout + stderr combined), or empty string if execution succeeds
    """
    try:
        # Create a temporary file to store the generated program
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False, dir='/tmp') as temp_file:
            # Strip markdown syntax before writing
            clean_code = strip_markdown_syntax(program_code)
            temp_file.write(clean_code)
            temp_file_path = temp_file.name
        
        try:
            # Execute the generated program with timeout
            result = subprocess.run(
                [sys.executable, temp_file_path],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            # Return error (if any)
            return result.stderr if result.stderr else ""
            
        finally:
            # Clean up temporary file
            if os.path.exists(temp_file_path):
                os.remove(temp_file_path)
                
    except subprocess.TimeoutExpired:
        return "ERROR: Program execution timed out after 30 seconds"
    except Exception as e:
        return f"ERROR: Failed to execute program: {str(e)}"

def clear_directory(path):
    if os.path.exists(path):
        shutil.rmtree(path)
    os.makedirs(path, exist_ok=True)

def program_gen_loop():
    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Go up one level to get the project root
    project_root = os.path.dirname(script_dir)
    # Define the base directory for saved circuits
    saved_circuits_dir = os.path.join(project_root, "local_saved_circuits")

    models = ["anthropic/claude-sonnet-4-5"] # , "openai/gpt-5.1", "deepseek/deepseek-reasoner", "gemini/gemini-3-pro-preview" 

    # Clear all previous saved programs
    for model in models:
        model_dir = os.path.join(saved_circuits_dir, model.replace('/', '_'))
        clear_directory(os.path.join(model_dir, "generated"))
        clear_directory(os.path.join(model_dir, "generated_fixed"))
    # Loop to generate programs, comprised of half freshly generated ones and half mutated ones. All non-working
    # programs will have to be put through the fixing generation flow once to attempt to fix them.
    n_programs = 20
    n_max_fixing_cycles = 2

    # Cycle through different LLMs
    for model in models:
        # Setup logging
        model_safe_name = model.replace('/', '_')
        log_file_path = os.path.join(saved_circuits_dir, model_safe_name, "generation.log")
        os.makedirs(os.path.dirname(log_file_path), exist_ok=True)
        
        def log(msg):
            tqdm.write(msg)
            with open(log_file_path, "a") as f:
                f.write(msg + "\n")

        # First run and save n_programs generated programs
        Generation_pbar = tqdm(range(n_programs), desc="Starting Generation", unit="Programs")
        for i in Generation_pbar:
            Generation_pbar.set_description(f"Generating {i+1}/{n_programs} programs with {model}")

            generation_prompt = get_dynamic_prompt("./generation_prompt.txt")
            generated_program = ask_any_model(model, generation_prompt)
            save_text_to_file(generated_program, os.path.join(saved_circuits_dir, model_safe_name, f"generated/output{i+1}.py"))

        # Then run and fix each generated program
        Fixing_pbar = tqdm(range(n_programs), desc="Starting Fixing", unit="Programs")

        # Create or overwrite the log file for execution results
        log_dir = os.path.join(saved_circuits_dir, model_safe_name, "generated")
        os.makedirs(log_dir, exist_ok=True)
        logfile_path = os.path.join(log_dir, "execution_log.txt")

        with open(logfile_path, "w") as logfile:
            logfile.write(f"Execution Log for programs in generated\n\n")
            
            for i in Fixing_pbar:
                Fixing_pbar.set_description(f"Fixing {i+1}/{n_programs} programs with {model}")
                filename = f"output{i+1}.py"
                file_path = os.path.join(log_dir, filename)
                fixed_file_path = os.path.join(saved_circuits_dir, model_safe_name, f"generated_fixed/{filename}")
                
                if not os.path.exists(file_path):
                    tqdm.write(f"Warning: {filename} not found.")
                    continue

                logfile.write(f"--- Running {filename} ---\n")
                
                with open(file_path, "r") as f:
                    program_code = f.read()
                
                run_error = run_generated_program(program_code)
                
                if run_error.strip() == "":
                    logfile.write(f"{filename} ran successfully with no errors.\n\n")
                else:
                    logfile.write(f"{filename} Error Message:\n{run_error}\n\n")
                    
                    current_code = program_code
                    current_error = run_error
                    
                    for cycle in range(n_max_fixing_cycles):
                        tqdm.write(f"Fixing cycle {cycle+1}/{n_max_fixing_cycles} for {filename}...")
                        
                        fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=current_code, error_message=current_error)
                        fixed_code = ask_any_model(model, fixing_prompt)
                        
                        # Save the fixed code (overwriting previous fix)
                        save_text_to_file(fixed_code, fixed_file_path)
                        
                        # Run the fixed code
                        logfile.write(f"--- Running fixed {filename} (Cycle {cycle+1}) ---\n")
                        fixed_run_error = run_generated_program(fixed_code)
                        
                        if fixed_run_error.strip() == "":
                            logfile.write(f"Fixed {filename} (Cycle {cycle+1}) ran successfully with no errors.\n\n")
                            break # Success, stop fixing
                        else:
                            logfile.write(f"Fixed {filename} (Cycle {cycle+1}) Error Message:\n{fixed_run_error}\n\n")
                            # Update for next cycle
                            current_code = fixed_code
                            current_error = fixed_run_error
                    else:
                        logfile.write(f"Failed to fix {filename} after {n_max_fixing_cycles} cycles.\n\n")


if __name__ == "__main__":
    program_gen_loop()
