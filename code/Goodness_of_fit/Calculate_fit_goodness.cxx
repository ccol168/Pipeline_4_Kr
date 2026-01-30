#include <vector>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <iostream>
#include <cmath>


const int BinsNumber = 700;
const double MinBinTime = 0.;
const double MaxBinTime = 4200.;
const int PaddedBins = 750;
const int BinSize = 6;


double calculatePeakGoodness(const std::vector<float>* corrTimes,
                             TH1F* kernel,
                             int peakBin,           // <-- PeakPosition is a BIN INDEX
                             int kernelPeakBin,
                             int NHits,
                             int binsBefore = 10,
                             int binsAfter  = 20
                            ) {
    if (!corrTimes || !kernel) return -9.0e10;

    TH1F corrTimesHisto ("corrTimesHisto", "CorrTimes histogram",BinsNumber,MinBinTime,MaxBinTime);

    for (float t : *corrTimes)
        corrTimesHisto.Fill(t);

    int BinsToCycle = binsBefore + binsAfter + 1;

    double KernelArea = 0.0;

    for (int i=0 ; i < BinsToCycle; i ++) {

        int binToCheck = kernelPeakBin - binsBefore + i;

        if (binToCheck > 0 && binToCheck <= kernel -> GetNbinsX() ) 
            KernelArea += kernel -> GetBinContent(binToCheck);
    }

    double chi2 = 0.0;
    int ndf = 0;

    for (int i = 0; i < BinsToCycle; i ++) {
        int KernelBin = kernelPeakBin - binsBefore + i;
        int WaveformBin = peakBin - binsBefore + i;

        if (KernelBin > 0 && WaveformBin > 0 && KernelBin <= kernel -> GetNbinsX() && WaveformBin <= BinsNumber ) {
            double exp_value = corrTimesHisto.GetBinContent(WaveformBin);
            double th_value = kernel->GetBinContent(KernelBin) / KernelArea * NHits;

            if (th_value > 0) 
                chi2 += pow(exp_value-th_value,2)/th_value;
            else return -9.0e10;

            ndf++;
        }
        
    }

    return chi2/ndf;
}


void addPeakGoodnessScores(std::string infile, std::string outfile)
{
    // Open input file
    TFile* fin = TFile::Open(infile.c_str());
    if (!fin || fin->IsZombie()) {
        std::cerr << "Cannot open input file!" << std::endl;
        return;
    }
    
    // Get input tree
    TTree* treeIn = (TTree*)fin->Get("CdEvents");
    if (!treeIn) {
        std::cerr << "Tree CdEvents not found!" << std::endl;
        return;
    }
    
    // Open kernel file and get kernel histogram
    const char* kernelfile = "/storage/gpfs_data/juno/junofs/users/ccoletta/Kernel_for_Deconvolution/Kernel_Kr85_delayed.root";
    TFile* fkernel = TFile::Open(kernelfile);
    if (!fkernel || fkernel->IsZombie()) {
        std::cerr << "Cannot open kernel file!" << std::endl;
        return;
    }
    
    TH1F* kernel = (TH1F*)fkernel->Get("h_ideal_delayed");
    if (!kernel) {
        std::cerr << "Kernel histogram 'histo' not found!" << std::endl;
        return;
    }
    
    // Find the peak position in the kernel (maximum bin)
    int kernelPeakBin = kernel->GetMaximumBin();
    float kernelPeakValue = kernel->GetBinContent(kernelPeakBin);
    
    std::cout << "Kernel peak found at bin: " << kernelPeakBin 
              << " with value: " << kernelPeakValue << std::endl;
    std::cout << "Kernel has " << kernel->GetNbinsX() << " bins" << std::endl;
    
    // Set up input branches
    std::vector<float>* CorrTimes = nullptr;
    std::vector<int>* PeakPos = nullptr;
    int PeakNumber, PromptNHits, DelayedNHits;
    
    treeIn->SetBranchAddress("CorrTime", &CorrTimes);
    treeIn->SetBranchAddress("PeakPositions", &PeakPos);
    treeIn->SetBranchAddress("PeakNumber", &PeakNumber);
	treeIn->SetBranchAddress("PromptNHits",&PromptNHits);
	treeIn->SetBranchAddress("DelayedNHits",&DelayedNHits);
    
    // Create output file and clone tree structure
    TFile* fout = TFile::Open(outfile.c_str(), "RECREATE");
    TTree* treeOut = treeIn->CloneTree(0);  // Clone structure, not entries
    
    // Add new branches for goodness scores
    double goodnessScore1 = -999.0;
    double goodnessScore2 = -999.0;
    
    TBranch* br1 = treeOut->Branch("GoodnessScore_Prompt", &goodnessScore1, "GoodnessScore_Prompt/D");
    TBranch* br2 = treeOut->Branch("GoodnessScore_Delayed", &goodnessScore2, "GoodnessScore_Delayed/D");
    
    // Process all events
    Long64_t nEntries = treeIn->GetEntries();
    std::cout << "Processing " << nEntries << " events..." << std::endl;
    
    for (Long64_t i = 0; i < nEntries; i++) {
        treeIn->GetEntry(i);
        
        if (i % 10000 == 0) {
            std::cout << "Processing event " << i << " / " << nEntries << std::endl;
        }
        
        // Reset scores
        goodnessScore1 = -9.0e10;
        goodnessScore2 = -9.0e10;
        
        // Only process double peak events
        if (PeakNumber == 2 && PeakPos->size() >= 2) {
            goodnessScore1 = calculatePeakGoodness(CorrTimes, kernel, 
                                                    PeakPos->at(0), 
                                                    kernelPeakBin, PromptNHits);
            
            goodnessScore2 = calculatePeakGoodness(CorrTimes, kernel, 
                                                    PeakPos->at(1), 
                                                    kernelPeakBin, DelayedNHits );
        }
        
        treeOut->Fill();
    }
    
    // Write and close
    fout->cd();
    treeOut->Write();
    fout->Close();
    fin->Close();
    fkernel->Close();
    
    std::cout << "Done! Output saved to " << outfile << std::endl;
}

int main(int argc, char** argv) {

    std::string macro = argv[0];

    if(argc!=3) {
            std::cout << "\n     USAGE:  "<< macro << " Input_Rootfile  Out_File_Name \n" << std::endl;
            return 1;
    }

    std::string infilename = argv[1];
	std::string outfilename = argv[2];

	addPeakGoodnessScores(infilename,outfilename);

    return 0;
}