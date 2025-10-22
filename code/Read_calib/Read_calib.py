import sys
import os
import Sniper
import argparse
import numpy as np

# -----------------------------
# Argument parsing
# -----------------------------
prs = argparse.ArgumentParser()
prs.add_argument("-input-file", "--inputfile", help="Single input file")
prs.add_argument("-input-list", "--input", help="Input ESD file list")
prs.add_argument("-output", "--output", required=True, help="Output file")

args = prs.parse_args()

# Check for invalid combinations
if args.inputfile and args.input:
    print("Error: Please provide either --input-file or --input-list, not both.")
    sys.exit(1)

if not args.inputfile and not args.input:
    print("Error: You must provide either --input-file or --input-list.")
    sys.exit(1)

script_dir = os.path.dirname(os.path.abspath(__file__))
outfilename = args.output

# -----------------------------
# Load DLL and create task
# -----------------------------
Sniper.loadDll(script_dir + "/Read_calib_cxx.so")
task = Sniper.Task("task")
task.setLogLevel(1)

alg = task.createAlg("Read_calib")

import BufferMemMgr
bufMgr = task.createSvc("BufferMemMgr")

File = "/cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/Jlatest/junosw/OEC/OECTutorial/share/DummyCommonConfig_1000t.json"
import OECComJSONSvc
oeccomjsonsvc = task.createSvc('OECComJSONSvc')
oeccomjsonsvc.property("OECReadComJSONFrom").set(0)
oeccomjsonsvc.property("OECComJSONFile").set(File)

import OECTagSvc
oectagsvc = task.createSvc('OECTagSvc')

import RootWriter
task.property("svcs").append("RootWriter")
rw = task.find("RootWriter")
rw.property("Output").set({"tree": outfilename})

import RootIOSvc
import RootIOTools
riSvc = task.createSvc("RootInputSvc/InputSvc")

# -----------------------------
# Handle input files
# -----------------------------
if args.inputfile:
    # Single file input
    riSvc.property("InputFile").set([args.inputfile])
else:
    # Input list file
    inputFileNumpy = np.loadtxt(args.input, usecols=(0), unpack=True, dtype=str)
    inputFileList = inputFileNumpy.tolist()
    riSvc.property("InputFile").set(inputFileList)

# -----------------------------
# Run the task
# -----------------------------
task.setEvtMax(-1)
task.show()
task.run()
