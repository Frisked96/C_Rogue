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
    width = 100
    height = 100
    depth = 100
    seed = 42
    stdout, total_run_time = run_command(f"map_diag.exe {width} {height} {seed} {depth}", "Running Map Generation Diagnostic")
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
    print(f"Configuration: {width}x{height}x{depth} map, Seed: {seed}")
    print("-" * 40)
    print("TIMING BREAKDOWN (seconds):")
    print(f"  Total Wall Time:      {metrics.get('total_time', 'N/A')} s")
    print(f"  Terrain Gen (Noise):  {metrics.get('terrain_gen_time', 'N/A')} s")
    print(f"  Hydrology Simulation: {metrics.get('hydrology_sim_time', 'N/A')} s")
    print("-" * 20)
    print("  Hydrology Phases:")
    phases = [k for k in metrics.keys() if k.startswith("sim_step_")]
    for key in sorted(phases):
        phase_name = key.replace("sim_step_", "")
        print(f"    - {phase_name:20}: {metrics[key]} s")
    
    print("-" * 40)
    print("WORLD STATISTICS (Final Map Data):")
    
    # Soil Saturation
    total_soil = int(metrics.get('soil_tiles_total', 0))
    moist_soil = int(metrics.get('soil_tiles_with_moisture', 0))
    dry_soil = total_soil - moist_soil
    sat_pct = (moist_soil / total_soil * 100) if total_soil > 0 else 0
    
    print(f"  Soil Tiles (Total):    {total_soil}")
    print(f"  Soil Tiles (Moist):    {moist_soil} ({sat_pct:.1f}%)")
    print(f"  Soil Tiles (Dry):      {dry_soil}")
    print("-" * 20)
    print(f"  Total Water Tiles (in Air): {metrics.get('water_surface_tiles', '0')}")
    print(f"    - Deep Water (>=0.4):   {metrics.get('deep_water_tiles', '0')}")
    print(f"    - Shallow Water (<0.4): {metrics.get('shallow_water_tiles', '0')}")
    print(f"  Frozen/Snow Tiles:          {metrics.get('frozen_tiles', '0')}")
    print("-" * 20)
    print(f"  Surface Water Vol:     {float(metrics.get('total_surface_water_vol', 0)):.2f} m^3")
    print(f"  Soil Moisture Vol:     {float(metrics.get('total_soil_moisture_vol', 0)):.2f} m^3")
    print(f"  Total Ice/Snow Vol:    {float(metrics.get('total_ice_vol', 0)):.2f} m^3")
    print("-" * 20)
    print(f"  Highest Peak (Soil):   Z = {metrics.get('max_z_soil', 'N/A')}")
    print(f"  Highest Surface Water: Z = {metrics.get('max_z_surface_water', 'N/A')}")
    print(f"  Deepest Liquid Found:  Z = {metrics.get('min_z_liquid', 'N/A')}")
    print("="*40)

if __name__ == "__main__":
    main()
