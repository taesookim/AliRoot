/**************************************************************************
 * Copyright(c) 1998-2007, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

//-------------------------------------------------------------------------
//     AOD track base class
//     Author: Markus Oldenburg, CERN
//-------------------------------------------------------------------------

#include "AliAODTrack.h"

ClassImp(AliAODTrack)

//______________________________________________________________________________
AliAODTrack::AliAODTrack() : 
  AliVirtualParticle(),
  fChi2(-999.),
  fID(-999),
  fLabel(-999),
  fCovMatrix(NULL),
  fProdVertex(0x0),
  fCharge(-999),
  fITSClusterMap(0),
  fType(kUndef)
{
  // default constructor

  SetP();
  SetPosition((Float_t*)NULL);
  SetPID((Float_t*)NULL);
}

//______________________________________________________________________________
AliAODTrack::AliAODTrack(Int_t id,
			 Int_t label, 
			 Double_t p[3],
			 Bool_t cartesian,
			 Double_t x[3],
			 Bool_t isDCA,
			 Double_t covMatrix[21],
			 Short_t charge,
			 UChar_t itsClusMap,
			 Double_t pid[10],
			 AliAODVertex *prodVertex,
			 AODTrk_t ttype) :
  AliVirtualParticle(),
  fChi2(-999.),
  fID(id),
  fLabel(label),
  fCovMatrix(NULL),
  fProdVertex(prodVertex),
  fCharge(charge),
  fITSClusterMap(itsClusMap),
  fType(ttype)
{
  // constructor
 
  SetP(p, cartesian);
  SetPosition(x, isDCA);
  if(covMatrix) SetCovMatrix(covMatrix);
  SetPID(pid);

}

//______________________________________________________________________________
AliAODTrack::AliAODTrack(Int_t id,
			 Int_t label, 
			 Float_t p[3],
			 Bool_t cartesian,
			 Float_t x[3],
			 Bool_t isDCA,
			 Float_t covMatrix[21],
			 Short_t charge,
			 UChar_t itsClusMap,
			 Float_t pid[10],
			 AliAODVertex *prodVertex,
			 AODTrk_t ttype) :
  AliVirtualParticle(),
  fChi2(-999.),
  fID(id),
  fLabel(label),
  fCovMatrix(NULL),
  fProdVertex(prodVertex),
  fCharge(charge),
  fITSClusterMap(itsClusMap),
  fType(ttype)
{
  // constructor
 
  SetP(p, cartesian);
  SetPosition(x, isDCA);
  if(covMatrix) SetCovMatrix(covMatrix);
  SetPID(pid);

}


//______________________________________________________________________________
AliAODTrack::~AliAODTrack() 
{
  // destructor
  delete fCovMatrix;
}


//______________________________________________________________________________
AliAODTrack::AliAODTrack(const AliAODTrack& trk) :
  AliVirtualParticle(trk),
  fChi2(trk.fChi2),
  fID(trk.fID),
  fLabel(trk.fLabel),
  fCovMatrix(NULL),
  fProdVertex(trk.fProdVertex),
  fCharge(trk.fCharge),
  fITSClusterMap(trk.fITSClusterMap),
  fType(trk.fType)
{
  // Copy constructor

  trk.GetP(fMomentum);
  trk.GetPosition(fPosition);
  if(trk.fCovMatrix) fCovMatrix=new AliAODTrkCov(*trk.fCovMatrix);
  SetPID(trk.fPID);

}

//______________________________________________________________________________
AliAODTrack& AliAODTrack::operator=(const AliAODTrack& trk)
{
  // Assignment operator
  if(this!=&trk) {

    AliVirtualParticle::operator=(trk);

    trk.GetP(fMomentum);
    trk.GetPosition(fPosition);
    trk.GetPID(fPID);

    fChi2 = trk.fChi2;

    fID = trk.fID;
    fLabel = trk.fLabel;    
    
    delete fCovMatrix;
    if(trk.fCovMatrix) fCovMatrix=new AliAODTrkCov(*trk.fCovMatrix);
    else fCovMatrix=NULL;
    fProdVertex = trk.fProdVertex;

    fCharge = trk.fCharge;
    fITSClusterMap = trk.fITSClusterMap;
    fType = trk.fType;
  }

  return *this;
}

//______________________________________________________________________________
template <class T> void AliAODTrack::SetP(const T *p, const Bool_t cartesian) 
{
  // set the momentum

  if (p) {
    if (cartesian) {
      Double_t pt = TMath::Sqrt(p[0]*p[0] + p[1]*p[1]);
      Double_t P = TMath::Sqrt(pt*pt + p[2]*p[2]);
      
      fMomentum[0] = 1./pt;
      fMomentum[1] = TMath::ACos(p[2]/P);
      fMomentum[2] = TMath::ATan2(p[1], p[0]);
    } else {
      fMomentum[0] = p[0];  // 1/pt
      fMomentum[1] = p[1];  // phi
      fMomentum[2] = p[2];  // theta
    }
  } else {
    fMomentum[0] = -999.;
    fMomentum[1] = -999.;
    fMomentum[2] = -999.;
  }
}

//______________________________________________________________________________
template <class T> void AliAODTrack::SetPosition(const T *x, const Bool_t dca) 
{
  // set the position

  if (x) {
    if (!dca) {
      ResetBit(kIsDCA);

      fPosition[0] = x[0];
      fPosition[1] = x[1];
      fPosition[2] = x[2];
    } else {
      SetBit(kIsDCA);
      // don't know any better yet
      fPosition[0] = -999.;
      fPosition[1] = -999.;
      fPosition[2] = -999.;
    }
  } else {
    ResetBit(kIsDCA);

    fPosition[0] = -999.;
    fPosition[1] = -999.;
    fPosition[2] = -999.;
  }
}

//______________________________________________________________________________
void AliAODTrack::SetDCA(Double_t d, Double_t z) 
{
  // set the dca
  fPosition[0] = d;
  fPosition[1] = z;
  fPosition[2] = 0.;
  SetBit(kIsDCA);
}

//______________________________________________________________________________
void AliAODTrack::Print(Option_t* /* option */) const
{
  // prints information about AliAODTrack

  printf("Object name: %s   Track type: %s\n", GetName(), GetTitle()); 
  printf("        px = %f\n", Px());
  printf("        py = %f\n", Py());
  printf("        pz = %f\n", Pz());
  printf("        pt = %f\n", Pt());
  printf("      1/pt = %f\n", OneOverPt());
  printf("     theta = %f\n", Theta());
  printf("       phi = %f\n", Phi());
  printf("      chi2 = %f\n", Chi2());
  printf("    charge = %d\n", Charge());
  printf(" PID object: %p\n", PID());
}

//-------------------------------------------------------------------------
//     AOD track cov matrix base class
//-------------------------------------------------------------------------

ClassImp(AliAODTrack::AliAODTrkCov)

//______________________________________________________________________________
template <class T> void AliAODTrack::AliAODTrkCov::GetCovMatrix(T *cmat) const
{
  //
  // Returns the external cov matrix
  //
  cmat[ 0] = fDiag[ 0]*fDiag[ 0];
  cmat[ 2] = fDiag[ 1]*fDiag[ 1];
  cmat[ 5] = fDiag[ 2]*fDiag[ 2];
  cmat[ 9] = fDiag[ 3]*fDiag[ 3];
  cmat[14] = fDiag[ 4]*fDiag[ 4];
  cmat[20] = fDiag[ 5]*fDiag[ 5];
  //
  cmat[ 1] = fODia[ 0]*fDiag[ 0]*fDiag[ 1];
  cmat[ 3] = fODia[ 1]*fDiag[ 0]*fDiag[ 2];
  cmat[ 4] = fODia[ 2]*fDiag[ 1]*fDiag[ 2];
  cmat[ 6] = fODia[ 3]*fDiag[ 0]*fDiag[ 3];
  cmat[ 7] = fODia[ 4]*fDiag[ 1]*fDiag[ 3];
  cmat[ 8] = fODia[ 5]*fDiag[ 2]*fDiag[ 3];
  cmat[10] = fODia[ 6]*fDiag[ 0]*fDiag[ 4];
  cmat[11] = fODia[ 7]*fDiag[ 1]*fDiag[ 4];
  cmat[12] = fODia[ 8]*fDiag[ 2]*fDiag[ 4];
  cmat[13] = fODia[ 9]*fDiag[ 3]*fDiag[ 4];
  cmat[15] = fODia[10]*fDiag[ 0]*fDiag[ 5];
  cmat[16] = fODia[11]*fDiag[ 1]*fDiag[ 5];
  cmat[17] = fODia[12]*fDiag[ 2]*fDiag[ 5];
  cmat[18] = fODia[13]*fDiag[ 3]*fDiag[ 5];
  cmat[19] = fODia[14]*fDiag[ 4]*fDiag[ 5];
}


//______________________________________________________________________________
template <class T> void AliAODTrack::AliAODTrkCov::SetCovMatrix(T *cmat)
{
  //
  // Sets the external cov matrix
  //
  if(cmat) {
    fDiag[ 0] = TMath::Sqrt(cmat[ 0]);
    fDiag[ 1] = TMath::Sqrt(cmat[ 2]);
    fDiag[ 2] = TMath::Sqrt(cmat[ 5]);
    fDiag[ 3] = TMath::Sqrt(cmat[ 9]);
    fDiag[ 4] = TMath::Sqrt(cmat[14]);
    fDiag[ 5] = TMath::Sqrt(cmat[20]);
    //
    fODia[ 0] = cmat[ 1]/(fDiag[ 0]*fDiag[ 1]);
    fODia[ 1] = cmat[ 3]/(fDiag[ 0]*fDiag[ 2]);
    fODia[ 2] = cmat[ 4]/(fDiag[ 1]*fDiag[ 2]);
    fODia[ 3] = cmat[ 6]/(fDiag[ 0]*fDiag[ 3]);
    fODia[ 4] = cmat[ 7]/(fDiag[ 1]*fDiag[ 3]);
    fODia[ 5] = cmat[ 8]/(fDiag[ 2]*fDiag[ 3]);
    fODia[ 6] = cmat[10]/(fDiag[ 0]*fDiag[ 4]);
    fODia[ 7] = cmat[11]/(fDiag[ 1]*fDiag[ 4]);
    fODia[ 8] = cmat[12]/(fDiag[ 2]*fDiag[ 4]);
    fODia[ 9] = cmat[13]/(fDiag[ 3]*fDiag[ 4]);
    fODia[10] = cmat[15]/(fDiag[ 0]*fDiag[ 5]);
    fODia[11] = cmat[16]/(fDiag[ 1]*fDiag[ 5]);
    fODia[12] = cmat[17]/(fDiag[ 2]*fDiag[ 5]);
    fODia[13] = cmat[18]/(fDiag[ 3]*fDiag[ 5]);
    fODia[14] = cmat[19]/(fDiag[ 4]*fDiag[ 5]);
  } else {
    for(Int_t i=0; i< 6; ++i) fDiag[i]=-999.;
    for(Int_t i=0; i<15; ++i) fODia[i]=0.;
  }
}
