#ifndef ExtrToSIT_h
#define ExtrToSIT_h 1

#include <marlin/Processor.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <time.h>
#include <math.h>
#include <sstream>
#include <assert.h>

#include <UTIL/LCRelationNavigator.h>
#include <UTIL/BitField64.h>
#include <UTIL/ILDConf.h>
#include <UTIL/BitSet32.h>

#include <EVENT/TrackerHit.h>
#include <EVENT/SimTrackerHit.h>
#include <EVENT/TrackerHitPlane.h>
#include <IMPL/TrackerHitImpl.h>
#include <EVENT/LCCollection.h>
#include <EVENT/LCRelation.h>
#include <IMPL/LCCollectionVec.h>
#include <IMPL/LCFlagImpl.h>
#include <IMPL/LCRelationImpl.h>
#include <EVENT/Track.h>
#include <IMPL/TrackStateImpl.h>
#include <IMPL/TrackImpl.h>

#include "KiTrack/SubsetHopfieldNN.h"
#include "KiTrack/SubsetSimple.h"

//---- GEAR ----
#include "marlin/Global.h"
#include "gear/GEAR.h"
#include <gear/ZPlanarParameters.h>
#include <gear/ZPlanarLayerLayout.h>
#include "gear/BField.h"

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"

#include "MarlinTrk/Factory.h"
#include "MarlinTrk/IMarlinTrack.h"
#include "MarlinTrk/MarlinTrkUtils.h"
#include "MarlinTrk/HelixTrack.h"

#include <TTree.h>
// to calculate chi2 probability
#include "Math/DistFunc.h"
#include <vector.h>

using namespace KiTrack;
namespace MarlinTrk{
  class IMarlinTrkSystem ;
}


class ExtrToSIT : public marlin::Processor {
  
public:


  virtual marlin::Processor*  newProcessor() { return new ExtrToSIT ; }
  
  ExtrToSIT() ;
  
  /** Called at the begin of the job before anything is read.
   * Use to initialize the processor, e.g. book histograms.
   */
  virtual void init() ;
  
  /** Called for every run.
   */
  virtual void processRunHeader( lcio::LCRunHeader* run ) ;
  
  /** Called for every event - the working horse.
   */
  virtual void processEvent( lcio::LCEvent * evt ) ; 
  
  
  virtual void check( lcio::LCEvent * evt ) ; 
  
  
  /** Called after data processing for clean up.
   */
  virtual void end() ;
  
  void  SelectBestCandidate(EVENT::TrackerHitVec &HitsInLayer, const float* &pivot, EVENT::TrackerHit* &BestHit, bool &BestHitFound, int &pointer) ; 

  void SelectBestCandidateLimited(EVENT::TrackerHitVec &HitsInLayer, const float* &pivot, EVENT::TrackerHit* &BestHit, const FloatVec& covLCIO, double& radius, bool &BestHitFound, double &sigma, int &pointer, int &PossibleHits, float &dU, float &dV, double &DimDist, TrackerHitVec &usedSiHits) ;
  
  int FitInit( std::vector < TrackerHit* > trackerHits , MarlinTrk::IMarlinTrack* _marlinTrk ) ;
  int FitInit2( Track* track , MarlinTrk::IMarlinTrack* _marlinTrk ) ;

  struct compare_r {
    bool operator()( EVENT::TrackerHit* a, EVENT::TrackerHit* b)  const { 
      double r_a_sqd = a->getPosition()[0] * a->getPosition()[0] + a->getPosition()[1] * a->getPosition()[1] ; 
      double r_b_sqd = b->getPosition()[0] * b->getPosition()[0] + b->getPosition()[1] * b->getPosition()[1] ; 
      return ( r_a_sqd < r_b_sqd ) ; 
    }
  } ;
  
  
  
protected:
  
  /* helper function to get collection using try catch block */
  lcio::LCCollection* GetCollection( lcio::LCEvent * evt, std::string colName ) ;
  
  /* helper function to get relations using try catch block */
  lcio::LCRelationNavigator* GetRelations(lcio::LCEvent * evt, std::string RelName ) ;
  
  /** Input track collection name for refitting.
   */
  std::string _input_track_col_name ;
  
  /** Input track relations name for refitting.
   */
  std::string _input_track_rel_name ;

  /** Input SIT tracker summer hit collection.
   */
  std::string _sitColName ;

  /** Input VXD tracker summer hit collection.
   */
  std::string _vxdColName ;
  
  /** refitted track collection name.
   */
  std::string _output_track_col_name ;
  
  /** Output track relations name for refitting.
   */
  std::string _output_track_rel_name ;

  /** Output silicon track collection.
   */
  std::string _siTrkColName ;
  
  /** pointer to the IMarlinTrkSystem instance 
   */
  MarlinTrk::IMarlinTrkSystem* _trksystem ;
  
  std::string _mcParticleCollectionName ;

  bool _MSOn ;
  bool _ElossOn ;
  bool _SmoothOn ;
  double _Max_Chi2_Incr ;
  int _tpcHitsCut ;
  float _chi2NDoFCut ;
  float _DoCut ;
  float _ZoCut ;
  double _searchSigma ;
  bool _isSpacePoints ;
  int _propToLayer ;
  
  int _n_run ;
  int _n_evt ;
  int SITHitsFitted ;
  int SITHitsNonFitted ;
  int TotalSITHits ;
  int _nHitsChi2 ;

  float _maxChi2PerHit;
  float _bField;

  StringVec  _colNamesTrackerHitRelations ;

  std::string _bestSubsetFinder;

  
} ;


class TrackCompatibilityShare1SP{
  
public:
   
   inline bool operator()( TrackImpl* trackA, TrackImpl* trackB ){
      
      
      TrackerHitVec hitsA = trackA->getTrackerHits();
      TrackerHitVec hitsB = trackB->getTrackerHits();

      for (TrackerHitVec::iterator itA=hitsA.begin(); itA!=hitsA.end(); ++itA) {

	for (TrackerHitVec::iterator itB=hitsB.begin(); itB!=hitsB.end(); ++itB) {

	  if ( *itA == *itB ){
	    return false ;
	  }
	}
      }

      return true;      
      
   }
   
};


/** A functor to return the quality of a track, in a CMS style. */
class DoItLikeCMS{
   
public:
   
   inline double operator()( TrackImpl* track ){ 

     std::vector< TrackerHit* > hitvec = track->getTrackerHits();
     int hits = hitvec.size();

     //double hits_norm = (hits * 1.0) / 8.0 ;

     return hits ;

   }
   
};


/** A functor to return the quality of a track, which is currently the chi2 probability. */
class TrackQI{
   
public:
   
  inline double operator()( TrackImpl* track ){ 

    double dof = double(track->getNdf());
    double xi2 = double(track->getChi2()) ;

    //return ROOT::Math::chisquared_cdf_c (xi2, dof) ;
    return xi2/dof ;

  }

};  


#endif



