import os
import shutil

def aggregate_circuits():
    # Base directory
    base_dir = os.path.abspath("local_saved_circuits")
    
    # Destination directory
    dest_dir = os.path.join(base_dir, "Big_aggregate", "generated")
    
    # Ensure destination directory exists
    os.makedirs(dest_dir, exist_ok=True)
    
    # Counter for renaming
    count = 1
    
    # Walk through the directory tree
    for root, dirs, files in os.walk(base_dir):
        # If 'Big_aggregate' is in the current directories, remove it so os.walk doesn't visit it
        if "Big_aggregate" in dirs:
            dirs.remove("Big_aggregate")
            
        # If we are in a 'generated' directory
        if os.path.basename(root) == "generated":
            print(f"Processing directory: {root}")
            
            # Sort files for deterministic order
            sorted_files = sorted(files)
            for file_name in sorted_files:
                if file_name.endswith(".py"):
                    src_file = os.path.join(root, file_name)
                    dest_file_name = f"circuit{count}.py"
                    dest_file = os.path.join(dest_dir, dest_file_name)
                    
                    shutil.copy2(src_file, dest_file)
                    print(f"Copied {src_file} -> {dest_file}")
                    
                    count += 1

if __name__ == "__main__":
    aggregate_circuits()
