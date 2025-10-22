#define SNIPER_VERSION_2 1
#include "Read_calib.h"
#include "Identifier/IDService.h"
#include "BufferMemMgr/IDataMemMgr.h"
#include "EvtNavigator/NavBuffer.h"
#include "EvtNavigator/EvtNavHelper.h"
#include "SniperKernel/AlgFactory.h"
#include "SniperKernel/SniperLog.h"
#include "Event/SimHeader.h"
#include "Event/CdLpmtElecHeader.h"
#include "Event/CdLpmtElecEvt.h"
#include "Event/CdLpmtCalibHeader.h"
#include "Event/CdLpmtCalibEvt.h"
#include "Event/CdTriggerHeader.h"
#include "Event/CdTriggerEvt.h"
#include "Event/WpCalibHeader.h"
#include "Event/WpCalibEvt.h"
#include "Event/WpTriggerHeader.h"
#include "Event/WpTriggerEvt.h"
#include "Event/CdVertexRecHeader.h"
#include "Event/CdVertexRecEvt.h"
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
	events->Branch("FirstHitTime",&first_hittime);
	events->Branch("SubtractedTime",&sub_hittime);
	events->Branch("ElecTime",&elec_time);
	events->Branch("ElecCharge",&elec_charge);
	events->Branch("RawTime",&raw_time);

    return true;
}

bool Read_calib::execute() {

	LogInfo << "=====================================" << std::endl;
	LogInfo << "executing: " << m_iEvt++ << std::endl;

	JM::CdLpmtCalibEvt* calibevent = 0;
	JM::CdLpmtElecEvt* elecevent = 0;
        JM::CdLpmtCalibEvt* rawcalibevent = 0;

	auto nav = m_buf->curEvt();

	if (m_iEvt == 1) {
		LogDebug << "FirstTimeStamp = " << (nav->TimeStamp()).AsString() << endl;
	}

	auto calibheader = JM::getHeaderObject<JM::CdLpmtCalibHeader>(nav,"/Event/CdLpmtCalib_FPGA");
	if (calibheader) calibevent = (JM::CdLpmtCalibEvt*)calibheader->event();

	auto rawcalibheader = JM::getHeaderObject<JM::CdLpmtCalibHeader>(nav,"/Event/CdLpmtCalib_FPGARaw");
        if (rawcalibheader) rawcalibevent = (JM::CdLpmtCalibEvt*)rawcalibheader->event();

	auto elecheader = JM::getHeaderObject<JM::CdLpmtElecHeader>(nav,"/Event/CdLpmtElec_FPGA");
	if (elecheader) elecevent = (JM::CdLpmtElecEvt*)elecheader->event();

	if (!calibevent || !elecevent || !rawcalibevent) {
		LogInfo << "No CalibEvt or ElecEvt found, skipping..." << std::endl;
		return true;
	}

	if (rawcalibevent && calibevent && elecevent) {
	
		charge.clear();
		time.clear();
		PMTID.clear();
		first_hittime.clear();
		sub_hittime.clear();
		raw_time.clear();

		for (const auto& element : calibevent->calibPMTCol()) {
	
			for (auto pmtChannel : element -> charge() ) {
				charge.push_back(pmtChannel);
				int PmtNo = idServ->id2CopyNo(Identifier(element->pmtId()));
				PMTID.push_back(PmtNo);
			}
			bool FirstFlag = false;
			double FirstTime;
			for (auto pmtChannel : element -> time() ) {
				if (FirstFlag == false) {
					FirstTime = pmtChannel;
					FirstFlag = true;
				}
				time.push_back(pmtChannel);
				sub_hittime.push_back(pmtChannel-FirstTime);
			}

			first_hittime.push_back(element->firstHitTime());

		}

		for (const auto& element : rawcalibevent->calibPMTCol()) {

                        for (auto pmtChannel : element -> time() ) {
                                raw_time.push_back(pmtChannel);
                                int PmtNo = idServ->id2CopyNo(Identifier(element->pmtId()));
                                PMTID.push_back(PmtNo);
                        }

                }


		elec_charge.clear();
		elec_time.clear();

		for (const auto& kv : elecevent->channelData()) {
			// kv is std::pair<const int, JM::ElecChannel*>
			JM::ElecChannel* ch = kv.second;
			if (!ch) continue;
			// push all times
			for (const auto& t : ch->time()) {
				elec_time.push_back(t);
			}
			// push all charges
			for (const auto& q : ch->charge()) {
				elec_charge.push_back(q);
			}
		}

		events->Fill();
		cdEvtID++;

	}

	return true;
}

bool Read_calib::finalize() {

    return true;
    
}
