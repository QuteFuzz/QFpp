#!/bin/bash
source .venv/bin/activate || { echo "Failed to activate virtual environment"; exit 1; }
export PYTHONPATH=$(pwd)/llm_generator:$PYTHONPATH

# Run the generator first, saving into a named run with date-stamped folder
# Get date and time for unique run naming
RUN_NAME="Complete_run_guppy_$(date +'%Y%m%d_%H%M%S')"
python llm_generator/generator.py --config_file llm_generator/configs/guppy_full_run_config.yaml --run_name $RUN_NAME

# After generation is complete and circuits are assembled, run the test_existing_circuits.py script to test assembled circuits
python llm_generator/test_existing_circuits.py local_saved_circuits/$RUN_NAME/assembled --workers 4 --language guppy

