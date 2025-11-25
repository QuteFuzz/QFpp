from litellm import completion
import dotenv
import os
from jinja2 import Template
import subprocess
import tempfile
import sys
import time


# Setup your keys (You can also set these in your OS environment variables)
os.environ["GEMINI_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "GEMINI_API_KEY")
os.environ["OPENAI_API_KEY"] = "YOUR_OPENAI_API_KEY" # Only needed if you switch to OpenAI

# For the template, replace any variables given in kgwargs
def get_dynamic_prompt(template_path, **kwargs) -> str:
    with open(template_path, 'r') as f:
        template_str = f.read()
    template = Template(template_str)
    return template.render(**kwargs)

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


generation_prompt = get_dynamic_prompt("./generation_prompt.txt")

# Format: provider/model-name
generated_program = ask_any_model("gemini/gemini-2.5-pro", generation_prompt)
print("Program Generated Successfully")

# Run the generated python program to get the error message
error_message = run_generated_program(generated_program)
print("Error Message:", error_message)

if error_message.strip() == "":
    mutation_prompt = get_dynamic_prompt("mutation_prompt.txt", input_code=generated_program)
    mutated_code = ask_any_model("gemini/gemini-2.5-flash", mutation_prompt)
    print("Mutated Code:\n", mutated_code)
    print("Running mutated code...")
    mutated_error = run_generated_program(mutated_code)
    if mutated_error.strip() == "":
        print("Mutated code ran successfully with no errors.")
    else:
        print("Mutated Code Error Message:", mutated_error)
else:
    fixing_prompt = get_dynamic_prompt("fixing_prompt_template.txt", faulty_code=generated_program, error_message=error_message)
    fixed_code = ask_any_model("gemini/gemini-2.5-flash", fixing_prompt)
    print("Fixed Code:\n", fixing_prompt)
    print("Running fixed code...")
    fixed_error = run_generated_program(fixed_code)
    if fixed_error.strip() == "":
        print("Fixed code ran successfully with no errors.")
    else:
        print("Fixed Code Error Message:", fixed_error)
