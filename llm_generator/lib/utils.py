import os
import shutil
import matplotlib.pyplot as plt
import numpy as np
from tqdm import tqdm
import re

def parse_summary_log_file(file_path):
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
    Creates separate plot files for Validity, Time, and Cost in the output directory.
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

    avg_scores = [s.get('avg_quality_score', 0.0) for s in stats_summary]

    x = np.arange(len(models)) 

    # --- Plot 1: Validity ---
    fig1, ax1 = plt.subplots(figsize=(10, 6))
    ax1.bar(x, valid_percentages, color='skyblue')
    ax1.set_title('Validity (%)')
    ax1.set_ylabel('Percentage')
    ax1.set_xticks(x)
    ax1.set_xticklabels(short_models, rotation=45, ha='right')
    ax1.grid(axis='y', linestyle='--', alpha=0.7)
    
    fig1.tight_layout()
    plot_path1 = os.path.join(output_dir, "performance_validity.png")
    plt.savefig(plot_path1, bbox_inches='tight')
    tqdm.write(f"Validity plot saved to {plot_path1}")
    plt.close(fig1)

    # --- Plot 2: Time ---
    fig2, ax2 = plt.subplots(figsize=(10, 6))
    ax2.bar(x, avg_times, color='lightgreen')
    ax2.set_title('Avg Time per Valid Program (s)')
    ax2.set_ylabel('Seconds')
    ax2.set_xticks(x)
    ax2.set_xticklabels(short_models, rotation=45, ha='right')
    ax2.grid(axis='y', linestyle='--', alpha=0.7)

    fig2.tight_layout()
    plot_path2 = os.path.join(output_dir, "performance_time.png")
    plt.savefig(plot_path2, bbox_inches='tight')
    tqdm.write(f"Time plot saved to {plot_path2}")
    plt.close(fig2)

    # --- Plot 3: Cost ---
    fig3, ax3 = plt.subplots(figsize=(10, 6))
    ax3.bar(x, costs_per_valid, color='salmon')
    ax3.set_title('Cost per Valid Program ($)')
    ax3.set_ylabel('Dollars')
    ax3.set_xticks(x)
    ax3.set_xticklabels(short_models, rotation=45, ha='right')
    ax3.grid(axis='y', linestyle='--', alpha=0.7)

    fig3.tight_layout()
    plot_path3 = os.path.join(output_dir, "performance_cost.png")
    plt.savefig(plot_path3, bbox_inches='tight')
    tqdm.write(f"Cost plot saved to {plot_path3}")
    plt.close(fig3)

    # --- Plot 4: Quality Score ---
    fig4, ax4 = plt.subplots(figsize=(10, 6))
    ax4.bar(x, avg_scores, color='violet')
    ax4.set_title('Avg Quality Score')
    ax4.set_ylabel('Score')
    ax4.set_xticks(x)
    ax4.set_xticklabels(short_models, rotation=45, ha='right')
    ax4.grid(axis='y', linestyle='--', alpha=0.7)

    fig4.tight_layout()
    plot_path4 = os.path.join(output_dir, "performance_quality.png")
    plt.savefig(plot_path4, bbox_inches='tight')
    tqdm.write(f"Quality Score plot saved to {plot_path4}")
    plt.close(fig4)

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

def generate_complexity_scatter_plots(all_metrics, output_dir):
    """
    Generates scatter plots comparing complexity metrics against compilation/execution time.
    all_metrics: list of dicts with 'model', 'metrics' keys.
    """
    if not all_metrics:
        return

    # Metrics to plot against Time
    plot_configs = [
        ('line_count', 'Line Count', 'Line Count vs Time'),
        ('function_count', 'Function Count', 'Function Count vs Time'),
        ('nesting_depth', 'Nesting Depth', 'Nesting Depth vs Time'),
        ('coverage_percent', 'Coverage (%)', 'Coverage vs Time')
    ]
    
    # Extract unique models for color coding
    models = sorted(list(set(m['model'] for m in all_metrics)))
    colors = plt.cm.tab10(np.linspace(0, 1, len(models)))
    model_color_map = dict(zip(models, colors))
    
    for metric_key, xlabel, title in plot_configs:
        fig, ax = plt.subplots(figsize=(10, 6))
        
        has_data = False
        for model in models:
            model_data = [m for m in all_metrics if m['model'] == model]
            
            x_vals = []
            y_vals = []
            
            for entry in model_data:
                metrics = entry.get('metrics', {})
                if metric_key in metrics and 'wall_time' in metrics:
                   # Only plot if we have valid data (e.g. coverage > 0 if plotting coverage)
                   # For coverage, we might only want ran programs.
                   if metric_key == 'coverage_percent' and metrics.get(metric_key, 0) == 0:
                       continue 

                   x_vals.append(metrics[metric_key])
                   y_vals.append(metrics['wall_time'])
            
            if x_vals:
                ax.scatter(x_vals, y_vals, label=model.split('/')[-1], color=model_color_map[model], alpha=0.6, edgecolors='w', s=50)
                has_data = True
        
        if has_data:
            ax.set_xlabel(xlabel)
            ax.set_ylabel('Time (s)')
            ax.set_title(title)
            ax.legend()
            ax.grid(True, linestyle='--', alpha=0.6)
            
            safe_title = title.replace(" ", "_").lower()
            plot_path = os.path.join(output_dir, f"complexity_{safe_title}.png")
            plt.savefig(plot_path, bbox_inches='tight')
            tqdm.write(f"Saved plot: {plot_path}")
        
        plt.close(fig)

def generate_coverage_plot(grouped_results, output_file):
    """
    Generates scatter plots of coverage vs function count.
    Generates a separate plot file for each group in grouped_results.
    """
    groups = sorted(grouped_results.keys())
    
    if not groups:
        tqdm.write("No data to plot.")
        return

    base_name, ext = os.path.splitext(output_file)
    if not ext:
        ext = ".png"
        
    for name in groups:
        files = grouped_results[name]
        successful_entries = [x for x in files if x.get('success', False)]
        
        fig, ax = plt.subplots(figsize=(10, 6))
        
        if successful_entries:
            x_vals = [entry.get('function_count', 0) for entry in successful_entries]
            y_vals = [entry.get('coverage_percent', 0.0) for entry in successful_entries]
            
            if x_vals and y_vals:
                ax.scatter(x_vals, y_vals, alpha=0.7, edgecolors='w', s=60)
        
        ax.set_title(f"Coverage Analysis: {name}")
        ax.set_xlabel('Number of Distinct Functions')
        ax.set_ylabel('Coverage (%)')
        ax.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.7)
        
        # Determine filename
        if len(groups) > 1:
            # Sanitize group name for filename
            safe_name = "".join(x for x in name if x.isalnum() or x in ('_', '-'))
            current_output = f"{base_name}_{safe_name}{ext}"
        else:
            current_output = output_file
            
        fig.tight_layout()
        
        try:
            plt.savefig(current_output, bbox_inches='tight', dpi=150)
            tqdm.write(f"Plot saved to {current_output}")
        except Exception as e:
            tqdm.write(f"Failed to save plot to {current_output}: {e}")
            
        plt.close(fig)
