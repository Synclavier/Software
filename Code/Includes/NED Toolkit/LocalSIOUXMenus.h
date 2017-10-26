/*  Metrowerks Standard Library  Version 2.1.2  1997 May  *//*****************************************************************************//*  Project...: C++ and ANSI-C Compiler Environment                          *//*  Name......: SIOUXMenus.h                                       		     *//*  Purpose...: Menu related functions for SIOUX			                 *//*  Copyright.: Copyright � 1993-1997 Metrowerks, Inc. All rights reserved.  *//*****************************************************************************/#ifndef __SIOUXMENUS__#define __SIOUXMENUS__#if __MWERKS__#pragma pack(2)#endif/* Menu IDs ...*/enum {	APPLEID 		= 32000,	APPLEABOUT 		= 1};enum {	FILEID			= 32001,	FILESAVE 		= 4,	FILEPAGESETUP	= 6,	FILEPRINT		= 7,	FILEQUIT		= 9,	FILEADDITIONS	= 10};enum {	EDITID			= 32002,	EDITCUT			= 3,	EDITCOPY		= 4,	EDITPASTE		= 5,	EDITCLEAR		= 6,	EDITSELECTALL	= 8,	EDITADDITIONS   = 9};#ifdef __cplusplusextern "C" {#endif/*Function prototypes ...*/extern void		SIOUXSetupMenus(void);extern void		SIOUXUpdateMenuItems(void);extern short	SIOUXDoSaveText(void);extern void		SIOUXDoEditCut(void);extern void		SIOUXDoEditCopy(void);extern void		SIOUXDoEditPaste(void);extern void		SIOUXDoEditClear(void);extern void		SIOUXDoEditSelectAll(void);extern void		SIOUXDoMenuChoice(long menuValue);extern	unsigned char *SIOUXFileMenuAdditions;		// set up pascal strings for additional menu itemsextern	unsigned char *SIOUXEditMenuAdditions;extern	void	(*SIOUXFileMenuAddtionsHandler) (int item);	// handler for additional menu items; passed **relative** itemextern	void	(*SIOUXEditMenuAddtionsHandler) (int item); // e.g. 0 = first additional item.#ifdef __cplusplus}#endif#if __MWERKS__#pragma options align=reset#endif#endif/*     Change record960930  mm Changed C++ comments to C comments for ANSI strict*/