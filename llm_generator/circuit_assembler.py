import sys
import logging
import glob
import random
import os
import argparse
from tqdm import tqdm

# Add project root to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Import from local library
from lib.circuit_assembler import assemble

if __name__ == "__main__":

    # Configuration via argparse
    parser = argparse.ArgumentParser(description="Assemble and test quantum circuits.")
    parser.add_argument("input_dir", nargs='?', default="local_saved_circuits/Correct_format/", help="Directory containing input circuit files")
    parser.add_argument("--output-dir", default="local_saved_circuits/Correct_format/combined_circuits", help="Directory to save assembled circuits")
    parser.add_argument("--n-generations", type=int, default=1, help="Number of circuits to generate")
    parser.add_argument("--min-files", type=int, default=2, help="Minimum number of files per assembly")
    parser.add_argument("--max-files", type=int, default=5, help="Maximum number of files per assembly")
    parser.add_argument("--language", default="guppy", choices=["guppy", "qiskit"], help="Language of circuits (guppy or qiskit)")
    
    args = parser.parse_args()

    INPUT_DIR = args.input_dir
    OUTPUT_DIR = args.output_dir
    
    # Ensure output directory exists
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    LOGFILE_DIR = os.path.join(args.output_dir, "assembler.log")
    N_GENERATIONS = args.n_generations
    MIN_FILES = args.min_files
    MAX_FILES = args.max_files

    # Setup logging to both file and stdout
    # Remove existing handlers to avoid duplication if run multiple times in same session or if configured elsewhere
    for handler in logging.root.handlers[:]:
        logging.root.removeHandler(handler)
        
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(LOGFILE_DIR),
            logging.StreamHandler(sys.stdout)
        ]
    )

    logging.info(f"Starting assembler with configurations: Input={INPUT_DIR}, Output={OUTPUT_DIR}, N={N_GENERATIONS}, Language={args.language}")

    # Get all python files
    input_files = glob.glob(os.path.join(INPUT_DIR, "*.py"))
    
    if not input_files:
        msg = f"No input files found in {INPUT_DIR}"
        logging.error(msg)
        print(msg)
        sys.exit(1)

    logging.info(f"Found {len(input_files)} input files. Starting assembly...")
    print(f"Found {len(input_files)} input files. Starting assembly...")

    seen_combinations = set()
    generated_count = 0
    assembled_files = []
    
    with tqdm(total=N_GENERATIONS, desc="Assembling") as pbar:
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
            
            try:
                assemble(selected_files, output_file, generated_count, language=args.language)
                assembled_files.append(output_file)
                generated_count += 1
                pbar.update(1)
            except Exception as e:
                logging.error(f"Failed to assemble combination {selected_files}: {e}")
