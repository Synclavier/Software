// =================================================================================
//	MacHFSLib.h
// =================================================================================

// Std C
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mac OS
#include <Files.h>

// PowerPlant
#include <LThread.h>

// Addons
#include "LCStr255.h"

// Local includes
#include "AbleDiskLib.h"
#include "MacHFSLib.h"
#include "CHFSImageFile.h"
#include "ScsiLib.h"
#include "XPLRuntime.h"

// =================================================================================
//		¥ MacHFSLib_ReadAbleHFSDirectory()
// =================================================================================

#define	MALLOC_SIZE 50
static	FSCatalogInfo*	catalogInfos = NULL;
static	FSSpec*			specs        = NULL;

int	MacHFSLib_ReadAbleHFSDirectory(InterChangeItemDirectory &itsData, char *the_tree_name)
{
	FSRefParam		myPB;
	FSRef			aRef;
	FSSpec			infoSpec;
	FSCatalogInfo	catInfo;
	OSErr			FSstatus;
	
	// See how many files are in the directory
	
	memset(&infoSpec, 0, sizeof(infoSpec));
	memset(&myPB,	  0, sizeof(myPB	));
	memset(&aRef,	  0, sizeof(aRef	));
	memset(&catInfo,  0, sizeof(catInfo ));

	infoSpec = itsData.root.HFSItem.file_spec;					// grab the file spec
	
	if ((FSstatus = FSpMakeFSRef(&infoSpec, &aRef)) != 0)
	{
		printf("InterChangeª: Could not get information on '%s'\n", the_tree_name);
		return (-1);
	}
	
	myPB.ref       = &aRef;
	myPB.catInfo   = &catInfo;
	myPB.whichInfo = kFSCatInfoNodeFlags | kFSCatInfoVolume | kFSCatInfoNodeID | kFSCatInfoValence;
	
	if ((FSstatus = PBGetCatalogInfoSync(&myPB)) != 0)
	{
		printf("InterChangeª: Could not get information on '%s'\n", the_tree_name);
		return (-1);
	}
	
	if ((catInfo.nodeFlags & kioFlAttribDirMask) == 0)
	{
		printf("InterChangeª: '%s' is not a directory\n", the_tree_name);
		return (-1);
	}
	
	// Loop over files and construct the directory
	long		numEntries = catInfo.valence;
	short		volRef     = catInfo.volume;
	long		parID      = catInfo.nodeID;
	FSIterator	anIterator = NULL;
	
	if (!catalogInfos)
		catalogInfos = new FSCatalogInfo[MALLOC_SIZE];
		
	if (!specs)
		specs = new FSSpec[MALLOC_SIZE];
		
	if (!catalogInfos || !specs)
	{
		printf("InterChangeª: Could not get information on '%s'\n", the_tree_name);
		return (-1);
	}
		
	if ((FSstatus = FSOpenIterator(&aRef,kFSIterateFlat, &anIterator)) != 0)
	{
		printf("InterChangeª: Could not get information on '%s'\n", the_tree_name);
		return (-1);
	}
	
	itsData.blocks_total = 0;
	itsData.blocks_used  = 0;
	itsData.num_items    = 0;

	// Get memory for the answer
	if (numEntries)										// allocate output storage
	{
		itsData.void_items = malloc(numEntries * sizeof(MacHFSItem));

		if (itsData.void_items == 0)
			{printf("InterChangeª: Ran out of memory reading directory\n"); return (-1);}

		itsData.item_size = sizeof(MacHFSItem);
	}

	// Get info for the directory contents
	while (itsData.num_items < numEntries)
	{
		long		index  = 0;
		ItemCount	chunk  = (numEntries - itsData.num_items < MALLOC_SIZE) ? numEntries - itsData.num_items : MALLOC_SIZE;
		ItemCount	actual = 0;
		Boolean		changed;
		long long	dataLength, rsrcLength;
		
		if ((FSstatus = FSGetCatalogInfoBulk(anIterator, chunk, &actual, &changed, kFSCatInfoNodeFlags | kFSCatInfoDataSizes | kFSCatInfoRsrcSizes | kFSCatInfoFinderInfo, catalogInfos, NULL, specs, NULL)) != 0)
		{
			if (FSstatus == errFSNoMoreItems)
				break;
				
			printf("InterChangeª: Could not get information on '%s'\n", the_tree_name);
			return (-1);
		}
		
		// If can't get any more, must be done.
		if (!actual)
			break;
		
		for (index = 0; index < actual; index++)
		{
			MacHFSItem	  &theEntry = itsData.mac_items[itsData.num_items];						// point to entry
			FSCatalogInfo &cinfo    = catalogInfos[index];
			FInfo		  &finfo    = * (FInfo *) &cinfo.finderInfo[0];
			FSSpec		  &spec     = specs[index];
			short		  fileType;
				
			memset(&theEntry, 0, sizeof(theEntry));
			
			// Map to file type
			if (spec.name[0] && spec.name[1] == '.')											// name begins with period - inherently invisible
				{fileType = InterChange_HFSFolderTypeCode_unk; dataLength = 0; rsrcLength = 0;}
				
			else if ((cinfo.nodeFlags & 0x10) != 0)												// nested folder type
				{fileType = InterChange_HFSFolderTypeCode_fold; dataLength = 0; rsrcLength = 0;}
				
			else if (finfo.fdType == 'fdrp')													// alias to a folder
				{fileType = InterChange_HFSFolderTypeCode_unk; dataLength = 0; rsrcLength = 0;}
			
			else if (finfo.fdType == 'AIFF')													// AIFF
			{
				fileType   = InterChange_HFSFolderTypeCode_AIFF;
				dataLength = cinfo.dataLogicalSize;
				rsrcLength = cinfo.rsrcLogicalSize;
			}
			
			else if (finfo.fdType == 'Sd2f')													// SD2
			{
				fileType   = InterChange_HFSFolderTypeCode_Sd2f;
				dataLength = cinfo.dataLogicalSize;
				rsrcLength = cinfo.rsrcLogicalSize;
			}

			else if (finfo.fdType == 'WAVE')													// WAVE
			{
				fileType   = InterChange_HFSFolderTypeCode_WAVE;
				dataLength = cinfo.dataLogicalSize;
				rsrcLength = cinfo.rsrcLogicalSize;
			}
			
			else
			{
				fileType   = GetAbleFileType(finfo.fdType, finfo.fdCreator, spec);
				dataLength = cinfo.dataLogicalSize;
				rsrcLength = cinfo.rsrcLogicalSize;
				
				if (fileType == (-1))
					fileType = InterChange_HFSFolderTypeCode_unk;
			}
			
			// Construct entry (name was set above).  Toss uknown files, invisible files, and alii
			if ((fileType != InterChange_HFSFolderTypeCode_unk)
			&&  ((finfo.fdFlags & (kIsInvisible | kIsAlias)) == 0))
			{
				int i;
				
				theEntry.file_system       = itsData.root.HFSItem.file_system;					// file system code				
				theEntry.file_type         = fileType;											// file type
				theEntry.file_size_bytes   = dataLength + rsrcLength;							// file size bytes
				theEntry.file_spec.vRefNum = spec.vRefNum;
				theEntry.file_spec.parID   = spec.parID;
				
				for (i=0; i<=spec.name[0]; i++)													// copy explicitly so name is zero filled
					theEntry.file_spec.name[i] = spec.name[i];

				itsData.blocks_used += (((dataLength + 511) / 512) + ((rsrcLength + 511) / 512));
				
				itsData.num_items++;
			}
		}
	}

	FSCloseIterator(anIterator);
	
	itsData.blocks_total = itsData.blocks_used;

	return (noErr);
}	//	end of MacHFSLib_ReadAbleHFSDirectory()


// =================================================================================
//		¥ MacHFSLib_IsolateTopDirectory()
// =================================================================================

// Routine takes a path name and finds the top able directory therein. Designed
// To find the image file with an HFS treename
// For example - MacintoshHD:A Folder:A Subfolder:A Disk Image File.simg:SND-1

void	MacHFSLib_IsolateTopDirectory(InterChangeItemUnion &rootUnion, LCStr255 &rootName, InterChangeItemUnion &topUnion, LCStr255 &topName)
{
	FSRefParam		myPB;
	FSRef			aRef;
	FSCatalogInfo	catInfo;
	FSSpec			infoSpec;
	FSSpec			parentSpec;
	LCStr255		infoName;
	OSErr			FSstatus;
	int				topStart;
	int				nextColon;
	short			itsRef  = 0;
	short			itsCode = 0;
	
	infoSpec  = rootUnion.HFSItem.file_spec;					// start with root file spec
	topStart  = 0;
	nextColon = rootName.Find(':', 1);

	if (nextColon == 0 || nextColon == rootName.Length())		// no colon? or looking for xyz:; return union pointing to directory with name == null string
	{
		topUnion = rootUnion;
		topName  = "\p";
		return;
	}
	
	// Snake down HFS directories until we get to the end or
	// we encounter an image file
	while (1)
	{
		memset(&myPB,	 0, sizeof(myPB	  ));
		memset(&aRef,    0, sizeof(aRef	  ));
		memset(&catInfo, 0, sizeof(catInfo));

		// Get information about this item
		if ((FSstatus = FSpMakeFSRef(&infoSpec, &aRef)) != 0)
		{
			printf("InterChangeª: MacHFSLib_IsolateTopDirectory: Could not get information on '%s'\n", (char *) rootName);
			
			topUnion = rootUnion;
			topName  = rootName;
			return;
		}
		
		myPB.ref       = &aRef;
		myPB.catInfo   = &catInfo;
		myPB.whichInfo = kFSCatInfoNodeFlags | kFSCatInfoVolume | kFSCatInfoNodeID | kFSCatInfoFinderInfo;

		if ((FSstatus = PBGetCatalogInfoSync(&myPB)) != 0)
		{
			printf("InterChangeª: MacHFSLib_IsolateTopDirectory: Could not get information on '%s'\n", (char *) rootName);
			
			topUnion = rootUnion;
			topName  = rootName;
			return;
		}
		
		// If is HFS directory
		if ((catInfo.nodeFlags & kioFlAttribDirMask) != 0)
		{
			int	startChar = nextColon+1;						// char after colon
			int endChar   = rootName.Find(':', startChar);		// next following colon
			
			// Handle case abc:xyz or abc:xyz: where xyz is a directory
			if (startChar >= rootName.Length())					// looking for abc:xyz or abc:xyz: where xyz is a directory
			{
				memset(&topUnion, 0, sizeof(topUnion));			// return union pointing to directory with null string name
				
				topUnion.HFSItem.file_system     = InterChange_MacHFSFIleSystemCode;
				topUnion.HFSItem.file_type       = InterChange_HFSFolderTypeCode_fold;
				topUnion.HFSItem.file_size_bytes = 0;
				
				topUnion.HFSItem.file_spec       = infoSpec;
				
				topName = "\p";
				
				return;
			}
			
			// Handle case abc:afilenamewhichiswaytoolongtobealegitimatemachfsfilename... or abc::
			if (endChar > startChar + 33 || (endChar == 0 && rootName.Length() > startChar + 33) || endChar == startChar)
			{
				memset(&topUnion, 0, sizeof(topUnion));			// return union pointing to directory with null string name
				
				topUnion.HFSItem.file_system     = InterChange_MacHFSFIleSystemCode;
				
				if (topStart == 0)
					topUnion.HFSItem.file_type   = rootUnion.HFSItem.file_type;		// Preserve 'fold' vs 'disk' type in case it ever matters
				else			
					topUnion.HFSItem.file_type   = InterChange_HFSFolderTypeCode_fold;

				topUnion.HFSItem.file_size_bytes = 0;
				topUnion.HFSItem.file_spec       = infoSpec;
				
				topName.Assign((void *) (&rootName[startChar]), (UInt8) (rootName.Length() + 1 - startChar));
				
				return;
			}
			
			// Else descend a level
			parentSpec = infoSpec;
			
			infoSpec.parID = catInfo.nodeID;			// get directory ID of this directory; by definition is parent dir ID of what's in it

			if (endChar == 0)
			{
				infoSpec.name[0] = rootName.Length() + 1 - startChar;	
				memcpy(&infoSpec.name[1], (char *) (&rootName[startChar]), rootName.Length() + 1 - startChar);
				nextColon = rootName.Length() + 1;
			}
			else
			{
				infoSpec.name[0] = endChar - startChar;	
				memcpy(&infoSpec.name[1], (char *) (&rootName[startChar]), endChar - startChar);
				nextColon = endChar;
			}

			topStart  = startChar;
			
			continue;
		}
		
		// Else this item is a file.  See if is an HFS image file.  If so, is another 'directory'
		
		FInfo& finfo   = * (FInfo *) &catInfo.finderInfo[0];
		short  itsType = GetAbleFileTypeFromExtension(infoSpec);
			
		if ((itsType == t_lsubc)
		||  ((finfo.fdCreator == 'SNCL')
		&&   (finfo.fdType    == 'SUBC')))
		{
			itsRef = CHFSImageFile::GetRefNum(infoSpec);							// see if alreay in data base
			
			if (itsRef == 0)
			{
				infoName.Assign((void *) (&rootName[1]), nextColon);				// name of this image file up to but not including possible trailing :
				
				try
				{
					itsRef = CHFSImageFile::Add(infoSpec, (char *) infoName);
				}
				catch (...)
				{
					printf("Failed MacHFSLib_IsolateTopDirectory (out of memory)\n");
				
					topUnion = rootUnion;
					topName  = rootName;
					return;
				}
			}
				
			if (itsRef)
				itsCode = CHFSImageFile::GetDevCode(itsRef);
					
			if (itsCode)
			{
				scsi_device* itsDevice = find_hfs_scsi_device(itsCode);

				if (itsDevice && ((itsDevice->fDeviceType == DEVICE_BLANK_ABLE_OPTICAL) || (itsDevice->fDeviceType == DEVICE_ABLE_OPTICAL)))
				{
					topUnion.OpticalItem.file_system       = InterChange_AbleOpticalFileSystemCode;
					topUnion.OpticalItem.file_type         = InterChange_AbleOpticalDirectory;
					topUnion.OpticalItem.file_size_bytes   = CHFSImageFile::GetCachedLength(itsRef);
					topUnion.OpticalItem.file_device_code  = itsCode;
					topUnion.OpticalItem.file_hfs_refnum   = itsRef;
					topUnion.OpticalItem.file_block_start  = 0;
					topUnion.OpticalItem.file_name[0]      = 0;
				}
				else
				{
					topUnion.WDItem.file_system       = InterChange_AbleWDFileSystemCode;
					topUnion.WDItem.file_type         = t_lsubc;
					topUnion.WDItem.file_size_bytes   = CHFSImageFile::GetCachedLength(itsRef);
					topUnion.WDItem.file_device_code  = itsCode;
					topUnion.WDItem.file_hfs_refnum   = itsRef;
					topUnion.WDItem.file_block_start  = 0;
					topUnion.WDItem.file_name[0]      = 0;
				}
				topName.Assign((void *) (&rootName[nextColon]), (UInt8) (rootName.Length() + 1 - nextColon));
				
				return;
			}
			
			// Else bomb
			topUnion = rootUnion;
			topName  = rootName;
			return;
		}
	
		// Else we are pointing to a real mac file
		if (topStart == 0)  							// Root union is actually a file; likely should never happen
		{
			topUnion = rootUnion;
			topName  = rootName;
			return;
		}

		memset(&topUnion, 0, sizeof(topUnion));			// return union pointing to directory with null string name
		
		topUnion.HFSItem.file_system     = InterChange_MacHFSFIleSystemCode;
		topUnion.HFSItem.file_type       = InterChange_HFSFolderTypeCode_fold;
		topUnion.HFSItem.file_size_bytes = 0;
		
		topUnion.HFSItem.file_spec       = parentSpec;
		
		topName.Assign((void *) (&rootName[topStart]), (UInt8) (rootName.Length() + 1 - topStart));

		return;
	}
}
