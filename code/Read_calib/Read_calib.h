#ifndef READ_CALIB_H
#define READ_CALIB_H

#include "SniperKernel/AlgBase.h"
#include "EvtNavigator/NavBuffer.h"
#include "Identifier/IDService.h"
#include "OECTagSvc/OECTagSvc.h"
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
		OECTagSvc* m_tagsvc;

		int cdEvtID = 0;        
		// Define variables that are globally used
		IDService* idServ;
		TTimeStamp timestamp;

		TTimeStamp LastMuon;

		TTimeStamp FirstTimeStamp, LastTimeStamp;

		double DeadTime = 0., TimeSinceLastMuon;

		std::vector<float> time, charge;
		std::vector <int> PMTID;
		std::string TriggerType;

		float NPE, OECRecoX, OECRecoY, OECRecoZ, LiveTime;
		int OECMuonTag, NHits;

		TTree* events ;
		TTree* runinfo;
};

#endif
