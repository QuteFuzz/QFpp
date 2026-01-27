import os
import sys
import argparse

# Add project root to path so we can import scripts if needed
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from llm_generator.lib.utils import generate_summary_plot, parse_summary_log_file
from lib.utils import generate_summary_plot, parse_summary_log_file


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
        if "execution.log" in files:
            log_path = os.path.join(root, "execution.log")
            
            # Skip logs that might be in the root if we only want subdirs, 
            # but usually scraping all found logs is the user's intent.
            
            print(f"Processing: {log_path}")
            stats = parse_summary_log_file(log_path)
            
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
