// MachUtilities.h

#ifndef	__MACHUTILITIES__
#define __MACHUTILITIES__

#include <IOKit/IOTypes.h>

extern	void	MU_Initialize_MachUtilities	(io_connect_t connection);
extern	void	MU_SetWakePort				(io_connect_t connection, int methodIndex);
extern	void	MU_Finalize_MachUtilities	();
extern	void	MU_AddAsyncCallback			(struct __CFRunLoop * runLoop, io_connect_t ref, int style, int code, void* proc, void* arg, int msb_code = 0);
extern	void	MU_RemoveAsyncCallback		(io_connect_t ref, int index, int code);

#endif
