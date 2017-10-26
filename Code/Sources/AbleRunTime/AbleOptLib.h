/* Able Disk Library */

// Header for Able Optical Library

#pragma once

#include "AbleDiskLib.h"


void	AbleOptLib_InitializeDeviceSetup	();
fixed	AbleOptLib_map_error_code			(fixed status);
int		AbleOptLib_ReadOpticalDirectory		(struct InterChangeItemDirectory &itsData, const char *the_tree_name);
int		AbleOptLib_LocateCategory			(InterChangeItemUnion& topUnion, LCStr255& topName, LCStr255& treeName, InterChangeItemUnion& outUnion,
											 interchange_settings& theSettings);
int		AbleOptLib_CreateVolumeHeader		(scsi_device *the_device, unsigned long block_len, char* itsName);
int		AbleOptLib_RenameFile				(InterChangeItemUnion& topUnion, LCStr255& topName, LCStr255& treeName, LCStr255& theNewName, interchange_settings& theSettings);
int		AbleOptLib_UnsaveFile				(InterChangeItemUnion& topUnion, LCStr255& topName, LCStr255& treeName, interchange_settings& theSettings);
int		AbleOptLib_ConstructOpticalIndex    (InterChangeItemUnion& itsUnion, class CSharedOpticalDataBase& itsOpticalData, long& abortMe, long& fatalUpdateErrorOccured,
											 void (*gearTuner) (void *), void *gearData);
int		AbleOptLib_UpdateSFHeaderFromEntry  (InterChangeItemUnion &theSource, scsi_device* source_device, scsi_device* dest_device, unsigned long dest_block,
											 interchange_settings& theSettings);
int     AbleOptLib_ConstructOpticalIndex    (scsi_device* itsDevice, CSharedOpticalDataBase& itsOpticalData, long& abortMe, long& fatalUpdateErrorOccured);
void    AbleOptLib_ReleaseOpticalIndex      (CSharedOpticalDataBase& itsOpticalData);

int     AbleOptLib_ParseSoundfileHeader     (uint32 num_words,  // word length of sound file: 256*3 for the header plus audio data
                                             class  SynclSFHeader& header,
                                             class  LCStr255 theCaps[8],
                                             int&   numCaps,
                                             class  AudioDataDescriptor& descriptor,
                                             char   Caption[256]);

int     AbleOptLib_FetchSoundfileHeader     (class  CSynclavierFileReference& fileRef,
                                             struct SynclSFHeader& header,
                                             class  LCStr255 theCaps[8],
                                             int&   numCaps,
                                             struct AudioDataDescriptor& descriptor,
                                             char   Caption[256]);
