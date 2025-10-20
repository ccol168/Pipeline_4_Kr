#!/bin/bash
export LC_ALL=C
export CMTCONFIG=amd64_linux26

source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/J25.6.0/setup.sh

python $TUTORIALROOT/share/tut_rtraw2calib.py \
      --evtmax -1 \
      --input /storage/gpfs_data/juno/junofs/production/storm/dirac/juno/juno-rtraw/J25.5.0/global_trigger/00010000/00010500/10554/RUN.10554.JUNODAQ.Physics.ds-2.global_trigger.20251009140223.002_J25.5.0.rtraw \
      --output /storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/root/test.root \
      --calibstep-config /storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/code/rtraw2calib.yaml \
      --pmtcalibsvc-ReadDB 1 --global-tag ReProd_MixedPhase_J25.8 \
      --pmtcalibsvc-ChargeAlgType 0 \
      --FPGAToCalibDet OnlyCd --FPGATQoutputEDMmode OEC \
      --calibstep-outputedm CD-LPMT-FPGATQ:/Event/CdLpmtCalib_FPGA \
      --output-stream /Event/CdLpmtCalib_FPGA:on \
      --output-stream /Event/CdLpmtElec_FPGA:on \
      >& /storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/log/Test.log
