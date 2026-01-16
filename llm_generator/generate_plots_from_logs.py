import os
import sys
import re
import argparse
from glob import glob

# Add project root to path so we can import scripts if needed
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Attempt to import generate_summary_plot from lib.utils
# We try multiple import paths to be robust to where the script is run from
try:
    # Try importing as part of the module
    from llm_generator.lib.utils import generate_summary_plot
except ImportError:
    try:
        # Try local import if running from llm_generator dir
        from lib.utils import generate_summary_plot
    except ImportError:
        print("Error: Could not import 'generate_summary_plot'. Ensure you are running this script from the project root or llm_generator directory.")
        sys.exit(1)

def parse_log_file(file_path):
    """
    Parses an execution log file to extract performance metrics.
    Looking for a block like:
    
    ============================================================
      PERFORMANCE SUMMARY for <model_name>
    ------------------------------------------------------------
      Target Number of Programs : <int>
      Total Valid Programs     : <int>
      Total Time Taken         : <float> seconds
      Avg Time per Valid Prog  : <float> seconds
    ------------------------------------------------------------
      Total Cost (Estimated)   : $<float>
      ...
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"  Error reading file {file_path}: {e}")
        return None

    # Find the PERFORMANCE SUMMARY block
    # We look for the model name line followed by the block content
    summary_match = re.search(r'PERFORMANCE SUMMARY for\s+(.+?)\s*\n(.*?)(?=============================================================|\Z)', content, re.DOTALL)
    
    if not summary_match:
        # Fallback for slightly different separators or end of file
        summary_match = re.search(r'PERFORMANCE SUMMARY for\s+(.+?)\s*\n(.*?)$', content, re.DOTALL)

    if not summary_match:
        return None

    model_name = summary_match.group(1).strip()
    block_content = summary_match.group(2)

    stats = {'model': model_name}

    # Extract metrics using regex
    # Target Number of Programs : 20
    total_programs_match = re.search(r'Target Number of Programs\s*:\s*(\d+)', block_content)
    # Total Valid Programs     : 19
    valid_programs_match = re.search(r'Total Valid Programs\s*:\s*(\d+)', block_content)
    # Total Time Taken         : 271.04 seconds
    total_time_match = re.search(r'Total Time Taken\s*:\s*([\d\.]+)', block_content)
    # Total Cost (Estimated)   : $2.479694
    total_cost_match = re.search(r'Total Cost \(Estimated\)\s*:\s*\$([\d\.]+)', block_content)

    if valid_programs_match:
        stats['valid_programs'] = int(valid_programs_match.group(1))
    else:
        # If we can't find valid programs count, this summary block might be malformed
        return None

    if total_programs_match:
        stats['total_programs'] = int(total_programs_match.group(1))
    else:
        # Default or fallback? Let's assume valid count if total is missing (unlikely)
        stats['total_programs'] = stats['valid_programs'] 

    if total_time_match:
        stats['total_time'] = float(total_time_match.group(1))
    else:
        stats['total_time'] = 0.0
    
    if total_cost_match:
        stats['total_cost'] = float(total_cost_match.group(1))
    else:
        stats['total_cost'] = 0.0

    return stats

def main():
    parser = argparse.ArgumentParser(description="Scrape execution logs from a directory and generate a performance comparison plot.")
    parser.add_argument("input_dir", help="Directory containing model subdirectories with execution_log.txt files")
    parser.add_argument("--output_dir", help="Directory to save the generated plot. Defaults to input_dir.")
    
    args = parser.parse_args()
    
    input_dir = os.path.abspath(args.input_dir)
    output_dir = os.path.abspath(args.output_dir) if args.output_dir else input_dir

    if not os.path.exists(input_dir):
        print(f"Error: Input directory '{input_dir}' does not exist.")
        sys.exit(1)

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    print(f"Scanning '{input_dir}' for execution logs...")

    stats_summary = []
    
    # Use os.walk to find all execution_log.txt files recursively
    for root, dirs, files in os.walk(input_dir):
        if "execution_log.txt" in files:
            log_path = os.path.join(root, "execution_log.txt")
            
            # Skip logs that might be in the root if we only want subdirs, 
            # but usually scraping all found logs is the user's intent.
            
            print(f"Processing: {log_path}")
            stats = parse_log_file(log_path)
            
            if stats:
                print(f"  + Found stats for model: {stats['model']}")
                stats_summary.append(stats)
            else:
                print(f"  - No valid summary block found in {log_path}")

    if not stats_summary:
        print("No valid performance summaries found in the specified directory.")
        sys.exit(0)

    # Sort stats by model name for consistent plotting
    stats_summary.sort(key=lambda x: x['model'])

    print(f"\nFound data for {len(stats_summary)} models. Generating plot...")
    
    try:
        generate_summary_plot(stats_summary, output_dir)
        print(f"Successfully generated plot in {output_dir}")
    except Exception as e:
        print(f"Error generating plot: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
