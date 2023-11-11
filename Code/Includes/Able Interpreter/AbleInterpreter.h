// AbleInterpreter.h

// Public Interface for Able Interpreter Modules

#ifndef ABLE_INTERPRETER_H
#define ABLE_INTERPRETER_H

// able hardware device bit definitions:

#define D1_ZFLAG			0x0001			/* z flag shows up here	(rd only)							*/
#define D1_CARRY			0x0002			/* c flag shows up here	(rd only)							*/
#define D1_MINUS			0x0004			/* n flag shows up here	(rd only)							*/

#define	D1_ION				0x0008			/* pc int enable in D1	(rd/wr)								*/
#define	D1_PC17				0x0010			/* pc select flag in D1	(wr only)							*/
#define D1_D50_IRQ			0x0040			/* d50  interrupt request bit								*/
#define	D1_D16_IRQ			0x0080			/* d16  interrupt request in D1								*/
#define	D1_D03_IRQ			0x0100			/* d3   interrupt request in D1								*/
#define D1_D54_IRQ			0x0400			/* d54  interrupt request in D1								*/
#define D1_D115_IRQ			0x2000			/* d115 interrupt request in D1								*/

#define	D51_XMIT_IRQ		0x0001			/* set in D51 if XMIT MT caused interrupt					*/
#define	D51_CHAR_RDY		0x0002			/* set if character is present in D50						*/
#define D51_XMIT_MT			0x0004			/* set if room in xmit output buffer						*/

#define	D51_D70_EXISTS		0x0100			/* set if D70   SMPTE is present							*/
#define	D51_D60_EXISTS		0x0200			/* set if D60   External Memory is present					*/
#define	D51_POLY_EXISTS		0x0400			/* set if       Poly System is present						*/
#define	D51_D32_EXISTS		0x0800			/* set if D32   Expansion Sysem is present					*/
#define	D51_D100A_EXISTS	0x1000			/* set if D100A Floppy is present							*/
#define	D51_D110A_EXISTS	0x2000			/* set if D110A Floppy is present							*/
#define	D51_D24_EXISTS		0x4000			/* set if D24	SCSI is present								*/
#define	D51_D57_EXISTS		0x8000			/* set if D57	info is present								*/

#define	D41_XMIT_IRQ		0x0001			/* set in D41 if XMIT MT caused interrupt					*/
#define	D41_CHAR_RDY		0x0002			/* set if character is present in D40						*/
#define D41_XMIT_MT			0x0004			/* set if room in xmit output buffer						*/

#define D54_D40_IRQ			0x0001			/* set in D54 if D40 is requesting an interrupt				*/
#define D54_D24_IRQ         0x0080          /* set in D54 if D24 is requesting an interrupt             */

#define D57_MODEL_D_EXISTS	0x0001			/* set by Model D processor									*/
#define D57_D40Q_EXISTS		0x0002			/* set if D40Q  Serial Port Interface is present			*/
#define D57_M64K_EXISTS		0x0004			/* set if M64k	is present									*/
#define D57_D34_EXISTS		0x0008			/* set if D34	GPI is present								*/
#define	D57_D115_EXISTS		0x0010			/* set if D115	SyncNet is present							*/
#define	D57_DIG_STM_EXISTS	0x4000			/* set if digital STM is present							*/
#define	D57_INTERPRETER		0x8000			/* set when running via interpreter							*/

typedef enum initialize_able_interpreter_statuses
{
	INITIALIZE_ABLE_INTERPRETER_NO_ERROR   = 0,
	INITIALIZE_ABLE_INTERPRETER_NO_MEMORY  = 1,
	INITIALIZE_ABLE_INTERPRETER_FILE_ERROR = 2,
	INITIALIZE_ABLE_INTERPRETER_QUIT       = 3,
	INITIALIZE_ABLE_INTERPRETER_RESTART    = 4,
	INITIALIZE_ABLE_INTERPRETER_NO_W0      = 5

}	initialize_able_interpreter_statuses;
	
// Public interface to interpreter routines (main loop level only)
extern	int		initialize_able_interpreter(class CSynclavierFileReference* mac_file_ref);
extern	int		execute_able_interpreter();
extern	void	cleanup_able_interpreter(bool hardware_failed);
extern	void	interpreter_settings_handler();
extern	bool    interpreter_has_shared();
extern	void    update_interpreter_real_time_prefs();
extern	bool	able_interpreter_calibrate_metronome(struct uint32_ratio& new_ratio);

extern  struct SynclavierSharedStruct* interpreters_shared_struct();

// And variables:
extern	int		able_interpreter_has_terminated;
extern	int		able_interpreter_should_d16calib;

extern  void	AbleInterpreterSignalNewPatching ();
extern  bool	AbleInterpreterMIDIAvailable     ();
extern  bool	AbleInterpreterCalibrateAvailable();

typedef	void    (*AbleInterpreterCallBackProc)(void* context);

extern  AbleInterpreterCallBackProc     able_interpreter_wakeup_callback_proc;
extern  void*                           able_interpreter_wakeup_callback_arg;

#endif
