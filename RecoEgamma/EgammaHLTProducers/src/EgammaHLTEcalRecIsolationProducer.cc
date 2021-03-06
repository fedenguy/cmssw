
//*****************************************************************************
// File:      EgammaHLTEcalRecIsolationProducer.cc
// ----------------------------------------------------------------------------
// OrigAuth:  Matthias Mozer , adapted from EgammaHcalIsolationProducer by S. Harper
// Institute: IIHE-VUB
//=============================================================================
//*****************************************************************************


#include "RecoEgamma/EgammaHLTProducers/interface/EgammaHLTEcalRecIsolationProducer.h"


// Framework
//#include "FWCore/Framework/interface/ESHandle.h"
//#include "FWCore/MessageLogger/interface/MessageLogger.h"
//#include "FWCore/Utilities/interface/Exception.h"
#include "DataFormats/RecoCandidate/interface/RecoEcalCandidateIsolation.h"

#include "DataFormats/EgammaReco/interface/SuperCluster.h"
//nclude "DataFormats/EgammaReco/interface/SuperClusterFwd.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"

#include "RecoLocalCalo/EcalRecAlgos/interface/EcalSeverityLevelAlgoRcd.h"

EgammaHLTEcalRecIsolationProducer::EgammaHLTEcalRecIsolationProducer(const edm::ParameterSet& config) : conf_(config) {
  // use configuration file to setup input/output collection names
  //inputs
  recoEcalCandidateProducer_      = consumes<reco::RecoEcalCandidateCollection>(conf_.getParameter<edm::InputTag>("recoEcalCandidateProducer"));
  ecalBarrelRecHitProducer_       = consumes<EcalRecHitCollection>(conf_.getParameter<edm::InputTag>("ecalBarrelRecHitProducer"));
  ecalEndcapRecHitProducer_       = consumes<EcalRecHitCollection>(conf_.getParameter<edm::InputTag>("ecalEndcapRecHitProducer"));
  //ecalBarrelRecHitCollection_     = conf_.getParameter<edm::InputTag>("ecalBarrelRecHitCollection");
  //ecalEndcapRecHitCollection_     = conf_.getParameter<edm::InputTag>("ecalEndcapRecHitCollection");
  rhoProducer_                    = consumes<double>(config.getParameter<edm::InputTag>("rhoProducer"));

  doRhoCorrection_                = config.getParameter<bool>("doRhoCorrection");
  rhoMax_                         = config.getParameter<double>("rhoMax"); 
  rhoScale_                       = config.getParameter<double>("rhoScale"); 

  //vetos
  egIsoPtMinBarrel_               = conf_.getParameter<double>("etMinBarrel");
  egIsoEMinBarrel_                = conf_.getParameter<double>("eMinBarrel");
  egIsoPtMinEndcap_               = conf_.getParameter<double>("etMinEndcap");
  egIsoEMinEndcap_                = conf_.getParameter<double>("eMinEndcap");
  egIsoConeSizeInBarrel_          = conf_.getParameter<double>("intRadiusBarrel");
  egIsoConeSizeInEndcap_          = conf_.getParameter<double>("intRadiusEndcap");
  egIsoConeSizeOut_               = conf_.getParameter<double>("extRadius");
  egIsoJurassicWidth_             = conf_.getParameter<double>("jurassicWidth");
  effectiveAreaBarrel_            = config.getParameter<double>("effectiveAreaBarrel");
  effectiveAreaEndcap_            = config.getParameter<double>("effectiveAreaEndcap");
  
  // options
  useIsolEt_ = conf_.getParameter<bool>("useIsolEt");
  tryBoth_   = conf_.getParameter<bool>("tryBoth");
  subtract_  = conf_.getParameter<bool>("subtract");
  useNumCrystals_ = conf_.getParameter<bool>("useNumCrystals");
  
  //register your products
  produces < reco::RecoEcalCandidateIsolationMap >();  
}

EgammaHLTEcalRecIsolationProducer::~EgammaHLTEcalRecIsolationProducer()
{}

void EgammaHLTEcalRecIsolationProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  
  // Get the RecoEcalCandidate Collection
  edm::Handle<reco::RecoEcalCandidateCollection> recoecalcandHandle;
  iEvent.getByToken(recoEcalCandidateProducer_,recoecalcandHandle);
  
  // Next get Ecal hits barrel
  edm::Handle<EcalRecHitCollection> ecalBarrelRecHitHandle; //EcalRecHitCollection is a typedef to
  iEvent.getByToken(ecalBarrelRecHitProducer_, ecalBarrelRecHitHandle);
  
  // Next get Ecal hits endcap
  edm::Handle<EcalRecHitCollection> ecalEndcapRecHitHandle;
  iEvent.getByToken(ecalEndcapRecHitProducer_, ecalEndcapRecHitHandle);

  //create the meta hit collections inorder that we can pass them into the isolation objects

  EcalRecHitMetaCollection ecalBarrelHits(*ecalBarrelRecHitHandle);
  EcalRecHitMetaCollection ecalEndcapHits(*ecalEndcapRecHitHandle);

  //Get Calo Geometry
  edm::ESHandle<CaloGeometry> pG;
  iSetup.get<CaloGeometryRecord>().get(pG);
  const CaloGeometry* caloGeom = pG.product();

  edm::ESHandle<EcalSeverityLevelAlgo> sevlv;
  iSetup.get<EcalSeverityLevelAlgoRcd>().get(sevlv);
  const EcalSeverityLevelAlgo* sevLevel = sevlv.product();
  
  edm::Handle<double> rhoHandle;
  double rho = 0.0;
  if (doRhoCorrection_) {
    iEvent.getByToken(rhoProducer_, rhoHandle);
    rho = *(rhoHandle.product());
  }

  if (rho > rhoMax_)
    rho = rhoMax_;

  rho = rho*rhoScale_;

  //prepare product
  reco::RecoEcalCandidateIsolationMap isoMap;

  //create algorithm objects
  EgammaRecHitIsolation ecalBarrelIsol(egIsoConeSizeOut_,egIsoConeSizeInBarrel_,egIsoJurassicWidth_,egIsoPtMinBarrel_,egIsoEMinBarrel_,edm::ESHandle<CaloGeometry>(caloGeom),&ecalBarrelHits,sevLevel,DetId::Ecal);
  ecalBarrelIsol.setUseNumCrystals(useNumCrystals_);
  EgammaRecHitIsolation ecalEndcapIsol(egIsoConeSizeOut_,egIsoConeSizeInEndcap_,egIsoJurassicWidth_,egIsoPtMinEndcap_,egIsoEMinEndcap_,edm::ESHandle<CaloGeometry>(caloGeom),&ecalEndcapHits,sevLevel,DetId::Ecal);
  ecalEndcapIsol.setUseNumCrystals(useNumCrystals_);

  for (reco::RecoEcalCandidateCollection::const_iterator iRecoEcalCand= recoecalcandHandle->begin(); iRecoEcalCand!=recoecalcandHandle->end(); iRecoEcalCand++) {
    
    //create reference for storage in isolation map
    reco::RecoEcalCandidateRef recoecalcandref(reco::RecoEcalCandidateRef(recoecalcandHandle,iRecoEcalCand -recoecalcandHandle ->begin()));
    
    //ecal isolation is centered on supecluster
    reco::SuperClusterRef superClus = iRecoEcalCand->get<reco::SuperClusterRef>();

    //i need to know if its in the barrel/endcap so I get the supercluster handle to find out the detector eta
    //this might not be the best way, are we guaranteed that eta<1.5 is barrel
    //this can be safely replaced by another method which determines where the emobject is
    //then we either get the isolation Et or isolation Energy depending on user selection
    float isol =0.;

    if(tryBoth_){ //barrel + endcap
      if(useIsolEt_) isol =  ecalBarrelIsol.getEtSum(&(*iRecoEcalCand)) + ecalEndcapIsol.getEtSum(&(*iRecoEcalCand));
      else           isol =  ecalBarrelIsol.getEnergySum(&(*iRecoEcalCand)) + ecalEndcapIsol.getEnergySum(&(*iRecoEcalCand));
    }
    else if( fabs(superClus->eta())<1.479) { //barrel
      if(useIsolEt_) isol =  ecalBarrelIsol.getEtSum(&(*iRecoEcalCand));
      else           isol =  ecalBarrelIsol.getEnergySum(&(*iRecoEcalCand));
    }
    else{ //endcap
      if(useIsolEt_) isol =  ecalEndcapIsol.getEtSum(&(*iRecoEcalCand));
      else           isol =  ecalEndcapIsol.getEnergySum(&(*iRecoEcalCand));
    }

    //we subtract off the electron energy here as well
    double subtractVal=0;

    if(useIsolEt_) subtractVal = superClus.get()->rawEnergy()*sin(2*atan(exp(-superClus.get()->eta())));
    else           subtractVal = superClus.get()->rawEnergy();

    if(subtract_) isol-= subtractVal;
    
    if (doRhoCorrection_) {
      if (fabs(superClus->eta()) < 1.442) 
	isol = isol - rho*effectiveAreaBarrel_;
      else
	isol = isol - rho*effectiveAreaEndcap_;
    }

    isoMap.insert(recoecalcandref, isol);
  }

  std::auto_ptr<reco::RecoEcalCandidateIsolationMap> isolMap(new reco::RecoEcalCandidateIsolationMap(isoMap));
  iEvent.put(isolMap);

}


