import argparse
from pathlib import Path
import re
import sys

DEFAULT_ROOT_DIR = Path("/storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/root")
DEFAULT_ANALYSIS_CODE = Path("/storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/code/Process_waveform/Process_waveform.x")
DEFAULT_ANALYSIS_OUTDIR = Path("/storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/analysis")

JUNO_CVMFS_SETUP = "/cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/Jlatest/setup.sh"


def setup_dirs(base: Path):
    for d in ["sh", "sub", "log", "out", "err", "c_launch"]:
        (base / d).mkdir(exist_ok=True)
    DEFAULT_ANALYSIS_OUTDIR.mkdir(parents=True, exist_ok=True)


def write_sh(sh_file: Path, inroot: Path, outroot: Path, analysis_code: Path, log: Path):
    with open(sh_file, "w") as f:
        f.write("#!/bin/bash\n")
        f.write("export LC_ALL=C\n")
        f.write("export CMTCONFIG=amd64_linux26\n\n")
        f.write(f"source {JUNO_CVMFS_SETUP}\n\n")
        f.write(f"{analysis_code} {inroot} CdEvents {outroot} > {log}\n")
    sh_file.chmod(0o755)


def write_sub(sub_file: Path, sh_file: Path, log: Path, out: Path, err: Path):
    with open(sub_file, "w") as f:
        f.write("universe = vanilla\n")
        f.write(f"executable = {sh_file}\n")
        f.write(f"log = {log}\n")
        f.write(f"output = {out}\n")
        f.write(f"error = {err}\n")
        f.write("+MaxRuntime = 86400\n")
        f.write("ShouldTransferFiles = YES\n")
        f.write("WhenToTransferOutput = ON_EXIT\n")
        f.write("+SingularityImage = false\n")
        f.write("queue 1\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--runlist", required=True, type=Path)
    parser.add_argument("--root-dir", default=DEFAULT_ROOT_DIR, type=Path)
    parser.add_argument("--analysis-code", default=DEFAULT_ANALYSIS_CODE, type=Path)

    args = parser.parse_args()
    basedir = Path(__file__).resolve().parent
    setup_dirs(basedir)

    with open(args.runlist) as f:
        runs = [line.strip() for line in f if line.strip().isdigit()]

    if not runs:
        print("No valid run numbers in runlist. Exiting.")
        sys.exit(1)

    jobs = []
    for run in runs:
        files = sorted(args.root_dir.glob(f"RUN_{run}_*.root"))
        if not files:
            print(f"Warning: no ROOT files found for run {run}")
            continue

        for rf in files:
            m = re.match(r"RUN_?\d+_(\d+)", rf.stem)
            idx = m.group(1) if m else "0"

            tag = f"RUN{run}_{idx}"
            outroot = DEFAULT_ANALYSIS_OUTDIR / f"{tag}_analysis.root"

            sh_file = basedir / "sh" / f"analysis_{tag}.sh"
            log_file = basedir / "log" / f"analysis_{tag}.log"
            sub_file = basedir / "sub" / f"analysis_{tag}.sub"

            write_sh(sh_file, rf, outroot, args.analysis_code, log_file)
            write_sub(sub_file, sh_file, log_file, basedir / "out" / f"{tag}.out", basedir / "err" / f"{tag}.err")

            jobs.append((run, idx, sub_file))

            print(f"Prepared job: {rf.name} -> {outroot.name}")

    if not jobs:
        print("No jobs created. Exiting.")
        sys.exit(1)

    master = basedir / "c_launch" / f"submit_analysis_{args.runlist.stem}.sh"
    with open(master, "w") as f:
        for run, idx, sf in jobs:
            batch = f"analysis_RUN{run}_{idx}"
            f.write(f"condor_submit -spool -name sn01-htc.cr.cnaf.infn.it -batch-name {batch} {sf}\n")
    master.chmod(0o755)

    print(f"\nMaster submit script: {master}")
    print("DONE")


if __name__ == "__main__":
    main()
