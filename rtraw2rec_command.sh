#!/bin/bash
export LC_ALL=C
export CMTCONFIG=amd64_linux26

source /cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/Jlatest/setup.sh

python {$TUTORIALROOT}/share/tut_rtraw2rec.py \
      --evtmax 5 \
      --input /storage/gpfs_data/juno/junofs/production/storm/dirac/juno/juno-rtraw/J25.5.0/global_trigger/00010000/00010500/10554/RUN.10554.JUNODAQ.Physics.ds-2.global_trigger.20251009140223.002_J25.5.0.rtraw \
      --output storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/test.root \
      --calibstep-config /storage/gpfs_data/juno/junofs/users/ccoletta/Pipeline_4_Kr/code/rtraw2calib.yaml \
      --pmtcalibsvc-ChargeAlgType 0 \
      --pmtcalibsvc-ReadDB 1 --global-tag ReProd_MixedPhase_J25.8 \
      --method steering-v2 \
      --steering oec \
      --use-mixedphase-in-steering \
      --recstep-config $TUTORIALROOT/share/RecConfigs/evtrec_reprod_wi_jvertex_steering_using_OEC.yaml \
      --output-stream /Event/CdVertexRecOMILREC:on \
      --output-stream /Event/CdVertexRecMixedPhase:on \
      --output-stream /Event/CdVertexRecJVertex:on \
      --output-stream /Event/CdTrackRecClassify:on \
      --output-stream /Event/WpRec:on \
      --output-stream /Event/Oec:on \
      --fullLSmode \
      --SignalWindowL 420 \
      --LPMTCalibEnergy 2.223 \
      --RfrIndxLS 1.63 \
      --RfrIndxWR 1.63 \
      --enableRunByRunDCR \
      --enableHybridTimePDF \
      --RecMapFile "LnPEMapFile_Ge68_20250824_fullLS_J25.5.0.root" \
      --TimePdfFile "TimePdfR3File_118rbin_ACU_CLS_hybrid_20250824_fullLS_J25.5.0.root" \
      --TimePdfR3File "TimePdfR3File_118rbin_fullLS.txt" \
      --AvgQPdfFile "AvgNPEQpdf_20250823_Run9417_fullLS.root" \
      --JVertexPDFPath /cvmfs/juno.ihep.ac.cn/dbdata/main/dbdata/offline-data/Reconstruction/QCtrRecAlg/JVertexTables/pdf_multipe_coincidences_AmC_prompt6MeV_9607_hama_smoothed.root \
      --JVertexNeff 1.60 \
      --JVertexPmtType Dynode \
      --JVertexAddTOCorrection /cvmfs/juno.ihep.ac.cn/dbdata/main/dbdata/offline-data/Reconstruction/QCtrRecAlg/JVertexTables/LSPhase-CalibrationConstant-Run9417-AmCCorrected.txt
