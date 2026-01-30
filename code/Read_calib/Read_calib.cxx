#define SNIPER_VERSION_2 1
#include "Read_calib.h"
#include "Identifier/IDService.h"
#include "BufferMemMgr/IDataMemMgr.h"
#include "EvtNavigator/NavBuffer.h"
#include "EvtNavigator/EvtNavHelper.h"
#include "OECTagID/OECTagID.h"
#include "OECTagSvc/OECTagSvc.h"
#include "OECTagTypes/OECTagTypes.h"
#include "OECConfigSvc/OECConfigSvc.h"
#include "SniperKernel/AlgFactory.h"
#include "SniperKernel/SniperLog.h"
#include "Event/SimHeader.h"
#include "Event/CdLpmtElecHeader.h"
#include "Event/CdLpmtElecEvt.h"
#include "Event/CdLpmtCalibHeader.h"
#include "Event/CdLpmtCalibEvt.h"
#include "Event/CdTriggerHeader.h"
#include "Event/CdTriggerEvt.h"
#include "RootWriter/RootWriter.h"
#include "Event/OecHeader.h"
#include "Event/OecEvt.h"
#include <numeric>
#include <TSpectrum.h>
#include <TFile.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <cmath>
#include <TGraph.h>
#include <TTimeStamp.h>

#include "TH1F.h"
#include "TTree.h"
#include "TParameter.h"

int BinsNumber = 200;

DECLARE_ALGORITHM(Read_calib);

Read_calib::Read_calib(const std::string& name) 
	: AlgBase(name),
	  m_iEvt(0),
	  m_buf(0)
{
}

// Provide an out-of-line destructor so the vtable/type-info are emitted here.
Read_calib::~Read_calib() {}

bool Read_calib::initialize() {

    LogDebug << "initializing" << std::endl;
    auto toptask = getRoot();

    idServ = IDService::getIdServ();
    idServ->init();
	
    SniperDataPtr<JM::NavBuffer> navBuf(getParent(), "/Event");

    if ( navBuf.invalid() ) {
        LogError << "cannot get the NavBuffer @ /Event" << std::endl;
        return false;
    }
	
    m_buf = navBuf.data();

	SniperPtr<OECTagSvc> tagsvc(getParent(),"OECTagSvc");
    if( tagsvc.invalid()) {
        LogError << "Unable to locate tagsvc" << std::endl;
        return false;
    }
    m_tagsvc = tagsvc.data();

    SniperPtr<RootWriter> rw(getParent(), "RootWriter");
    if (rw.invalid()) {
        LogError << "Can't Locate RootWriter. If you want to use it, please "
                 << "enable it in your job option file."
                 << std::endl;
         return false;
    }

    //wp_events_seen = 0;    
    events = rw->bookTree(*m_par,"tree/CdEvents","Events Tree");
    events->Branch("EvtID",&cdEvtID,"EvtID/I");
    events->Branch("TimeStamp",&timestamp);
    events->Branch("Time",&time);
    events->Branch("Charge",&charge);
	events->Branch("PMTID",&PMTID);
	events->Branch("OECMuonTag",&OECMuonTag);
	events->Branch("TimeSinceLastMuon",&TimeSinceLastMuon);
	events->Branch("NPE",&NPE);
	events->Branch("NHits",&NHits);
	events->Branch("OECRecoX",&OECRecoX);
	events->Branch("OECRecoY",&OECRecoY);
	events->Branch("OECRecoZ",&OECRecoZ);
	events->Branch("TriggerType",&TriggerType);

	runinfo = rw->bookTree(*m_par,"tree/RunInfo","Run-level informations");
	runinfo -> Branch("LiveTime",&LiveTime);

    return true;
}

bool Read_calib::execute() {

	LogInfo << "=====================================" << std::endl;
	LogInfo << "executing: " << m_iEvt << std::endl;

	JM::CdLpmtCalibEvt* calibevent = 0;
	JM::OecEvt* oecevent = 0;
	JM::CdTriggerEvt* triggerevent = 0; 

	auto nav = m_buf->curEvt();

	if (m_iEvt == 0) {
		FirstTimeStamp = nav -> TimeStamp();
		LastTimeStamp = nav -> TimeStamp();
		LogDebug << "FirstTimeStamp = " << (nav->TimeStamp()).AsString() << std::endl;
	}

	if ( std::abs(nav -> TimeStamp().GetSec() - LastTimeStamp.GetSec()) > 30.  ) {
		LogInfo << "############# LONG DEAD TIME DETECTED! ##################" << std::endl;
		LogInfo << "Estimated value = " << std::abs(nav -> TimeStamp().GetSec() - LastTimeStamp.GetSec()) << " s" << std::endl;
		DeadTime += std::abs(nav -> TimeStamp().GetSec() - LastTimeStamp.GetSec());
	}

	LastTimeStamp = nav -> TimeStamp();

	auto calibheader = JM::getHeaderObject<JM::CdLpmtCalibHeader>(nav,"/Event/CdLpmtCalib_FPGA");
	if (calibheader) calibevent = (JM::CdLpmtCalibEvt*)calibheader->event();

	auto oecheader = JM::getHeaderObject<JM::OecHeader>(nav,"/Event/Oec");
        if (oecheader) oecevent = (JM::OecEvt*)oecheader->event("JM::OecEvt");

	auto triggerheader = JM::getHeaderObject<JM::CdTriggerHeader>(nav,"/Event/CdTrigger");
	if (triggerheader) triggerevent = (JM::CdTriggerEvt*)triggerheader->event();

	if (!calibevent || !triggerevent || !oecevent) {
		LogInfo << "No CalibEvt, OecEvt or TriggerEvt found, skipping..." << std::endl;
		return true;
	}

	if (calibevent && triggerevent && oecevent) {
	
		charge.clear();
		time.clear();
		PMTID.clear();

		timestamp = LastTimeStamp;
		OECMuonTag = m_tagsvc -> isMuon(oecevent);

		OECRecoX = oecevent -> getVertexX();
		OECRecoY = oecevent -> getVertexY();
		OECRecoZ = oecevent -> getVertexZ();

		if (OECMuonTag == 1) {
			LastMuon = timestamp;
			TimeSinceLastMuon = 0.;
		}

		else {
			TimeSinceLastMuon = static_cast<long double>((timestamp.GetSec() - LastMuon.GetSec()) * 1e9 ) + 
								static_cast<long double>(timestamp.GetNanoSec() - LastMuon.GetNanoSec());
		}

		NHits = 0;
		NPE = 0;
		for (const auto& element : calibevent->calibPMTCol()) {

			NPE += element -> nPE();
	
			for (auto pmtChannel : element -> charge() ) {
				charge.push_back(pmtChannel);
				int PmtNo = idServ->id2CopyNo(Identifier(element->pmtId()));
				PMTID.push_back(PmtNo);
			}

			for (auto pmtChannel : element -> time() ) {
				time.push_back(pmtChannel);
				NHits++;
			}

		}

		TriggerType = (triggerevent -> triggerType())[0];

		events->Fill();
		cdEvtID++;

	}

	m_iEvt++;
	return true;
}

bool Read_calib::finalize() {

	LiveTime = LastTimeStamp.GetSec() - FirstTimeStamp.GetSec() - DeadTime;
	runinfo -> Fill();

    return true;
    
}
