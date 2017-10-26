// =================================================================================
//	AbleInterpreterShared.cpp
// =================================================================================

// Specific functions shared between Interchange, Synclavier PowerPC and the PCI-1 Kernel Driver

#ifdef	COMPILE_OSX_KERNEL
	#include <IOKit/IOLib.h>
#else
	#include <string.h>
#endif

#include "AbleInterpreterShared.h"
#include "GlobLits.h"

// =================================================================================
//		¥ init_shared_struct
// =================================================================================

// Called to initialize the core interpreter once the shared struct has been set up. We are passed in memory addresses and hardware addresses.

#define LOG_MIDI_TRAFFIC	0

void	init_shared_struct(SynclavierSharedStruct& sharedStruct)
{
	int	  i;
	
	memset(&sharedStruct, 0, sizeof(SynclavierSharedStruct));

	sharedStruct.struct_version      = SYNCLAVIER_POWERPC_STRUCT_VERSION;
	sharedStruct.to_interchange_size = SYNCLAVIER_POWERPC_MESSAGE_Q_SIZE;
	sharedStruct.to_synclavier_size  = SYNCLAVIER_POWERPC_MESSAGE_Q_SIZE;
	
	sharedStruct.message_buf_size    = SYNCLAVIER_POWERPC_MESSAGE_BUF_SIZE;
	sharedStruct.message_write_ptr   = 0;

    // Init MIDI variables
	for (i=0; i != SYNCLAVIER_MIDI_PORTS; i++)
	{
		SynclavierMidiChannel& it = sharedStruct.midi_channels[i];
		
		it.oms_unused = 0;
		it.is_active  = LOG_MIDI_TRAFFIC;
		
		if (i < num_kbd_tracks)
		{
			it.oms_cable   = SYNCLAVIER_KEYBOARD_CABLE;
			it.std_channel = 0;
		}
		else
		{
	    	it.oms_cable   = SYNCLAVIER_TRACKS_CABLE + ((i - num_kbd_tracks) / 16);
			it.std_channel = (i - num_kbd_tracks) & 0xF;
		}
	}
	
	for (i=0; i != SYNCLAVIER_VIRTUAL_PORTS; i++)
	{
		SynclavierMidiChannel& it = sharedStruct.midi_vports[i];
		
		it.oms_unused = 0;
		it.is_active  = LOG_MIDI_TRAFFIC;
		
		it.oms_cable   = SYNCLAVIER_VPORTS_CABLE + i;
		it.std_channel = 0;
	}
	
	sharedStruct.mtc_channel.oms_unused      = 0;
	sharedStruct.mtc_channel.is_active       = LOG_MIDI_TRAFFIC;
	sharedStruct.mtc_channel.oms_cable       = SYNCLAVIER_MTC_CABLE;
	sharedStruct.mtc_channel.std_channel     = 0;
	
	sharedStruct.mclk_channel.oms_unused     = 0;
	sharedStruct.mclk_channel.is_active      = LOG_MIDI_TRAFFIC;
	sharedStruct.mclk_channel.oms_cable      = SYNCLAVIER_MIDICLK_CABLE;
	sharedStruct.mclk_channel.std_channel    = 0;

	sharedStruct.oms_midi_sync_size   	     = SYNCLAVIER_POWERPC_OMS_SYNC_SIZE;
	sharedStruct.oms_midi_sync_write_ptr     = 0;
	sharedStruct.oms_midi_sync_any_bytes     = NULL;
    
    // Init user changeable timing settings
    sharedStruct.scsi_d24_read_time_setting  = SCSI_D24_READ_TIME;
    sharedStruct.scsi_d24_write_time_setting = SCSI_D24_WRITE_TIME;
    sharedStruct.scsi_d25_read_time_setting  = SCSI_D25_READ_TIME;
    sharedStruct.scsi_d25_write_time_setting = SCSI_D25_WRITE_TIME;
    sharedStruct.scsi_d27_write_time_setting = SCSI_D27_WRITE_TIME;
    sharedStruct.scsi_end_timing_setting     = SCSI_END_TIMING;
    
    sharedStruct.poly_read_time_setting      = POLY_READ_TIME;
    sharedStruct.poly_write_time_setting     = POLY_WRITE_TIME;
    sharedStruct.poly_end_timing_setting     = POLY_END_TIMING;
    
    sharedStruct.generic_read_time_setting   = GENERIC_READ_TIME;
    sharedStruct.generic_setup_time_setting  = GENERIC_SETUP_TIMING;
    sharedStruct.generic_write_time_setting  = GENERIC_WRITE_TIME;
    sharedStruct.generic_end_timing_setting  = GENERIC_END_TIMING;
}
