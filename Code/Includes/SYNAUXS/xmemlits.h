/* Xmemlits - literals for use with xmem module: *//* The following declarations are used to set up information neeededduring calls to polyread, polywrite, extread, extwrite.  These routinesdo diskreading and diskwriting directly in and out of poly sampling memoryand external memory. */#define	_start_sec		diskio_info[0]			/* Starting sector addr in memory				*/#define	_start_wd		diskio_info[1]			/* Starting word   addr in memory				*/#define	_sec_len		diskio_info[2]			/* Sectors to read or write						*/#define	_wd_len			diskio_info[3]			/* Words   to read or write						*/#define	copy_blen		64						/* Length of copy.buf buffer					*/#define	alt_copy_blen	256						/* Actual length; new code can use this value	*/