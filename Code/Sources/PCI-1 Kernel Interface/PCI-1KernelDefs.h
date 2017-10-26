// PCI1KernelDefs.h

#pragma once

// This file defines constants and routines for accessing the various PCI and PCIe cards.

#define	PCI1_ACCESSOR_STRUCT_VERSION	3

// Struct is shared between the PCI-1 kernel driver and user land applications vio IOConnectMapMemory.
// This struct stores information that is of interest to both the kernel driver and the user space application(s).

// Constants

// These are the default timings to be used with the BTB-1.
// The unit of measure is 160 nsec. That was the per-write cycle time achieved
// on the first TB-1/PCIe-DIO-48/BTB-1 rig.

#define     SCSI_D24_READ_TIME      6
#define     SCSI_D24_WRITE_TIME     5
#define     SCSI_D25_READ_TIME      6
#define     SCSI_D25_WRITE_TIME     5
#define     SCSI_D27_WRITE_TIME     10
#define     SCSI_END_TIMING         2

#define     POLY_READ_TIME          6
#define     POLY_WRITE_TIME         6
#define     POLY_END_TIMING         2

#define     GENERIC_READ_TIME       10
#define     GENERIC_SETUP_TIMING    5
#define     GENERIC_WRITE_TIME      10
#define     GENERIC_END_TIMING      2

#define     PCI1_TIMING_SCALE       160

// PCI Return Codes

#define     SUCCESSFUL               0x00
#define     NOT_SUCCESSFUL           0x01
#define     NO_HARDWARE              0x81
#define     UNPROGRAMMED_DIO_CARD    0x82
#define     BAD_VENDOR_ID            0x83
#define     NO_BTB1_HARDWARE         0x84
#define     BAD_BOARD_REV            0x85
#define     DEVICE_NOT_FOUND         0x86
#define     BAD_REGISTER_NUMBER      0x87
#define     DEVICE_IN_USE      		 0x88

#pragma pack(push,2)

typedef	struct PCI1AccessorStruct
{
	unsigned int	struct_version;					// Holds PCI1_ACCESSOR_STRUCT_VERSION
	unsigned int    inspected;						// True if PCI-1 board and associated synclav have been inspected
	unsigned int    snarfed;						// True if connected to synclav at this instant and we have grabbed it
	volatile int 	d24_requested;                  // Set by InterChange when it desired the D24.
	volatile int 	d24_in_use;                     // Semaphore to gain access to D24 and POLY hardware from outside the interpreter
	volatile int 	d24_released;                   // Set by Interpreter when it is done with the D24.
	
			 char   slotname[64];					// Slot name, c string, for diagnostics

	unsigned int 	inuse_time;						// TLim register in use by
	unsigned int 	fast_time;						// TLim register for fast devices
	unsigned int 	scsi_time;						// TLim register for scsi devices
	unsigned int 	slow_time;						// TLim register for slow devices
	
	unsigned short	d51_hardware_device_bits;		// Results of D51 hardware read during startup
	unsigned short  d57_hardware_device_bits;		// Results of D57 hardware read during startup
	
	unsigned int 	devreads [128];					// PCI-1 Command Opcode for device reads ; 0 means device is not available
	unsigned int 	devwrites[128];					// PCI-1 Command Opcode for device writes; 0 means device is not available
	
	char			errorString[512];				// Error string returned by startup & test code
	int				errorStatus;					// Startup status
	
	int				XPL_Read_Data;					// XPL Read data returned form XPLRead()
    int             XPL_Failed_Device;              // Nonzero indicates device failed
	
	short			sector_data[256];				// Scsi read/write data

    // Timing values to use with clock_delay_until (e.g. absolute time units, which in today's world are nsecs)
    int             time_scale_num;                 // Scale factor to absolute time units from user setting
    int             time_scale_den;                 // Scale factor to absolute time units from user setting
    
    int             single_write_time_abt;          // Uptime ticks we need    in one DIO card write cycle
    int             actual_write_time_abt;          // Uptime ticks we achieve in one DIO card write cycle
    
    int             scsi_d24_read_time_abt;
    int             scsi_d24_write_time_abt;
    int             scsi_d25_read_time_abt;
    int             scsi_d25_write_time_abt;
    int             scsi_d27_write_time_abt;
    int             scsi_end_timing_abt;
    
    int             poly_read_time_abt;
    int             poly_write_time_abt;
    int             poly_end_timing_abt;
    
    int             generic_read_time_abt;
    int             generic_setup_time_abt;
    int             generic_write_time_abt;
    int             generic_end_timing_abt;

} PCI1AccessorStruct;

#pragma pack(pop)

// Actual hardware access to the board is limited to the kernel. This struct and associated routines
// provide a c-style interface to the actual PCI-1 hardware.

// The BTB-1 constants are, however, made available to the test program
// BTB-1 Control Bits
#define     BTB1_WCR            0x01
#define     BTB1_WADR           0x02
#define     BTB1_CREAD          0x04
#define     BTB1_CWRIT          0x08

#define     BTB1_SYNC           0x01
#define     BTB1_REV            0xFE

#define     BTB1_DISABLE_CR_OUT 0x10    // Active low enables control register drivers (CWCR, CWADR, CREAD, CWRIT)
#define     BTB1_ENABLE_CR_OUT  0x00

#define     BTB1_DISABLE_SYN_IN 0x20    // Active low enables sync/rev input driver
#define     BTB1_ENABLE_SYN_IN  0x00

#define     BTB1_DISABLE_CBOUT  0x40    // Active low enables cable data output drivers (DIO card outputs to twisted cable)
#define     BTB1_ENABLE_CBOUT   0x00

#define     BTB1_DISABLE_CBIN   0x80    // Active low enables cable input drivers (cable inputs to DIO card)
#define     BTB1_ENABLE_CBIN    0x00

#define     BTB1_DIO_OUTPUT_ENA 0x80    // 8255 control bits for all outputs (Adlink - MSB not needed)
#define     BTB1_DIO_INPUT_ENA  0x9B    // 8255 control bits for all inputs  (Adlink - MSB not needed)