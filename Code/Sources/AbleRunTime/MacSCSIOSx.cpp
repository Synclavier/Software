/*  macsim.c                                                                */

/*  Created:                                                                */
/*      06/18/96        C. Jones                                            */

/*  Contents:                                                               */
/*      Macintosh OS X SCSI implementation									*/

#undef printf

#include    "Standard.h"
#include    "ScsiLib.h"
#include    "ScsiChip.h"
#include    "Utility.h"

#include	"XPL.h"
#include    "XPLRuntime.h"

#include	"MacSCSIOSx.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <sys/param.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMediaBSDClient.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOStorageProtocolCharacteristics.h>

BSD_Block_Device	bsd_simulated_disks[7];

chip_error_code	initialize_mac_scsi_port_scsi_chip	()
{
	CFMutableDictionaryRef	classesToMatch = NULL;
	io_iterator_t			mediaIterator  = (io_iterator_t) NULL;
	io_object_t				nextMedia	   = (io_object_t) NULL;
	
	// Look for IOMedia objects
	if ((classesToMatch = IOServiceMatching(kIOMediaClass)) == NULL)
		return CHIP_BAD_STATUS;			// Fail if can't get dictionary
		
	// Look for leaf media on writable disks that represent the whole diski
	CFDictionarySetValue(classesToMatch, CFSTR(kIOMediaLeafKey), kCFBooleanTrue); 
	CFDictionarySetValue(classesToMatch, CFSTR(kIOMediaWritableKey), kCFBooleanTrue); 
	CFDictionarySetValue(classesToMatch, CFSTR(kIOMediaWholeKey), kCFBooleanTrue);
	
	if (IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &mediaIterator)) // (classesToMatch one reference is consumed by this function)
		return (CHIP_GOOD_STATUS);		// Normal termination if no disks found
	
	nextMedia = IOIteratorNext(mediaIterator);
	
	while (nextMedia)
	{
		CFTypeRef	bsdPathAsCFString    = NULL;
		CFTypeRef	deviceSizeAsCFNumber = NULL;
		CFTypeRef	blockSizeAsCFNumber  = NULL;
		char		bsdPath[ MAXPATHLEN ] = {0};
		io_name_t	objectName			  = {0};
		SInt64		deviceSize = 0;
		SInt64		blockSize  = 0;
		SInt32		unitID     = -1;
		
		// Get device size, block size and BSD path for this IOMedia object
		deviceSizeAsCFNumber = IORegistryEntryCreateCFProperty(nextMedia, CFSTR(kIOMediaSizeKey), kCFAllocatorDefault, 0);
		blockSizeAsCFNumber  = IORegistryEntryCreateCFProperty(nextMedia, CFSTR(kIOMediaPreferredBlockSizeKey), kCFAllocatorDefault, 0);
		bsdPathAsCFString    = IORegistryEntryCreateCFProperty(nextMedia, CFSTR(kIOBSDNameKey), kCFAllocatorDefault, 0);
														
		if (deviceSizeAsCFNumber && blockSizeAsCFNumber && bsdPathAsCFString)
		{
			io_registry_entry_t us     = nextMedia;
			io_registry_entry_t parent = (io_registry_entry_t) NULL;
			int					i      = 0;
			size_t devPathLength;

			IORegistryEntryGetName(nextMedia, objectName);
			
			if (CFGetTypeID(deviceSizeAsCFNumber) == CFNumberGetTypeID())
				CFNumberGetValue((CFNumberRef) deviceSizeAsCFNumber, kCFNumberSInt64Type, &deviceSize);
			
			if (CFGetTypeID(blockSizeAsCFNumber) == CFNumberGetTypeID())
				CFNumberGetValue((CFNumberRef) blockSizeAsCFNumber, kCFNumberSInt64Type, &blockSize);
				
			// Look for IOUnit, although appears to not be very useful
			while (i < 10)
			{
				CFTypeRef	ioUnitAsCFNumber = NULL;

				// Look for IOUnit specifier
				if ((ioUnitAsCFNumber = IORegistryEntryCreateCFProperty(us, CFSTR("IOUnit"), kCFAllocatorDefault, 0)) != 0)
				{
					if (CFGetTypeID(ioUnitAsCFNumber) == CFNumberGetTypeID())
					{
						if (CFNumberGetValue((CFNumberRef) ioUnitAsCFNumber, kCFNumberSInt32Type, &unitID))
						{
							CFRelease(ioUnitAsCFNumber);
							break;
						}
					}
							
					CFRelease(ioUnitAsCFNumber);
				}

				// Also look for earlier "SCSI Target Identifier" field
				if ((ioUnitAsCFNumber = IORegistryEntryCreateCFProperty(us, CFSTR(kIOPropertySCSITargetIdentifierKey), kCFAllocatorDefault, 0)) != 0)
				{
					if (CFGetTypeID(ioUnitAsCFNumber) == CFNumberGetTypeID())
					{
						if (CFNumberGetValue((CFNumberRef) ioUnitAsCFNumber, kCFNumberSInt32Type, &unitID))
						{
							CFRelease(ioUnitAsCFNumber);
							break;
						}
					}
							
					CFRelease(ioUnitAsCFNumber);
				}

				// Also look for earlier "SCSI Target" field - seen with Ratoc cardbus unit
				if ((ioUnitAsCFNumber = IORegistryEntryCreateCFProperty(us, CFSTR("SCSI Target"), kCFAllocatorDefault, 0)) != 0)
				{
					if (CFGetTypeID(ioUnitAsCFNumber) == CFNumberGetTypeID())
					{
						if (CFNumberGetValue((CFNumberRef) ioUnitAsCFNumber, kCFNumberSInt32Type, &unitID))
						{
							CFRelease(ioUnitAsCFNumber);
							break;
						}
					}
							
					CFRelease(ioUnitAsCFNumber);
				}

				// No IOUnit at this level - look upwards
				if (!IORegistryEntryGetParentEntry(us, kIOServicePlane, &parent))
					{us = parent; i++;}
				else
					break;
			}
			
			// Now construct path name
			strcpy(bsdPath, _PATH_DEV);
			
			// Add "r" before the BSD node name from the I/O Registry to specify the raw disk
			// node. The raw disk nodes receive I/O requests directly and do not go through
			// the buffer cache.
			
			strcat(bsdPath, "r");
			devPathLength = strlen(bsdPath);
			
			// If could get path, done.
			if (CFStringGetCString((CFStringRef) bsdPathAsCFString, bsdPath + devPathLength, MAXPATHLEN - devPathLength, kCFStringEncodingUTF8))
			{
				// printf("Got device: %s %s %d %d %d\n", objectName, bsdPath, (int) (deviceSize / blockSize), (int) blockSize, (int) unitID);
				if (unitID < 0 || unitID > 6)
					unitID = 6;
					
				// Find free slot if duplicat unit ID; indicates bogus information (most likely) or multiple SCSI buses (also likely with FireWire)
				while (unitID >= 0 && bsd_simulated_disks[unitID].entry_valid)
				{
					// Reuse unit ID if bsd name matches
					if (strcmp(bsd_simulated_disks[unitID].bsd_path, bsdPath) == 0)
						break;
						
					unitID--;
				}
				
				// Add to table
				if (unitID >= 0 && !bsd_simulated_disks[unitID].entry_valid && deviceSize >= blockSize && blockSize > 0)
				{
					BSD_Block_Device& theDisk =	bsd_simulated_disks[unitID];
					
					theDisk.entry_valid = true;
					theDisk.bsd_file_id = -1;
					theDisk.unt_id      = unitID;
					theDisk.num_blocks  = (int) (deviceSize / blockSize);
					theDisk.block_size  = (int)blockSize;
					
					if (strlen(bsdPath) < sizeof(theDisk.bsd_path))
						strcpy(theDisk.bsd_path, bsdPath);

					if (strlen(objectName) < sizeof(theDisk.device_info))
						strcpy(theDisk.device_info, objectName);
				}
			}
		}
		
		if (deviceSizeAsCFNumber) CFRelease(deviceSizeAsCFNumber);
		if (blockSizeAsCFNumber)  CFRelease(blockSizeAsCFNumber);
		if (bsdPathAsCFString)	  CFRelease(bsdPathAsCFString);
		
		IOObjectRelease(nextMedia);

		nextMedia = IOIteratorNext(mediaIterator);
	}

	IOObjectRelease(mediaIterator);
	
	return (CHIP_GOOD_STATUS);
}

// Routine to close all open BSD files - call before quitting just to be nice
chip_error_code	finalize_mac_scsi_port_scsi_chip	()
{
	int i;
	
	for (i=0; i<7; i++)
	{
		BSD_Block_Device& theDisk =	bsd_simulated_disks[i];
		
		if (theDisk.entry_valid && theDisk.bsd_file_id != (-1))
		{
			close(theDisk.bsd_file_id);
			theDisk.bsd_file_id = (-1);
		}
	}
	
	return (CHIP_GOOD_STATUS);
}


// Simulate SCSI commands using BSD API

chip_error_code issue_mac_scsi_port_scsi_command (scsi_device& the_device, scsi_command& the_command)
{
	BSD_Block_Device& theDisk =	bsd_simulated_disks[the_device.fTargetId];
	byte	opcode = the_command.fWCommand[0] >> 8;
	uint32	count;

	// Init return values	
	the_command.fNumBytesXferd = 0;
	the_command.fScsiStatus    = (uint16) (-1);
	
	if (!theDisk.entry_valid)
		return (CHIP_NO_RESPONSE);

	// Do it

	switch (opcode)
	{
		case	INQUIRY:
		{
			standard_inquiry_data* inq = (standard_inquiry_data *) the_command.fData;
			uint16 asked_for           = the_command.fWCommand[2] >> 8;
			int	i = 0;
			int j;

			zero_mem(the_command.fData, asked_for);

			if (asked_for > 0)
				inq->ptype = DISK_TYPE;

			// Stuff first word of object name as vendor - e.g. SEAGATE
			if (((byte *) inq->vendor   - (byte *) inq) +  8 < asked_for)
			{
				j = 0;
				
				while (j<8)
				{
					if (theDisk.device_info[i] == 0 || theDisk.device_info[i] == ' ')	// Space fill after first word
						inq->vendor[j++] = ' ';
					else
						inq->vendor[j++] = theDisk.device_info[i++];
				}
				
				// Skip over space to next word
				while (theDisk.device_info[i] == ' ')
					i++;
			}

			if (((byte *) inq->product  - (byte *) inq) + 16 < asked_for)
			{
				j = 0;
				
				while (j<16)
				{
					if (theDisk.device_info[i] == 0 || theDisk.device_info[i] == ' ')	// Space fill after first word
						inq->product[j++] = ' ';
					else
						inq->product[j++] = theDisk.device_info[i++];
				}
				
				// Skip over space to next word
				while (theDisk.device_info[i] == ' ')
					i++;
			}

			if (((byte *) inq->revision - (byte *) inq) +  8 < asked_for)
			{
				j = 0;
				
				while (j<4)
				{
					if (theDisk.device_info[i] == 0 || theDisk.device_info[i] == ' ')	// Space fill after first word
						inq->revision[j++] = ' ';
					else
						inq->revision[j++] = theDisk.device_info[i++];
				}
				
				// Skip over space to next word
				while (theDisk.device_info[i] == ' ')
					i++;
			}

			if (asked_for > 1)
				inq->rmb = 1;

			the_command.fNumBytesXferd += asked_for;
			break;
		}

		case	START_UNIT:
			// Look for eject
			if ((the_command.fWCommand[2] >> 8) == 2)
			{
				// Try and issue a simple eject. This works if we are the only ap running. It fails if file is shared. It will also fail if the file had been shared but is no longer being shared.
				int pissmeoff = ioctl(theDisk.bsd_file_id, DKIOCEJECT, 0);

				if (pissmeoff != 0)
				{
					close(theDisk.bsd_file_id);
					theDisk.bsd_file_id = (-1);
				
					theDisk.bsd_file_id = open(theDisk.bsd_path, O_RDWR);
					ioctl(theDisk.bsd_file_id, DKIOCEJECT, 0);
				}

				// Close file after ejecting so that we re-open it for next access (assuming user has put in new media)
				close(theDisk.bsd_file_id);
				theDisk.bsd_file_id = (-1);
			}
			break;			

		case	TEST_UNIT_READY:			/* no data, good status == device is ready!		*/
		case	PREVENT_REMOVAL:
		case	RESERVE_UNIT:
		case	RELEASE_UNIT:
			break;

		case	READ_CAPACITY:
		{
			read_capacity_data* cap = (read_capacity_data *) the_command.fData;

			// Store info in native format - words are swizzled when capacity data is forwarded to Able
			cap->num_blocks = theDisk.num_blocks;
			cap->block_size = theDisk.block_size;

			the_command.fNumBytesXferd += sizeof(*cap);
			break;
		}


		case	READ_EXTENDED:
		{
			uint32 block_address =  (((uint32) (the_command.fWCommand[1])) << 16) | ((uint32) the_command.fWCommand[2]);
			uint32 block_count   = ((((uint32) (the_command.fWCommand[3])) << 16) | ((uint32) the_command.fWCommand[4])) >> 8;
			off_t  filePos  = 0;
			size_t numBytes = 0;
			
			if (the_device.fBlockSize == 0)
				return (CHIP_BAD_STATUS);
				
			// printf("got %d\n", system("/usr/bin/tweak_rdisk2"));
			
			if (theDisk.bsd_file_id == (-1))
				theDisk.bsd_file_id = open(theDisk.bsd_path, O_RDWR);

			if (theDisk.bsd_file_id == (-1))
				return (CHIP_BAD_STATUS);
			
			filePos  = ((off_t ) block_address) * ((off_t ) the_device.fBlockSize);
			numBytes = ((size_t) block_count  ) * ((size_t) the_device.fBlockSize);

			if (lseek(theDisk.bsd_file_id, filePos, SEEK_SET) != filePos)
				return (CHIP_BAD_STATUS);
				
			count = (uint32) read(theDisk.bsd_file_id, the_command.fData, numBytes);
			
			if (count != (uint32) numBytes)
				return (CHIP_BAD_STATUS);
			
			the_command.fNumBytesXferd += count;
			
			#if __LITTLE_ENDIAN__
			{
				ufixed * dataPtr = (ufixed *) the_command.fData;
				uint32   i       = count >> 1;
				
				while (i--) {
					*dataPtr = CFSwapInt16BigToHost(*dataPtr);
                    dataPtr++;
                }
			}
			#endif
			
			break;
		}
			
		case	WRITE_EXTENDED:
		{
			uint32 block_address =  (((uint32) (the_command.fWCommand[1])) << 16) | ((uint32) the_command.fWCommand[2]);
			uint32 block_count   = ((((uint32) (the_command.fWCommand[3])) << 16) | ((uint32) the_command.fWCommand[4])) >> 8;
			off_t  filePos  = 0;
			size_t numBytes = 0;
			
			if (the_device.fBlockSize == 0)
				return (CHIP_BAD_STATUS);
				
			if (theDisk.bsd_file_id == (-1))
				theDisk.bsd_file_id = open(theDisk.bsd_path, O_RDWR);

			if (theDisk.bsd_file_id == (-1))
				return (CHIP_BAD_STATUS);

			filePos  = ((off_t ) block_address) * ((off_t ) the_device.fBlockSize);
			numBytes = ((size_t) block_count  ) * ((size_t) the_device.fBlockSize);

			if (lseek(theDisk.bsd_file_id, filePos, SEEK_SET) != filePos)
				return (CHIP_BAD_STATUS);

			#if __BIG_ENDIAN__
				count = (uint32) write(theDisk.bsd_file_id, the_command.fData, numBytes);
				
				if (count != (uint32) numBytes)
					return (CHIP_BAD_STATUS);
				
				the_command.fNumBytesXferd += count;
			#else
				unsigned long  globCount = block_count * the_device.fBlockSize;
				char*		   globBuf   = (char *) the_command.fData;
				unsigned long  globDone  = 0;
            
                SyncSwizzleBuffer swb;
				
				if (!swb.Buffer())
					return (CHIP_BAD_STATUS);
					
				while (globCount)
				{
					unsigned long chunkCount = swb.Size();;
					ufixed* dataPtr = (ufixed *) globBuf;
					ufixed* writPtr = (ufixed *) swb.Buffer();
					long i;
					
					if (chunkCount > globCount)
						chunkCount = globCount;
					
					i = chunkCount >> 1;
					
					while (i--)
						*writPtr++ = CFSwapInt16BigToHost(*dataPtr++);
					
					count = (uint32) write(theDisk.bsd_file_id, (void *) swb.Buffer(), (size_t) chunkCount);
					
					if (count != (uint32) chunkCount)
						return (CHIP_BAD_STATUS);
					
					the_command.fNumBytesXferd += count;
					
					globDone  += count;
					
					globBuf   += chunkCount;
					globCount -= chunkCount;
					
					block_address += (chunkCount / the_device.fBlockSize);
				}
			#endif

			break;
		}

		case	FORMAT_UNIT:
		{
			byte	buffer[2048];
			zero_mem(&buffer[0], sizeof(buffer));								/* create format data of all zeroes for our purposes	*/
			off_t  filePos  = 0;
			size_t numBytes = sizeof(buffer);
			
			if (the_device.fBlockSize == 0)
				return (CHIP_BAD_STATUS);
				
			if (theDisk.bsd_file_id == (-1))
				theDisk.bsd_file_id = open(theDisk.bsd_path, O_RDWR);

			if (theDisk.bsd_file_id == (-1))
				return (CHIP_BAD_STATUS);

			if (lseek(theDisk.bsd_file_id, filePos, SEEK_SET) != filePos)
				return (CHIP_BAD_STATUS);

			count = (uint32) write(theDisk.bsd_file_id, (void *) buffer, numBytes);
			
			if (count != (uint32) numBytes)
				return (CHIP_BAD_STATUS);

			break;
		}

        case    MODE_SENSE:
        {
            uint16 asked_for = the_command.fWCommand[2] >> 8;
            
            zero_mem(the_command.fData, asked_for);
            the_command.fNumBytesXferd += asked_for;
            break;
        }
            
		default:
			printf("MAC SCSI: Unrecognized SCSI command for file (%d)\n", opcode);
			return (CHIP_BAD_STATUS);
	}

	the_command.fScsiStatus = (uint16) 0;

	return (CHIP_GOOD_STATUS);
}

