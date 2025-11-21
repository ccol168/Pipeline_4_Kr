import ROOT
import glob
import os
import argparse
from array import array

def main():
    parser = argparse.ArgumentParser(
        description="Merge CdEvents trees with DelayedNHits > 600 and sum RunInfo values."
    )
    parser.add_argument(
        "folder",
        help="Path to the folder containing RUNxxxxx_yyy_analysis.root files"
    )
    args = parser.parse_args()

    folder = args.folder
    if not os.path.isdir(folder):
        print(f"Error: '{folder}' is not a valid directory.")
        return

    input_pattern = os.path.join(folder, "RUN?????_???_analysis.root")
    input_files = sorted(glob.glob(input_pattern))
    if not input_files:
        print(f"No matching ROOT files found in {folder}")
        return

    # --- Output file ---
    output_path = os.path.join(folder, "summary.root")
    output_file = ROOT.TFile(output_path, "RECREATE")

    # --- Merge CdEvents trees with cut ---
    print("Building TChain for CdEvents ...")
    chain = ROOT.TChain("CdEvents")
    for f in input_files:
        chain.Add(f)

    print("Copying entries with DelayedNHits > 200 ...")
    merged_tree = chain.CopyTree("DelayedNHits > 200 && DelayedNHits < 1600")

    output_file.cd()
    merged_tree.Write("CdEvents")

    print(f"Merged CdEvents tree written with {merged_tree.GetEntries()} entries")

    # --- Sum RunInfo trees ---
    total_muons_sum = 0.0
    bad_events_sum = 0.0
    live_time_sum = 0.0

    print("Summing RunInfo trees ...")
    for filename in input_files:
        infile = ROOT.TFile.Open(filename)
        if not infile or infile.IsZombie():
            print(f"Could not open {filename}")
            continue

        run_tree = infile.Get("RunInfo")
        if not run_tree:
            print(f"No RunInfo tree in {filename}")
            infile.Close()
            continue

        run_tree.GetEntry(0)
        total_muons_sum += getattr(run_tree, "TotalMuons", 0)
        bad_events_sum += getattr(run_tree, "BadEvents", 0)
        live_time_sum += getattr(run_tree, "LiveTime", 0)

        infile.Close()

    # --- Write merged RunInfo tree ---
    runinfo_tree = ROOT.TTree("RunInfo", "Summed run information")

    total_muons = array('f', [total_muons_sum])
    bad_events = array('f', [bad_events_sum])
    live_time = array('f', [live_time_sum])

    runinfo_tree.Branch("TotalMuons", total_muons, "TotalMuons/F")
    runinfo_tree.Branch("BadEvents", bad_events, "BadEvents/F")
    runinfo_tree.Branch("LiveTime", live_time, "LiveTime/F")

    runinfo_tree.Fill()
    runinfo_tree.Write()
    output_file.Close()

    print(f"Summary file created: {output_path}")
    print(f"  TotalMuons sum = {total_muons_sum}")
    print(f"  BadEvents sum  = {bad_events_sum}")
    print(f"  LiveTime sum   = {live_time_sum}")

if __name__ == "__main__":
    main()
