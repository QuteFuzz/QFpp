from litellm import completion
import dotenv
import os
from jinja2 import Template
import subprocess
import tempfile
import sys
import time
import shutil


# Setup your keys (You can also set these in your OS environment variables)
os.environ["GEMINI_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "GEMINI_API_KEY")
os.environ["OPENAI_API_KEY"] = "YOUR_OPENAI_API_KEY" # Only needed if you switch to OpenAI

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
    print(f"Saved content to {file_path}")

def ask_any_model(model_name, prompt):
    print(f"Sending to {model_name}...")
    
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
                    print(f"Max retries ({max_retries}) reached. Error: {e}")
                    return None
                
                # Wait 60 seconds for per-minute rate limits
                print(f"Rate limit hit (per-minute). Waiting {wait_per_minute} seconds... (Attempt {retry_count}/{max_retries})")
                time.sleep(wait_per_minute)
            else:
                print(f"Error: {e}")
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

# Clear all previous saved programs
clear_directory("../local_saved_circuits/generated_first_try")
clear_directory("../local_saved_circuits/generated_fixed_1_cycle")
clear_directory("../local_saved_circuits/mutated_first_try")
clear_directory("../local_saved_circuits/mutated_fixed_1_cycle")

# Loop to generate programs, comprised of half freshly generated ones and half mutated ones. All non-working
# programs will have to be put through the fixing generation flow once to attempt to fix them.
cycles = 1

for i in range(cycles):
    print(f"\n\n=== PROGRAM GENERATION CYCLE {i+1}/{cycles} ===")
    
    # Get the generation prompt from file
    generation_prompt = get_dynamic_prompt("./generation_prompt.txt")
    generated_program = ask_any_model("gemini/gemini-2.5-flash", generation_prompt)
    print("Program Generated Successfully")

    # Run the generated python program to get the error message
    error_message = run_generated_program(generated_program)

    ########### Mutation and Fixing Logic ###########
    if error_message.strip() == "":
        print("Generated code ran successfully with no errors.")
        # If no error, save the program first, then mutate it
        save_text_to_file(generated_program, f"../local_saved_circuits/generated_first_try/output{i+1}.py")

        mutation_prompt = get_dynamic_prompt("mutate_prompt_template.txt", input_code=generated_program)
        mutated_code = ask_any_model("gemini/gemini-2.5-flash", mutation_prompt)
        print("Running mutated code...")
        mutated_error = run_generated_program(mutated_code)

        if mutated_error.strip() == "":
            print("Mutated code ran successfully with no errors.")
            # Save the mutated code to a file
            save_text_to_file(mutated_code, f"../local_saved_circuits/mutated_first_try/output{i+1}.py")
        else:
            print("Mutated Code Had An Error")
            # Now attempt to fix the mutated code
            fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=mutated_code, error_message=mutated_error)
            fixed_code = ask_any_model("gemini/gemini-2.5-flash", fixing_prompt)
            # Now save the code to a file
            save_text_to_file(fixed_code, f"../local_saved_circuits/mutated_fixed_1_cycle/output{i+1}.py")

    else:
        fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=generated_program, error_message=error_message)
        fixed_code = ask_any_model("gemini/gemini-2.5-flash", fixing_prompt)
        # Now save the code to a file
        save_text_to_file(fixed_code, f"../local_saved_circuits/generated_fixed_1_cycle/output{i+1}.py")

        # Now mutate the fixed code and save it
        mutation_prompt = get_dynamic_prompt("mutate_prompt_template.txt", input_code=fixed_code)
        mutated_code = ask_any_model("gemini/gemini-2.5-flash", mutation_prompt)
        print("Running mutated fixed generated code...")
        mutated_error = run_generated_program(mutated_code)

        if mutated_error.strip() == "":
            print("Mutated fixed generated code ran successfully with no errors.")
            # Save the mutated code to a file
            save_text_to_file(mutated_code, f"../local_saved_circuits/mutated_first_try/output{i+1}.py")
        else:
            print("Mutated fixed generated code Had An Error")
            # Now attempt to fix the mutated code
            fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=mutated_code, error_message=mutated_error)
            fixed_code = ask_any_model("gemini/gemini-2.5-flash", fixing_prompt)
            # Now save the code to a file
            save_text_to_file(fixed_code, f"../local_saved_circuits/mutated_fixed_1_cycle/output{i+1}.py")


# Now run the saved programs and log their outputs in local_saved_circuits/ as a text file
for dir_name in ["generated_first_try", "generated_fixed_1_cycle", "mutated_first_try", "mutated_fixed_1_cycle"]:
    # Create or overwrite the log file
    os.makedirs(f"../local_saved_circuits/{dir_name}", exist_ok=True)
    logfile_path = f"../local_saved_circuits/{dir_name}/execution_log.txt"
    
    with open(logfile_path, "w") as logfile:
        logfile.write(f"Execution Log for programs in {dir_name}\n\n")
        saved_programs_path = f"../local_saved_circuits/{dir_name}/"
        for filename in os.listdir(saved_programs_path):
            logfile.write(f"--- Running {filename} ---\n")
            if filename.endswith(".py"):
                file_path = os.path.join(saved_programs_path, filename)
                with open(file_path, "r") as f:
                    program_code = f.read()
                run_error = run_generated_program(program_code)
                if run_error.strip() == "":
                    logfile.write(f"{filename} ran successfully with no errors.\n")
                else:
                    logfile.write(f"{filename} Error Message:\n{run_error}\n")