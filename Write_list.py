import argparse
from pathlib import Path

FILES_PER_LIST = 100  # number of files per output list
BASE_PATH = Path("/storage/gpfs_data/juno/junofs/production/storm/dirac/juno/juno-rtraw")

def main():
    parser = argparse.ArgumentParser(description="Generate .list files for JUNO rtraw runs.")
    
    parser.add_argument( "--version",type=str,default="J25.5.0", help="Software version tag (default: J25.5.0)" )
    parser.add_argument( "--listfile",type=str,help="File containing the list of runs to launch" )

    args = parser.parse_args()

    version_dir = BASE_PATH / args.version
    script_dir = Path(__file__).resolve().parent
    list_dir = script_dir / "list_rtraw"
    run_list_file = args.listfile

    # === Sanity checks ===
    if not version_dir.is_dir():
        print(f"Error: Version directory not found: {version_dir}")
        return
    if not run_list_file.is_file():
        print(f"Error: Run list file not found: {run_list_file}")
        return

    # Create list_rtraw directory if not present
    list_dir.mkdir(exist_ok=True)

    # === Read run list ===
    with open(run_list_file) as f:
        runs = [line.strip() for line in f if line.strip().isdigit()]

    if not runs:
        print(f"Warning: No valid run numbers found in {run_list_file}")
        return

    print(f"Using version directory: {version_dir}")
    print(f"Output directory: {list_dir}")
    print(f"Reading runs from: {run_list_file}")

    # === Process each run ===
    for run_str in runs:
        run = int(run_str)

        # Compute intermediate directories
        dir_lvl1 = f"{run // 10000:08d}"  # e.g., 00010000
        dir_lvl2 = f"{run // 100:08d}"    # e.g., 00010500

        # Construct full path
        run_dir = version_dir / "global_trigger" / dir_lvl1 / dir_lvl2 / f"{run:05d}"

        if not run_dir.is_dir():
            print(f"Warning: Directory not found for run {run}: {run_dir}")
            continue

        # Find matching files
        files = sorted(run_dir.glob(f"RUN.{run}.JUNODAQ.Physics.ds-2.global_trigger.*.rtraw"))
        if not files:
            print(f"Warning: No matching files for run {run}")
            continue

        print(f"Run {run}: found {len(files)} matching files")

        # Write output lists in groups of 100
        for i in range(0, len(files), FILES_PER_LIST):
            chunk = files[i:i+FILES_PER_LIST]
            index = (i // FILES_PER_LIST) + 1
            out_filename = list_dir / f"list_rtraw_RUN{run}_{index:03d}.list"

            with open(out_filename, "w") as out:
                for f in chunk:
                    out.write(str(f.resolve()) + "\n")

            print(f"  Wrote {len(chunk)} paths to {out_filename}")

if __name__ == "__main__":
    main()
