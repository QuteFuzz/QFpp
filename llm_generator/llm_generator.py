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
                reasoning_effort="high"
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

### START OF MAIN LOGIC ###

# Get the directory where this script is located
script_dir = os.path.dirname(os.path.abspath(__file__))
# Go up one level to get the project root
project_root = os.path.dirname(script_dir)
# Define the base directory for saved circuits
saved_circuits_dir = os.path.join(project_root, "local_saved_circuits")

models = ["deepseek/deepseek-reasoner"] #"gemini/gemini-2.5-flash", "openai/gpt-5.1"]

# Clear all previous saved programs
for model in models:
    model_dir = os.path.join(saved_circuits_dir, model.replace('/', '_'))
    clear_directory(os.path.join(model_dir, "generated"))
    clear_directory(os.path.join(model_dir, "generated_fixed"))
    clear_directory(os.path.join(model_dir, "mutated"))
    clear_directory(os.path.join(model_dir, "mutated_fixed"))

# Loop to generate programs, comprised of half freshly generated ones and half mutated ones. All non-working
# programs will have to be put through the fixing generation flow once to attempt to fix them.
cycles = 1

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

    pbar = tqdm(range(cycles), desc="Starting Generation", unit="cycle")
    for i in pbar:
        pbar.set_description(f"Cycle {i+1}/{cycles}: Generating Program")
        log(f"\n\n=== PROGRAM GENERATION CYCLE {i+1}/{cycles} ===")
        
        generation_prompt = get_dynamic_prompt("./generation_prompt.txt")
        generated_program = ask_any_model(model, generation_prompt)
        log("Program Generated Successfully")

        pbar.set_description(f"Cycle {i+1}/{cycles}: Running Generated Program")
        error_message = run_generated_program(generated_program)

        ########### Mutation and Fixing Logic ###########
        if error_message.strip() == "":
            log("Generated code ran successfully with no errors.")
            save_text_to_file(generated_program, os.path.join(saved_circuits_dir, model_safe_name, f"generated/output{i+1}.py"))

            pbar.set_description(f"Cycle {i+1}/{cycles}: Mutating Program")
            mutation_prompt = get_dynamic_prompt("mutate_prompt_template.txt", input_code=generated_program)
            mutated_code = ask_any_model(model, mutation_prompt)
            log("Running mutated code...")
            pbar.set_description(f"Cycle {i+1}/{cycles}: Running Mutated Program")
            mutated_error = run_generated_program(mutated_code)

            if mutated_error.strip() == "":
                log("Mutated code ran successfully with no errors.")
                save_text_to_file(mutated_code, os.path.join(saved_circuits_dir, model_safe_name, f"mutated/output{i+1}.py"))
            else:
                log("Mutated Code Had An Error")
                pbar.set_description(f"Cycle {i+1}/{cycles}: Fixing Mutated Program")
                fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=mutated_code, error_message=mutated_error)
                fixed_code = ask_any_model(model, fixing_prompt)
                save_text_to_file(fixed_code, os.path.join(saved_circuits_dir, model_safe_name, f"mutated/output{i+1}.py"))

        else:
            log("Generated code Had An Error, attempting fix...")
            pbar.set_description(f"Cycle {i+1}/{cycles}: Fixing Generated Program")
            fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=generated_program, error_message=error_message)
            fixed_code = ask_any_model(model, fixing_prompt)
            save_text_to_file(fixed_code, os.path.join(saved_circuits_dir, model_safe_name, f"generated_fixed/output{i+1}.py"))

            log("Mutating fixed generated code...")
            pbar.set_description(f"Cycle {i+1}/{cycles}: Mutating Fixed Program")
            mutation_prompt = get_dynamic_prompt("mutate_prompt_template.txt", input_code=fixed_code)
            mutated_code = ask_any_model(model, mutation_prompt)
            log("Running mutated fixed generated code...")
            pbar.set_description(f"Cycle {i+1}/{cycles}: Running Mutated Fixed Program")
            mutated_error = run_generated_program(mutated_code)

            if mutated_error.strip() == "":
                log("Mutated fixed generated code ran successfully with no errors.")
                save_text_to_file(mutated_code, os.path.join(saved_circuits_dir, model_safe_name, f"mutated/output{i+1}.py"))
            else:
                log("Mutated fixed generated code Had An Error, attempting fix...")
                pbar.set_description(f"Cycle {i+1}/{cycles}: Fixing Mutated Fixed Program")
                fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=mutated_code, error_message=mutated_error)
                fixed_code = ask_any_model(model, fixing_prompt)
                save_text_to_file(fixed_code, os.path.join(saved_circuits_dir, model_safe_name, f"mutated_fixed/output{i+1}.py"))


    # Now run the saved programs and log their outputs in local_saved_circuits/ as a text file
    for dir_name in ["generated", "generated_fixed", "mutated", "mutated_fixed"]:
        # Create or overwrite the log file
        log_dir = os.path.join(saved_circuits_dir, model_safe_name, dir_name)
        os.makedirs(log_dir, exist_ok=True)
        logfile_path = os.path.join(log_dir, "execution_log.txt")
        
        with open(logfile_path, "w") as logfile:
            logfile.write(f"Execution Log for programs in {dir_name}\n\n")
            saved_programs_path = log_dir
            for filename in os.listdir(saved_programs_path):
                logfile.write(f"--- Running {filename} ---\n")
                if filename.endswith(".py"):
                    file_path = os.path.join(saved_programs_path, filename)
                    with open(file_path, "r") as f:
                        program_code = f.read()
                    run_error = run_generated_program(program_code)
                    if run_error.strip() == "":
                        logfile.write(f"{filename} ran successfully with no errors.\n\n")
                    else:
                        logfile.write(f"{filename} Error Message:\n{run_error}\n\n")