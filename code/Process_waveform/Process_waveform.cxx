#include "JUNO_PMTs.h"

#include <numeric>
#include <TSpectrum.h>
#include <TFile.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <cmath>
#include <vector>
#include <tuple>

#include "TH1F.h"
#include "TTree.h"
#include "TTimeStamp.h"
#include "TGraph.h"

const int BinsNumber = 700;
const double MinBinTime = 0.;
const double MaxBinTime = 4200.;
const int PaddedBins = 750;

const float FindPeakThreshold = 5.;
const int MaxBufferSize = 20;
const float MultOverBaseline = 7.;
const float MinPeakHeight = 50.;
const int MinimumDistance = 33;

std::vector<int> MakeHistogram (std::vector <float> inVec) {

	
	double BinSize = (MaxBinTime - MinBinTime) / BinsNumber;
	std::vector <int> Histogram(BinsNumber,0);

    for (auto element : inVec) {
        int BinIndex = (element - MinBinTime) / BinSize ;
        if (BinIndex < 0) BinIndex = 0;
        if (BinIndex >= BinsNumber) BinIndex = BinsNumber - 1;
        Histogram[BinIndex]++;
    }

	return Histogram;
}	

std::vector <double> GetKernel (const char* filename) {
	TFile* file = new TFile(filename, "READ");
		if (!file || file->IsZombie()) {
		std::cout << "Error opening file" << std::endl;
		return {};
		}
	TH1F* hist = nullptr;
	file->GetObject("histo", hist);

	if (!hist) {
		std::cout << "Histogram named histo not found" << std::endl;
		file->Close();
		return {};
	}

	std::vector<double> entries;
	int nBins = hist->GetNbinsX();
	for (int i = 1; i <= nBins; ++i) {
		entries.push_back(hist->GetBinContent(i));
	}

	file->Close();
	return entries;
}

double distance (float x, float y, float z, float x1, float y1, float z1) {
	return sqrt(pow(x-x1,2)+pow(y-y1,2)+pow(z-z1,2));
}

tuple<float,float,float> Intersection (float x_int, float y_int, float z_int, float x_PMT, float y_PMT, float z_PMT, float Interface_level) {

    float t = (Interface_level - z_int) / (z_PMT - z_int) ;
    float x_on_interface = x_int + (x_PMT - x_int) * t;
    float y_on_interface = y_int + (y_PMT - y_int) * t;

    return std::make_tuple(x_on_interface,y_on_interface,Interface_level);
}

double calculate_ToF (float x_int, float y_int, float z_int, float x_PMT, float y_PMT, float z_PMT) {

    float n_water = 1.29;
    float n_scint = 1.54;
    float c = 299.792 ; // mm/ns


    return distance(x_int,y_int,z_int,x_PMT,y_PMT,z_PMT)/c * n_scint;
}

void SelectPeaks (const vector <int>& CorrTimesHistogram, const vector <double>& KernelVector, TSpectrum& spectrum, int BinsNumber,
				  vector<int>* PeakPositions_out, vector<int>* DeconvolutedSignal_out) {

	std::vector<double> paddedWaveform(PaddedBins, 0.0);
	std::vector<double> paddedKernel(PaddedBins, 0.0);

	//padding Kernel

	for (size_t i=0; i < PaddedBins; i++) {
		if (i < KernelVector.size()) {
			paddedKernel[i] = KernelVector[i];
		} else {
			paddedKernel[i] = 0;
		}
	}

	// padding Waveform

	for (size_t i=0; i < PaddedBins; i++) {
		if (i < (PaddedBins - CorrTimesHistogram.size())) {
			paddedWaveform[i] = 0;
		} else {
			paddedWaveform[i] = CorrTimesHistogram[i-(PaddedBins - CorrTimesHistogram.size())];
		}
	}

	spectrum.Clear();
	spectrum.Deconvolution(paddedWaveform.data(), paddedKernel.data(), PaddedBins, 1000, 1, 1);

	for (int i=0; i<CorrTimesHistogram.size(); i++) {
		DeconvolutedSignal_out -> push_back(paddedWaveform[i+(PaddedBins - CorrTimesHistogram.size())]);
	}

	std::vector <int> Buffer;
	std::vector <std::pair<int,int>> PeaksFeatures;

	for (int j=1; j<BinsNumber; j++) {
		if(DeconvolutedSignal_out -> at(j-1) < FindPeakThreshold && DeconvolutedSignal_out -> at(j) >= FindPeakThreshold) {
			Buffer.clear();
			while(j<BinsNumber) {
				Buffer.push_back(DeconvolutedSignal_out -> at(j));
				if (DeconvolutedSignal_out -> at(j) < FindPeakThreshold) break;
				j++;
			}
			if (Buffer.size() > MaxBufferSize) {
				return;
			}
			auto MaxItr = std::max_element(Buffer.begin(),Buffer.end());
			int MaxIndex = std::distance(Buffer.begin(),MaxItr);
			PeaksFeatures.emplace_back(*MaxItr,j-Buffer.size()+MaxIndex+1);
		}

	}

	if (PeaksFeatures.size() == 0) return;

	std::vector <std::pair <int,int>> SelectedPeakPositions;
	std::pair <int,int> Provv;
	bool FirstCycleFlag = true;
	double sum, mean;

	auto AbsMax = std::max_element(PeaksFeatures.begin(),PeaksFeatures.end(),[](const std::pair <int,int>&a , const std::pair <int,int>& b){return a.first < b.first;});
	std::pair<int,int> ProvvMax = {AbsMax->first,AbsMax->second};
	PeaksFeatures.erase(AbsMax);

	while (PeaksFeatures.size() > 0) {
		auto maxIterator = std::max_element(PeaksFeatures.begin(),PeaksFeatures.end(),[](const std::pair <int,int>&a , const std::pair <int,int>& b){return a.first < b.first;});
		Provv = {maxIterator->first,maxIterator->second};
		PeaksFeatures.erase(maxIterator);
		sum = std::accumulate(PeaksFeatures.begin(),PeaksFeatures.end(),0.0,[](double acc, const std::pair <int,int>&a) {return acc + a.first;});
		if (PeaksFeatures.size() != 0) mean = sum/PeaksFeatures.size();
		else mean = 0.;
		if (FirstCycleFlag) {
			if (ProvvMax.first > mean*MultOverBaseline && ProvvMax.first > MinPeakHeight) {
				SelectedPeakPositions.emplace_back(ProvvMax.first,ProvvMax.second);
				FirstCycleFlag = false;
			} else break;
		}
		if (Provv.first > mean*MultOverBaseline && Provv.first > MinPeakHeight) {
			SelectedPeakPositions.emplace_back(Provv.first,Provv.second);          
		} else break;
	}

	std::sort(SelectedPeakPositions.begin(),SelectedPeakPositions.end(),[](const std::pair <int,int>&a, const std::pair <int,int>&b){return a.second < b.second;});

	for (int j=1; j<SelectedPeakPositions.size();j++) {
		if (SelectedPeakPositions[j].second-SelectedPeakPositions[j-1].second < MinimumDistance) {
			if(SelectedPeakPositions[j].first > SelectedPeakPositions[j-1].first) {
				SelectedPeakPositions.erase(SelectedPeakPositions.begin() + (j-1));
			} else {
				SelectedPeakPositions.erase(SelectedPeakPositions.begin() + j);
			}
			j--;
		}
	}

	for(int j=0; j<SelectedPeakPositions.size(); j++) {
		PeakPositions_out -> push_back(SelectedPeakPositions[j].second);
	}

	return ;
} 

void execute (string filename, string treename, string outfilename) {


    TSpectrum spectrum;

    JUNO_PMTs PMTs_Pos;
    PMTs_Pos.SetCdPmts("/cvmfs/juno.ihep.ac.cn/el9_amd64_gcc11/Release/Jlatest/data/Detector/Identifier/pmt_CDLPMT_latest.csv");

    std::vector <double> KernelVector = GetKernel("/storage/gpfs_data/juno/junofs/users/ccoletta/Kernel_for_Deconvolution/Kernel.root");

    
    // --- Prepare output file and tree ---
    TFile *file_out = TFile::Open(outfilename.c_str(), "RECREATE");
    TTree *out_tree = new TTree("CdEvents", "Prompt-Delayed Couples Identified");

	TTimeStamp TimeStamp_out;
	float RecoX_out, RecoY_out, RecoZ_out;
	float NPE_out;
	int NHits_out;
	double TimeSinceLastMuon_out;
	int PeakNumber_out, EvtID_out = 0;
	float PromptPE, DelayedPE;
	int PromptNHits, DelayedNHits;
	std::vector<int>    *PMTID_out = new std::vector<int>();
    std::vector<float>  *Time_out = new std::vector<float>();
    std::vector<float>  *Charge_out = new std::vector<float>();
    std::vector<float>  *CorrTime_out  = new std::vector<float>();
	std::vector<int>  *DeconvolutedWaveform_out  = new std::vector<int>();
	std::vector<int>  *PeakPositions_out  = new std::vector<int>();
    
	out_tree->Branch("EvtID",&EvtID_out,"EvtID/I");
	out_tree->Branch("TimeStamp", &TimeStamp_out);
    out_tree->Branch("RecoX", &RecoX_out, "RecoX/F");
    out_tree->Branch("RecoY", &RecoY_out, "RecoY/F");
    out_tree->Branch("RecoZ", &RecoZ_out, "RecoZ/F");
    out_tree->Branch("TimeSinceLastMuon", &TimeSinceLastMuon_out, "TimeSinceLastMuon/D");
    out_tree->Branch("NPE", &NPE_out, "NPE/F");
    out_tree->Branch("NHits", &NHits_out, "NHits/I");
	out_tree->Branch("Time",&Time_out);
	out_tree->Branch("Charge",&Charge_out);
	out_tree->Branch("PMTID",&PMTID_out);
	out_tree->Branch("CorrTime",&CorrTime_out);
	out_tree->Branch("DeconvolutedWaveform",&DeconvolutedWaveform_out);
	out_tree->Branch("PeakPositions",&PeakPositions_out);
	out_tree->Branch("PeakNumber",&PeakNumber_out,"PeakNumber/I");
	out_tree->Branch("PromptPE",&PromptPE,"PromptPE/F");
	out_tree->Branch("DelayedPE",&DelayedPE,"DelayedPE/F");
	out_tree->Branch("PromptNHits",&PromptNHits,"PromptNHits/I");
	out_tree->Branch("DelayedNHits",&DelayedNHits,"DelayedNHits/I");

	TTree *out_runinfo = new TTree("RunInfo", "Run informations");
	int nMuonsTotal = 0;
	int nBadEvents = 0;
	double LiveTime;

	out_runinfo->Branch("TotalMuons",&nMuonsTotal,"TotalMuons/I");
	out_runinfo->Branch("BadEvents",&nBadEvents,"BadEvents/I");
	out_runinfo->Branch("LiveTime",&LiveTime,"LiveTime/D");
    
	TFile *fin = new TFile (filename.c_str());
    TTree* intree = (TTree*)fin->Get(treename.c_str());

    if (!intree) {
        std::cerr << "Error: Tree " << treename << " not found in file " << filename << std::endl;
        return;
    }

	int    EvtID = 0;
    TTimeStamp* TimeStamp = nullptr;
    std::vector<float>* Time   = nullptr;
    std::vector<float>* Charge = nullptr;
    std::vector<int>* PMTID    = nullptr;
    
    int OECMuonTag = 0;
    double TimeSinceLastMuon = 0;
    float NPE = 0;
    int NHits = 0;
    float OECRecoX = 0, OECRecoY = 0, OECRecoZ = 0;
    std::string* TriggerType = nullptr;

    // Set branch addresses
    intree->SetBranchAddress("EvtID", &EvtID);
    intree->SetBranchAddress("TimeStamp", &TimeStamp);
    intree->SetBranchAddress("Time", &Time);
    intree->SetBranchAddress("Charge", &Charge);
    intree->SetBranchAddress("PMTID", &PMTID);
    intree->SetBranchAddress("OECMuonTag", &OECMuonTag);
    intree->SetBranchAddress("TimeSinceLastMuon", &TimeSinceLastMuon);
    intree->SetBranchAddress("NPE", &NPE);
    intree->SetBranchAddress("NHits", &NHits);
    intree->SetBranchAddress("OECRecoX", &OECRecoX);
    intree->SetBranchAddress("OECRecoY", &OECRecoY);
    intree->SetBranchAddress("OECRecoZ", &OECRecoZ);
    intree->SetBranchAddress("TriggerType", &TriggerType);


    int TotalEvents = intree -> GetEntries();
    
	TTimeStamp LastMuonTime;
	TTimeStamp FirstEvent;
	bool LastEventWasMuon = false;

    for (int i = 0; i < TotalEvents; i++) {

		if (i % 100000 == 0) {
            cout << "Processing event " << i << " / " << TotalEvents << std::endl;
        }

        intree->GetEntry(i);

		if (i==0) FirstEvent = *TimeStamp;
		if (*TimeStamp < FirstEvent) FirstEvent = *TimeStamp;

		if (*TriggerType != "Multiplicity") continue;

		if (NPE > 30000) {
			if (LastEventWasMuon) {
				LastEventWasMuon = true;
				continue;
			} else {
				LastEventWasMuon = true;
				LastMuonTime = *TimeStamp;
				nMuonsTotal++;
				continue;
			}
		} else {
			LastEventWasMuon = false;
		}

		int SecondDifference = 0;
		if (LastMuonTime.GetSec() == 0 && LastMuonTime.GetNanoSec() == 0) {
			TimeSinceLastMuon_out = -1;
		} else {
			SecondDifference = TimeStamp -> GetSec() - LastMuonTime.GetSec();
			TimeSinceLastMuon_out = (SecondDifference*1.e9 + (TimeStamp -> GetNanoSec() - LastMuonTime.GetNanoSec()))/1.e9;
		}

		float MinimumValue = *std::min_element(Time -> begin(), Time -> end());

		if (MinimumValue > -2500) {
			nBadEvents++;
			continue;
		}

		CorrTime_out -> clear();
		for (size_t j = 0; j < Time -> size(); j++) {
				CorrTime_out -> push_back(Time -> at(j) - 
					calculate_ToF(OECRecoX,OECRecoY,OECRecoZ,
					PMTs_Pos.GetX(PMTID -> at(j)),PMTs_Pos.GetY(PMTID->at(j)),PMTs_Pos.GetZ(PMTID->at(j))) 
				);
		}

		auto MinCorrTime = *std::min_element(CorrTime_out -> begin(),CorrTime_out -> end());

		for (auto& element : *CorrTime_out ) {
			element -= MinCorrTime;
		}

		PeakPositions_out -> clear();
		DeconvolutedWaveform_out -> clear();

		std::vector <int> CorrTimesHistogram = MakeHistogram(*CorrTime_out);

		SelectPeaks(CorrTimesHistogram,KernelVector,spectrum,BinsNumber,PeakPositions_out,DeconvolutedWaveform_out);

		PeakNumber_out = PeakPositions_out -> size();

		PromptPE = 0.;
        DelayedPE = 0.;
		PromptNHits = 0;
		DelayedNHits = 0;

		if (PeakNumber_out == 2) {

            for (int j = 0; j < Charge -> size();j++) {

                if (CorrTime_out -> at(j) < PeakPositions_out -> at(0)*6 + 6*20 && CorrTime_out -> at(j) > PeakPositions_out -> at(0)*6 - 6*10) {
                    PromptPE += Charge -> at(j);
					PromptNHits++;
                }
				if (CorrTime_out -> at(j) < PeakPositions_out -> at(1)*6 + 6*20 && CorrTime_out -> at(j) > PeakPositions_out -> at(1)*6 - 6*10) {
                   	DelayedPE += Charge -> at(j);
					DelayedNHits++;
                }
                                
        	}

		}

			TimeStamp_out = *TimeStamp;
			RecoX_out = OECRecoX;
			RecoY_out = OECRecoY;
			RecoZ_out = OECRecoZ;
			NPE_out = NPE;
			NHits_out = NHits;

			Time_out -> clear();
			Charge_out -> clear();
			PMTID_out -> clear();

			*Time_out = *Time;
			*Charge_out = *Charge;
			*PMTID_out = *PMTID;
			EvtID_out++;
			out_tree -> Fill();
	}

    fin->Close();

	LiveTime = *TimeStamp - FirstEvent;

	out_runinfo -> Fill();

    file_out->cd();
    out_tree->Write();

	out_runinfo -> Write();

    file_out -> Close();

	delete PMTID_out; 
    delete Time_out;
    delete Charge_out;
    delete CorrTime_out;
	delete DeconvolutedWaveform_out;
	delete PeakPositions_out;

    return;

}


int main(int argc, char** argv) {

    string macro = argv[0];

    if(argc!=4) {
            cout << "\n     USAGE:  "<< macro << " Input_Rootfile  In_Tree_Name Out_File_Name \n" << endl;
            return 1;
    }

    string filename = argv[1];
	string intreename = argv[2];
	string outfilename = argv[3];

	execute (filename, intreename, outfilename);

    return 0;
}
