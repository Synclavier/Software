/*	Header file for: Optical.c	*//*	Created: 12/22/99 at 22:29	*//*	Version: 0.000				*/#include "OptLits.h"extern	fixed	o_bufptr;												// buffer pointerextern	fixed	o_bufmed;												// 0: internal memory, 1: external memoryextern	array	o_volume_name;											// volume nameextern	fixed	o_ms_dirstart;											// sector start of directoryextern	fixed	o_ls_dirstart;extern	fixed	o_ms_dirlen;											// sector length of directoryextern	fixed	o_ls_dirlen;extern	fixed	o_ms_datastart;											// sector start of data areaextern	fixed	o_ls_datastart;extern	fixed	o_ms_datalen;											// sector length of data areaextern	fixed	o_ls_datalen;extern	fixed	o_entrycount;											// no. directory entries on optical diskextern	void	init_optical_controller();								// call FIRST to set up controller typeextern	void	set_optbuf(fixed, fixed);								// set buffer pointersextern	void	pbuf(fixed, fixed);										// store a word in the bufferextern	fixed	gbuf(fixed);											// retrieve a word from the bufferextern	boolean	getdatetime(fixed[]);									// get date and time from clockextern	fixed	sectoentry(fixed, fixed);								// convert sector no. to directory entry no.extern	void	entrytosec(fixed);										// convert entry no. to a sector no.extern	fixed	transferoptical(fixed, fixed, fixed, fixed, fixed);		// read data into the bufferextern	fixed	readentry(fixed, fixed);								// read a directory entryextern	fixed	writeentry(fixed, fixed);								// write a directory entryextern	fixed	readentries(fixed, fixed, fixed);						// read directory entriesextern	fixed	writeentries(fixed, fixed, fixed);						// write directory entriesextern	boolean	searchemptyblocks(fixed, fixed, fixed, fixed, fixed);	// search for free space on diskextern	boolean	searchempty1024kblocks(fixed, fixed, fixed, fixed, fixed);	// search for free space on diskextern	boolean	findnextunused(fixed, fixed, fixed, fixed, fixed);		// find next unused block in the given sectionextern	fixed	findentrycount(fixed);									// find no. directory entriesextern	boolean	readheader(fixed);										// read volume header and set up globals for this diskextern	boolean	writeheader(fixed[], fixed);							// write volume headerextern	boolean	get_statistics(fixed, fixed[]);							// get disk statisticsextern	boolean	optical_format(fixed[], fixed[], fixed, fixed, fixed, fixed);	// format a new diskextern	boolean	optical_replace(fixed[], fixed, fixed, fixed, fixed, fixed[], fixed, fixed);	// replace on opticalextern	boolean	optical_delete(fixed[], fixed, fixed);					// delete fileextern	boolean	optical_update(fixed[], fixed[], fixed[], fixed, fixed);	// update fileextern	boolean	optical_locate(fixed[], fixed);							// find fileextern	void	optical_main();// ####if 0extern	fixed	o_bufptr;												// buffer pointerextern	fixed	o_bufmed;												// 0: internal memory, 1: external memoryextern	array	o_volume_name;											// volume nameextern	fixed	o_ms_dirstart;											// sector start of directoryextern	fixed	o_ls_dirstart;extern	fixed	o_ms_dirlen;											// sector length of directoryextern	fixed	o_ls_dirlen;extern	fixed	o_ms_datastart;											// sector start of data areaextern	fixed	o_ls_datastart;extern	fixed	o_ms_datalen;											// sector length of data areaextern	fixed	o_ls_datalen;extern	fixed	o_entrycount;											// no. directory entries on optical diskextern	void	init_optical_controller();								// set optical controller type based on system configurationextern	fixed	dev10type;												// Worm device numberextern	void	dev10read(fixed msw, fixed lsw, fixed buffer[], fixed length);	// read WORM deviceextern	void	dev10write(fixed msw, fixed lsw, fixed buffer[], fixed length);	// write WORM deviceextern	void	pbuf(fixed index, fixed value);							// store a word in the bufferextern	fixed	gbuf(fixed index);										// retrieve a word from the bufferextern	void	set_optbuf(fixed bufptr, fixed medium);					// set buffer pointersextern	fixed	transferoptical(fixed cmd, fixed ms_sec, fixed ls_sec, fixed seclen, fixed level);	// read data into the bufferextern	fixed	sectoentry(fixed msw, fixed lsw);						// convert sector no. to directory entry no.extern	void	entrytosec(fixed entry);								// convert entry no. to a sector no.extern	fixed	readentry(fixed entry, fixed level);					// read a directory entryextern	fixed	writeentry(fixed entry, fixed level);					// write a directory entryextern	fixed	readentries(fixed entry, fixed entrycount, fixed level);	// read directory entriesextern	fixed	writeentries(fixed entry, fixed entrycount, fixed level);	// write directory entriesextern	boolean	getdatetime(fixed buf[]);								// get date and time from clockextern	boolean	searchempty1024kblocks(fixed ms_start, fixed ls_start, fixed scanlen, fixed emptylen, fixed level);	// search for empty data blocksextern	boolean	searchemptyblocks(fixed ms_start, fixed ls_start, fixed scanlen, fixed emptylen, fixed level);	// search for empty data blocksextern	boolean	findnextunused(fixed ms_start, fixed ls_start, fixed ms_len, fixed ls_len, fixed level);	// find next unused block in the given sectionextern	fixed	findentrycount(fixed level);							// find no. directory entriesextern	boolean	readheader(fixed level);								// read volume header and set up globals for this diskextern	boolean	writeheader(fixed header[], fixed level);				// write volume headerextern	boolean	get_statistics(fixed level, fixed buf[]);				// get disk statisticsextern	boolean	optical_format(fixed name[], fixed caption[], fixed date, fixed time, fixed serial, fixed level);	// format a new diskextern	boolean	optical_replace(fixed name[], fixed type, fixed ms_len, fixed ls_len, fixed wordlen, fixed header[], fixed entry, fixed level);	// replace on opticalextern	boolean	optical_delete(fixed name[], fixed entry, fixed level);	// delete fileextern	boolean	optical_update(fixed oldname[], fixed newname[], fixed header[], fixed entry, fixed level);	// update fileextern	boolean	optical_locate(fixed name[], fixed level);				// find file#endif