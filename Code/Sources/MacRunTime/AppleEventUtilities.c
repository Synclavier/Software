/* AppleEventUtilities.C *///	Mac OS Includes#include <Files.h>#include <Processes.h>#include <AERegistry.h>// PowerPlant#include <LModelObject.h>#include <LModelDirector.h>#include <LModelProperty.h>#include <UAppleEventsMgr.h>#include <UExtractFromAEDesc.h>//	Local Includes#include "AppleEventUtilities.h"// -----------------------// � Apple Event Utilities// -----------------------// � AEU_LookUpFinderPSNstatic	ProcessSerialNumber	AEU_LookUpFinderPSN(){	ProcessSerialNumber	thePSN = {0, 0};	ProcessSerialNumber	noPSN  = {0, 0};	ProcessInfoRec		theProc;	theProc.processInfoLength = sizeof(ProcessInfoRec);	theProc.processName       = nil;	theProc.processAppSpec    = nil;	theProc.processLocation   = nil;		while (true) 	{		if (::GetNextProcess(&thePSN))			return (noPSN);					if (::GetProcessInformation(&thePSN, &theProc))			return (noPSN);				if ((theProc.processType      == 'FNDR') &&		    (theProc.processSignature == 'MACS'))		    	return (thePSN);	}	return (noPSN);}//	select folder "Projects" of folder "Development" of folder "SDC" of folder "Daily Incremental" of folder "CJData" of disk "Data"//	reveal selection//	activate// � AEU_SendRevealInFindervoid	AEU_SendRevealInFinder(struct FSSpec& theSpec){	ProcessSerialNumber	finderPSN    = AEU_LookUpFinderPSN();	StAEDescriptor		finderAEDesc(typeProcessSerialNumber, &finderPSN, sizeof(finderPSN));	AppleEvent			selectEvent;	AppleEvent			revealEvent;	AppleEvent			activateEvent;	OSErr      			err;	// Select the file	err = ::AECreateAppleEvent(kAEMiscStandards, kAESelect, &finderAEDesc.mDesc,	                           kAutoGenerateReturnID, kAnyTransactionID, &selectEvent);	if (err) return;		err = ::AEPutParamPtr(&selectEvent, keyDirectObject, typeFSS,								&theSpec, sizeof(FSSpec));	if (err) return;		err = ::AESend(&selectEvent, NULL, kAENoReply + kAECanSwitchLayer + kAEAlwaysInteract,                   kAENormalPriority, kAEDefaultTimeout, NULL, NULL);	if (err) return;			// Make Object Visible	err = ::AECreateAppleEvent(kAEMiscStandards, kAEMakeObjectsVisible, &finderAEDesc.mDesc,	                           kAutoGenerateReturnID, kAnyTransactionID, &revealEvent);		if (err) return;		err = ::AEPutParamPtr(&revealEvent, keyDirectObject, typeFSS,								&theSpec, sizeof(FSSpec));	if (err) return;		err = ::AESend(&revealEvent, NULL, kAENoReply + kAECanSwitchLayer + kAEAlwaysInteract,                   kAENormalPriority, kAEDefaultTimeout, NULL, NULL);			if (err) return;		// Activate the Finder	err = ::AECreateAppleEvent(kAEMiscStandards, kAEActivate, &finderAEDesc.mDesc,	                           kAutoGenerateReturnID, kAnyTransactionID, &activateEvent);		if (err) return;		err = ::AESend(&activateEvent, NULL, kAENoReply + kAECanSwitchLayer + kAEAlwaysInteract,                   kAENormalPriority, kAEDefaultTimeout, NULL, NULL);}