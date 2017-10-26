// Master record of Release Version Names

#ifndef	VERSION_H
#define VERSION_H

#if __LP64__
    #define	ABLE_INTERPRETER_VERSION_NAME   	"Synclavier³ 5.3.0"
    #define TERMULATOR_VERSION_NAME         	"Termulator³"
    #define	RELEASE_VERSION_NAME                "Release 5.3"

    #define PCI1_CONTROL_PANEL_NAME				"Synclavier³"
    #define	SYNCLAVIERX_OMS_MANUF_NAME			"Synclavier Digital"
    #define	SYNCLAVIERX_OMS_MODEL_NAME			"Synclavier³"

#else
    #define	ABLE_INTERPRETER_VERSION_NAME   	"SynclavierX 5.3.0"
    #define TERMULATOR_VERSION_NAME         	"TermulatorX"
    #define	RELEASE_VERSION_NAME                "Release 5.3"

    #define PCI1_CONTROL_PANEL_NAME				"SynclavierX"
    #define	SYNCLAVIERX_OMS_MANUF_NAME			"Synclavier Digital"
    #define	SYNCLAVIERX_OMS_MODEL_NAME			"Synclavier³"

#endif

#define	RTP_RELEASE_VERSION_NAME				"5.3"
#define	RTP_RELEASE_MAIN_VERSION				5
#define	RTP_RELEASE_SUB_VERSION					3
#define	RTP_RELEASE_FIX_VERSION					0

#define	VERSION_COPYRIGHT_NOTICE				"(c) 2017 Cameron W. Jones"
#define	VERSION_COPYRIGHT_LINE_1				"(c) 2017 Cameron W. Jones"
#define	VERSION_COPYRIGHT_LINE_2				"All rights reserved."

#define INTERCHANGE_VERSION_NAME                "InterChangeX"
#define INTERCHANGE_2_VERSION_NAME				"InterChangeX"
#define INTERCHANGE_2_REGISTRATION_CODE		registration_interchange_2_0

#endif
