./venv/bin/activate
export PYTHONPATH=$(pwd)/llm_generator:$PYTHONPATH

# Run the generator first, saving into a named run with date-stamped folder
# Get date and time for unique run naming
RUN_NAME="Complete_run_qiskit_$(date +'%Y%m%d_%H%M%S')"
python llm_generator/generator.py --config_file llm_generator/configs/qiskit_full_run_config.yaml --run_name $RUN_NAME

# After generation is complete and circuits are assembled, run the test_existing_circuits.py script to test assembled circuits
python llm_generator/test_existing_circuits.py --input_dir local_saved_circuits/$RUN_NAME/assembled --workers 4 --language qiskit
