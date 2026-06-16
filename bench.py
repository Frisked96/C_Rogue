import subprocess
import time
import sys
import os

def run_command(command, description):
    print(f"--- {description} ---")
    print(f"Executing: {command}")
    start = time.time()
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    end = time.time()
    
    if result.returncode != 0:
        print(f"Error during {description}:")
        print(result.stderr)
        return None, end - start
    
    return result.stdout, end - start

def main():
    # 1. Compile
    stdout, compile_time = run_command("make diag", "Compiling map_diag")
    if stdout is None:
        print("Compilation failed. Aborting.")
        return

    # 2. Run Diagnostic
    # You can pass width, height, seed as arguments
    width = 100
    height = 100
    seed = 42
    stdout, total_run_time = run_command(f"map_diag.exe {width} {height} {seed}", "Running Map Generation Diagnostic")
    if stdout is None:
        print("Run failed. Aborting.")
        return

    # 3. Parse Output
    metrics = {}
    parsing = False
    for line in stdout.splitlines():
        if "METRIC_START" in line:
            parsing = True
            continue
        if "METRIC_END" in line:
            parsing = False
            continue
        if parsing:
            if ":" in line:
                key, val = line.split(":", 1)
                metrics[key.strip()] = val.strip()

    # 4. Display Results
    print("\n" + "="*40)
    print("       MAP GENERATION BENCHMARK")
    print("="*40)
    print(f"Configuration: {width}x{height} map, Seed: {seed}")
    print("-" * 40)
    print("TIMING BREAKDOWN (seconds):")
    print(f"  Total Wall Time:      {metrics.get('total_time', 'N/A')} s")
    print(f"  Terrain Gen (Noise):  {metrics.get('terrain_gen_time', 'N/A')} s")
    print(f"  Hydrology Simulation: {metrics.get('hydrology_sim_time', 'N/A')} s")
    print("-" * 20)
    print("  Hydrology Phases:")
    for key in sorted(metrics.keys()):
        if key.startswith("sim_step_"):
            phase_name = key.replace("sim_step_", "")
            print(f"    - {phase_name:20}: {metrics[key]} s")
    
    print("-" * 40)
    print("WORLD STATISTICS:")
    print(f"  Pond/River Tiles:      {metrics.get('water_fresh_tiles', 'N/A')}")
    print(f"  'Water' Material Tiles: {metrics.get('tiles_with_water_in_name', 'N/A')}")
    print(f"  Tiles with Liquid:      {metrics.get('tiles_with_moisture', 'N/A')}")
    print(f"  Total World Liquid:     {float(metrics.get('total_moisture', 0)):.2f} units")
    print(f"  Highest Water Level:    Z = {metrics.get('max_z_water_moisture', 'N/A')}")
    print(f"  Deepest Water Level:    Z = {metrics.get('min_z_water_moisture', 'N/A')}")
    print("="*40)

if __name__ == "__main__":
    main()
