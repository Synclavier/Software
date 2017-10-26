/*	Header file for: Opupdate.c	*//*	Created: 12/22/99 at 23:22	*//*	Version: 0.000				*/extern	void	when_d03int();	extern	fixed	examount;												// Sectors of external memory availextern	fixed	alloc_examount(fixed num);								// Allocate & zero out external memoryextern	array	copy_buf;												// Buf used by copy routines - can be used by others with careextern	void	copy_in(fixed des, fixed len);							// Copy block into main memory - set up mam & mal before callingextern	void	copy_out(fixed sou, fixed len);							// Copy block to ex mem - set up mam & mal before callingextern	void	copy_ext_mem(fixed soum, fixed soul, fixed desm, fixed desl, fixed len);	// High speed copies - copies up or down, any length, in external memoryextern	void	copy_ext_mem_sec(fixed sou, fixed des, fixed secl);		// Sector boundaries & lenghextern	void	abort_scsi();	extern	void	display_update_status(fixed mode, fixed count, fixed total);	// display status message on screen from laser disk updateextern	array	mnullptr;												// null pointerextern	fixed	r_stkbase;												// base of stack in external memoryextern	fixed	r_stkmax;												// max no. items on stackextern	void	ptrtowords(fixed ptr[], fixed buf[]);					// turn pointer address into a word addressextern	void	wordstoptr(fixed buf[], fixed ptr[]);					// turn word address into a pointer addressextern	void	readmblock(fixed ptr[], fixed offset);					// setup to read block of memoryextern	void	writemblock(fixed ptr[], fixed offset);					// setup to write block of memoryextern	void	readmptrblock(fixed ptr[], fixed ptroffset, fixed offset);	// setup to read memory block given by pointerextern	void	writemptrblock(fixed ptr[], fixed ptroffset, fixed offset);	// setup to write memory block given by pointerextern	void	copymblock(fixed sourceptr[], fixed sourceoffset, fixed destptr[], fixed destoffset, fixed len);	// copy block of memoryextern	void	getmfree(fixed buf[]);									// return no. words free in buffer areaextern	fixed	getmblocksize(fixed ptr[]);								// return no. words used by memory blockextern	boolean	newmblock(fixed ptr[], fixed len, boolean relocate);	// allocate new memory blockextern	boolean	disposemblock(fixed ptr[]);								// remove memory blockextern	fixed	compareblock(fixed ptr[], fixed keyoffset, fixed target[]);	// compare memory block string to given stringextern	boolean	treeinit(fixed h[]);									// intialize treeextern	boolean	treesearch(fixed s[], fixed h[], fixed x[]);			// search tree for stringextern	fixed	treeinsert(fixed s[], fixed h[], fixed result[]);		// insert string into treeextern	boolean	treetraverse(fixed headptr[], boolean initialize, fixed nodeptr[]);	// traverse treeextern	void	definemarea(fixed bin, fixed ms_start, fixed ls_start, fixed ms_len, fixed ls_len);	// define memory areaextern	void	definestack(fixed base, fixed max);						// define stack areaextern	void	stackinit();											// intialize stackextern	file	filelist;												// file variable for file listextern	file	catlist;												// file variable for category listextern	fixed	i_indexstart;											// sector offset of file list indexextern	fixed	i_indexcount;											// no. entries in file list indexextern	fixed	i_filestart;											// sector offset of file listextern	fixed	i_filecount;											// no. entries in file listextern	fixed	i_catstart;												// sector offset of category listextern	fixed	i_catcount;												// no. entries in catetgory listextern	fixed	i_entrycount;											// no. directory entries on optical diskextern	fixed	i_stride;												// file list index strideextern	void	definefilebuffers(fixed filesec, fixed filelen, fixed filemed, fixed catsec, fixed catlen, fixed catmed);	// define index file buffersextern	void	get_index_directory_name(fixed treename[]);				// construct index file tree nameextern	void	get_index_name(fixed filename[], fixed treename[]);		// construct index file tree nameextern	void	get_index1_directory_name(fixed treename[]);			// construct index file tree nameextern	boolean	mount_volume(fixed level);								// mount volumeextern	boolean	check_volume(fixed level);								// see if volume has changedextern	boolean	check_index();											// see if index has changedextern	boolean	open_optical(fixed treename[], fixed level);			// open volume and index fileextern	boolean	check_optical(fixed level);								// see if optical system is still openextern	boolean	update_index_file(fixed treename[], fixed level);		// save index to diskextern	fixed	recnum;													// record number of entry in file listextern	boolean	index_locate(fixed name[], fixed record[], fixed level);	// locate file using index fileextern	boolean	index_replace(fixed name[], fixed type, fixed ms_len, fixed ls_len, fixed wordlen, fixed header[], fixed record[], fixed level);	// replace on opticalextern	boolean	index_delete(fixed name[], fixed record[], fixed level);	// delete file using index fileextern	boolean	index_update(fixed oldname[], fixed newname[], fixed header[], fixed record[], fixed level);	// update file using index fileextern	void	opupdate_main();