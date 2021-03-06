/********************************************************************************	Open Midi System: OMS Timing*	*	OMSTIMER.H*	constants, structures, and prototypes**	Copyright �1994-95 Steinberg Soft+Hardware GmbH, All Rights Reserved********************************************************************************/#ifndef __OMSTIMER#define __OMSTIMER/*--------------------------------------------------------------------------*//* constants */#include "OMSTypes.h"#if OMS_WINDOWS	typedef struct MasterTrackEntry _huge *MasterTrackPtr;#else	typedef struct MasterTrackEntry *MasterTrackPtr;#endiftypedef MasterTrackPtr *MasterTrackHandle;typedef char *MIDIFilePtr;typedef MIDIFilePtr *MIDIFileHandle;typedef short *SyncListPtr;#define MAXPOSITION 0x7ffffffeL#define NEVER 		0x80000000L#define BPMTOTEMPO(a)	(60000000L / a)/* callback messages */enum{	omscSetTime,	omscStart,	omscStop,	omscSetCycleStart,	omscSetCycleEnd,	omscSetCycleState,	omscCycleHasLooped,	omscMasterHasChanged,	omscOffsetHasChanged,	omscTempoHasChanged,	omscFramesHaveChanged,	omscSyncHasChanged,	omscArmHasChanged,	omscCountOff};/* sync modes */enum{	syncFree,	syncMTC,	syncClock,	syncMMC};/* frame rates */enum{   	fr24frame,	fr25frame,	fr2997dropframe,	fr2997nondropframe,	fr30dropframe,	fr30nondropframe};/* errors */enum{	noMoreSupported = 1,	notEnoughMemErr,	tooManyClientsErr,	tooManyTasksErr,	granularityUnavailableErr,	unknownRateErr,	outOfRangeErr,	sourceUnknownErr,	externalSyncErr,	alreadyRunningErr,	alreadyStoppedErr,	noClientErr,	noTaskErr,	masterNeverOpened,	masterAlreadyOpen,	noMoreEntries,	noPermissionErr,	noCallBackProcErr,	noWakeupProcErr,	alreadyArmedErr,	alreadyDisarmedErr};/* granularities */#define OMS_BASE_GRANULARITY_PER_BAR 7680L#define OMS_BASE_GRANULARITY_PER_QUARTER 1920Lenum{	GRANULARITY12 = 12,	GRANULARITY24 = 24,	GRANULARITY48 = 48,	GRANULARITY96 = 96,	GRANULARITY120 = 120,	GRANULARITY160 = 160,	GRANULARITY192 = 192,	GRANULARITY240 = 240,	GRANULARITY320 = 320,	GRANULARITY384 = 384,	GRANULARITY480 = 480,	GRANULARITY640 = 640,	GRANULARITY960 = 960,	GRANULARITY1920 = 1920};enum{	GRANULARITY_DIVIDER12 = 160,	GRANULARITY_DIVIDER24 = 80,	GRANULARITY_DIVIDER48 = 40,	GRANULARITY_DIVIDER96 = 20,	GRANULARITY_DIVIDER120 = 16,	GRANULARITY_DIVIDER160 = 12,	GRANULARITY_DIVIDER192 = 10,	GRANULARITY_DIVIDER240 = 8,	GRANULARITY_DIVIDER320 = 6,	GRANULARITY_DIVIDER384 = 5,	GRANULARITY_DIVIDER480 = 4,	GRANULARITY_DIVIDER640 = 3,	GRANULARITY_DIVIDER960 = 2,	GRANULARITY_DIVIDER1920 = 1};/*--------------------------------------------------------------------------*//* structures */#if OMS_MAC_PRAGMA_ALIGN#pragma pack(2)#endif/* client */typedef struct OMSTimerClient{	long refCon;	char timerPrivate[1];	/* rest is hidden from client */} OMSTimerClient;/* callback */typedef struct OMSTimerMessage{	OMSTimerClient *client;	short message;	short shortArg;			/* state, mode */	long longArg;			/* sometimes pointer */	long SMPTETime;	long BeatTime;			/* scaled to callback granularity */	long eternalSMPTETime;	long eternalBeatTime;	/* scaled to callback granularity */} OMSTimerMessage;TYPEDEF_OMSPROC (void, OMSTimerMessageProc) (OMSTimerMessage *message);typedef struct OMSTimerClockInfo OMSTimerClockInfo;TYPEDEF_OMSPROC (long, OMSTimerClockProviderProc) (OMSTimerClockInfo *info);/* timer task */typedef struct OMSTimerTask OMSTimerTask;TYPEDEF_OMSPROC (OMSTimerTask *, OMSWakeUpTask) (OMSTimerTask *task);// NOTE: on 68K machines, the return value is expected in register D0.// THINK compilers return pointers in D0, but Metrowerks 68k compilers// return pointers in A0 by default. There is a #pragma, however:// #if __MWERKS__// #pragma pointers_in_D0// #endif// ...and to reset:// #if __MWERKS__// #pragma pointers_in_A0// #endif#include "OMSTUPPS.H"struct OMSTimerTask{	char timerPrivate[64];	/* don't touch! */	OMSWakeUpTaskUPP WakeUpTask;	OMSTimerClient *client;	long granularity;	long nextWakeUp;	short priority;	short flags;	long systemTime;		/* in OMS native resolution! */	long systemEternalTime;	/* in OMS native resolution! */	long systemCountOffTime;	/* in OMS native resolution! */	char spare[8];	short taskPrivate[1];	/* rest hidden from OMS Timing */};/* OMSTimerTask and time mode flags */#define currentBased 0#define eternalBased 1#define countOffBased 2#define unSyncedSMPTEBased 4#define nextCycle 0x8000/* priority conventions */#define highPriority 25000#define midPriority 0#define lowPriority -25000/*--------------------------------------------------------------------------------------------------------------*//* clock Provider */struct OMSTimerClockInfo{	long SMPTEcurrent;	long SMPTEunsynced;	long SMPTEatTrigger; 	long ClockPosition;	long ClockPositionAtTrigger;	long tempo;	short frameRate; 	short flags;	char reserved[16];};// flags#define CIF_TRIGGERED			1#define CIF_EXTERNALLY_SYNCED 	2#define CIF_EXTERNAL_SYNCED_ENABLED	4#define CIF_CYCLE_ON 			8#define CIF_MASTER_ON 		16/*--------------------------------------------------------------------------------------------------------------*//* Master Track */enum{	omsmTempo = 1,	omsmTimeSignature,	omsmNext,	omsmExtended,	omsmEnd};typedef union MTValue{	struct	{		unsigned long tempo;	} Tempo;	struct	{		short numerator;		short denominator;	} TimeSignature;	struct	{		struct MasterTrackEntry *next;	} NextMaster;	struct	{		short extended_type;		unsigned short bytesize;	/* of following data */	} Extended;	struct	{		unsigned long setPosTo7fffffff;	} End;} MTValue;typedef struct MasterTrackEntry{	short type;					/* omsmTempo, omsmTimeSignature etc */#if OMS_WINDOWS	short filler;#endif	unsigned long position;		/* in native granularity */	unsigned long time;			/* SMPTE subframes, calculated by SetMaster */	MTValue value;				/* union */} MasterTrackEntry;#if OMS_MAC_PRAGMA_ALIGN#pragma options align=reset#endif/*--------------------------------------------------------------------------*//* prototypes */#ifdef __cplusplusextern "C" {#endifOMSAPI(long) OMSTimerVersion(void);OMSAPI(OMSErr) OMSTimerRegisterClient(OMSTimerClient **client, long refcon);OMSAPI(OMSErr) OMSTimerReleaseClient(OMSTimerClient *client);OMSAPI(OMSErr) OMSTimerSetMessageProc(OMSTimerClient *client, long granularity,	OMSTimerMessageProcUPP messageProc);OMSAPI(OMSErr) OMSTimerTrigger(void);OMSAPI(OMSErr) OMSTimerHalt(void);OMSAPI(OMSErr) OMSTimerArm(void);OMSAPI(OMSErr) OMSTimerDisarm(void);OMSAPI(OMSErr) OMSTimerSetTime(long time, long granularity);OMSAPI(long) OMSTimerGetTime(long granularity, short flags);OMSAPI(long) OMSTimerGetOMSClockPosition(void);		/* native granularity */OMSAPI(long) OMSTimerGetOMSSMPTETime(void);			/* subframes */OMSAPI(void) OMSTimerGetTimeSignature(short *numerator, short *denominator);OMSAPI(OMSErr) OMSTimerWakeUpTask(OMSTimerTask *task);OMSAPI(void) OMSTimerRemoveTask(OMSTimerTask *task);#if OMS_WINDOWSOMSAPI(void) OMSTimerCheckForTasks(void);#endifOMSAPI(OMSErr) OMSTimerSetFrameRate(short frameRate);OMSAPI(short) OMSTimerGetFrameRate(void);OMSAPI(OMSErr) OMSTimerSetSync(short mode, short source);OMSAPI(void) OMSTimerGetSync(short *mode, short *source);OMSAPI(OMSErr) OMSTimerClaimMasterTrack(OMSTimerClient *client);OMSAPI(OMSErr) OMSTimerSetMasterTrack(OMSTimerClient *client, MasterTrackEntry *master,	long bytesize);OMSAPI(OMSErr) OMSTimerGetMasterTrack(MasterTrackHandle *master);OMSAPI(OMSErr) OMSTimerOverrideMasterTrack(long tempo);OMSAPI(long) OMSTimerGetCurrentTempo(void);OMSAPI(OMSErr) OMSTimerSetOffset(long offset);OMSAPI(long) OMSTimerGetOffset(void);OMSAPI(OMSErr) OMSTimerClock2SMPTE(long clock, long granularity, long *smpte, short RealTime);OMSAPI(OMSErr) OMSTimerSMPTE2Clock(long smpte, long granularity, long *clock, short RealTime);OMSAPI(OMSErr) OMSTimerMidiFile2Master(MIDIFilePtr midifile, MasterTrackHandle master);OMSAPI(OMSErr) OMSTimerMaster2MidiFile(MasterTrackPtr master, MIDIFileHandle midifile, long granularity);OMSAPI(OMSErr) OMSTimerSetCycleStart(long cycleStart, long granularity);OMSAPI(OMSErr) OMSTimerSetCycleEnd(long cycleEnd, long granularity);OMSAPI(OMSErr) OMSTimerSetCycleState(short cycleState);OMSAPI(OMSErr) OMSTimerGetCycleStart(long *cycleStart, long granularity);OMSAPI(OMSErr) OMSTimerGetCycleEnd(long *cycleEnd, long granularity);OMSAPI(short) OMSTimerGetCycleState(void);OMSAPI(void) OMSTimerSetSyncRecipients(SyncListPtr syncList);OMSAPI(SyncListPtr) OMSTimerGetSyncRecipients(void);// NOTE: the pointer is returned in both registers D0 and A0 on 68k machines.OMSAPI(void) OMSTimerLockTimer(void);OMSAPI(void) OMSTimerUnLockTimer(void);OMSAPI(OMSErr) OMSTimerCountOff(long duration, long granularity);OMSAPI(OMSErr) OMSTimerSetClockProvider(OMSClockProviderUPP clockProvider);OMSAPI(OMSErr) NO_MORE_SUPPORTED1 (void);OMSAPI(OMSErr) NO_MORE_SUPPORTED2 (void);#ifdef __cplusplus}#endif#endif /* __OMSTIMER */