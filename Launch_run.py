import argparse
from pathlib import Path
import re

FILES_PER_LIST = 100  # Number of files per output list
BASE_PATH = Path("/storage/gpfs_data/juno/junofs/production/storm/dirac/juno/juno-rtraw")

def generate_list_files(run_list_file, version_dir, list_dir):
    """Generate .list files for each run"""
    with open(run_list_file) as f:
        runs = [line.strip() for line in f if line.strip().isdigit()]

    if not runs:
        print(f"No valid run numbers found in {run_list_file}")
        return []

    list_dir.mkdir(exist_ok=True)
    generated_lists = []

    for run_str in runs:
        run = int(run_str)

        # Correct directory naming
        dir_lvl1 = f"{(run // 10000) * 10000:08d}"  # e.g., 10554 -> 00010000
        dir_lvl2 = f"{(run // 100) * 100:08d}"      # e.g., 10554 -> 00010500

        run_dir = version_dir / "global_trigger" / dir_lvl1 / dir_lvl2 / f"{run:05d}"

        if not run_dir.is_dir():
            print(f"Warning: Directory not found for run {run}: {run_dir}")
            continue

        files = sorted(run_dir.glob(f"RUN.{run}.JUNODAQ.Physics.ds-2.global_trigger.*.rtraw"))
        if not files:
            print(f"Warning: No matching files for run {run}")
            continue

        for i in range(0, len(files), FILES_PER_LIST):
            chunk = files[i:i+FILES_PER_LIST]
            index = (i // FILES_PER_LIST) + 1
            out_filename = list_dir / f"list_rtraw_RUN{run}_{index:03d}.list"
            with open(out_filename, "w") as out:
                for f in chunk:
                    out.write(str(f.resolve()) + "\n")
            generated_lists.append(out_filename)
            print(f"Wrote {len(chunk)} paths to {out_filename}")

    return generated_lists

def generate_sh_file(list_file, run, idx, sh_dir, output_esd_dir, output_root_dir, log_dir, junosw_setup, calib_config):
    """Generate a single .sh file for a given run/index."""
    sh_filename = sh_dir / f"run_rtraw2calib_RUN{run}_{idx}.sh"
    output_esd = Path(output_esd_dir) / f"RUN_{run}_{idx}.esd"
    log_esd = Path(log_dir) / f"RUN_{run}_{idx}.esd.log"
    output_root = Path(output_root_dir) / f"RUN_{run}_{idx}.root"
    log_root = Path(log_dir) / f"RUN_{run}_{idx}.log"

    with open(sh_filename, "w") as sh:
        sh.write("#!/bin/bash\n")
        sh.write("export LC_ALL=C\n")
        sh.write("export CMTCONFIG=amd64_linux26\n\n")
        sh.write("source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/Jlatest/setup.sh\n")
        sh.write(f"source {junosw_setup}\n\n")

        sh.write("python $TUTORIALROOT/share/tut_rtraw2calib.py \\\n")
        sh.write("      --evtmax -1 \\\n")
        sh.write(f"      --input-list {list_file.resolve()} \\\n")
        sh.write(f"      --output {output_esd} \\\n")
        sh.write(f"      --calibstep-config {calib_config} \\\n")
        sh.write("      --pmtcalibsvc-ReadDB 1 --global-tag ReProd_MixedPhase_J25.8 \\\n")
        sh.write("      --pmtcalibsvc-ChargeAlgType 0 \\\n")
        sh.write("      --FPGAToCalibDet OnlyCd --FPGATQoutputEDMmode OEC \\\n")
        sh.write("      --output-stream /Event/CdLpmtCalib_FPGA:on \\\n")
        sh.write("      --output-stream /Event/CdLpmtElec_FPGA:on \\\n")
        sh.write("      --output-stream /Event/CdLpmtCalib_FPGARaw:on \\\n")
        sh.write("      --output-stream /Event/CdTrigger:on \\\n")
        sh.write("      --output-stream /Event/Oec:on \\\n")
        sh.write(f"      >& {log_esd}\n\n")

        sh.write("sleep 5\n\n")

        sh.write(f"python /storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/code/Read_calib/Read_calib.py \\\n")
        sh.write(f"      -input-file {output_esd} \\\n")
        sh.write(f"      -output {output_root} \\\n")
        sh.write(f"      > {log_root}\n")

    sh_filename.chmod(0o755)
    print(f"Wrote {sh_filename}")
    return sh_filename

def write_sub_file(sh_file, run, idx, script_dir):
    sub_dir = script_dir / "sub"
    sub_dir.mkdir(exist_ok=True)

    sub_filename = sub_dir / f"RUN{run}_{idx}.sub"
    log_dir = script_dir / "log"
    out_dir = script_dir / "out"
    err_dir = script_dir / "err"
    log_dir.mkdir(exist_ok=True)
    out_dir.mkdir(exist_ok=True)
    err_dir.mkdir(exist_ok=True)

    log_file = log_dir / f"RUN{run}_{idx}.log"
    out_file = out_dir / f"RUN{run}_{idx}.out"
    err_file = err_dir / f"RUN{run}_{idx}.err"

    with open(sub_filename, "w") as sub:
        sub.write("universe = vanilla\n")
        sub.write(f"executable = {sh_file.resolve()}\n")
        sub.write(f"log = {log_file}\n")
        sub.write(f"output = {out_file}\n")
        sub.write(f"error = {err_file}\n")
        sub.write("+MaxRuntime = 86400\n")
        sub.write("ShouldTransferFiles = YES\n")
        sub.write("WhenToTransferOutput = ON_EXIT\n")
        sub.write("+SingularityImage = false\n")
        sub.write("queue 1\n")

    print(f"Wrote {sub_filename}")
    return sub_filename

def main():
    parser = argparse.ArgumentParser(description="Generate JUNO RTRAW .list, .sh, .sub, and master submit script.")
    parser.add_argument("--version", type=str, default="J25.6.0", help="Software version (default J25.6.0)")
    parser.add_argument("--runlist", type=str, required=True, help="Text file with run numbers")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    list_dir = script_dir / "list_rtraw"
    sh_dir = script_dir / "sh"
    sh_dir.mkdir(exist_ok=True)

    version_dir = BASE_PATH / args.version
    if not version_dir.is_dir():
        print(f"Error: version directory not found: {version_dir}")
        return

    # Step 1: generate list files
    generated_lists = generate_list_files(Path(args.runlist), version_dir, list_dir)
    if not generated_lists:
        print("No .list files generated. Exiting.")
        return

    # Config paths
    JUNOSW_SETUP = "/storage/gpfs_data/juno/junofs/users/ccoletta/junosw/setup.sh"
    CALIB_CONFIG = "/storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/code/rtraw2calib.yaml"
    OUTPUT_ESD_DIR = "/storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/root_esd"
    OUTPUT_ROOT_DIR = "/storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/root"

    sub_files = []

    for list_file in generated_lists:
        m = re.search(r"RUN(\d+)_(\d+)\.list$", list_file.name)
        if not m:
            continue
        run, idx = m.groups()

        sh_file = generate_sh_file(list_file, run, idx, sh_dir, OUTPUT_ESD_DIR, OUTPUT_ROOT_DIR, script_dir / "log", JUNOSW_SETUP, CALIB_CONFIG)
        sub_file = write_sub_file(sh_file, run, idx, script_dir)
        sub_files.append(sub_file)

    # Step 3: master submit script
    c_launch_dir = script_dir / "c_launch"
    c_launch_dir.mkdir(exist_ok=True)
    runlist_stem = Path(args.runlist).stem
    master_sh = c_launch_dir / f"submit_{runlist_stem}.sh"

    with open(master_sh, "w") as f:
        f.write("#!/bin/bash\n\n")
        for sub_file in sub_files:
            batch_name = sub_file.stem
            f.write(f"condor_submit -spool -name sn01-htc.cr.cnaf.infn.it -batch-name {batch_name} {sub_file.resolve()}\n")

    master_sh.chmod(0o755)
    print(f"Wrote master submit script: {master_sh}")

if __name__ == "__main__":
    main()
