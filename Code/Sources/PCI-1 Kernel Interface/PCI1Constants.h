// Constant file for Synclavier Digital PCI Devices

#ifndef	PCI1_CONSTANTS_H
#define PCI1_CONSTANTS_H

#define	PCI1_PREFS_FILE_NAME			"SynclavierX Settings"
#define	PCI1_PREFS_SIGNATURE			'SNCL'
#define	PCI1_PREFS_CREATOR				'SNCL'
#define	PCI1_PREFS_TYPE					'pref'

#define	PCI1_PREFS_VERSION				(1)

#if !defined(COMPILE_OSX_DRIVERKIT)
    #define SyncBasePCIDevice		    com_Synclavier_BasePCIDevice
    #define SyncVirtualPCIDevice	    com_Synclavier_VirtualPCIDevice
    #define SyncPCI1PCIDevice		    com_Synclavier_PCI1PCIDevice
    #define SyncPCIePCIDevice		    com_Synclavier_PCIePCIDevice
    #define PCI1UserClient              com_Synclavier_PCIUserClient
 #endif

#define SyncBasePCIClassName            "com_Synclavier_BasePCIDevice"
#define SyncVirtualPCIClassName         "com_Synclavier_VirtualPCIDevice"
#define SyncPCI1PCIClassName            "com_Synclavier_PCI1PCIDevice"
#define SyncPCIePCIClassName            "com_Synclavier_PCIePCIDevice"
#define PCI1UserClientClassName         "com_Synclavier_PCIUserClient"

#define PCI1MIDIImagePath				"/Library/Audio/MIDI Devices/Generic/Images/SynclavierDigitalAudioMIDIIcon.tiff"

#define SynclavierXMIDIDriverFactory	NewSynclavierDigitalMIDIDriver

#define kSynclavierXIdNumber			CFSTR("SynclavierDigitalUnitNumber")
#define kSynclavierXCableNumber			CFSTR("SynclavierDigitalCableNumber")

// UUID for SynclavierX MIDI Driver
// 7D65E8A3-AAFD-11DB-A474-0017F2C8B9E9
#define kMIDIFactoryUUID 				CFUUIDGetConstantUUIDWithBytes(NULL, 0x7D, 0x65, 0xE8, 0xA3, 0xAA, 0xFD, 0x11, 0xDB, 0xA4, 0x74, 0x00, 0x17, 0xF2, 0xC8, 0xB9, 0xE9)

#endif
