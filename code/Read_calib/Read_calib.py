import sys
import os
import Sniper
import argparse
import numpy as np

prs = argparse.ArgumentParser()
prs.add_argument('-input-list', '--input', help='Input esd file list')
prs.add_argument('-output', '--output', help='output file')

args = prs.parse_args()
script_dir = os.path.dirname(os.path.abspath(__file__))

outfilename = args.output
listname = args.input

Sniper.loadDll(script_dir + "/Read_calib_cxx.so")
#Sniper.loadDll("libSimEvent.so")

task = Sniper.Task("task")
task.setLogLevel(1)

alg = task.createAlg("Read_calib")

import BufferMemMgr
bufMgr = task.createSvc("BufferMemMgr")

import OECTagSvc
oectagsvc = task.createSvc('OECTagSvc')

import RootWriter
task.property("svcs").append("RootWriter")
rw = task.find("RootWriter")
rw.property("Output").set({"tree":outfilename})

import RootIOSvc
import RootIOTools
riSvc = task.createSvc("RootInputSvc/InputSvc")
inputFileNumpy = np.loadtxt(listname,usecols=(0),unpack=True,dtype=str)
inputFileList = inputFileNumpy.tolist()
riSvc.property("InputFile").set(inputFileList)

task.setEvtMax(-1)
task.show()
task.run()
