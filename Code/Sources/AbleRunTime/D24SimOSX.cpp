/*  d24sim.c                                                                */

/*  Created:                                                                */
/*      10/28/98        C. Jones                                            */

/*  Contents:                                                               */
/*      Access D24 SCSI divice from mac using PCI-1 bus bridge				*/

// Local includes
#include    "Standard.h"
#include    "ScsiChip.h"

#include	"XPL.h"
#include    "XPLRuntime.h"
#include	"scsilits.h"
#include    "PCI-1KernelDefs.h"
#include	"D24Sim.h"
#include 	"SynclavierPCILib.h"

#include    <QuartzCore/QuartzCore.h>

// SYNC_USE_ASYNC_IO signifies carbon build (SynclavierX, InterChangeX)
// Sync3 uses !SYNC_USE_ASYNC_IO

#if SYNC_USE_ASYNC_IO
    #include <Carbon/Carbon.h>
#endif

#define DO_TIMING   0

// Global Variables
static	PCI1AccessorStruct* 	D24Sim_PCI1AbleAccessor   = NULL;
static	void					(*D24Sim_ThreadYielder)() = NULL;
static  bool					D24Sim_IsInterpreter      = false;
static	int						nonMainThreadsWaiting	  = 0;

// Initialize & CleanUp
static  int gD24InitedInstances = 0;

void	D24Sim_Initialize(struct PCI1AccessorStruct* itsStruct, void (*threadYielder)(), bool isInterpreter)
{
    // See if already inited within this app.
    if (gD24InitedInstances++ > 0)
        return;
    
	D24Sim_PCI1AbleAccessor = itsStruct;        // May be null
	D24Sim_ThreadYielder    = threadYielder;    // May be null
	D24Sim_IsInterpreter    = isInterpreter;    // May be true
}

void	D24Sim_CleanUp()
{
    // See if multiple instances in use within this app.
    if (--gD24InitedInstances > 0)
        return;
    
	D24Sim_PCI1AbleAccessor = NULL;
	D24Sim_ThreadYielder    = NULL;
}

// Forward reference
static	void	Cleanup(fixed d24_selection);

void	D24Sim_Reset()
{
    fixed	d24_selection = 0;
    fixed	d24id         = 0;
    fixed	bit[] = {1, 2, 4, 8, 16, 32, 64, 128};					// binary to Bit mapping

    // Check for proper setup
    if ((!D24Sim_PCI1AbleAccessor)									// make sure accessor has been set
    ||  (!D24Sim_PCI1AbleAccessor->snarfed)							// make sure able has been snarfed
    ||  (!D24Sim_PCI1AbleAccessor->devreads[024]))					// make sure d24 is available
        return;
    
    SynclavierSharedStruct* sharedStruct = SynclavierPCILib_FetchSharedStruct(SYNCLAVIER_POWERPC_STRUCT_VERSION);
    
    if (!sharedStruct)
        return;
    
    // Grab for reset. We grab it without checking to handle the case of a crashed application that
    // had grabbed it.
    D24Sim_PCI1AbleAccessor->d24_in_use = 1;
    
    // Stop kernel interpretation
    SynclavierPCILib_Suspend();

    // Check for failure
    if (!sharedStruct->ui_processing_needed) {
        OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
        return;
    }
    
    // Save current selection
    d24_selection = XPL_read(024) & s_selmask;						// Get current board selection

    /*** Select the D24 SCSI Host Adapter ***/
    fixed i = shl(bit[d24id], 8);
    XPL_write(024, s_selectenable | i);                             // Enable the D24
    
    if ((XPL_read(024) & s_selmask) != i)                           // Board selected is not there
    {
        Cleanup(d24_selection);
        return;
    }
    
    // Free up our possible use of the bus
    XPL_write(024, 0);                                              // Release data bus
    XPL_write(025, 0);                                              // Release signal bus
    
    // Issue reset
    int count = 0;
    
    // Assert reset line
    XPL_write(025, s_rst);                                          // Assert RST for hard reset
    usleep(1000);
    
    // Wait for up to 1 second for other devices on the bus to release
    while ((count++ < 1000) && (((XPL_read(025) & (s_busmask^s_rst)) != 0) || ((XPL_read(024) & s_datamask) != 0)))
        usleep(1000);
    
    XPL_write(025, 0);                                              // Release signal bus
    
    usleep(1000000);                                                // Wait 1 second
    
    Cleanup(d24_selection);
}

bool	D24Sim_Busy()
{
    fixed	d24_selection = 0;
    fixed	d24id         = 0;
    fixed	bit[] = {1, 2, 4, 8, 16, 32, 64, 128};					// binary to Bit mapping
    bool    isBusy = false;
    
    // Check for proper setup
    if ((!D24Sim_PCI1AbleAccessor)									// make sure accessor has been set
    ||  (!D24Sim_PCI1AbleAccessor->snarfed)							// make sure able has been snarfed
    ||  (!D24Sim_PCI1AbleAccessor->devreads[024]))					// make sure d24 is available
        return false;                                               // can't be busy if not available
    
    SynclavierSharedStruct* sharedStruct = SynclavierPCILib_FetchSharedStruct(SYNCLAVIER_POWERPC_STRUCT_VERSION);
    
    if (!sharedStruct)
        return false;                                               // can't be busy if not available
    
    if (OSAtomicIncrement32(&D24Sim_PCI1AbleAccessor->d24_in_use) > 1) {
        isBusy = true;
        OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
    }
    
    else {
        // Stop kernel interpretation
        SynclavierPCILib_Suspend();
        
        // Check for failure
        if (!sharedStruct->ui_processing_needed) {
            OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
            isBusy = true;
        }
        
        else {
            // Save current selection
            d24_selection = XPL_read(024) & s_selmask;						// Get current board selection
            
            /*** Select the D24 SCSI Host Adapter ***/
            fixed i = shl(bit[d24id], 8);
            XPL_write(024, s_selectenable | i);                             // Enable the D24
            
            if ((XPL_read(024) & s_selmask) != i)                           // Board selected is not there
                isBusy = false;                                             // Can't be busy if no D24 #0
            
            else if ((XPL_read(024) & s_datamask) != 0)                     // Somebody (could be us) is using data bus
                isBusy = true;
            
            else if ((XPL_read(025) & s_busmask ) != 0)                     // Somebody (could be us) is using control bus
                isBusy = true;
            
            else
                isBusy = false;                                             // Can't be busy if no D24 #0
            
            XPL_write(024, s_selectenable | d24_selection);					// Restore board selection
            
            OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
            
            // Goose interpreter to resume
            SynclavierPCILib_WakeMainLoop();
        }
    }

    return isBusy;
}

int		D24Sim_AnyNonMainThreadsWaiting()
{
	return (nonMainThreadsWaiting);
}

// Kill time.  If the D24 is busy, it is being used by another application (not another thread).
// That is, threads don't busy out the D24 and then yield.  The only application that does
// that is Synclavier¨ PowerPCª.

// So to free up the D24:
//	If we are running in a thread, just yield.  The main thread will wait next event which
//     will give time to the other application so it can free up the D24
//  If we are the main thread, we just wait next event ourselves
static	void	kill_time()
{
    #if SYNC_USE_ASYNC_IO
        if (D24Sim_ThreadYielder)                           // if operating in thread environment
        {
            boolean saved_throw   = g_throw_on_disk_error;  // save throw on disk error status so we don't throw across threads
            g_throw_on_disk_error = 0;
            
            nonMainThreadsWaiting++;                        // publish number of non-main threads waiting for the D24
            D24Sim_ThreadYielder();
            nonMainThreadsWaiting--;
            
            g_throw_on_disk_error = saved_throw;
        }
    #endif
   
    usleep(1000);
}

// XPL Translator Glue
ufixed	XPL_read(ufixed which)
{
	return (ufixed) SynclavierPCILib_XPL_Read((int) which);
}

void	XPL_write(ufixed which, fixed data)
{
	SynclavierPCILib_XPL_Write((int) which, (int) data);
}


// SCSI Reset routine
static	void    scsireset()
{
	XPL_write(025, s_rst);												// Assert RST for hard reset
	
    usleep(25);
		
	XPL_write(024, 0);													// Clear the data bus
	XPL_write(025, 0);													// Clear the signal bus
	
    usleep(1000000);                                                    // 1 second reset time
}

// Initialize Chip
chip_error_code	initialize_d24_scsi_port_scsi_chip	()
{
	return (CHIP_GOOD_STATUS);
}

// Command Cleanup
static	void	Cleanup(fixed d24_selection)
{
	XPL_write(024, s_selectenable | d24_selection);						// Restore board selection

	D24Sim_PCI1AbleAccessor->inuse_time = D24Sim_PCI1AbleAccessor->fast_time;

	SynclavierPCILib_JamTLIMRegister(D24Sim_PCI1AbleAccessor->inuse_time);
	
    D24Sim_PCI1AbleAccessor->d24_requested = false;
    D24Sim_PCI1AbleAccessor->d24_released  = false;
    
    OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
    
    // Goose interpreter to resume
    SynclavierPCILib_WakeMainLoop();
}

// Issue SCSI Command

#if DO_TIMING
    static  CFTimeInterval last_time;
    static  CFTimeInterval start_time;
    static  CFTimeInterval end_time;
#endif

chip_error_code issue_d24_scsi_port_scsi_command (scsi_device& the_device, scsi_command& the_command)
{
	short   scsi_message_byte;

	fixed   *DataPointer_w;
	byte	*DataPointer_b;
	uint32  NumDataBytes;
	uint32  num_xferd;
	int     retry_counter = 0;
	
	fixed   *prevadr_w;
	byte	*prevadr_b;
	uint32	prevlen;

	fixed	d24id;
	fixed	initiator;
	fixed	target;
	fixed	lun;

	fixed	i;
	fixed	gotbus;
	fixed	count2;
	fixed	arb_bit;
	
	fixed	bit[] = {1, 2, 4, 8, 16, 32, 64, 128};					// binary to Bit mapping

	fixed	d24_selection = 0;
	
	// Check for proper setup
	if ((!D24Sim_PCI1AbleAccessor)									// make sure accessor has been set
	||  (!D24Sim_PCI1AbleAccessor->snarfed)							// make sure able has been snarfed
	||  (!D24Sim_PCI1AbleAccessor->devreads[024]))					// make sure d24 is available
		return ((chip_error_code) NO_RESPONSE);

    SynclavierSharedStruct* sharedStruct = SynclavierPCILib_FetchSharedStruct(SYNCLAVIER_POWERPC_STRUCT_VERSION);

    if (!sharedStruct)
        return ((chip_error_code) NO_RESPONSE);
    
	// Wait for other possible application using the D24.
    // Grab the semaphore so we have exclusive access to the D24 (as well as poly ram)
	while (OSAtomicIncrement32(&D24Sim_PCI1AbleAccessor->d24_in_use) > 1)
	{
        OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
        
		kill_time();
	}
	
    // Stop kernel interpretation. Main loop will not release the latch
    // while we have d24_in_use.
	SynclavierPCILib_Suspend();
    
    // Check for failure
    if (!sharedStruct->ui_processing_needed) {
        OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
        return ((chip_error_code) NO_RESPONSE);
    }

	// Set up SCSI bus timing and latch the current board selection
	D24Sim_PCI1AbleAccessor->inuse_time = D24Sim_PCI1AbleAccessor->scsi_time;
	
	SynclavierPCILib_JamTLIMRegister(D24Sim_PCI1AbleAccessor->inuse_time);

	d24_selection = XPL_read(024) & s_selmask;						// Get current board selection
	
	// Initialize return and working values
	retry:;                                         				/* re-enter for retry   */
	
	DataPointer_w  = (fixed *) the_command.fData;   				/* for data             */
	DataPointer_b  = (byte  *) the_command.fData;   				/* for data             */
	NumDataBytes   = the_command.fNumDataBytes;     				/* for counting         */
	num_xferd      = 0;
	
	prevadr_w = DataPointer_w;
	prevadr_b = DataPointer_b;
	prevlen   = NumDataBytes;										// Store this address and length

	the_command.fNumBytesXferd = 0;
	the_command.fScsiStatus    = (uint16) (-1);

	d24id     = 0;
	initiator = s_initiator;
	target    = the_device.fTargetId;
	lun       = 0;
	
    // Test access of O1 using W0
    if (0) {
        if (target == 1)    // O1 selected
            target = 5;     // Acess W0
    }
    
	/*** Select the D24 SCSI Host Adapter ***/	
	i = shl(bit[d24id], 8);
	XPL_write(024, s_selectenable | i);                             // Enable the D24
	
	if ((XPL_read(024) & s_selmask) != i)                           // Board selected is not there
	{
		Cleanup(d24_selection);
		return ((chip_error_code) NO_RESPONSE);
	}
	
	/*** Arbitration Phase ***/
	
	gotbus  = false;                                                // init to not done
	count2  = 0;
	arb_bit = bit[initiator];                                       // bit to arbitrate with
	
	
	/* Arbitrate for and latch on to SCSI bus: */
	
	while (gotbus == false) {                                       // wait here till we get buss
	
		/* Wait for bus free state: */
		
		while ((XPL_read(025) & s_busmask) != 0) {
            int count1  = 0;
           
            // The only practical occurence of a bus busy state is if S3 is polling for a DTD
            if (++count2 > 4) {
                Cleanup(d24_selection);
                return ((chip_error_code) NO_RESPONSE);             // Actually could not get bus; don't know why
            }
            
            XPL_write(024, s_selectenable | d24_selection);         // Restore board selection
            
            D24Sim_PCI1AbleAccessor->inuse_time = D24Sim_PCI1AbleAccessor->fast_time;
            
            SynclavierPCILib_JamTLIMRegister(D24Sim_PCI1AbleAccessor->inuse_time);
            
            // Ask Sync3 to release the D24
            D24Sim_PCI1AbleAccessor->d24_released  = false;
            D24Sim_PCI1AbleAccessor->d24_requested = true;
            
            OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);

            SynclavierPCILib_WakeMainLoop();
            
            while (D24Sim_PCI1AbleAccessor->d24_released == false && count1++ < 500)
                kill_time();
            
            // Wait for other possible application using the D24.
            // Grab the semaphore so we have exclusive access to the D24 (as well as poly ram)
            // This will also cause the interpreter to stop interpreting
            while (OSAtomicIncrement32(&D24Sim_PCI1AbleAccessor->d24_in_use) > 1)
            {
                OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
                
                kill_time();
            }
            
            SynclavierPCILib_Suspend();
        
            // Check for failure
            if (!sharedStruct->ui_processing_needed) {
                OSAtomicDecrement32(&D24Sim_PCI1AbleAccessor->d24_in_use);
                return ((chip_error_code) NO_RESPONSE);
            }
            
            D24Sim_PCI1AbleAccessor->inuse_time = D24Sim_PCI1AbleAccessor->scsi_time;
            
            SynclavierPCILib_JamTLIMRegister(D24Sim_PCI1AbleAccessor->inuse_time);

            D24Sim_PCI1AbleAccessor->d24_released  = false;
            D24Sim_PCI1AbleAccessor->d24_requested = false;

            d24_selection = XPL_read(024) & s_selmask;                  // Get current board selection
        
            i = shl(bit[d24id], 8);
            XPL_write(024, s_selectenable | i);                         // Enable the D24
		}
			
		/* Bus is free; try to grab it: */
		
		XPL_write(025, s_bsy);											// start arbitration - set busy
		XPL_write(024, arb_bit);										// arbitrate with desired bit
		
		ufixed d24_data = 0;
		
		d24_data |= XPL_read(024);										// sample data bus
		d24_data |= XPL_read(024);										// sample data bus
		
		if ((d24_data & s_datamask) < shl(arb_bit, 1))					// if no higher arbiter seen
		{
			gotbus = true;												// then we got bus; keep bsy
			
			while ((gotbus == true)
			&&        (((XPL_read(024) & s_datamask) != arb_bit)		// wait for lower priority ID's to be removed
			||         ((XPL_read(025) & s_busmask ) != s_bsy  ))) {	// or detect someone else grabbing bus
				if (((XPL_read(025) & s_busmask ) != s_bsy          )	// if someone else grabs bus
				||  ((XPL_read(024) & s_datamask) >= shl(arb_bit, 1)))	// or higher host starts to arbitrate
				{
					XPL_write(025, 0);
					XPL_write(024, 0);
					gotbus = false;
				}
			}
		}
		else {															// else we lost arb; release
			XPL_write(025, 0);											// our busy and stand clear
			XPL_write(024, 0);
		}
	}
		
	/* We won arbitration (at least, we believe so!) */
	XPL_write(024, (bit [initiator] | bit [target]));					// Set Initiator and Target IDs on data bus
	XPL_write(025, (s_bsy | s_atn));									// Assert ATN
	
	/*** Selection Phase ***/
	
	XPL_write(025, (s_bsy | s_atn | s_sel));							// Assert SEL
	XPL_write(025, (        s_atn | s_sel));							// Release BSY
	
	/* Wait until Target asserts BSY or a Selection Timeout Delay (250 ms) goes by */
	int tick_time = 0;
    
	while ((XPL_read(025) & s_bsy) == 0) {
        usleep(1000);
        
		if (++tick_time > 250)
		{
			XPL_write(024, 0);											// Release data bus
			XPL_write(025, 0);											// Release signal bus
		
			Cleanup(d24_selection);

			return ((chip_error_code) NO_RESPONSE);
		}
	}
		
	XPL_write(024, 0);													// Release data bus
	XPL_write(025, (s_atn));											// Release SEL
	
	/*** Identify ***/
	
	while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}				// Wait for our hardware to release ACK, and target to assert REQ
		
	i = (XPL_read(025) & s_sigmask);									// store bus at this point
	XPL_write(025, 0);													// Release ATN
	
	XPL_write(026, (0x0080 | lun));										// Send Identify Message with no disconnect priviledges
	
	if (i != s_messout)
	{
		scsireset();													// reset if some other phase; we don't handle it yet
		Cleanup(d24_selection);
		return ((chip_error_code) NO_RESPONSE);
	}
		
	while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}				// Wait for our hardware to release ACK, and target to assert REQ
		
	if ((XPL_read(025) & s_sigmask) != s_command)						// if not command phase, wow...
	{
		scsireset();													// reset if some other phase; we don't handle it yet
	
		Cleanup(d24_selection);
		return ((chip_error_code) NO_RESPONSE);
	}
	
	// Write out the command
	for (i=0; i<the_command.fNumCommandBytes>>1; i++)
        XPL_write(027, the_command.fWCommand[i]);                       // Scsi operation Code

	while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}				// Wait for our hardware to release ACK, and target to assert REQ after command

	if ((XPL_read(025) & s_sigmask) == s_datain)						// got data in phase
	{
		while ( (XPL_read(025) & s_sigmask) == s_datain ) {				// Repeat until phase changes
		
			if ((NumDataBytes >= 512)									// more than a sector
			&&  ((NumDataBytes & 511) == 0))							// complete sectors
			{
                #if DO_TIMING
                    #undef printf
                    start_time = CACurrentMediaTime();
                    
                    SynclavierPCILib_ReadSector();                      // grab 512 bytes
                    
                    end_time   = CACurrentMediaTime();
                    
                    if (last_time != 0.0)
                        printf("Outside: %0.6f  Inside: %0.6f\n", start_time - last_time, end_time - start_time);
                    
                    last_time = end_time;
                #else
                    SynclavierPCILib_ReadSector();                      // grab 512 bytes
                #endif
                
				for (i=0; i<256; i++)									// read 512 bytes
					*DataPointer_w++ = D24Sim_PCI1AbleAccessor->sector_data[i];
					
				the_command.fNumBytesXferd += 512;    					/* accumulate bytes transferred */
				NumDataBytes               -= 512;    					/* compute bytes remaining      */
				DataPointer_b = (byte *) DataPointer_w;
                
                // Don't call yielder - slows things down by about a factor of 10
                #if 0&&SYNC_USE_ASYNC_IO
                    if (SCSILib_yielder)
                    {
                        SCSILib_yield_count++;
                        SCSILib_yielder();                              // Note: SCSILib_yielder could disappear during this call
                        SCSILib_yield_count--;
                    }
                #endif
			}
			
			else if (NumDataBytes != 0)									// else read byte-by-byte
			{
				*DataPointer_b++ = XPL_read(026);
                
                the_command.fNumBytesXferd += 1;    					// one byte transferred
				NumDataBytes               -= 1;    					/* compute bytes remaining      */
				DataPointer_w = (fixed *) DataPointer_b;
			}
			
			else
			{
				scsireset();											// reset if some other phase; we don't handle it yet
			
				Cleanup(d24_selection);
				return ((chip_error_code) NO_RESPONSE);
			}
			
			while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}		// Wait for our hardware to release ACK, and target to assert REQ
				
			if ((XPL_read(025) & s_sigmask) == s_messin) {				// Message In Phase appeared?
				i = XPL_read(026);										// Get message
				
				if (i == s_savepointer) {								// Save data pointer
					prevadr_w = DataPointer_w;
					prevadr_b = DataPointer_b;
					prevlen   = NumDataBytes;
				}
				else if (i == s_restorepointer) {						// Restore data pointer
					DataPointer_w = prevadr_w;
					DataPointer_b = prevadr_b;
					NumDataBytes  = prevlen;
				}

				while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}	// Wait for our hardware to release ACK, and target to assert REQ
			}															// of Message In phase
		}																// of Repeat until phase changes
	}
	
	else if ((XPL_read(025) & s_sigmask) == s_dataout)					// got data out phase
	{
		while ( (XPL_read(025) & s_sigmask) == s_dataout ) {			// Repeat until phase changes
		
			if ((NumDataBytes >= 512)									// more than a sector
			&&  ((NumDataBytes & 511) == 0))							// complete sectors
			{
				for (i=0; i<256; i++)									// write 512 bytes
					D24Sim_PCI1AbleAccessor->sector_data[i] = *DataPointer_w++;
				
				SynclavierPCILib_WriteSector();
				
				the_command.fNumBytesXferd += 512;    					/* accumulate bytes transferred */
				NumDataBytes               -= 512;    					/* compute bytes remaining      */
				DataPointer_b = (byte *) DataPointer_w;
               
                // Don't call yielder - slows things down by about a factor of 10
                #if 0&&SYNC_USE_ASYNC_IO
                    if (SCSILib_yielder)
                    {
                        SCSILib_yield_count++;
                        SCSILib_yielder();                              // Note: SCSILib_yielder could disappear during this call
                        SCSILib_yield_count--;
                    }
                #endif
			}
			
			else if (NumDataBytes != 0)
			{
				XPL_write(026, *DataPointer_b++);
					
				the_command.fNumBytesXferd += 1;    					// one byte transferred
				NumDataBytes               -= 1;    					/* compute bytes remaining      */
				DataPointer_w = (fixed *) DataPointer_b;
			}
			
			else														// else device is asking for too much data
			{
				scsireset();											// reset if some other phase; we don't handle it yet
			
				Cleanup(d24_selection);
				return ((chip_error_code) NO_RESPONSE);
			}

			while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}		// Wait for our hardware to release ACK, and target to assert REQ
				
			if ((XPL_read(025) & s_sigmask) == s_messin) {				// Message In Phase appeared?
				i = XPL_read(026);										// Get message
				
				if (i == s_savepointer) {								// Save data pointer
					prevadr_w = DataPointer_w;
					prevadr_b = DataPointer_b;
					prevlen   = NumDataBytes;
				}
				else if (i == s_restorepointer) {						// Restore data pointer
					DataPointer_w = prevadr_w;
					DataPointer_b = prevadr_b;
					NumDataBytes  = prevlen;
				}

				while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}	// Wait for our hardware to release ACK, and target to assert REQ
			}															// of Message In phase
		}
	}
	
	if ((XPL_read(025) & s_sigmask) != s_status)						// better be status now
	{
		scsireset();													// reset if some other phase; we don't handle it yet
	
		Cleanup(d24_selection);
		return ((chip_error_code) NO_RESPONSE);
	}
		
	the_command.fScsiStatus = XPL_read(026);							// get status byte
    
	while ( (XPL_read(025) & (s_req | s_ack)) != s_req ) {}             // Wait for our hardware to release ACK, and target to assert REQ

    scsi_message_byte = XPL_read(026);	
    
	// Handly busy tries on this level
	if (the_command.fScsiStatus == BUSY)
	{
		if (retry_counter++ < RETRY_LIMIT)
		{
			uint32 i = TickCount();
			
			while (TickCount() < i + 10)
				;
			
			goto retry;
		}
		
		Cleanup(d24_selection);
		return ((chip_error_code) CHIP_BUSY_STATUS);
	}

	Cleanup(d24_selection);
	return ((chip_error_code) GOOD_STATUS);				
}
