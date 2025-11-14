import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import subprocess
import re
import numpy as np

def run_simulation(n_procs, n_threads):
    """Runs the simulation and returns the execution time."""
    cmd = f"mpirun -np {n_procs} --use-hwthread-cpus ./hybrid_simulation"
    env = {"OMP_NUM_THREADS": str(n_threads)}
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, env=env)

    # Extract the execution time from the output
    match = re.search(r"(\d+\.\d+)", result.stdout)
    if match:
        return float(match.group(1))
    return float('inf')

def generate_scaling_graphs():
    """Generates scaling graphs for the simulation."""
    # Strong scaling: constant problem size, increasing processors
    n_procs_strong = [1, 2, 4, 8]
    times_strong = [run_simulation(p, 1) for p in n_procs_strong]

    plt.figure()
    plt.plot(n_procs_strong, times_strong, 'o-')
    plt.xlabel("Number of Processes")
    plt.ylabel("Execution Time (s)")
    plt.title("Strong Scaling")
    plt.savefig("strong_scaling.png")

    # Weak scaling: problem size per processor is constant
    # (for this, we would need to adjust the simulation size, which is not implemented)

    # Hybrid scaling: compare pure MPI with MPI + OpenMP
    n_procs_hybrid = [1, 2, 4]
    times_mpi = [run_simulation(p, 1) for p in n_procs_hybrid]
    times_hybrid = [run_simulation(p, 2) for p in n_procs_hybrid]

    plt.figure()
    plt.plot(n_procs_hybrid, times_mpi, 'o-', label="MPI only")
    plt.plot(n_procs_hybrid, times_hybrid, 's-', label="MPI + OpenMP (2 threads)")
    plt.xlabel("Number of Processes")
    plt.ylabel("Execution Time (s)")
    plt.title("MPI vs. Hybrid Scaling")
    plt.legend()
    plt.savefig("hybrid_scaling.png")
    plt.close()

def animate(i, data, ax):
    """Animation function for car movement."""
    step_data = data[data['step'] == i]
    ax.clear()
    ax.scatter(step_data['road'], step_data['pos'], label=f'Step {i}')
    ax.set_xlim(0, 100)
    ax.set_ylim(0, 1)
    ax.set_xlabel("Road Segment")
    ax.set_ylabel("Position on Road")
    ax.legend()

# Main execution
if __name__ == "__main__":
    # First, generate the animation data
    subprocess.run("mpirun -np 2 ./hybrid_simulation", shell=True)

    # Load and animate
    data = pd.read_csv('car_positions.csv', names=['step', 'car_id', 'pos', 'road'])
    fig, ax = plt.subplots()
    ani = FuncAnimation(fig, animate, fargs=(data, ax), frames=data['step'].unique(), interval=100)
    ani.save('car_animation.gif', writer='pillow')
    plt.close()

    # Then, generate scaling graphs
    generate_scaling_graphs()
