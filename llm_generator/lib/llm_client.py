import os
import time
import random
import dotenv
from jinja2 import Template
import litellm
from litellm import completion, completion_cost
from tqdm import tqdm

# Suppress verbose output from litellm that interferes with tqdm
litellm.suppress_debug_info = True
# litellm.set_verbose = False # Uncomment if needed, but suppress_debug_info usually suffices

# Setup your keys (You can also set these in your OS environment variables)
# We try to load from .env file up the tree
dotenv.load_dotenv(dotenv.find_dotenv())

os.environ["GEMINI_API_KEY"] = os.getenv("GEMINI_API_KEY", "")
os.environ["OPENAI_API_KEY"] = os.getenv("OPENAI_API_KEY", "")
os.environ["DEEPSEEK_API_KEY"] = os.getenv("DEEPSEEK_API_KEY", "")
os.environ["ANTHROPIC_API_KEY"] = os.getenv("ANTHROPIC_API_KEY", "")

def get_dynamic_prompt(template_path, **kwargs) -> str:
    with open(template_path, 'r') as f:
        template_str = f.read()
    template = Template(template_str)
    return template.render(**kwargs)

def ask_any_model(model_name, prompt):
    # tqdm.write(f"Sending to {model_name}...") # Too noisy for concurrent execution
    
    max_retries = 10
    base_delay = 5
    max_delay = 120
    
    for attempt in range(max_retries):
        try:
            # distinct "completion" function works for ALL providers
            response = completion(
                model=model_name, 
                messages=[{ "content": prompt, "role": "user" }],
                reasoning_effort="high",
                drop_params=True
            )
            
            answer = response['choices'][0]['message']['content']
            
            try:
                cost = completion_cost(completion_response=response)
            except:
                cost = 0.0
                
            usage = response.get('usage', {})
            stats = {
                "prompt_tokens": usage.get('prompt_tokens', 0),
                "completion_tokens": usage.get('completion_tokens', 0),
                "total_tokens": usage.get('total_tokens', 0),
                "cost": cost
            }
            
            return answer, stats, None
            
        except Exception as e:
            error_str = str(e).lower()
            
            # Check for rate limit or quota exceeded errors
            if "rate_limit" in error_str or "quota" in error_str or "429" in error_str or "503" in error_str:
                if attempt + 1 == max_retries:
                    tqdm.write(f"Max retries ({max_retries}) reached for {model_name}.")
                    return None, None, str(e)
                
                # Exponential backoff with jitter
                delay = min(max_delay, base_delay * (2 ** attempt))
                jitter = random.uniform(0, 0.1 * delay)
                wait_time = delay + jitter
                
                tqdm.write(f"Rate limit hit for {model_name}. Waiting {wait_time:.2f}s... (Attempt {attempt+1}/{max_retries})")
                time.sleep(wait_time)
            else:
                tqdm.write(f"Error for {model_name}: {e}")
                return None, None, str(e)
    return None, None, "Unknown error (loop finished without return)"
