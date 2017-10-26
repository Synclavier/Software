/* Mac HFS Disk Library */

// Header for HFS Disk Library routines

#pragma once

// PowerPlant addons
#include "LCStr255.h"

int		MacHFSLib_ReadAbleHFSDirectory (struct InterChangeItemDirectory &itsData, char *the_tree_name);
void	MacHFSLib_IsolateTopDirectory  (union  InterChangeItemUnion &rootUnion, class LCStr255 &rootName, union InterChangeItemUnion &topUnion, class LCStr255 &topName);
