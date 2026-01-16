import os
import shutil
import time
import json
import matplotlib.pyplot as plt
import numpy as np
from tqdm import tqdm

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

def save_text_to_file(text, file_path, verbose=True):
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
    
    if verbose:
        tqdm.write(f"Saved content to {file_path}")

def parse_time_metrics(output):
    """Parses output from /usr/bin/time -v"""
    metrics = {}
    try:
        for line in output.splitlines():
            line = line.strip()
            if "User time (seconds):" in line:
                metrics["user_time"] = float(line.split(":")[-1].strip())
            elif "System time (seconds):" in line:
                metrics["sys_time"] = float(line.split(":")[-1].strip())
            elif "Maximum resident set size (kbytes):" in line:
                metrics["max_rss_kb"] = int(line.split(":")[-1].strip())
            elif "Percent of CPU this job got:" in line:
                metrics["cpu_percent"] = line.split(":")[-1].strip()
    except Exception as e:
        # Using print/tqdm.write might be noisy if this function is called frequently
        pass 
    return metrics

def clear_directory(path):
    if os.path.exists(path):
        shutil.rmtree(path)
    os.makedirs(path, exist_ok=True)

def generate_summary_plot(stats_summary, output_dir):
    """
    Generates summary plots comparing performance metrics across models.
    Creates 3 side-by-side subplots for Validity, Time, and Cost.
    """
    if not stats_summary:
        return

    models = [s['model'] for s in stats_summary]
    short_models = [m.split('/')[-1] for m in models]
    
    valid_percentages = [(s['valid_programs'] / s['total_programs']) * 100 for s in stats_summary]
    
    # Handle potentially 0 valid programs to avoid division by zero
    avg_times = []
    for s in stats_summary:
        if s['valid_programs'] > 0:
            avg_times.append(s['total_time'] / s['valid_programs'])
        else:
            avg_times.append(0)

    costs_per_valid = []
    for s in stats_summary:
        if s['valid_programs'] > 0:
            costs_per_valid.append(s['total_cost'] / s['valid_programs'])
        else:
            # If no valid programs, the cost per valid program is theoretically infinite.
            # We show total sunk cost here as a proxy, or could be 0.
            costs_per_valid.append(s['total_cost']) 

    x = np.arange(len(models)) 
    
    # Create 3 subplots
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(18, 6))
    
    # Plot 1: Validity
    ax1.bar(x, valid_percentages, color='skyblue')
    ax1.set_title('Validity (%)')
    ax1.set_ylabel('Percentage')
    ax1.set_xticks(x)
    ax1.set_xticklabels(short_models, rotation=45, ha='right')
    ax1.grid(axis='y', linestyle='--', alpha=0.7)

    # Plot 2: Time
    ax2.bar(x, avg_times, color='lightgreen')
    ax2.set_title('Avg Time per Valid Program (s)')
    ax2.set_ylabel('Seconds')
    ax2.set_xticks(x)
    ax2.set_xticklabels(short_models, rotation=45, ha='right')
    ax2.grid(axis='y', linestyle='--', alpha=0.7)

    # Plot 3: Cost
    ax3.bar(x, costs_per_valid, color='salmon')
    ax3.set_title('Cost per Valid Program ($)')
    ax3.set_ylabel('Dollars')
    ax3.set_xticks(x)
    ax3.set_xticklabels(short_models, rotation=45, ha='right')
    ax3.grid(axis='y', linestyle='--', alpha=0.7)

    fig.suptitle('Model Performance Comparison', fontsize=16)
    fig.tight_layout()
    
    plot_path = os.path.join(output_dir, "performance_summary.png")
    # Use bbox_inches='tight' to ensure the legend and labels are included and not cut off
    plt.savefig(plot_path, bbox_inches='tight')
    tqdm.write(f"Summary plot saved to {plot_path}")

def generate_coverage_text_report(grouped_results, output_file):
    """Generates a structured text report of coverage results."""
    with open(output_file, 'w') as f:
        f.write("Coverage Analysis Report\n")
        f.write("========================\n\n")
        
        for group_name in sorted(grouped_results.keys()):
            files = grouped_results[group_name]
            f.write(f"=== Group: {group_name} ===\n")
            
            successful_coverages = []
            files_sorted = sorted(files, key=lambda x: x['file'])
            
            # List individual files
            for entry in files_sorted:
                fname = os.path.basename(entry['file'])
                if entry['success']:
                    f.write(f"  {fname}: {entry['coverage_percent']:.2f}%\n")
                    if entry.get("verbose_report"):
                        f.write("\n    Detailed Coverage:\n")
                        for line in entry["verbose_report"].splitlines():
                            f.write(f"    {line}\n")
                        f.write("\n")
                    successful_coverages.append(entry['coverage_percent'])
                else:
                    err_msg = entry['error'].replace('\n', ' ')[:100]
                    f.write(f"  {fname}: ERROR ({err_msg}...)\n")
            
            # Summary for the group
            if successful_coverages:
                avg = sum(successful_coverages) / len(successful_coverages)
                f.write(f"\n  Summary for {group_name}:\n")
                f.write(f"    Average Coverage: {avg:.2f}%\n")
                f.write(f"    Valid Programs: {len(successful_coverages)}/{len(files)}\n")
            else:
                f.write(f"\n  Summary for {group_name}:\n")
                f.write(f"    Average Coverage: N/A\n")
                f.write(f"    Valid Programs: 0/{len(files)}\n")
            
            f.write("\n" + "-"*40 + "\n\n")

def generate_coverage_plot(grouped_results, output_file):
    """Generates a bar chart comparing average coverage across groups."""
    group_names = []
    averages = []
    
    for name in sorted(grouped_results.keys()):
        files = grouped_results[name]
        successful = [x['coverage_percent'] for x in files if x['success']]
        if successful:
            avg = sum(successful) / len(successful)
            group_names.append(name)
            averages.append(avg)
            
    if not group_names:
        print("No successful data to plot.")
        return

    plt.figure(figsize=(10 + len(group_names)*0.5, 6)) # Adjust width based on number of groups
    bars = plt.bar(group_names, averages, color='skyblue')
    
    plt.xlabel('Group / Model')
    plt.ylabel('Average Coverage (%)')
    plt.title('Average Code Coverage by Group')
    
    # Rotate labels to prevent overlap
    plt.xticks(rotation=45, ha='right')
    
    # Add values on top of bars
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.1f}%',
                ha='center', va='bottom')
                
    plt.tight_layout()
    try:
        plt.savefig(output_file)
        print(f"Plot saved to {output_file}")
    except Exception as e:
        print(f"Failed to save plot: {e}")
