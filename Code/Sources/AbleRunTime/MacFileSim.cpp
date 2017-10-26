//  MacFileSim.c

/*  Contents:                                                               */
/*      Routine provides simulation of SCSI hard disk using file system		*/

// Our inclues
#include    "ScsiChip.h"
#include    "SCSILib.h"
#include    "Utility.h"
#include    "MacFileSim.h"
#include    "XPL.h"
#include    "XPLRuntime.h"
#include    "SyncAudioUtilities.h"
#include    "catrtns.h"

// Mac inculdes
#include 	<String.h>
#include	<StdLib.h>
#include	<StdIO.h>

// Callback for detecting sequence callup
MacFileSimCallback  mac_file_sim_sequence_file_accessed_proc;
void*               mac_file_sim_sequence_file_accessed_arg;

// Initialize file simulation

chip_error_code initialize_mac_file_sim_scsi_chip()
{
	return (CHIP_GOOD_STATUS);  
}


// Perform file simulation

chip_error_code issue_mac_file_sim_scsi_command  (scsi_device& the_device, scsi_command& the_command)
{
	byte	opcode = the_command.fWCommand[0] >> 8;
	unsigned int	count;

	// Init return values	
	the_command.fNumBytesXferd = 0;
	the_command.fScsiStatus    = (uint16) (-1);

	if (the_device.fFRefNum == 0)
		return (CHIP_NO_RESPONSE);

	// Do it

	switch (opcode)
	{
		case	INQUIRY:
		{
			standard_inquiry_data* inq = (standard_inquiry_data *) the_command.fData;
			uint16 asked_for           = the_command.fWCommand[2] >> 8;

			initialize_scsi_chip();

			zero_mem(the_command.fData, asked_for);

			if (asked_for > 0)
				inq->ptype = DISK_TYPE;

			if (((byte *) inq->vendor   - (byte *) inq) +  8 < asked_for)
				strncpy((char *) &(inq->vendor  [0]), "Synclav ", 8);

			if (((byte *) inq->product  - (byte *) inq) + 16 < asked_for)
				strncpy((char *) &(inq->product [0]), "Disk Image File ", 16);

			if (((byte *) inq->revision - (byte *) inq) +  8 < asked_for)
				strncpy((char *) &(inq->revision[0]), "0   ", 4);

			if (asked_for > 1)
				inq->rmb = 1;

			the_command.fNumBytesXferd += asked_for;
			break;
		}

		case	TEST_UNIT_READY:			/* no data, good status == device is ready!		*/
		case	START_UNIT:
		case	PREVENT_REMOVAL:
		case	RESERVE_UNIT:
		case	RELEASE_UNIT:
			break;

		case	READ_CAPACITY:
		{
			read_capacity_data* cap = (read_capacity_data *) the_command.fData;

            SInt64 file_size = 0;
            
            // Report synthesized size of segmented disk
            if (the_device.fSegManager) {
                SyncMutexWaiter   waiter(SyncScsiSegmentizerMutex);

                if (the_device.fSegManager) {
                    scsi_segmentizer& izer = * (scsi_segmentizer*) the_device.fSegManager;
                    file_size = ((SInt64) izer.fBlockStartOnDevice + (SInt64) izer.fNumBlocksOnDevice) * 512;
                }
            }

            else {
                #if __LP64__
                    file_size = (long long) lseek(the_device.fFRefNum, 0, SEEK_END);
                
                    if (file_size == -1) return (CHIP_BAD_STATUS);
                #else
                    if (FSGetForkSize(the_device.fFRefNum, &file_size))	return (CHIP_BAD_STATUS);
                #endif
             }
            
            if (file_size >= 512)			/* reduce by 1 sector since read-cap			*/
                file_size -= 512;			/* returns last available block address			*/
            
            cap->num_blocks = (uint32) (file_size >> (8+1));
			cap->block_size = 512;

			the_command.fNumBytesXferd += sizeof(*cap);
			break;
		}


		case	READ_EXTENDED:
		{
			uint32 block_address =  (((uint32) (the_command.fWCommand[1])) << 16) | ((uint32) the_command.fWCommand[2]);
			uint32 block_count   = ((((uint32) (the_command.fWCommand[3])) << 16) | ((uint32) the_command.fWCommand[4])) >> 8;
			
			if (the_device.fBlockSize == 0)
				return (CHIP_BAD_STATUS);
            
            // Look for segmentized disk
            if (the_device.fSegManager) {
                SyncMutexWaiter   waiter(SyncScsiSegmentizerMutex);
                scsi_segmentizer& izer = * (scsi_segmentizer*) the_device.fSegManager;
                
                if (&izer) {
                    // Handle read of catalog sectors
                    if (block_address + block_count <= 4) {
                        count = block_count * the_device.fBlockSize;
                        memcpy((void *) the_command.fData, (const void *) &izer.fCatBuf[block_address*256], count); // Note stride of 2 bytes in fCatBuf...
                        
                        the_command.fNumBytesXferd += count;
                        
                        break;
                    }
                    
                    // See if read request is in a simulated ABLE file. Note we don't support a single read spanning multiple files
                    if ((block_address             >= izer.fBlockStartOnDevice)
                    &&  (block_address+block_count <= izer.fBlockStartOnDevice + izer.fNumBlocksOnDevice)) {
                        scsi_segment* segP = scsi_segment_for_range(izer, block_address, block_count);
                        
                        if (segP) {
                            scsi_segment& seg   = *segP;
                            size_t        req   = (size_t) (block_count                          ) * (size_t) the_device.fBlockSize;
                            off_t         where = (off_t ) (block_address - seg.fSegStartOnDevice) * (off_t ) the_device.fBlockSize;
                            
                            // Handle converted sound file - file is in memory. Copy the data being read.
                            if (seg.fSegStash) {
                                // Only handle raw synclavier sound files at this level
                                if (seg.fSegStash->stashType != SYNCLAVIER_SOUND_FILE_STASH)
                                    return (CHIP_BAD_STATUS);
                                
                                // Detect read off end of sound file. Happens if valid data is incorrectly swizzled.
                                if (where >= seg.fSegStash->stashLength) {
                                    SyncPrintf("Read off end of audio data stash\n");
                                    
                                    memset((void *) the_command.fData, 0, req);
                                }
                                
                                // Detect read past end of sound file. This is normal; we zero fill to the sector boundary
                                else if (where+req > seg.fSegStash->stashLength) {
                                    memcpy((void *) the_command.fData, (const void *) &seg.fSegStash->stashData.stashData[where], seg.fSegStash->stashLength - where);
                                    memset((void *) (the_command.fData + (seg.fSegStash->stashLength - where)), 0, req - (seg.fSegStash->stashLength - where));
                                }
                                
                                // Else is a good read
                                else
                                    memcpy((void *) the_command.fData, (const void *) &seg.fSegStash->stashData.stashData[where], req);
                                
                                the_command.fNumBytesXferd += req;
 
                                break;
                            }
                            
                            // Detect start of a sequence recall. Latch path
                            if (seg.fSegAbleType == t_sync && (block_address == seg.fSegStartOnDevice) && seg.fSegURLRef) {
                                
                                // Report to app for publishing to Save menu
                                if (mac_file_sim_sequence_file_accessed_proc)
                                    mac_file_sim_sequence_file_accessed_proc(mac_file_sim_sequence_file_accessed_arg, seg.fSegURLRef, false);
                            }
                            
                            // Synthesize empty sequence files here. That is, the file exists in the able catalog
                            // structure, but the file does not yet exist on the mac. Synthesize all zeroes.
                            if (seg.fSegFileRef->GetFile() == 0) {
                                if (seg.fSegSynthsZeroes) {
                                    memset((void *) the_command.fData, 0, req);
                                    
                                    the_command.fNumBytesXferd += req;
                                    
                                    break;
                                }
                                
                                // File does not exist and does not allow us to create it or synthesize zeroes
                                return (CHIP_BAD_STATUS);
                            }
                            
                            //SyncPrintf("Read Segment %s %d %d\n", SyncCFSTR(seg.fSegFileRef->GetName()), block_address, block_count);
                            
                            ssize_t result = pread(seg.fSegFileRef->GetFile(), (void *) the_command.fData, req, where);
                            
                            // No data - see if we can return synthesized zeroes
                            if (result == -1) {
                                // Synthesize zeroes if reading past end of a file that allows synthesized zeroes
                                if ((where >= seg.fSegFileRef->Size()) && seg.fSegSynthsZeroes) {
                                    memset((void *) the_command.fData, 0, req);
                                    
                                    the_command.fNumBytesXferd += req;
                                    
                                    break;
                                }
                                
                                return (CHIP_BAD_STATUS);
                            }
                            
                            // Else swizle the data that was read in
                            else {
                                #if __LITTLE_ENDIAN__
                                {
                                    ufixed * dataPtr = (ufixed *) the_command.fData;
                                    ssize_t   i      = result >> 1;
                                    
                                    while (i--) {
                                        *dataPtr = CFSwapInt16BigToHost(*dataPtr);
                                        dataPtr++;
                                    }
                                }
                                #endif
                            }
                            
                            // Allow reading past end of file - return zeroes.
                            if (result < req && seg.fSegSynthsZeroes)
                                memset((void *) (the_command.fData + result), 0, req - result);

                            the_command.fNumBytesXferd += req;
     
                            break;
                        }
                        
                        else
                            SyncPrintf("Missing Segment For Read: %d %d\n", block_address, block_count);
                    }
                }
            }
			
			// Synchronous
			#if (!SYNC_USE_ASYNC_IO)
				count = block_count * the_device.fBlockSize;
		
				if ((XPLRunTime_FSReadFork (the_device.fFRefNum, fsFromStart + kFSNoCacheMask, (long long) block_address * (long long) the_device.fBlockSize, count, the_command.fData, &count))
				||  (count != (block_count * the_device.fBlockSize)))
					return (CHIP_BAD_STATUS);
		
				the_command.fNumBytesXferd += count;
			#endif
			
			// Asynchronous
			#if (SYNC_USE_ASYNC_IO)
                FSForkIOParam rec;
				
                memset(&rec, 0, sizeof(rec));
				
				rec.forkRefNum     = the_device.fFRefNum;
				rec.buffer         = (Ptr) the_command.fData;
				rec.requestCount   = block_count * the_device.fBlockSize;
				rec.positionMode   = fsFromStart + kFSNoCacheMask;
				rec.positionOffset = (SInt64) block_address * (SInt64) the_device.fBlockSize;
				rec.ioResult       = 1;
				
				if (SCSILib_yielder)
				{
					PBReadForkAsync(&rec);

					while (rec.ioResult > 0)
						if (SCSILib_yielder)
						{
							SCSILib_yield_count++;
							SCSILib_yielder();				// Note: SCSILib_yielder could disappear during this call
							SCSILib_yield_count--;
						}
				}
				
				else
					PBReadForkSync(&rec);
				
				if (rec.ioResult != noErr)
					return (CHIP_BAD_STATUS);
			
				if (rec.actualCount != rec.requestCount)
					return (CHIP_BAD_STATUS);
				
				the_command.fNumBytesXferd += rec.actualCount;
				
				#if __LITTLE_ENDIAN__
				{
					ufixed * dataPtr = (ufixed *) the_command.fData;
					UInt32   i       = rec.actualCount >> 1;
					
					while (i--) {
						*dataPtr = CFSwapInt16BigToHost(*dataPtr);
                        dataPtr++;
                    }
				}
				#endif
			#endif
			
			break;
		}
			
		case	WRITE_EXTENDED:
		{
			uint32 block_address =  (((uint32) (the_command.fWCommand[1])) << 16) | ((uint32) the_command.fWCommand[2]);
			uint32 block_count   = ((((uint32) (the_command.fWCommand[3])) << 16) | ((uint32) the_command.fWCommand[4])) >> 8;
			
			if (the_device.fBlockSize == 0)
				return (CHIP_BAD_STATUS);
			
            // Look for segmentized disk
            if (the_device.fSegManager) {
                SyncMutexWaiter   waiter(SyncScsiSegmentizerMutex);
                scsi_segmentizer& izer = * (scsi_segmentizer*) the_device.fSegManager;
                
                if (&izer) {
                    // Handle write to catralog sectors. Not allowed from able; we own it.
                    if (block_address + block_count <= 4)
                        return (CHIP_BAD_STATUS);
                    
                    // See if read request is in a simulated ABLE file. Note we don't support a single read spanning multiple files
                    if ((block_address             >= izer.fBlockStartOnDevice)
                    &&  (block_address+block_count <= izer.fBlockStartOnDevice + izer.fNumBlocksOnDevice)) {
                        scsi_segment* segP = scsi_segment_for_range(izer, block_address, block_count);
                        
                        if (segP) {
                            scsi_segment& seg   = *segP;
                            size_t        req   = (size_t) (block_count                          ) * (size_t) the_device.fBlockSize;
                            off_t         where = (off_t ) (block_address - seg.fSegStartOnDevice) * (off_t ) the_device.fBlockSize;
                            
                            // Disallow writes to synthesized files (e.g. converted sound files)
                            if (seg.fSegStash)
                                return (CHIP_BAD_STATUS);
                           
                            // Detect start of a sequence save. Latch path
                            if (seg.fSegAbleType == t_sync && (block_address == seg.fSegStartOnDevice) && seg.fSegURLRef) {
                                
                                // Report to app for publishing to Save menu
                                if (mac_file_sim_sequence_file_accessed_proc)
                                    mac_file_sim_sequence_file_accessed_proc(mac_file_sim_sequence_file_accessed_arg, seg.fSegURLRef, true);
                            }
                           
                            // If file does not exist, create it here
                            if (seg.fSegFileRef->GetFile() == 0) {
                                if (seg.fSegAllowsCreate) {
                                    if (seg.fSegFileRef->MkPath() != noErr)
                                        return (CHIP_BAD_STATUS);
                                    
                                    if (seg.fSegFileRef->Create(seg.fSegCreatedType, 'SNCL') != noErr)
                                        return (CHIP_BAD_STATUS);
                                    
                                    if (seg.fSegFileRef->Open(O_RDWR))
                                        return (CHIP_BAD_STATUS);
                                 }
                                
                                // File does not exist and does not allow us to create it or synthesize zeroes
                                else
                                    return (CHIP_BAD_STATUS);
                            }
                           
                            //SyncPrintf("Write Segment %s %d %d\n", SyncCFSTR(seg.fSegFileRef->GetName()), block_address, block_count);
                            
                            #if __BIG_ENDIAN__
                                ssize_t result = pwrite(seg.fSegFileRef->GetFile(), (void *) the_command.fData, req, where);
                                
                                if (result == -1)
                                    return (CHIP_BAD_STATUS);
                                
                                if (result < req)
                                    return (CHIP_BAD_STATUS);
                                
                                the_command.fNumBytesXferd += req;
                            #else
                                unsigned int   globCount = (unsigned int) req;
                                char*		   globBuf   = (char *) the_command.fData;
                                unsigned int   globDone  = 0;
                                
                                SyncSwizzleBuffer swb;
                                
                                if (!swb.Buffer())
                                    return (CHIP_BAD_STATUS);
                                
                                while (globCount)
                                {
                                    unsigned int chunkCount = swb.Size();
                                    ufixed* dataPtr = (ufixed *) globBuf;
                                    ufixed* writPtr = (ufixed *) swb.Buffer();
                                    int i;
                                    
                                    if (chunkCount > globCount)
                                        chunkCount = globCount;
                                    
                                    i = chunkCount >> 1;
                                    
                                    while (i--)
                                        *writPtr++ = CFSwapInt16BigToHost(*dataPtr++);
                                    
                                    ssize_t result = pwrite(seg.fSegFileRef->GetFile(), (void *)swb.Buffer(), (size_t) chunkCount, where);
                                    
                                    if (result == -1)
                                        return (CHIP_BAD_STATUS);
                                    
                                    if (result < (ssize_t) chunkCount)
                                        return (CHIP_BAD_STATUS);
                                    
                                    the_command.fNumBytesXferd += (uint32)       result;
                                    globDone                   += (unsigned int) result;
                                    
                                    globBuf   += chunkCount;
                                    globCount -= chunkCount;
                                    
                                    where     += (off_t) chunkCount;
                                }
                            #endif
                            
                            break;
                        }
                        
                        else
                            SyncPrintf("Missing Segment For Write: %d %d\n", block_address, block_count);
                    }
                }
            }
			
			// Synchronous
			#if (!SYNC_USE_ASYNC_IO)
				count = block_count * the_device.fBlockSize;
		
				if ((XPLRunTime_FSWriteFork (the_device.fFRefNum, fsFromStart + kFSNoCacheMask, (long long) block_address * (long long) the_device.fBlockSize, count, the_command.fData, &count))
				||  (count != (block_count * the_device.fBlockSize)))
					return (CHIP_BAD_STATUS);
				
				the_command.fNumBytesXferd += count;
			#endif
			
			// Asynchronous
			#if (SYNC_USE_ASYNC_IO)
                FSForkIOParam rec;
				
                #if __BIG_ENDIAN__
					memset(&rec, 0, sizeof(rec));
					
					rec.forkRefNum     = the_device.fFRefNum;
					rec.buffer         = (Ptr) the_command.fData;
					rec.requestCount   = block_count * the_device.fBlockSize;
					rec.positionMode   = fsFromStart + kFSNoCacheMask;
					rec.positionOffset = (SInt64) block_address * (SInt64) the_device.fBlockSize;
					rec.ioResult       = 1;
					
					if (SCSILib_yielder)
					{
						PBWriteForkAsync(&rec);

						while (rec.ioResult > 0)
							if (SCSILib_yielder)
							{
								SCSILib_yield_count++;
								SCSILib_yielder();				// Note: SCSILib_yielder could disappear during this call
								SCSILib_yield_count--;
							}
					}
					
					else
						PBWriteForkSync(&rec);
						
					if (rec.ioResult != noErr)
						return (CHIP_BAD_STATUS);
				
					if (rec.actualCount != rec.requestCount)
						return (CHIP_BAD_STATUS);
					
					the_command.fNumBytesXferd += rec.actualCount;
				#else
					unsigned int   globCount = block_count * the_device.fBlockSize;
					char*		   globBuf   = (char *) the_command.fData;
					unsigned int   globDone  = 0;

                    SyncSwizzleBuffer swb;

					if (!swb.Buffer())
						return (CHIP_BAD_STATUS);
						
					while (globCount)
					{
						unsigned int chunkCount = swb.Size();
						ufixed* dataPtr = (ufixed *) globBuf;
						ufixed* writPtr = (ufixed *) swb.Buffer();
						int i;
						
						if (chunkCount > globCount)
							chunkCount = globCount;
						
						i = chunkCount >> 1;
						
						while (i--)
							*writPtr++ = CFSwapInt16BigToHost(*dataPtr++);
						
						memset(&rec, 0, sizeof(rec));
						
						rec.forkRefNum     = the_device.fFRefNum;
						rec.buffer         = (Ptr) swb.Buffer();
						rec.requestCount   = chunkCount;
						rec.positionMode   = fsFromStart + kFSNoCacheMask;
						rec.positionOffset = (SInt64) block_address * (SInt64) the_device.fBlockSize;
						rec.ioResult       = 1;
						
						PBWriteForkAsync(&rec);

						while (rec.ioResult > 0)
							if (SCSILib_yielder)
							{
								SCSILib_yield_count++;
								SCSILib_yielder();				// Note: SCSILib_yielder could disappear during this call
								SCSILib_yield_count--;
							}
							
						if (rec.ioResult != noErr)
							return (CHIP_BAD_STATUS);
					
						if (rec.actualCount != rec.requestCount)
							return (CHIP_BAD_STATUS);
						
						the_command.fNumBytesXferd += rec.actualCount;
						
						globDone += rec.actualCount;
						
						globBuf   += chunkCount;
						globCount -= chunkCount;
						
						block_address += (chunkCount / the_device.fBlockSize);
					}
				#endif
			#endif

			break;
		}

		case	FORMAT_UNIT:
		{
			byte	buffer[2048];
			zero_mem(&buffer[0], sizeof(buffer));								/* create format data of all zeroes for our purposes	*/

            count = 0;

            if ((XPLRunTime_FSWriteFork(the_device.fFRefNum, fsFromStart, 0, sizeof(buffer), buffer, &count))
            ||	(count != sizeof(buffer)))
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
			printf("SCSI: Unrecognized SCSI command for file (%d)\n", opcode);
			return (CHIP_BAD_STATUS);
	}

	the_command.fScsiStatus = (uint16) 0;

	return (CHIP_GOOD_STATUS);
}
