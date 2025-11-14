import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import subprocess
import re
import os
import sys
from matplotlib.animation import FuncAnimation

def run_simulation(n_procs, n_threads, max_retries=2):
    """Runs the simulation and returns the execution time."""
    
    # Ensure binary exists
    if not os.path.exists('../traffic_sim'):
        print("ERROR: traffic_sim not found. Run 'make' first.")
        return None
    
    # Set environment
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(n_threads)
    
    cmd = f"mpirun -np {n_procs} ../traffic_sim"
    
    for attempt in range(max_retries):
        try:
            result = subprocess.run(
                cmd, 
                shell=True, 
                capture_output=True, 
                text=True, 
                env=env, 
                timeout=120  # 2 minute timeout
            )
            
            if result.returncode != 0:
                print(f"Simulation failed (attempt {attempt+1}/{max_retries})")
                print(f"STDERR: {result.stderr}")
                continue
            
            # Parse execution time from output
            match = re.search(r'Total time:\s*([\d.]+)\s*seconds', result.stdout)
            if match:
                return float(match.group(1))
            else:
                print(f"Could not parse time from output:\n{result.stdout}")
                return None
                
        except subprocess.TimeoutExpired:
            print(f"Simulation timed out (attempt {attempt+1}/{max_retries})")
        except Exception as e:
            print(f"Error running simulation: {e}")
    
    return None

def generate_strong_scaling():
    """Strong scaling: constant problem size, increasing processors."""
    print("\n=== Generating Strong Scaling Data ===")
    
    n_procs_list = [1, 2, 4]
    times = []
    
    for n_procs in n_procs_list:
        print(f"Running with {n_procs} process(es)...")
        time = run_simulation(n_procs, 1)
        if time is not None:
            times.append(time)
            print(f"  Time: {time:.3f}s")
        else:
            times.append(float('nan'))
            print(f"  FAILED")
    
    # Plot
    plt.figure(figsize=(10, 6))
    plt.plot(n_procs_list, times, 'o-', linewidth=2, markersize=10)
    plt.xlabel("Number of MPI Processes", fontsize=12)
    plt.ylabel("Execution Time (seconds)", fontsize=12)
    plt.title("Strong Scaling (MPI Only)", fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig("strong_scaling.png", dpi=300)
    print("Saved: strong_scaling.png")
    plt.close()
    
    # Calculate speedup
    if times[0] > 0:
        speedups = [times[0] / t if t > 0 else 0 for t in times]
        plt.figure(figsize=(10, 6))
        plt.plot(n_procs_list, speedups, 'o-', linewidth=2, markersize=10, label='Actual')
        plt.plot(n_procs_list, n_procs_list, '--', linewidth=2, label='Ideal (Linear)', alpha=0.5)
        plt.xlabel("Number of MPI Processes", fontsize=12)
        plt.ylabel("Speedup", fontsize=12)
        plt.title("Speedup vs Ideal", fontsize=14, fontweight='bold')
        plt.legend(fontsize=11)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig("speedup.png", dpi=300)
        print("Saved: speedup.png")
        plt.close()

def generate_hybrid_scaling():
    """Compare pure MPI vs MPI+OpenMP hybrid."""
    print("\n=== Generating Hybrid Scaling Data ===")
    
    configs = [
        (1, 1, "1 proc × 1 thread"),
        (2, 1, "2 procs × 1 thread"),
        (2, 2, "2 procs × 2 threads"),
        (4, 1, "4 procs × 1 thread"),
    ]
    
    times = []
    labels = []
    
    for n_procs, n_threads, label in configs:
        print(f"Running: {label}...")
        time = run_simulation(n_procs, n_threads)
        if time is not None:
            times.append(time)
            labels.append(label)
            print(f"  Time: {time:.3f}s")
        else:
            times.append(float('nan'))
            labels.append(label)
            print(f"  FAILED")
    
    # Plot
    plt.figure(figsize=(12, 6))
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']
    bars = plt.bar(range(len(labels)), times, color=colors[:len(labels)])
    plt.xlabel("Configuration", fontsize=12)
    plt.ylabel("Execution Time (seconds)", fontsize=12)
    plt.title("MPI vs Hybrid (MPI+OpenMP) Performance", fontsize=14, fontweight='bold')
    plt.xticks(range(len(labels)), labels, rotation=15, ha='right')
    plt.grid(True, alpha=0.3, axis='y')
    
    # Add value labels on bars
    for i, (bar, time) in enumerate(zip(bars, times)):
        if not np.isnan(time):
            plt.text(bar.get_x() + bar.get_width()/2, time + max(times)*0.02, 
                    f'{time:.2f}s', ha='center', va='bottom', fontsize=10)
    
    plt.tight_layout()
    plt.savefig("hybrid_scaling.png", dpi=300)
    print("Saved: hybrid_scaling.png")
    plt.close()

def generate_congestion_heatmap():
    """Generate congestion heatmap from car positions."""
    print("\n=== Generating Congestion Heatmap ===")
    
    csv_file = "../car_positions.csv"
    if not os.path.exists(csv_file):
        print(f"ERROR: {csv_file} not found. Run simulation first.")
        return
    
    try:
        data = pd.read_csv(csv_file)
        
        # Count cars at each intersection over time
        # Using the last 100 steps for steady-state analysis
        max_step = data['step'].max()
        recent_data = data[data['step'] > max_step - 100]
        
        congestion = recent_data.groupby('intersection').size()
        
        # Create 10x10 grid
        grid = np.zeros((10, 10))
        for intersection, count in congestion.items():
            if intersection < 100:  # Valid intersection
                i = intersection // 10
                j = intersection % 10
                grid[i, j] = count
        
        # Plot heatmap
        plt.figure(figsize=(10, 8))
        im = plt.imshow(grid, cmap='hot', interpolation='nearest')
        plt.colorbar(im, label='Car Count (last 100 steps)')
        plt.xlabel("Y Coordinate", fontsize=12)
        plt.ylabel("X Coordinate", fontsize=12)
        plt.title("Traffic Congestion Heatmap", fontsize=14, fontweight='bold')
        
        # Add grid
        for i in range(11):
            plt.axhline(i-0.5, color='white', linewidth=0.5, alpha=0.3)
            plt.axvline(i-0.5, color='white', linewidth=0.5, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig("congestion_heatmap.png", dpi=300)
        print("Saved: congestion_heatmap.png")
        plt.close()
        
    except Exception as e:
        print(f"Error generating heatmap: {e}")

def generate_car_animation():
    """Generate animation of car movement (sampling every 10 steps)."""
    print("\n=== Generating Car Animation ===")
    
    csv_file = "../car_positions.csv"
    if not os.path.exists(csv_file):
        print(f"ERROR: {csv_file} not found. Run simulation first.")
        return
    
    try:
        data = pd.read_csv(csv_file)
        
        # Sample every 10 steps for reasonable file size
        steps_to_plot = sorted(data['step'].unique())[::10]
        
        fig, ax = plt.subplots(figsize=(10, 10))
        
        def animate(step):
            ax.clear()
            step_data = data[data['step'] == step]
            
            # Convert intersection IDs to coordinates
            x = step_data['intersection'] // 10
            y = step_data['intersection'] % 10
            
            ax.scatter(y, x, c='red', s=100, alpha=0.6, edgecolors='black')
            ax.set_xlim(-0.5, 9.5)
            ax.set_ylim(-0.5, 9.5)
            ax.set_xlabel("Y Coordinate", fontsize=12)
            ax.set_ylabel("X Coordinate", fontsize=12)
            ax.set_title(f"Traffic Simulation - Step {step}/{data['step'].max()}", 
                        fontsize=14, fontweight='bold')
            ax.grid(True, alpha=0.3)
            ax.set_aspect('equal')
            
            # Draw grid lines
            for i in range(10):
                ax.axhline(i, color='gray', linewidth=0.5, alpha=0.3)
                ax.axvline(i, color='gray', linewidth=0.5, alpha=0.3)
        
        anim = FuncAnimation(fig, animate, frames=steps_to_plot, 
                           interval=200, repeat=True)
        
        anim.save('car_animation.gif', writer='pillow', fps=5, dpi=100)
        print("Saved: car_animation.gif")
        plt.close()
        
    except Exception as e:
        print(f"Error generating animation: {e}")

def main():
    """Main execution."""
    print("=" * 60)
    print("Traffic Simulation Visualization Suite")
    print("=" * 60)
    
    # Check if we're in the right directory
    if not os.path.exists('../traffic_sim') and not os.path.exists('../Makefile'):
        print("\nERROR: Run this script from the 'visualization' directory")
        print("Usage: cd visualization && python3 visualize.py")
        sys.exit(1)
    
    # Generate all visualizations
    generate_strong_scaling()
    generate_hybrid_scaling()
    generate_congestion_heatmap()
    generate_car_animation()
    
    print("\n" + "=" * 60)
    print("Visualization complete! Generated files:")
    print("  - strong_scaling.png")
    print("  - speedup.png")
    print("  - hybrid_scaling.png")
    print("  - congestion_heatmap.png")
    print("  - car_animation.gif")
    print("=" * 60)

if __name__ == "__main__":
    main()
