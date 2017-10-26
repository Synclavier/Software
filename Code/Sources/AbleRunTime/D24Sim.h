/*  D24Sim.h                                                                */

#ifndef d24sim_h
#define d24sim_h

#include    "SCSILib.h"

extern	void	D24Sim_Initialize(struct PCI1AccessorStruct* itsStruct, void (*threadYielder)(), bool isInterpreter);
extern	void	D24Sim_CleanUp                 ();
extern	void	D24Sim_Reset                   ();
extern	bool	D24Sim_Busy                    ();
extern	int		D24Sim_AnyNonMainThreadsWaiting();

// Entry for d24 scsi port access
chip_error_code issue_d24_scsi_port_scsi_command  	(scsi_device& the_device, scsi_command& the_command);
chip_error_code	initialize_d24_scsi_port_scsi_chip	();

#endif
