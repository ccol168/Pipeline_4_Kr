#ifndef READ_CALIB_H
#define READ_CALIB_H

#include "SniperKernel/AlgBase.h"
#include "EvtNavigator/NavBuffer.h"
#include "Identifier/IDService.h"
#include <fstream>
#include <map>
#include <vector>
#include <TSpectrum.h>
#include <TString.h>

class TH1F;
class TTree;

class Read_calib : public AlgBase {
    public :
        Read_calib() : Read_calib("Read_calib") {}
        Read_calib(const std::string& name); // Constructor, must have same name as the class
        virtual ~Read_calib();               // declare destructor (defined out-of-line in .cxx)

    // Following functions are needed by SNiPER, so they are mandatory
        bool initialize();
        bool execute();
        bool finalize();

    private :

        int m_iEvt, NPeaks; // To count the loops
        JM::NavBuffer* m_buf; // Our buffer with the events
		int cdEvtID = 0;        
		// Define variables that are globally used
		IDService* idServ;
		TTimeStamp timestamp, last_muon_timestamp;
		float total_npe, my_total_npe, TimeSinceLastMuon = -1;
		std::vector<int> PMTID;
		std::vector<double> charge ,time, corr_time, first_hittime, sub_hittime, raw_time;
		std::vector<uint64_t> elec_time,elec_charge;
		TString trigger_type; //, wptrigger_type;
		float CdRecox, CdRecoy, CdRecoz, CdRecoenergy, PEBi, PEPo;
		float Interface_level;

		TTree* events ;
		TTree* summaryTree ;
		int nMuonsTotal = 0;
		double runLength = 0.0;
		TTimeStamp minEventTimestamp, maxEventTimestamp;
		std::vector<double> KernelVector;
		TSpectrum spectrum;

		int nMuons = 0; // Counter for detected muons

};

#endif
