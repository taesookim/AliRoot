/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
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

//*-- Author: Rachid Guernane (LPCCFd)
//*   Manager class for muon trigger electronics
//*   Client of trigger board classes
//*
//*

#include "AliMUONTriggerElectronics.h"
#include "AliMUONTriggerCrate.h"
#include "AliMUONTriggerCrateStore.h"
#include "AliMUONConstants.h"
#include "AliMUONLocalTriggerBoard.h"
#include "AliMUONRegionalTriggerBoard.h"
#include "AliMUONGlobalTriggerBoard.h"
#include "AliMUONLocalTrigger.h"
#include "AliMUONRegionalTrigger.h"
#include "AliMUONGlobalTrigger.h"
#include "AliMUON.h" 
#include "AliMUONData.h" 
#include "AliMUONDigit.h"
#include "AliMUONSegmentation.h"
#include "AliMUONCalibrationData.h"
#include "AliMUONVCalibParam.h"

#include "AliMpVSegmentation.h"

#include "AliLog.h"
#include "AliLoader.h"
#include "AliRun.h"

//#include "Riostream.h"
#include "TBits.h"
#include "TSystem.h"

ClassImp(AliMUONTriggerElectronics)

//___________________________________________
AliMUONTriggerElectronics::AliMUONTriggerElectronics(AliMUONData *Data, AliMUONCalibrationData* calibData) 
: TTask("AliMUONTriggerElectronics",
        "From trigger digits to Local and Global Trigger objects"),
  fCrates(new AliMUONTriggerCrateStore),
  fGlobalTriggerBoard(new AliMUONGlobalTriggerBoard),
  fMUONData(Data)
{
//* CONSTRUCTOR
//*
  if (!fMUONData)
  {  
    AliFatal("NO MUON TRIGGER DATA");
  }
    
  SetDataSource();
  Factory(calibData);
  LoadMasks(calibData);
}

//______________________________________________________________________________
AliMUONTriggerElectronics::AliMUONTriggerElectronics(const AliMUONTriggerElectronics& right) 
  : TTask(right) 
{  
/// Protected copy constructor (not implemented)

  AliFatal("Copy constructor not provided.");
}

//___________________________________________
AliMUONTriggerElectronics::~AliMUONTriggerElectronics()
{
//* DESTRUCTOR
//*
  delete fGlobalTriggerBoard;
  delete fCrates;
}

//______________________________________________________________________________
AliMUONTriggerElectronics& 
AliMUONTriggerElectronics::operator=(const AliMUONTriggerElectronics& right)
{
/// Protected assignement operator (not implemented)

  // check assignement to self
  if (this == &right) return *this;

  AliFatal("Assignement operator not provided.");
    
  return *this;  
}    

//___________________________________________
void AliMUONTriggerElectronics::Factory(AliMUONCalibrationData* calibData)
{  
 //* BUILD ALL ELECTRONICS
 //*
 
  fCrates->ReadFromFile(gSystem->ExpandPathName(fSourceFileName.Data()));
  
  if ( !calibData ) return;
  
  AliMUONTriggerLut* lut = calibData->TriggerLut();
  
  if (!lut) return;
  
  AliMUONLocalTriggerBoard* localBoard;
  
  fCrates->FirstLocalBoard();
  
  while ( (localBoard=fCrates->NextLocalBoard()) )
  {
    localBoard->SetLUT(lut);
  }
}

//___________________________________________
void AliMUONTriggerElectronics::FeedM()
{
//* FILL INPUTS
//*
	for (Int_t ichamber=10; ichamber<14; ichamber++) 
	{
      TClonesArray *muonDigits = fMUONData->Digits(ichamber);
      Int_t ndigits = muonDigits->GetEntriesFast();

      for (Int_t digit=0; digit<ndigits; digit++)
		{
			AliMUONDigit *mdig = static_cast<AliMUONDigit*>(muonDigits->UncheckedAt(digit));

//       CHECKME ! The TrackCharge is not ok with new digitizerV3 !
//			for (Int_t ichg=0; ichg<10; ichg++) schg += mdig->TrackCharge(ichg);
//       assert(schg==mdig->Signal());
			Int_t schg = mdig->Signal();
         
//       APPLY CONDITION ON SOFT BACKGROUND	
			Int_t tchg = schg - (Int_t(schg/10))*10;	

			if (schg<=10 || tchg>0) 
			{
//   				mdig->Print();

				Int_t digitindex = digit;
				Int_t detElemId  = mdig->DetElemId();
				Int_t cathode    = mdig->Cathode();

				const AliMpVSegmentation *seg = ((AliMUON*)gAlice->GetDetector("MUON"))->GetSegmentation()->GetMpSegmentation(detElemId,cathode);

				Int_t ix = mdig->PadX(), iy = mdig->PadY();
				
				AliDebug(3,Form("cathode %d ix %d iy %d ",cathode,ix,iy));

				AliMpPad pad = seg->PadByIndices(AliMpIntPair(ix,iy),kTRUE);
				
				for (Int_t i=0; i<pad.GetNofLocations(); i++) 
				{
					AliMpIntPair location = pad.GetLocation(i);
					
					Int_t nboard = location.GetFirst();

					Int_t ibitxy = location.GetSecond();

					AliMUONLocalTriggerBoard *b = fCrates->LocalBoard(nboard);

					if (b) 
					{
						if (cathode && b->GetSwitch(6)) ibitxy += 8;
						
						b->SetbitM(ibitxy,cathode,ichamber-10);
						
						DigitFiredCircuit(b->GetI(), cathode, ichamber, digitindex);
					}
          else
          {
            AliError(Form("Could not get local board number %d",b->GetNumber()));
          }
				}
			}			
		}
	}

// Particular case of the columns with 22 local boards (2R(L) 3R(L))   
	AliMUONTriggerCrate *crate = 0x0; TObjArray *bs = 0x0;

	char *scratess[4] = {  "2R",   "2L",   "3L",   "3R"}; 
	char *scratesd[4] = {"2-3R", "2-3L", "2-3L", "2-3R"}; 
	Int_t    slotf[4] = {     2,      2,     10,     10}; 
	Int_t    slotd[4] = {     1,      1,      9,      9}; 

	for (Int_t i=0; i<4; i++)
	{
      crate = fCrates->Crate(scratess[i]); 
      bs = crate->Boards();
      AliMUONLocalTriggerBoard *desybb = (AliMUONLocalTriggerBoard*)bs->At(14);
      AliMUONLocalTriggerBoard *fromcb = (AliMUONLocalTriggerBoard*)bs->At(15);
      AliMUONLocalTriggerBoard *desxbb = (AliMUONLocalTriggerBoard*)bs->At(16);

      crate = fCrates->Crate(scratesd[i]); 
      bs = crate->Boards();
      AliMUONLocalTriggerBoard *frombb = (AliMUONLocalTriggerBoard*)bs->At(slotf[i]);
      AliMUONLocalTriggerBoard *desycb = (AliMUONLocalTriggerBoard*)bs->At(slotd[i]);

      UShort_t cX[2];

//    COPY X3-4 FROM BOARD  2 OF CRATE 2-3 TO BOARD 16 OF CRATE 2
//    COPY X3-4 FROM BOARD 10 OF CRATE 2-3 TO BOARD 16 OF CRATE 3
      frombb->GetX34(cX); desxbb->SetX34(cX);

//    COPY X3-4 FROM BOARD 15 OF CRATE 2 TO BOARD 1 OF CRATE 2-3
//    COPY X3-4 FROM BOARD 15 OF CRATE 3 TO BOARD 9 OF CRATE 2-3
      fromcb->GetX34(cX); desycb->SetX34(cX);

      UShort_t cY[4];

      desybb->GetY(cY); frombb->SetY(cY);

      frombb->GetY(cY); desxbb->SetY(cY);
      fromcb->GetY(cY); desycb->SetY(cY);
	}

// FILL UP/DOWN OF CURRENT BOARD (DONE VIA J3 BUS IN REAL LIFE)
 AliMUONTriggerCrate* cr;
 
 fCrates->FirstCrate();
 
 while ( ( cr = fCrates->NextCrate() ) )
	{            
		TObjArray *boards = cr->Boards();
		
		for (Int_t j=1; j<boards->GetEntries()-1; j++)
		{
			TObject *o = boards->At(j);
			
			if (!o) break;
			
			AliMUONLocalTriggerBoard *currboard = (AliMUONLocalTriggerBoard*)o;
			
			AliMUONLocalTriggerBoard *neighbour = (AliMUONLocalTriggerBoard*)boards->At(j+1);
			
			UShort_t cXY[2][4];
			
			if (j==1) {neighbour->GetXY(cXY); currboard->SetXYU(cXY);}
			
//       LAST BOARD IN THE CRATE HAS NO UP EXCEPT FOR CRATES 2 & 3
			if (j<boards->GetEntries()-2)  
			{
				AliMUONLocalTriggerBoard *nextboard = (AliMUONLocalTriggerBoard*)boards->At(j+2);
				
				currboard->GetXY(cXY); neighbour->SetXYD(cXY);
				nextboard->GetXY(cXY); neighbour->SetXYU(cXY);
				
				if (j==boards->GetEntries()-3) {neighbour->GetXY(cXY); nextboard->SetXYD(cXY);}
			}
		}
	}
}

//___________________________________________
void AliMUONTriggerElectronics::Feed(UShort_t pattern[2][4])
{
  //* FILL INPUTS
  //*
  AliMUONTriggerCrate* cr;
   
   fCrates->FirstCrate();
   
   while ( ( cr = fCrates->NextCrate() ) )
   {                 
     TObjArray *boards = cr->Boards();
     
     for (Int_t j=1; j<boards->GetEntries(); j++)
     {
       TObject *o = boards->At(j);
       
       if (!o) break;
       
       AliMUONLocalTriggerBoard *board = (AliMUONLocalTriggerBoard*)o;
       
       board->SetXY(pattern);
     }
   }
}

//___________________________________________
void AliMUONTriggerElectronics::DumpOS()
{
//* DUMP IN THE OLD WAY
//*
   for (Int_t i=0;i<234;i++)
   {
      AliMUONLocalTriggerBoard *board = fCrates->LocalBoard(i);

			if (board) board->Scan("ALL");
   }
}

//___________________________________________
void AliMUONTriggerElectronics::Scan(Option_t *option)
{
  //* SCAN
  //*

  AliMUONTriggerCrate* cr;
  
  fCrates->FirstCrate();
  
  while ( ( cr = fCrates->NextCrate() ) )
	{                
    TObjArray *boards = cr->Boards();
    
    for (Int_t j=0; j<boards->GetEntries(); j++)
    {
      TObject *o = boards->At(j);
      
      TString op = option;
      
      Bool_t cdtion = kFALSE;
      
      if (op.Contains("LOCAL"))    cdtion = o->IsA() == AliMUONLocalTriggerBoard::Class();
      if (op.Contains("REGIONAL")) cdtion = o->IsA() == AliMUONRegionalTriggerBoard::Class();
      if (op.Contains("GLOBAL"))   cdtion = o->IsA() == AliMUONGlobalTriggerBoard::Class();
      
      if (!o || !cdtion) continue;
      
      AliMUONLocalTriggerBoard *board = (AliMUONLocalTriggerBoard*)o;
      
      board->Scan();
    }
  }
}

//___________________________________________
void AliMUONTriggerElectronics::Reset()
{
  //* RESET
  //*
  
   AliMUONTriggerCrate* cr;
   
   fCrates->FirstCrate();
   
   while ( ( cr = fCrates->NextCrate() ) )
   {            
      TObjArray *boards = cr->Boards();
            
      for (Int_t j=0; j<boards->GetEntries(); j++)
      {     
         AliMUONTriggerBoard *b = (AliMUONTriggerBoard*)boards->At(j);

         if (b) b->Reset();
      }
   }
}

//_______________________________________________________________________
void AliMUONTriggerElectronics::LoadMasks(AliMUONCalibrationData* calibData)
{
  // LOAD MASKS FROM CDB
  

  // SET MASKS
  
  AliMUONTriggerCrate* cr;
  
  fCrates->FirstCrate();
  
  Int_t irb(0);
  
  while ( ( cr = fCrates->NextCrate() ) )
	{            
    TObjArray *boards = cr->Boards();
    
    AliMUONRegionalTriggerBoard *regb =
      (AliMUONRegionalTriggerBoard*)boards->At(0);

    AliMUONVCalibParam* regionalBoardMasks = calibData->RegionalTriggerBoardMasks(irb);
    
    for ( Int_t i = 0; i < regionalBoardMasks->Size(); ++i )
    {
      UShort_t rmask = static_cast<UShort_t>(regionalBoardMasks->ValueAsInt(i) & 0x3F);
      regb->Mask(i,rmask);
    }
    
    for (Int_t j=1; j<boards->GetEntries(); j++)
    {
      AliMUONLocalTriggerBoard *b = (AliMUONLocalTriggerBoard*)boards->At(j);
      
      Int_t cardNumber = b->GetNumber();
      
      if (cardNumber) // interface board are not interested
      {
        AliMUONVCalibParam* localBoardMasks = calibData->LocalTriggerBoardMasks(cardNumber);
        for ( Int_t i = 0; i < localBoardMasks->Size(); ++i )
        {
          UShort_t lmask = static_cast<UShort_t>(localBoardMasks->ValueAsInt(i) & 0xFFFF);
          b->Mask(i,lmask);
        }
      }
    }
    ++irb;
  }
  
  AliMUONVCalibParam* globalBoardMasks = calibData->GlobalTriggerBoardMasks();
  for ( Int_t i = 0; i < globalBoardMasks->Size(); ++i )
  {
    UShort_t gmask = static_cast<UShort_t>(globalBoardMasks->ValueAsInt(i) & 0xFFF);
    fGlobalTriggerBoard->Mask(i,gmask);
  }
}


//___________________________________________
void AliMUONTriggerElectronics::LocalResponse()
{
  // INTERFACE BOARDS
	struct crates_t 
  {
    TString name;
    Int_t slots[5];
    Int_t ns;
  } crate[6];

	crate[0].name = "2R";   crate[0].ns = 1; crate[0].slots[0] = 16;
	crate[1].name = "2L";   crate[1].ns = 1; crate[1].slots[0] = 16;
	crate[2].name = "3L";   crate[2].ns = 1; crate[2].slots[0] = 16;
	crate[3].name = "3R";   crate[3].ns = 1; crate[3].slots[0] = 16;
	crate[4].name = "2-3R"; crate[4].ns = 2; crate[4].slots[0] = 1;  crate[4].slots[1] = 9;
	crate[5].name = "2-3L"; crate[5].ns = 2; crate[5].slots[0] = 1;  crate[5].slots[1] = 9; 
	
  AliMUONTriggerCrate* cr;
  
  fCrates->FirstCrate();
  
  while ( ( cr = fCrates->NextCrate() ) )
	{            
    Int_t iib = -1;
    
    for (Int_t icr=0; icr<6; icr++) 
    {
			const char *n = (crate[icr].name).Data();
			
      AliMUONTriggerCrate *dcr = fCrates->Crate(n);
      
      //       THIS CRATE CONTAINS AN INTERFACE BOARD
      if ( dcr && !strcmp(cr->GetName(),dcr->GetName()) ) iib = icr;
    }
    
    TObjArray *boards = cr->Boards();
    
    AliMUONRegionalTriggerBoard *regb = (AliMUONRegionalTriggerBoard*)boards->At(0);
    
    UShort_t thisl[16]; for (Int_t j=0; j<16; j++) thisl[j] = 0;
        
    for (Int_t j=1; j<boards->GetEntries(); j++)
    {     
      TObject *o = boards->At(j);
      
      if (!o) break;
      
      AliMUONLocalTriggerBoard *board = (AliMUONLocalTriggerBoard*)o;
      
      if (board) 
      {
        board->Response();
				
        UShort_t tmp = board->GetResponse();            
        
        //          CRATE CONTAINING INTERFACE BOARD
        if ( iib>-1 ) 
        {
          for (Int_t iid = 0; iid<crate[iib].ns; iid++) 
					{
            if ( j == crate[iib].slots[iid] )
						{
              if ( tmp != 0 ) 
                AliWarning(Form("Interface board %s in slot %d of crate %s has a non zero response",
                                board->GetName(),j,cr->GetName()));
						}
					}					
        }
        
        thisl[j-1] = tmp;
      }
    }
    
    regb->SetLocalResponse(thisl);
  }
}

//___________________________________________
void AliMUONTriggerElectronics::RegionalResponse()
{
  /// Compute the response for all regional cards.
  AliMUONTriggerCrate* cr;
  
  fCrates->FirstCrate();
  
  while ( ( cr = fCrates->NextCrate() ) )
	{            
      TObjArray *boards = cr->Boards();

      AliMUONRegionalTriggerBoard *regb = (AliMUONRegionalTriggerBoard*)boards->At(0);
      
      if (regb) 
      {
         regb->Response();
      }  
   }
}

//___________________________________________
void AliMUONTriggerElectronics::GlobalResponse()
{
  /// Compute the global response

  UShort_t regional[16];
  
  AliMUONTriggerCrate* cr;
  
  fCrates->FirstCrate();
  Int_t irb(0);
  
  if ( !fCrates->NumberOfCrates() >= 16 ) 
  {
    AliFatal(Form("Something is wrong : too many crates %d",
                  fCrates->NumberOfCrates()));
  }
  
  while ( ( cr = fCrates->NextCrate() ) )
	{            
    AliMUONTriggerBoard* rb = 
      static_cast<AliMUONTriggerBoard*>(cr->Boards()->At(0));
    regional[irb] = rb->GetResponse();
    ++irb;
  }
  
  fGlobalTriggerBoard->SetRegionalResponse(regional);
  fGlobalTriggerBoard->Response();
}

//___________________________________________
void AliMUONTriggerElectronics::BoardName(Int_t ix, Int_t iy, char *name)
{
//* BOARD NAME FROM PAD INFO (OLD MAPPING)
//*
   TString s = (ix>0) ? "R" : "L"; 

   Int_t board = iy / 16, bid[4] = {12,34,56,78}; 

   ix = abs(ix);

   Int_t line = ix / 10, column = ix - 10 * line;

// old scheme: line==1 is line==9
   line -= 9; line = TMath::Abs(line); line++;

   sprintf(name,"%sC%dL%dB%d", s.Data(), column, line, bid[board]);
   
   AliDebug(3, Form("Strip ( %d , %d ) connected to board %s ", ix, iy, name));
}

//___________________________________________
void AliMUONTriggerElectronics::BuildName(Int_t icirc, char name[20])
{
//* GET BOARD NAME FROM OLD NUMBERING
//*
   const Int_t kCircuitId[234] = 
      {
          111,  121,  131,  141,  151,  161,  171,
          211,  212,  221,  222,  231,  232,  241,  242,  251,  252,  261,  262,  271,
          311,  312,  321,  322,  331,  332,  341,  342,  351,  352,  361,  362,  371,
          411,  412,  413,  421,  422,  423,  424,  431,  432,  433,  434,  441,  442,  451,  452,  461,  462,  471,
          521,  522,  523,  524,  531,  532,  533,  534,  541,  542,  551,  552,  561,  562,  571, 
          611,  612,  613,  621,  622,  623,  624,  631,  632,  633,  634,  641,  642,  651,  652,  661,  662,  671,
          711,  712,  721,  722,  731,  732,  741,  742,  751,  752,  761,  762,  771,
          811,  812,  821,  822,  831,  832,  841,  842,  851,  852,  861,  862,  871,
          911,  921,  931,  941,  951,  961,  971,
         -111, -121, -131, -141, -151, -161, -171,
         -211, -212, -221, -222, -231, -232, -241, -242, -251, -252, -261, -262, -271,
         -311, -312, -321, -322, -331, -332, -341, -342, -351, -352, -361, -362, -371,
         -411, -412, -413, -421, -422, -423, -424, -431, -432, -433, -434, -441, -442, -451, -452, -461, -462, -471,
         -521, -522, -523, -524, -531, -532, -533, -534, -541, -542, -551, -552, -561, -562, -571, 
         -611, -612, -613, -621, -622, -623, -624, -631, -632, -633, -634, -641, -642, -651, -652, -661, -662, -671,
         -711, -712, -721, -722, -731, -732, -741, -742, -751, -752, -761, -762, -771,
         -811, -812, -821, -822, -831, -832, -841, -842, -851, -852, -861, -862, -871,
         -911, -921, -931, -941, -951, -961, -971 
      };

   Int_t b[4] = {12, 34, 56, 78};

   Int_t code = TMath::Abs(kCircuitId[icirc]);

   Int_t lL = code / 100;

   Int_t cC = ( code - 100 * lL ) / 10;
   
   Int_t bB = code - 100 * lL - 10 * cC;
   
   const char *side = (kCircuitId[icirc]>0) ? "R" : "L";

// lL=1 AT TOP
   lL -= 9; lL = abs(lL); lL++;

   sprintf(name,"%sC%dL%dB%d",side,cC,lL,b[bB-1]);
}

//_______________________________________________________________________
void 
AliMUONTriggerElectronics::Exec(Option_t*)
{
//*
//*
  Digits2Trigger();
}

//_______________________________________________________________________
void AliMUONTriggerElectronics::Trigger()
{
//*
//*
   FeedM();
   LocalResponse();
   RegionalResponse();      
   GlobalResponse();
}

//_______________________________________________________________________
void AliMUONTriggerElectronics::Digits2Trigger()
{
  /// Main method to go from digits to trigger decision

  AliMUONRegionalTrigger *pRegTrig = new AliMUONRegionalTrigger();

  ClearDigitNumbers();
  
  fMUONData->ResetTrigger(); 
  
  // RUN THE FULL BEE CHAIN
  Trigger();
  //  	DumpOS();
	
  AliMUONTriggerCrate* cr;
  
  fCrates->FirstCrate();
  
  while ( ( cr = fCrates->NextCrate() ) )
  {            
    TObjArray *boards = cr->Boards();

    UInt_t regInpLpt = 0;
    UInt_t regInpHpt = 0;
    UShort_t localMask = 0x0;

    AliMUONRegionalTriggerBoard *regBoard = (AliMUONRegionalTriggerBoard*)boards->At(0);

    for (Int_t j = 1; j < boards->GetEntries(); j++)
    {     
      TObject *o = boards->At(j);
      
      if (!o) break;
      
      AliMUONLocalTriggerBoard *board = (AliMUONLocalTriggerBoard*)o;
      
      if (board) 
      {
        //          L0 TRIGGER
        if (board->Triggered())
        {
          Int_t localtr[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0};
          
          Int_t icirc = board->GetNumber();
          
          localtr[0] = icirc;
          localtr[1] = board->GetStripX11();
          localtr[2] = board->GetDev();
          localtr[3] = board->GetStripY11();
          
          //             SAVE LUT OUTPUT 
          UShort_t response = board->GetResponse();
          localtr[4] = (response & 12) >> 2;
          localtr[5] = (response & 48) >> 4;
          localtr[6] = (response &  3);

	  // calculates regional inputs from local for the moment
	  UInt_t hPt = (response >> 4) & 0x3;
	  UInt_t lPt = (response >> 2) & 0x3;
	    
	  regInpHpt |= hPt << (30 - (j-1)*2);
	  regInpLpt |= lPt << (30 - (j-1)*2);
	  localMask |= (0x1 << (j-1)); // local mask


          TBits rrr;
          rrr.Set(6,&response);
          
          //             SAVE BIT PATTERN
          localtr[7]  = board->GetXY(0,0);
          localtr[8]  = board->GetXY(0,1);
          localtr[9]  = board->GetXY(0,2);
          localtr[10] = board->GetXY(0,3);
          
          localtr[11] = board->GetXY(1,0);
          localtr[12] = board->GetXY(1,1);
          localtr[13] = board->GetXY(1,2);
          localtr[14] = board->GetXY(1,3);
          
          //             ADD A NEW LOCAL TRIGGER
          AliMUONLocalTrigger *pLocTrig = new AliMUONLocalTrigger(localtr, fDigitNumbers[icirc]);
          
          fMUONData->AddLocalTrigger(*pLocTrig);  
	  delete pLocTrig;
        }
      }
    }
    pRegTrig->SetLocalOutput(regInpLpt, 0);
    pRegTrig->SetLocalOutput(regInpHpt, 1);
    pRegTrig->SetLocalMask(localMask);
    pRegTrig->SetOutput((regBoard->GetResponse() >> 4) & 0xF); // to be uniformized (oct06 ?)

    fMUONData->AddRegionalTrigger(*pRegTrig);  

  }
  delete pRegTrig;
  
  // GLOBAL TRIGGER INFORMATION: [0] -> LOW PT 
  //                             [1] -> HIGH PT
  //                             [2] -> ALL PT 
  Int_t globalSinglePlus[3], globalSingleMinus[3], globalSingleUndef[3]; 
  Int_t globalPairUnlike[3], globalPairLike[3];   
  
  UShort_t global = fGlobalTriggerBoard->GetResponse();
  
  globalPairUnlike[0] = (global &  16) >> 4;
  globalPairUnlike[1] = (global & 256) >> 8;
  globalPairUnlike[2] = (global &   1);
  
  globalPairLike[0] = (global &  32) >> 5;
  globalPairLike[1] = (global & 512) >> 9;
  globalPairLike[2] = (global &   2) >> 1;
  
  globalSinglePlus[0] = ((global &  192) >>  6) == 2;
  globalSinglePlus[1] = ((global & 3072) >> 10) == 2;
  globalSinglePlus[2] = ((global &   12) >>  2) == 2;
  
  globalSingleMinus[0] = ((global &  192) >>  6) == 1;
  globalSingleMinus[1] = ((global & 3072) >> 10) == 1;
  globalSingleMinus[2] = ((global &   12) >>  2) == 1;
  
  globalSingleUndef[0] = ((global &  192) >>  6) == 3;
  globalSingleUndef[1] = ((global & 3072) >> 10) == 3;
  globalSingleUndef[2] = ((global &   12) >>  2) == 3;
  
  AliMUONGlobalTrigger *pGloTrig = new AliMUONGlobalTrigger(globalSinglePlus, globalSingleMinus,
                                                            globalSingleUndef, globalPairUnlike, 
                                                            globalPairLike);
  
  // ADD A LOCAL TRIGGER IN THE LIST 
  fMUONData->AddGlobalTrigger(*pGloTrig);
  delete pGloTrig;

  // NOW RESET ELECTRONICS
  Reset();
}

//_______________________________________________________________________
void AliMUONTriggerElectronics::ClearDigitNumbers()
{
// RESET fDigitNumbers
	for (Int_t i=0; i<AliMUONConstants::NTriggerCircuit(); i++) fDigitNumbers[i].Set(0);
}

//_______________________________________________________________________
void AliMUONTriggerElectronics::DigitFiredCircuit(Int_t circuit, Int_t cathode,
                                                  Int_t chamber, Int_t digit)
{
// REGISTERS THAT THE SPECIFIED DIGIT FIRED THE SPECIFIED CIRCUIT
// THIS DIGIT GETS ADDED TO AN ARRAY WHICH WILL BE COPIED TO
// AliMUONLocalTrigger WHEN SUCH AN OBJECT IS CREATED FOR EACH CIRCUIT
	Int_t digitnumber = AliMUONLocalTrigger::EncodeDigitNumber(chamber, cathode, digit);
	Int_t last = fDigitNumbers[circuit].GetSize();
	fDigitNumbers[circuit].Set(last + 1);
	fDigitNumbers[circuit][last] = digitnumber;
}

