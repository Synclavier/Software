// =================================================================================
//	AbleInterpreterShared.h
// =================================================================================

// Certain routines shared between Interchange, Synclavier PowerPC and PCI-1 Kernel Driver

#pragma once

#include "XPL.h"

#include "PCI-1KernelDefs.h"
#include "PCI-1SharedMemory.h"
#include "SynclavierPCILib.h"
#include "AbleInterpreterStructs.h"


// Public functions
extern void			init_shared_struct					(SynclavierSharedStruct& sharedStruct);
