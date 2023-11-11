/*	xpl_run_time.c																*/

/*	Contents: Executable code that supports XPL translations					*/

/*	Created:	11/09/96 - C. Jones												*/

// Std C Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

// Local includes
#include "XPL.h"
#include "XPLRuntime.h"

#include "syslits.h"
#include "ScsiLib.h"
#include "Utility.h"
#include "catrtns.h"
#include "CustomPrintf.h"
#include "MacSCSIOSx.h"

#include "InterChange.h"
#include "SynclavierFileReference.h"
#include "SyncAudioUtilities.h"

#ifndef SYNC_USE_KERNEL
    #define SYNC_USE_KERNEL 1
#endif

#if SYNC_USE_KERNEL
    #include "Synclavier3Constants.h"
    #include "SynclavierPCILib.h"
#endif


/*------------------------------------------------------------------------------*/
/* Multiple Contexts															*/
/*------------------------------------------------------------------------------*/

// Mutlple XPL contexts can be greated. They share the same scsi device
// data base.

// The will allow, for example, one XPL context to be used for interpreting
// while another context is in use for import/export functions.

class XPLContext {
public:
    
    XPLContext ();
    
    _able_context	global_able_context;
    
    int             able_style;                     /* style: kernel or user	*/
    handle          able_memory;                    /* handle for memory		*/
    handle          able_xmem;                      /* handle for ext mem		*/
    long            able_smem_size;
    
};

XPLContext::XPLContext() {
    memset(&global_able_context, 0, sizeof(global_able_context));
    
    able_style		 = 0;                           /* style: kernel or user	*/
    able_memory      = NULL;                        /* handle for memory		*/
    able_xmem		 = NULL;                        /* handle for ext mem		*/
    able_smem_size   = 0;
}

/*------------------------------------------------------------------------------*/
/* Global Variables																*/
/*------------------------------------------------------------------------------*/

static  XPLContext      globalContext;
static  XPLContext*     global_XPL_context = &globalContext;

		_able_context*  g_able_context;				/* working copy				*/

int				g_disallow_run_host_exit;			/* disallow exit			*/
int				g_disallow_atexit;					/* disallow atexit setup	*/
boolean			g_break_received;					/* break received			*/

scsi_device*	g_scsi_device_data_base;
scsi_device* 	g_indexed_device [8];
scsi_settings* 	g_indexed_setting[8];

SCSI_board_list XPL_scsi_code_map;
boolean			g_throw_on_disk_error;

void			(*able_exit_callout)() = NULL;          /* called during atexit */
ufixed			mem_siz = 63 * 1024;                    /* memory size, words   */

static	xpl_file_ref_num    assigned_hfs_rnum [256];	/* holds f ref num      */
static	short               assigned_ref_count[256];	/* ref count therefore  */
static	scsi_device*        assigned_device   [256];	/* scsi_device therefore*/
static	xpl_file_ref        assigned_spec     [256];	/* FSSpec* therefore    */

#if __LP64__
    SyncMutex gXPLMutex;
#endif

/*------------------------------------------------------------------------------*/
/* Byte Swap Glue																*/
/*------------------------------------------------------------------------------*/

// Note -

// This routine is used by (for example) the XPL Compiler running on the mac.
// It is not used by the interpreter to read from W0. As such, we do not
// have to synthesize mac file reads here.
xpl_result  XPLRunTime_FSRead (xpl_file_ref_num refNum, int* count, void* buffPtr)
{
	xpl_result  status = noErr;
	
    #if __LP64__
        size_t  req    = (size_t) *count;
        ssize_t result = read(refNum, buffPtr, req);
    
        if (result == -1) {
            *count = 0;
            status = errno;
        }
    
        else
            *count = (int) result;
    #else
        SInt32  lcount = (SInt32) *count;
    
        status = FSRead(refNum, &lcount, buffPtr);
        
        *count = (int) lcount;
    #endif
    
	#if __LITTLE_ENDIAN__
	{
		ufixed* dataPtr = (ufixed *) buffPtr;
		int     i       = (*count) >> 1;
		
		while (i--) {
			*dataPtr = CFSwapInt16BigToHost(*dataPtr);
            dataPtr++;
        }
	}
	#endif

	return (status);
}

xpl_result	XPLRunTime_FSWrite (xpl_file_ref_num refNum, int* count, const void* buffPtr)
{
	#if __BIG_ENDIAN__
		long  lcount = *count;
		OSErr status = (FSWrite(refNum, &lcount, buffPtr));

		*count=lcount;
		
		return (status);

	#else
		xpl_result  status    = noErr;
		int         globCount = *count;
		char*       globBuf   = (char *) buffPtr;
		int         globDone  = 0;
    
        SyncSwizzleBuffer swb;
		
		*count = globDone;				// Prep return value in case of failure
    
        if (!swb.Buffer())
			return (memFullErr);
		
		while (globCount)
		{
			int     chunkCount = swb.Size();
			ufixed* dataPtr    = (ufixed *) globBuf;
			ufixed* writPtr    = (ufixed *) swb.Buffer();
			int     i;
			
			if (chunkCount > globCount)
				chunkCount = globCount;
			
			i = chunkCount >> 1;
			
			while (i--)
				*writPtr++ = CFSwapInt16BigToHost(*dataPtr++);
			
            #if __LP64__
                size_t  req    = (size_t) chunkCount;
                ssize_t result = write(refNum, (const void *) swb.Buffer(), req);
                
                if (result == -1) {
                    req    = 0;
                    status = errno;
                }
                
                else
                    req = (size_t) result;
            
                globDone += (int) req;
                
                *count = globDone;
                
                if (status || (int) req != chunkCount)
                    return (status);
            #else
                SInt32 reqCount = (SInt32) chunkCount;
			
                status = FSWrite(refNum, &reqCount, (const void *) swb.Buffer());
                
                globDone += (int) reqCount;
                
                *count = globDone;
                
                if (status || (int) reqCount != chunkCount)
                    return (status);
            #endif
				
			globBuf   += chunkCount;
			globCount -= chunkCount;
		}
		
        return (status);
	
	#endif
}

xpl_result  XPLRunTime_FSReadFork (xpl_file_ref_num     forkRefNum,
                                   unsigned short       positionMode,
                                   long long            positionOffset,
                                   unsigned int         requestCount,
                                   void*                buffer,
                                   unsigned int*        actualCount)
{
	xpl_result status = noErr;
    
    #if __LP64__
        // We support fsAtMark and fsFromStart. fsFromStart use pread to set the file position
        if ((positionMode & fsFromStart) != 0) {
            size_t  req    = (size_t) requestCount;
            ssize_t result = pread(forkRefNum, buffer, req, (off_t) positionOffset);
            
            if (result == -1) {
                *actualCount = 0;
                status = errno;
            }
            
            else
                *actualCount = (unsigned int) result;
        }
    
        // fsAtMark use read
        else {
            size_t  req    = (size_t) requestCount;
            ssize_t result = read(forkRefNum, buffer, req);
            
            if (result == -1) {
                *actualCount = 0;
                status = errno;
            }
            
            else
                *actualCount = (unsigned int) result;
        }
    
    #else
        ByteCount  bcount = (ByteCount) *actualCount;
        
        status = FSReadFork(forkRefNum, positionMode, positionOffset, requestCount, buffer, &bcount);
        
        *actualCount = (unsigned int) bcount;
    #endif

	#if __LITTLE_ENDIAN__
	{
		ufixed *     dataPtr = (ufixed *) buffer;
		unsigned int i       = (*actualCount) >> 1;
		
		while (i--) {
			*dataPtr = CFSwapInt16BigToHost(*dataPtr);
            dataPtr++;
        }
	}
	#endif

	return (status);
}


xpl_result  XPLRunTime_FSWriteFork (xpl_file_ref_num    forkRefNum,
                                    unsigned short      positionMode,
                                    long long           positionOffset,
                                    unsigned int        requestCount,
                                    const void*         buffer,
                                    unsigned int*       actualCount)
{
	#if __BIG_ENDIAN__
		return (FSWriteFork(forkRefNum, positionMode, positionOffset, requestCount, buffer, (ByteCount *) actualCount));
	
	#else
        xpl_result      status    = noErr;
		unsigned int    globCount = requestCount;
		char*           globBuf   = (char *) buffer;
		unsigned int    globDone  = 0;
		
        SyncSwizzleBuffer swb;
    
		*actualCount = globDone;				// Prep return value in case of failure
		
        if (!swb.Buffer())
            return (memFullErr);
    
		while (globCount)
		{
			unsigned int chunkCount = swb.Size();
			ufixed* dataPtr = (ufixed *) globBuf;
			ufixed* writPtr = (ufixed *) swb.Buffer();
			int i;
			unsigned int actCount = 0;
			
			if (chunkCount > globCount)
				chunkCount = globCount;
			
			i = chunkCount >> 1;
			
			while (i--)
				*writPtr++ = CFSwapInt16BigToHost(*dataPtr++);
			
            #if __LP64__
                // We support fsAtMark and fsFromStart. fsFromStart use pwrite to set the file position
                if ((positionMode & fsFromStart) != 0) {
                    size_t  req    = (size_t) chunkCount;
                    ssize_t result = pwrite(forkRefNum, (const void *) swb.Buffer(), req, (off_t) positionOffset);
                    
                    if (result == -1) {
                        actCount = 0;
                        status = errno;
                    }
                    
                    else
                        actCount = (unsigned int) result;
                    
                    positionOffset += (long long) actCount;
                }
                
                // fsAtMark use write
                else {
                    size_t  req    = (size_t) chunkCount;
                    ssize_t result = write(forkRefNum, (const void *) swb.Buffer(), req);
                    
                    if (result == -1) {
                        actCount = 0;
                        status = errno;
                    }
                    
                    else
                        actCount = (unsigned int) result;
                }
            
            #else
                ByteCount  bcount = (ByteCount) *actualCount;
            
                status = FSWriteFork(forkRefNum, positionMode, positionOffset, chunkCount, (const void *) swb.Buffer(), &bcount);
            
                actCount = (unsigned int) bcount;

                if ((positionMode & fsFromStart) != 0)
                    positionOffset += (long long) actCount;
            #endif
			
			globDone += actCount;
			
			*actualCount = globDone;
			
			if (status || actCount != chunkCount)
				return (status);
				
			globBuf   += chunkCount;
			globCount -= chunkCount;
		}
		
        return (status);
	
	#endif
}


// Handy swizzle buffer with stack-based mutex. One mutex-protected
// swizzle buffer is available aplication-wide.

SyncSwizzleBuffer::SyncSwizzleBuffer () : waiter(mutex) {
    if (!buffer)
        buffer = (ufixed*) valloc(Size());
}

SyncSwizzleBuffer::~SyncSwizzleBuffer() {
    
}
    
SyncMutex SyncSwizzleBuffer::mutex;
ufixed*   SyncSwizzleBuffer::buffer;


// Handy stack-based buffer
// Class to provide a stack-based self-releasing buffer
SyncStackBuffer::SyncStackBuffer(int size) {
    itsSize = size;
    
    if (itsSize > 0)
        itsBuffer = (ufixed*) valloc(size);
    else
        itsBuffer = NULL;
}

SyncStackBuffer::~SyncStackBuffer() {
    if (itsBuffer)
        free(itsBuffer);
}

void SyncStackBuffer::Resize(int newSize) {
    if (itsBuffer)
        free(itsBuffer);
    
    itsSize = newSize;
    
    if (itsSize > 0)
        itsBuffer = (ufixed*) valloc(itsSize);
    
    else
        itsBuffer = NULL;
}

/*------------------------------------------------------------------------------*/
/* Mac-specific host routines													*/
/*------------------------------------------------------------------------------*/

handle	get_big_memory(int size)
{
    void**	it    = new(void*);
    void*   where = valloc(size);
    
    if (!it || !where)
    {
        if (it   ) delete(it   );
        if (where) free  (where);
        return NULL;
    }
    
    *it = where;
    
    return it;
}

void	free_big_memory(handle it)
{
    if (!it)
        return;
        
    if (*it) free (*it);
    
    *it = NULL;
    
    delete (it);
}

ulong	host_milliseconds()
{
	return (TickCount() * 17);
}


/*------------------------------------------------------------------------------*/
/* print: interpreted able PRINT statement										*/
/*------------------------------------------------------------------------------*/

void (*printChar) (char c) = NULL;
void (*XPLPrinter)(const char *format, va_list) = NULL;

static	void	out_char(char it)					/* handy routine to run		*/
{													/* host environment			*/
	if (it == 7)									/* beep directly to avoid	*/
		/*SysBeep(1)*/;								/* buffering issues			*/
	else if (printChar)
		printChar(it);
	else
		printf("%c", it);	
		
	if (it == '\n' || it == 7)						/* at every end of line		*/
		run_host_environment();						/* or beep, check for break	*/
		
	if (it == 7)									/* after beeping, let		*/
	{												/* system churn for a bit	*/
		long time = TickCount();
		
		while ((TickCount() - time) < 30)
			run_host_environment();
	}
}

void	print(const char *format, ...)
{													/* routine with special		*/
    va_list	vaList;
    va_start(vaList, format);
    
    if (XPLPrinter)
        XPLPrinter(format, vaList);
    else if (printChar)
        parse_printf(printChar, format, vaList);
    else if (strstr(format, "%a") || strstr(format, "%p") || strstr(format, "%1p"))
        parse_printf(out_char, format, vaList);
    else
        vprintf(format, vaList);
}


/*------------------------------------------------------------------------------*/
/* linput: simulated able LINPUT statement										*/
/*------------------------------------------------------------------------------*/

void	linput(array the_line)
{
	int i, j=0;
	
	while (1)
	{
		i = getc(stdin) & 0x7F;				/* get an input character			*/
		
		if (i=='\n') i = 13;				/* map to ables CR					*/
		
		pbyte(the_line, j++, i);
		
		if (i == 13)
		{
			the_line[0] = j;
			pbyte(the_line, j, 0);			/* zero fill upper byte				*/
			return;							/* donezo							*/
		}
	}
}


/*------------------------------------------------------------------------------*/
/* print: simulated able CORE fuction											*/
/*------------------------------------------------------------------------------*/

fixed	_allocate_able_heap(fixed n)
{
	fixed where;
	
	if (!g_able_context)
		return (0);
		
	where = ABLE_CONTEXT._able_heap_pointer;
	
	ABLE_CONTEXT._able_heap_pointer += n;
	
	return (where);

}

static	fixed	set_up_able_core()
{
	int i;
	
	if (!g_able_context)
		return(-1);
		
	if (!global_XPL_context->able_style)
		global_XPL_context->able_style = XPL_RUN_TIME_USE_BIG_MEMORY;

	if (global_XPL_context->able_style == XPL_RUN_TIME_USE_BIG_MEMORY)
	{
		if (!global_XPL_context->able_memory)
			global_XPL_context->able_memory = get_big_memory(_ABLE_MEM_WORD_SIZE << 1);
		
		if (!global_XPL_context->able_memory || !*global_XPL_context->able_memory)
		{
			printf("XPL Run Time: no ABLE memory available...\n");
			return(-1);
		}
		
		ABLE_CONTEXT._able_memory_ = (fixed *) *global_XPL_context->able_memory;
	}
	
    #if SYNC_USE_KERNEL
        else if (global_XPL_context->able_style == XPL_RUN_TIME_USE_KERNEL_MEMORY)
        {
            ABLE_CONTEXT._able_memory_ = SynclavierPCILib_FetchInternalMemory(_ABLE_MEM_WORD_SIZE << 1);
            
            if (!ABLE_CONTEXT._able_memory_)
                return(-1);
        }
    #endif
    
	else
		return(-1);
		
	ABLE_CONTEXT._able_memory_size_bytes = _ABLE_MEM_WORD_SIZE << 1;
	
	memset(ABLE_CONTEXT._able_memory_, 0, _ABLE_MEM_WORD_SIZE << 1);
	
	
	/* Synthesize low core configuration area: */
	
	set_able_core(1, 100);								/* put mock config table at 100			*/
	
	set_able_core(c_contab + c_vstart, 100 + c_strend);	/* put variables after storage table	*/
	set_able_core(c_contab + c_vlngth, 200);			/* assume 200 variables					*/
	set_able_core(c_contab + c_stklen, 200);			/* plus 200 for stack					*/
	
	set_able_core(c_contab + c_memsiz, mem_siz >> 8);
	set_able_core(c_contab + c_cmopt,  shl(3,8) | o_d4567); // 3 for Model D, includes Mul/Div
	set_able_core(c_contab + c_curdev, 1);				/* cur dev to w0						*/
	
	for (i = c_strdev; i < c_strend; i++)				/* storage devices will get set			*/
		set_able_core(c_contab + i, -1);				/* here later							*/
	
	
	/* publish heap pointer: */
	
	ABLE_CONTEXT._able_heap_pointer = able_core(c_contab + c_vstart) + able_core(c_contab + c_vlngth) + able_core(c_contab + c_stklen);
	
	
	/* set up high memory simulation */
	
	to_able_string(".CURRENT", &(ABLE_CONTEXT._able_memory_[(uint32) (ufixed) loc_cfn]));
	
	set_able_core(loc_ftyp	  , 0);					/* current file type							*/
	set_able_core(loc_svd	  , 0);					/* True if file is saved						*/
	set_able_core(loc_strd	  , 0);					/* True if file stored							*/
	set_able_core(loc_cmed	  , 0);					/* current file media:  0 = file, 1 = core		*/
	set_able_core(loc_csec	  , 0);					/* starting sector/device of current file or word ptr if in core	*/
	set_able_core(loc_csec + 1, 0);
	set_able_core(loc_csln	  , 0);					/* number of sectors in current file			*/
	set_able_core(loc_clen	  , 0);					/* current file length mod 64K					*/

	/* system, path, and user (current) catalog definitions */
	set_able_core(loc_scat	  , 0     );			/* system catalog sector						*/
	set_able_core(loc_scat + 1, ABLE_W0_READDATA_CODE << 8);
	set_able_core(loc_sctl	  , 1024  );			/* system catalog length						*/
	set_able_core(loc_smax	  , 4     );			/* system catalog maximum						*/
	set_able_core(loc_smax + 1, 0     );
	set_able_core(loc_pcat	  , 0     );			/* path   catalog sector (0 if no path)			*/
	set_able_core(loc_pcat + 1, 0     );			/* path   catalog sector (0 if no path)			*/
	set_able_core(loc_pctl	  , 0     );			/* path   catalog length						*/
	set_able_core(loc_pmax	  , 0     );			/* path   catalog maximum						*/
	set_able_core(loc_pcnm    , 0     );			/* path   catalog name							*/
	set_able_core(loc_ucat    , 0     );			/* user   catalog sector						*/
	set_able_core(loc_ucat + 1, ABLE_W0_READDATA_CODE << 8);
	set_able_core(loc_uctl	  , 1024  );			/* user   catalog length						*/
	set_able_core(loc_umax	  , 4     );			/* user   catalog maximum						*/
	set_able_core(loc_umax + 1, 0     );
	set_able_core(loc_ccnm	  , 0     );			/* user   catalog name							*/
	set_able_core(loc_ccnm + 1, 0     );
	set_able_core(loc_ccnm + 2, 0     );
	set_able_core(loc_ccnm + 3, 0     );
	set_able_core(loc_ccnm + 4, 0     );

	to_able_string("W0:", &(ABLE_CONTEXT._able_memory_[(uint32) (ufixed) loc_ccnm]));

	/* work file definitions */
	set_able_core(loc_wmed	    ,0);				/* work file media:  0 = file, 1 = core			*/
	set_able_core(loc_wsec	    ,0);				/* starting sector of work file or word ptr if in core	*/
	set_able_core(loc_wsln	    ,0);				/* number of sectors in work file				*/
	set_able_core(loc_wlen	    ,0);				/* work file length mod 64K						*/

	/* command file definitions */
	set_able_core(loc_perform	,0);				/* indicates in a command file					*/
	set_able_core(loc_perptr	,0);				/* byte pointer into command file				*/
	set_able_core(loc_perfsec	,0);				/* starting sector of command file				*/
	set_able_core(loc_perflen	,0);				/* word length of command file					*/

	/* system file locations/lengths (named after the system file they're most often used for) */
	set_able_core(loc_mon		,0);				/* monitor (starting sector at LOC.MON, word length (or -1) at LOC.MON - 1)	*/
	set_able_core(loc_p1		,0);				/* pass1										*/
	set_able_core(loc_p2		,0);				/* pass2										*/
	set_able_core(loc_p3		,0);				/* pass3										*/
	set_able_core(loc_st		,0);				/* symbol table									*/
	set_able_core(loc_rt		,0);				/* runtime file									*/
	set_able_core(loc_mplt		,0);				/* music printing								*/

	/* overlay parameters/return values */
	set_able_core(loc_rst		,0);				/* run status (to MONITOR)						*/
	set_able_core(loc_usr1		,0);				/* user defined									*/
	set_able_core(loc_usr2		,0);				/* user defined									*/
	set_able_core(loc_usr3		,0);				/* user defined									*/

	/* monitor bit definitions */
	set_able_core(loc_monbits	,0);				/* bits for monitor ON/OFF states				*/

	/* Monitor state variables */
	set_able_core(loc_tyb		,0);				/* 110 char tty buffer (plus length)			*/
	set_able_core(loc_tybp		,0);				/* byte pointer into tty buffer					*/
	set_able_core(loc_inch		,0);				/* input character								*/
	set_able_core(loc_cnum		,0);				/* current command number						*/
	
	to_able_string("Ready > ", &(ABLE_CONTEXT._able_memory_[(uint32) loc_prmt]));
	set_able_core(loc_screen	,52);				/* lines/screen on the terminal (0 means the terminal isn't screen-oriented)	*/

	/* Monitor bootload state variables */
	set_able_core(loc_magic		,12345);			/* location of magic number						*/
	set_able_core(loc_rst		,    2);			/* run status 2 simulates play command			*/
	set_able_core(loc_psys		,1    );			/* previous SYSTYP to detect bootload error		*/

	/* Sed state variables */
	set_able_core(loc_sed1		,0);				/* reserved for sed use							*/
	set_able_core(loc_sed2		,0);				/* reserved for sed use							*/

	/* configuration/device information */
	set_able_core(loc_headpos	,0);				/* drive F0, F1, R0, and R1 head positions		*/
	
	if (ABLE_CONTEXT._able_xmem_size > 32)			/* set em size if allocated already				*/
		set_able_core(loc_emsize, ABLE_CONTEXT._able_xmem_size - 32);
	else
		set_able_core(loc_emsize, 0);

	/* word (MEM.SIZ - 245) is currently unused */

	/* Synclavier (r)/SCRIPT state variables */
	to_able_string("SYN-4.00", &(ABLE_CONTEXT._able_memory_[(uint32) (ufixed) loc_synrtpn]));
	set_able_core(loc_synmed	,0);				/* synclavier file media (0 = normal memory, 1 = expanded memory)	*/
	set_able_core(loc_synmisc	,0);				/* pointer to synclavier miscellaneous area		*/
	set_able_core(loc_syntimb	,0);				/* pointer to synclavier timbre area			*/
	set_able_core(loc_synseq	,0);				/* pointer to synclavier sequence notes			*/
	set_able_core(loc_synstat	,0);				/* synclavier status word						*/
	set_able_core(loc_synret	,0);				/* pointer to word trio (above) describing program to return to if no room	*/
	
	for (i = c_offset; i < c_strend; i++)
		set_able_core(loc_ctab + i - c_offset, able_core(c_contab + i));

	return (0);
}

fixed	able_core(fixed  where)
{
	if (!g_able_context)
		return (0);
		
	if (!ABLE_CONTEXT._able_memory_)
	{
		if (set_up_able_core())
			exit(-1);
	}
	
	return (ABLE_CONTEXT._able_memory_[(uint32) (ufixed) where]);
}

fixed	host_core(fixed *where)
{
    // Handle case where 'where' is likely a pointer into the able memory word space instead of a host-native fixed*.
	if (((ptrtyp) where) < _ABLE_MEM_WORD_SIZE)
	{
		printf("XPL Run Time: bad location passed to host_core (0x%p)\n", where);
		exit(-1);
	}
	
	return *where;
}

void	set_able_core(fixed  where, fixed value)
{
	if (!g_able_context)
		return;
		
	if (!ABLE_CONTEXT._able_memory_)
	{
		if (set_up_able_core())
			exit(-1);
	}
	
	ABLE_CONTEXT._able_memory_[(uint32) (ufixed) where] = value;
}

void	set_host_core(fixed *where, fixed value)
{
    // Handle case where 'where' is likely a pointer into the able memory word space instead of a host-native fixed*.
	if (((ptrtyp) where) < _ABLE_MEM_WORD_SIZE)
	{
		printf("XPL Run Time: bad location passed to set_host_core (0x%p)\n", where);
		exit(-1);
	}
	
	*where = value;
}


/*------------------------------------------------------------------------------*/
/* Model ABLE external memory													*/
/*------------------------------------------------------------------------------*/

static	fixed	set_up_able_xmem(ulong ext_mem_size_sectors)
{
	if (!g_able_context)
		return(-1);

	if (!global_XPL_context->able_style)
		global_XPL_context->able_style = XPL_RUN_TIME_USE_BIG_MEMORY;
		
	if (global_XPL_context->able_style == XPL_RUN_TIME_USE_BIG_MEMORY)
	{
		if (global_XPL_context->able_xmem)  /* if exists, see if is too small   */
		{
			if (global_XPL_context->able_smem_size < ext_mem_size_sectors)
			{
				free_big_memory(global_XPL_context->able_xmem);
				global_XPL_context->able_xmem      = NULL;
				global_XPL_context->able_smem_size = 0;
			
				if (g_able_context)		
					ABLE_CONTEXT.d60 = NULL;
			}
		}

		if (!global_XPL_context->able_xmem)
		{
			global_XPL_context->able_xmem      = get_big_memory(ext_mem_size_sectors << 9);
			global_XPL_context->able_smem_size = ext_mem_size_sectors;
		}
			
		if (!global_XPL_context->able_xmem || !*global_XPL_context->able_xmem)
		{
			printf("XPL Run Time: no ABLE external memory available...\n");
			return(-1);
		}

		ABLE_CONTEXT.d60 = (fixed *) *global_XPL_context->able_xmem;
	}
	
    #if SYNC_USE_KERNEL
        else if (global_XPL_context->able_style == XPL_RUN_TIME_USE_KERNEL_MEMORY)
        {
            ABLE_CONTEXT.d60 = SynclavierPCILib_FetchExternalMemory(ext_mem_size_sectors << 9);
            
            if (!ABLE_CONTEXT.d60)
                return(-1);
        }
    #endif
    
	else
		return(-1);
	
	ABLE_CONTEXT._able_xmem_size_bytes = ext_mem_size_sectors << 9;
			
	memset(ABLE_CONTEXT.d60, 0, ext_mem_size_sectors << 9);
	ABLE_CONTEXT._able_xmem_addr = 0;
	ABLE_CONTEXT._able_xmem_size = (fixed) (ext_mem_size_sectors);

	if (ABLE_CONTEXT._able_memory_ && ABLE_CONTEXT._able_xmem_size > 32)
		ABLE_CONTEXT._able_memory_[(uint32) (ufixed) loc_emsize] = ABLE_CONTEXT._able_xmem_size - 32;
		
	return (0);
}

void	_write_60(fixed address)
{
	if (!ABLE_CONTEXT.d60 || !g_able_context)
	{
		if (set_up_able_xmem(3*512*1024/256))
			exit(-1);
	}
		
	ABLE_CONTEXT._able_xmem_addr = ((ulong) (ufixed) address) << 8;
}

void	_write_61(fixed address)
{
	if (!ABLE_CONTEXT.d60 || !g_able_context)
	{
		if (set_up_able_xmem(3*512*1024/256))
			exit(-1);
	}
		
	ABLE_CONTEXT._able_xmem_addr = (ABLE_CONTEXT._able_xmem_addr & 0xFFFFFF00) | ((ulong) (ufixed) (address & 0xFF));
}

void	_write_62(fixed data)
{
	ABLE_CONTEXT.d60[ABLE_CONTEXT._able_xmem_addr  ]=data;
}

void	_write_63(fixed data)
{
	ABLE_CONTEXT.d60[ABLE_CONTEXT._able_xmem_addr++]=data;
}

fixed	_read_60()
{
	if (!ABLE_CONTEXT.d60 || !g_able_context)
	{
		if (set_up_able_xmem(3*1024*512/256))
			exit(-1);
	}
		
	if (ABLE_CONTEXT._able_xmem_addr >= (ABLE_CONTEXT._able_xmem_size << 8))
		return (0);
	else
		return ((fixed) (ABLE_CONTEXT._able_xmem_addr >> 8));
}

fixed	_read_61()
{
	if (!ABLE_CONTEXT.d60 || !g_able_context)
	{
		if (set_up_able_xmem(3*512*1024/256))
			exit(-1);
	}
		
	if (ABLE_CONTEXT._able_xmem_addr >= (ABLE_CONTEXT._able_xmem_size << 8))
		return (0);
	else
		return ((fixed) (ABLE_CONTEXT._able_xmem_addr & 0xFF));
}

fixed	_read_62()
{
	return ABLE_CONTEXT.d60[ABLE_CONTEXT._able_xmem_addr  ];
}

fixed	_read_63()
{
	return ABLE_CONTEXT.d60[ABLE_CONTEXT._able_xmem_addr++];
}



/*------------------------------------------------------------------------------*/
/* Misc memory routines															*/
/*------------------------------------------------------------------------------*/

void	blockset(array where, fixed num, fixed val)
{
	while (num--)
		*where++ = val;
}

void	blockmove(array source, array dest, fixed length)
{
	if (((ptrtyp) source) < ((ptrtyp) dest))					/* moving up		*/
	{
		source += (ufixed) length;
		dest   += (ufixed) length;
		
		while (length--)
			*--dest = *--source;
	}
	
	else													/* moving down		*/
	{
		while (length--)
			*dest++ = *source++;
	}
}

void	XPLexport   (fixed mam, fixed mal, array where, fixed words)
{
	_write_60(mam + shr(mal,8));
	_write_61(mal);
	
	while (words--)
		_write_63(*where++);
}

void	XPLimport   (fixed mam, fixed mal, array where, fixed words)
{
	_write_60(mam + shr(mal,8));
	_write_61(mal);
	
	while (words--)
		*where++ = _read_63();
}

void	XPLextset	 (fixed mam, fixed mal, fixed words, fixed value)
{
	_write_60(mam + shr(mal,8));
	_write_61(mal);
	
	while (words--)
		_write_63(value);
}


/*------------------------------------------------------------------------------*/
/* initialize able run time environment											*/
/*------------------------------------------------------------------------------*/

static	void	MySignalHandler(int x)
{
	#pragma unused (x)
	g_break_received = true;
}

static	void	XPL_run_time_cleanup(void)
{
	if (able_exit_callout)						/* call higher level one 		*/
	{
		able_exit_callout();					/* if any...					*/
		able_exit_callout = NULL;
	}
	
	if (global_XPL_context->able_memory)
	{
		free_big_memory(global_XPL_context->able_memory);
		global_XPL_context->able_memory = NULL;
	}
				
	if (g_able_context)
		ABLE_CONTEXT._able_memory_ = NULL;
	
	if (global_XPL_context->able_xmem)
	{
		free_big_memory(global_XPL_context->able_xmem);
		global_XPL_context->able_xmem = NULL;
	}
	
	if (g_able_context)		
		ABLE_CONTEXT.d60 = NULL;
	
	global_XPL_context->able_smem_size = 0;
    global_XPL_context->able_style     = 0;
}

// In the OS X implementation this must be set very early in the game. It tells us whether we are joining forces
// with a kernel driver to do the work, or whether we are on our own in user land
void	set_run_time_situation(int it)
{
	global_XPL_context->able_style = it;
}

fixed	initialize_run_time_environment(ulong ext_mem_size_sectors)
{
	// Get "static" memory first time through
	if (!g_scsi_device_data_base)
		g_scsi_device_data_base = (scsi_device*) malloc(sizeof(scsi_device)*MAX_NUM_DEVICES);
	
	if (!g_scsi_device_data_base)
		return (-1);

    // One global_XPL_context is staticly created; it must be around early on - for example so set_run_time_situation can be called...
	if (!global_XPL_context)
		return (-1);

	// Reset global variables in case we are being restarted
	memset(&global_XPL_context->global_able_context, 0, sizeof(global_XPL_context->global_able_context));
	memset(g_scsi_device_data_base, 0, sizeof(scsi_device)*MAX_NUM_DEVICES);
	g_able_context = &global_XPL_context->global_able_context;

	mem_siz = 63 * 1024;
	
	if (!g_disallow_atexit)
	{
		atexit (XPL_run_time_cleanup);
	
		g_disallow_atexit = true;
	}
	
	// Set up able core and external memory in call cases
	if (set_up_able_core())
		return (-1);
		
	if (set_up_able_xmem(ext_mem_size_sectors))
		return (-1);
	
	// Initialize for possible SCSI use
    for (int i=0; i<NUM_TARGETS_IN_TARGET_LIST; i++) {
        if (i != ABLE_IMPORT_SCSI_ID) {
            g_indexed_device[ i] = NULL;
            g_indexed_setting[i] = NULL;
        }
    }

    memset(assigned_hfs_rnum,  0, sizeof(assigned_hfs_rnum ));
	memset(assigned_ref_count, 0, sizeof(assigned_ref_count));
	memset(assigned_device,    0, sizeof(assigned_device   ));
	memset(assigned_spec,      0, sizeof(assigned_spec     ));
	
	signal(SIGINT, MySignalHandler);

	return (0);
}

void	cleanup_run_time_environment()
{
	// Remove our break intercept
	signal(SIGINT, SIG_DFL);
	
	// Free memory & otherwise terminate
	XPL_run_time_cleanup();
	
	// Reset global variables to avoid leaving dangling pointers
	memset(&global_XPL_context->global_able_context, 0, sizeof(global_XPL_context->global_able_context));
	g_able_context = &global_XPL_context->global_able_context;

	mem_siz = 63 * 1024;
    
    if (g_scsi_device_data_base) {
        free(g_scsi_device_data_base);
        g_scsi_device_data_base = NULL;
    }
	
	return;
}


/*------------------------------------------------------------------------------*/
/* xpl-style interface to host file system										*/
/*------------------------------------------------------------------------------*/

#if INCLUDE_open_able_file

// Used by XPL compiler - not used by interpreter.

void* open_able_file(char *file_name)
{
	FILE *in_file;
	int  i,j;
    char opened_file_name[256] = {0};

	if (!g_able_context)
		{printf("XPL Run Time: can't open file '%s' (missing context)\n", file_name); exit(-1);}
	
	if (file_name[0] == ':' || file_name[0] == '/')		/* file name begins with ":" or "/" : use master dir	*/
	{
		i = strlen(ABLE_CONTEXT.able_master_dir_name);	/* get length of master directory				*/
		strncpy (ABLE_CONTEXT.opened_file_name, ABLE_CONTEXT.able_master_dir_name, sizeof(ABLE_CONTEXT.opened_file_name));
	
		if (i && ABLE_CONTEXT.opened_file_name[i-1] == ':')	/* remove trailing : from master dir name		*/
			ABLE_CONTEXT.opened_file_name[i-1] = 0;			/* since our file name starts with :			*/
			
        else if (i && ABLE_CONTEXT.opened_file_name[i-1] == '/')	/* remove trailing / from master dir name		*/
            ABLE_CONTEXT.opened_file_name[i-1] = 0;			/* since our file name starts with :			*/
        
		strncat (ABLE_CONTEXT.opened_file_name, file_name, sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name));

        // Also compute file name in output hierarchy. Handles open of .srel files built just moments ago.
        i = strlen(ABLE_CONTEXT.host_output_dir_name);	/* get length of master directory				*/
        strncpy (opened_file_name, ABLE_CONTEXT.host_output_dir_name, sizeof(opened_file_name));
        
        if (i && opened_file_name[i-1] == ':')	/* remove trailing : from master dir name		*/
            opened_file_name[i-1] = 0;			/* since our file name starts with :			*/
        
        else if (i && opened_file_name[i-1] == '/')	/* remove trailing / from master dir name		*/
            opened_file_name[i-1] = 0;			/* since our file name starts with :			*/
        
        strncat (opened_file_name, file_name, sizeof(opened_file_name) - strlen(opened_file_name) - 1);
    }
	
	else if (ABLE_CONTEXT.able_cur_dir_name[0])
	{
		strncpy (ABLE_CONTEXT.opened_file_name, ABLE_CONTEXT.able_cur_dir_name, sizeof(ABLE_CONTEXT.opened_file_name));
		strncat (ABLE_CONTEXT.opened_file_name, file_name, sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name));
	}
	
	else
	{	
		strncpy (ABLE_CONTEXT.opened_file_name, ":", sizeof(ABLE_CONTEXT.opened_file_name));	/* else need colon here...						*/
		strncat (ABLE_CONTEXT.opened_file_name, file_name, sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name) - 1);
	}

	// Convert ':' to '/'
	i = strlen(ABLE_CONTEXT.opened_file_name);
	
	for (j=0; j<i; j++)
		if (ABLE_CONTEXT.opened_file_name[j] == ':')
			ABLE_CONTEXT.opened_file_name[j] = '/';
    
    i = strlen(opened_file_name);
    
    for (j=0; j<i; j++)
        if (opened_file_name[j] == ':')
            opened_file_name[j] = '/';
    
    in_file = fopen(ABLE_CONTEXT.opened_file_name, "rb");
    
    // Check in output hierarchy as well
    if (in_file == NULL && opened_file_name[0] != 0)
        in_file = fopen(opened_file_name, "rb");
    
	if (in_file == NULL) {
        // Filename has extension - bail immediately
        if (strchr(file_name, '.') != NULL)
            {printf("XPL Run Time: can't open file '%s' ('%s') ('%s')\n", file_name, ABLE_CONTEXT.opened_file_name, opened_file_name); exit(-1);}
        
        // Append .xpl to file name. Handles open of XPL insert files with 8-character file names
        strncat (ABLE_CONTEXT.opened_file_name, ".xpl", sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name));
        
        in_file = fopen(ABLE_CONTEXT.opened_file_name, "rb");
        
        // Check in output hierarchy as well
        if (in_file == NULL && opened_file_name[0] != 0)
        {
            strncat (opened_file_name, ".xpl", sizeof(opened_file_name) - strlen(opened_file_name) - 1);
            in_file = fopen(opened_file_name, "rb");
        }
        
        if (in_file  == NULL)
            {printf("XPL Run Time: can't open file '%s' ('%s') ('%s')\n", file_name, ABLE_CONTEXT.opened_file_name, opened_file_name); exit(-1);}
	}
    
	setvbuf(in_file, NULL, _IOFBF, 128*1024);

	return ((void *) in_file);
}

void* open_able_file_for_output(char *file_name, int type, int creator)
{
	FILE 	*out_file;
	int i,j;
	
	if (!g_able_context)
		{printf("XPL Run Time: can't open file '%s' (missing context)\n", file_name); exit(-1);}
	
	if (file_name[0] == ':' || file_name[0] == '/')			/* file name begins with ":" or "/" : use output directory or master dir	*/
	{
		int i = strlen(ABLE_CONTEXT.host_output_dir_name);	/* See if output directory specified */
		
        if (i)
            strncpy (ABLE_CONTEXT.opened_file_name, ABLE_CONTEXT.host_output_dir_name, sizeof(ABLE_CONTEXT.opened_file_name));
	
        else {
            i = strlen(ABLE_CONTEXT.able_master_dir_name);	/* get length of master directory				*/
            strncpy (ABLE_CONTEXT.opened_file_name, ABLE_CONTEXT.able_master_dir_name, sizeof(ABLE_CONTEXT.opened_file_name));
        }
        
		if (i && ABLE_CONTEXT.opened_file_name[i-1] == ':')	/* remove trailing : from master/output dir name		*/
			ABLE_CONTEXT.opened_file_name[i-1] = 0;			/* since our file name starts with :			*/
			
        else if (i && ABLE_CONTEXT.opened_file_name[i-1] == '/')	/* remove trailing / from master/output dir name		*/
            ABLE_CONTEXT.opened_file_name[i-1] = 0;			/* since our file name starts with :			*/
        
		strncat (ABLE_CONTEXT.opened_file_name, file_name, sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name));
	}
	
	else if (ABLE_CONTEXT.host_output_dir_name[0])	/* else if overriding output dir is specified	*/
	{
		strncpy (ABLE_CONTEXT.opened_file_name, ABLE_CONTEXT.host_output_dir_name, sizeof(ABLE_CONTEXT.opened_file_name));
		strncat (ABLE_CONTEXT.opened_file_name, file_name, sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name));
	}
	
	else										/* else if non specified						*/
	{											/* then use mac current dir						*/
        // For 64-bit builds use unix current directory
        #if __LP64__
            strncpy (ABLE_CONTEXT.opened_file_name, "",        sizeof(ABLE_CONTEXT.opened_file_name));	/* else need colon here...	*/
            strncat (ABLE_CONTEXT.opened_file_name, file_name, sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name) - 1);
        #else
            OSErr           FSstatus;
            FSVolumeRefNum	VRefNum      = 0;
            SInt32          dirID        = 0;
        
            char VolName[512] = {""};
            
            FSstatus = HGetVol((StringPtr) VolName, &VRefNum, &dirID);
            
            if (FSstatus)
                {printf("XPL Run Time: Failed HGetVol (%d)\n", FSstatus); p2cstr((uint8 *) VolName); exit(-1);}
            
            p2cstr((uint8 *) VolName);
            
            strncpy (ABLE_CONTEXT.opened_file_name, ":",        sizeof(ABLE_CONTEXT.opened_file_name));	/* else need colon here...	*/
            strncat (ABLE_CONTEXT.opened_file_name, file_name,  sizeof(ABLE_CONTEXT.opened_file_name) - strlen(ABLE_CONTEXT.opened_file_name));
        #endif
    }
	
	// Append mandatory file extension
	if (type == 'RLOC')
	{
		strncat (ABLE_CONTEXT.opened_file_name, ".srel",  sizeof(ABLE_CONTEXT.opened_file_name) - strlen(".srel"));
	}
	
	if (type == 'EXEC')
	{
		strncat (ABLE_CONTEXT.opened_file_name, ".sprg",  sizeof(ABLE_CONTEXT.opened_file_name) - strlen(".sprg"));
	}

	if (type == 'DATA')
	{
		strncat (ABLE_CONTEXT.opened_file_name, ".sdat",  sizeof(ABLE_CONTEXT.opened_file_name) - strlen(".sdat"));
	}

	if (type == 'TEXT')
	{
		strncat (ABLE_CONTEXT.opened_file_name, ".txt",  sizeof(ABLE_CONTEXT.opened_file_name) - strlen(".txt"));
	}

	// Convert ":" in file names to '/'
	i =  strlen(ABLE_CONTEXT.opened_file_name);
	
	for (j=0; j<i; j++)
		if (ABLE_CONTEXT.opened_file_name[j] == ':')
			ABLE_CONTEXT.opened_file_name[j] = '/';

	/* delete prior file: */
	
	out_file = fopen(ABLE_CONTEXT.opened_file_name, "rb+");		/* if can open...	*/
	
    if (out_file)
	{
		fclose(out_file);
		remove(ABLE_CONTEXT.opened_file_name);
		out_file = NULL;
	}

	/* create new file: */
	
	if ((out_file = fopen(ABLE_CONTEXT.opened_file_name, "wb+")) == 0)
		{printf("XPL Run Time: can't open output file %s (%s)\n", file_name, ABLE_CONTEXT.opened_file_name); exit(-1);}
		
	setvbuf(out_file, NULL, _IOFBF, 128*1024);

	return ((void *) out_file);
}

#endif

/*------------------------------------------------------------------------------*/
/* run_host_environment															*/
/*------------------------------------------------------------------------------*/

static	unsigned long prior_time;
void    (*host_yielder)() = NULL;           /* builds using async IO call this thread yielder   */

void	run_host_environment()
{
    // Yielder provided for non-preemptive builds
    if (host_yielder)
        host_yielder();
    
	prior_time = host_milliseconds();

	if (g_break_received)
		if (!g_disallow_run_host_exit)
			exit(-9);
}

void	run_host_environment_250()
{
	unsigned long	x;
	long			y;

	if (prior_time == 0)
	{
		run_host_environment();
		return;
	}
	
	x = host_milliseconds();
		
	if ((y = (x - prior_time)) >= 250)
		run_host_environment();
}


/*------------------------------------------------------------------------------*/
/* Access ABLE hard drive														*/
/*------------------------------------------------------------------------------*/
// Obsolete routine: use XPLRunTime_ConfigureSCSIMap for future applications
void	configure_able_hard_drives (int w0_scsi_id, int w0_cyls, int w0_secs,
									int w1_scsi_id, int w1_cyls, int w1_secs)
{
	int    i       = c_contab + c_strdev;					/* jam into first entry	*/
	long   w0_size = w0_cyls*w0_secs;	
	long   w1_size = w1_cyls*w1_secs;
    #pragma unused(w0_size)
	
    if (!g_able_context || able_core(i) != (-1))			/* make sure free		*/
	{
		printf("XPL Run Time: Error configuring W0\n");
		exit(-1);
	}
	
	if (w0_cyls || w0_secs)
	{
		set_able_core(i + s_devtyp, 0x0101);				/* w0, scsi				*/
		set_able_core(i + s_seccyl, (fixed) w0_secs   );	/* secs/cyl				*/
		set_able_core(i + s_totcyl, (fixed) w0_cyls   );	/* cyls					*/
		set_able_core(i + s_devadr, (fixed) w0_scsi_id);	/* target				*/

		i += 4;
	}
	
	if (w1_cyls || w1_secs)
	{
		set_able_core(i + s_devtyp, 0x0111);				/* w1, scsi				*/
		set_able_core(i + s_seccyl, (fixed) w1_secs   );	/* secs/cyl				*/
		set_able_core(i + s_totcyl, (fixed) w1_cyls   );	/* cyls					*/
		set_able_core(i + s_devadr, (fixed) w1_scsi_id);	/* target				*/

		i += 4;
        (void)i;
	}

	/* system, path, and user (current) catalog definitions */
	set_able_core(loc_scat	  , 0     );						/* system catalog sector						*/
	set_able_core(loc_scat + 1, ABLE_W0_READDATA_CODE << 8);	/* w0											*/
	set_able_core(loc_sctl	  , 1024  );						/* system catalog length						*/
	set_able_core(loc_smax	  , w1_size & 0xFFFF);				/* system catalog maximum						*/
	set_able_core(loc_smax + 1, w1_size >> 16   );
	set_able_core(loc_pcat	  , 0     );						/* path   catalog sector (0 if no path)			*/
	set_able_core(loc_pcat + 1, 0     );						/* path   catalog sector (0 if no path)			*/
	set_able_core(loc_pctl	  , 0     );						/* path   catalog length						*/
	set_able_core(loc_pmax	  , 0     );						/* path   catalog maximum						*/
	set_able_core(loc_pcnm    , 0     );						/* path   catalog name							*/
	set_able_core(loc_ucat    , 0     );						/* user   catalog sector						*/
	set_able_core(loc_ucat + 1, ABLE_W0_READDATA_CODE << 8);			/* w0:											*/
	set_able_core(loc_uctl	  , 1024  );						/* user   catalog length						*/
	set_able_core(loc_umax	  , w1_size & 0xFFFF);				/* user   catalog maximum						*/
	set_able_core(loc_umax + 1, w1_size >> 16   );
	set_able_core(loc_ccnm	  , 0     );						/* user   catalog name							*/
	set_able_core(loc_ccnm + 1, 0     );
	set_able_core(loc_ccnm + 2, 0     );
	set_able_core(loc_ccnm + 3, 0     );
	set_able_core(loc_ccnm + 4, 0     );
	
	to_able_string("W0:", &(ABLE_CONTEXT._able_memory_[(uint32) (ufixed) loc_ccnm]));
}

/*Logical device numbers are as follows:
 *   0: System device (represented by first entry in storage device table)
 *   1: Current device (encoded device number just before storage device table)
 *   2: F0 - floppy 0
 *   3: F1 - floppy 1
 *   4: R0 - remote 0
 *   5: R1 - remote 1
 *   6: W0 - winchester 0
 *   7: W1 - winchester 1
 *   8-15: user devices
 *
 *Note that the LSB of each logical code represents a drive number while
 *the rest of the code represents a class of device.  This happens to be
 *coincidental, but the code takes advantage of this fact.  If this changes,
 *the code must also change.
 *
 *Encoded device numbers are formatted as follows:
 *   bits  0-3: Device type (0: floppy, 1: winchester, 2: remote)
 *   bits  4-7: Drive number (currently only zero or one)
 *   bits 8-15: Zero
*/

xpl_file_ref_num    find_hfs_device (ufixed device)		// find a device which is a mac HFS image file
{
	if ((device >= ABLE_HFS_READDATA_CODE)
	&&  (device < 256)
	&&  (assigned_hfs_rnum[device]))
		return (assigned_hfs_rnum[device]);
	else
		return (0);
}

scsi_device*	find_hfs_scsi_device (ufixed device)	// find a scsi_device which is a mac HFS image file
{
	if ((device >= ABLE_HFS_READDATA_CODE)
	&&  (device < 256)
	&&  (assigned_device[device]))
		return (assigned_device[device]);
	else
		return (NULL);
}

xpl_file_ref	find_hfs_scsi_spec (ufixed device)		// find a fsspec which is a mac HFS image file
{
	if ((device >= ABLE_HFS_READDATA_CODE)
	&&  (device < 256)
	&&  (assigned_spec[device]))
		return (xpl_file_ref) (assigned_spec[device]);
	else
		return (NULL);
}

ufixed	find_device 	(fixed device)					/* find device entry	*/
{
	ufixed i       = c_contab + c_strdev;				/* check first entry	*/
	ufixed counter = 0;
	ufixed type;

	if (device == 0)									/* sys dev is 1st entry */
		return (i);
		
	if (device == 1)									/* look up cur dev      */
		type = able_core(c_contab + c_curdev);			/* e.g 0 flop, 1 winch  */
	
	else												/*  map to device       */
	{
		if (device == 2 || device == 3)					/* floppy				*/
			type = 0;
	
		else if (device == 4 || device == 5)			/* remote floppy		*/
			type = 2;
		
		else if (device == 6 || device == 7)			/* winch				*/
			type = 1;
		
		else if (device == 10 || device == 11)			/* optical				*/
			type = 4;
		
		else 											/* not found			*/
			return (0);
			
		if (device & 1)									/* look for drive 1		*/
			type += 16;
	}
	
	// Look for that device
	
	while (1)
	{
		if (i > c_contab + c_strend)					/* if past end			*/
			return (0);
			
		if ((able_core(i) & 0xFF) == type)
			return (i);
		
		i += 4;
		counter ++;
		
		if (counter >= 100)								/* in case no -1		*/
			return (0);
	}
}

fixed	set_curdev (fixed device)						// sets current device to be logical device
{
	ufixed	where = find_device(device);

	if (where == 0)										// failed if not there
		return (false);
		
	set_able_core(c_contab + c_curdev, able_core(where) & 0xFF);
	return (true);
}	

static	void	issue_writedata (fixed ms_sector, fixed ls_sector, fixed *_bufptr, uint32 length)
{
	ufixed	device = ((uint16) ms_sector) >> 8;			/* get device code		*/
	ufixed	usec   = ms_sector & 0xFF;					/* get upper sector		*/
	ufixed	ptr    = find_device(device);				/* get its entry		*/
	uint32	sec    = (((uint32) (uint16) usec) << 16) | ((uint32) (uint16) ls_sector);
	ufixed	targ;
	scsi_device *the_device;
	
    // Handle writing to "device" (e.g. an image file) that is not W0:, W1:, O0:, O1:
	if (!ptr)											/* if oops				*/
	{
		xpl_file_ref_num ref_num;
		
		if ((ref_num = find_hfs_device(device))	!= 0)	// HFS Device
		{
            xpl_result status = noErr;
		
            unsigned int num_bytes_requested = length << 1, num_bytes_copied = 0;

            status = XPLRunTime_FSWriteFork(ref_num,
                                            fsFromStart,
                                            ((long long) sec) << (8+1),
                                            num_bytes_requested,
                                            (void *)_bufptr,
                                            &num_bytes_copied);

            if ((status) || (num_bytes_copied != num_bytes_requested))
			{
				print("XPL Run Time: Writedata error (0)\n");

				#if ALLOW_THROW
					if (g_throw_on_disk_error)
						throw(e_diskerror);
				#endif

				exit(-1);
			}
			
			return;
		}
		
		print("XPL Run Time: Writedata error (1)\n");
		exit(-1);
	}
	
	targ = able_core(ptr + s_devadr);					// get RTP scsi id reference
	
	if (targ >= MAX_NUM_DEVICES)
	{
		print("XPL Run Time: Writedata error (2)\n");
		exit(-1);
	}
	
	the_device = g_indexed_device[targ];				/* pointer					*/

	if (!the_device)
	{
		print("XPL Run Time: Writedata error (3)\n");

		#if ALLOW_THROW
			if (g_throw_on_disk_error)
				throw(e_diskerror);
		#endif

		exit(-1);
	}

	/* round up as XPL did, but don't bother to zero-fill... */

	if (issue_write_extended(the_device, (byte *) _bufptr, sec,  (length + 255) >> 8))
	{
		print("XPL Run Time: Writedata error (4)\n");

		#if ALLOW_THROW
			if (g_throw_on_disk_error)
				throw(e_diskerror);
		#endif

		exit(-1);
	}
}

void	writedata (fixed ms_sector, fixed ls_sector, fixed *_bufptr, fixed dir_size)
{
	issue_writedata(ms_sector, ls_sector, _bufptr, (uint32) (uint16) dir_size);
}

scsi_device*	access_scsi_device(fixed readdata_code)
{
	ufixed		ptr = find_device(readdata_code);		/* get its entry		*/
	ufixed   	targ;
	scsi_device *the_device;

	if (!ptr)											// not a legacy device; check a mac hfs image file
	{
		the_device = find_hfs_scsi_device(readdata_code);
		
		if (the_device)
			return (the_device);
	}
	
	if (!ptr) return (NULL);							// device was not configured

	targ = able_core(ptr + s_devadr);					// access config area

	if (targ >= MAX_NUM_DEVICES) return (NULL);			// oops

	the_device = g_indexed_device[targ];				// look up device

	if (!the_device)									// not configured properly
		return (NULL);
		
	return the_device;									// that's the device...
}

scsi_settings*	access_scsi_setting(fixed readdata_code)
{
	ufixed		  ptr = find_device(readdata_code);		/* get its entry		*/
	ufixed   	  targ;
	scsi_settings *the_setting;

	if (!ptr) return (NULL);							// device was not configured

	targ = able_core(ptr + s_devadr);					// access config area

	if (targ >= MAX_NUM_DEVICES) return (NULL);			// oops

	the_setting = g_indexed_setting[targ];				// look up setting

	if (!the_setting)									// not configured properly
		return (NULL);
		
	return the_setting;									// that's the setting...
}

void	update_scsi_device_size(fixed readdata_code)
{
	ufixed		 ptr = find_device(readdata_code);		/* get its entry		*/
	ufixed   	 targ;
	scsi_device *the_device;

	if (!ptr) return;									// device was not configured

	targ = able_core(ptr + s_devadr);					// access config area

	if (targ >= MAX_NUM_DEVICES) return;				// oops

	the_device = g_indexed_device[targ];				// look up device

	if (!the_device)									// not configured properly
		return;
	
	// Update low mem config area
	set_able_core(ptr + s_seccyl, (fixed) the_device->fTotSec);	/* secs/cyl				*/
	set_able_core(ptr + s_totcyl, (fixed) the_device->fTotCyl);	/* cyls					*/

	// Update hi mem config area
	if ((able_core(loc_magic) == 12345)
	&&  (able_core(loc_ctab + (ptr - c_contab) - c_offset) == able_core(ptr)))
	{
		set_able_core(loc_ctab + (ptr - c_contab) - c_offset + s_seccyl, (fixed) the_device->fTotSec);
		set_able_core(loc_ctab + (ptr - c_contab) - c_offset + s_totcyl, (fixed) the_device->fTotCyl);
	}
}

static	void	issue_readdata (fixed ms_sector, fixed ls_sector, fixed *_bufptr, uint32 length)
{
	ufixed	device = ((uint16) ms_sector) >> 8;			/* get device code		*/
	ufixed	usec   = ms_sector & 0xFF;					/* get upper sector		*/
	ufixed	ptr    = find_device(device);				/* get its entry		*/
	uint32	sec    = (((uint32) (uint16) usec) << 16) | ((uint32) (uint16) ls_sector);
	ufixed	targ;
	scsi_device *the_device;
	fixed	buf[256];
		
    // Handle reading from "device" (e.g. an image file) that is not W0:, W1:, O0:, O1:

	if (!ptr)											/* if oops				*/
	{
		xpl_file_ref_num ref_num;
		
		if ((ref_num = find_hfs_device(device))	!= 0)	// HFS Device
		{		
            xpl_result status = noErr;
		
            unsigned int num_bytes_requested = length << 1, num_bytes_copied = 0;

            status = XPLRunTime_FSReadFork(ref_num,
                                           fsFromStart,
                                           ((long long) sec) << (8+1),
                                           num_bytes_requested,
                                           (void *) _bufptr,
                                           &num_bytes_copied);

            if ((status) || (num_bytes_copied != num_bytes_requested))
			{
				print("XPL Run Time: Readdata error (0)\n");

				#if ALLOW_THROW
					if (g_throw_on_disk_error)
						throw(e_diskerror);
				#endif

				exit(-1);
			}
			
			return;
		}
		
		print("XPL Run Time: Readdata error (1)\n");
		exit(-1);
	}
	
	targ = able_core(ptr + s_devadr);
	
	if (targ >= MAX_NUM_DEVICES)
	{
		print("XPL Run Time: Readdata error (2)\n");
		exit(-1);
	}
	
	the_device = g_indexed_device[targ];				/* pointer					*/

	if (!the_device)
	{
		print("XPL Run Time: Readdata error (3)\n");

		#if ALLOW_THROW
			if (g_throw_on_disk_error)
				throw(e_diskerror);
		#endif

		exit(-1);
	}

	if (issue_read_extended(the_device, (byte *) _bufptr, sec, length >> 8))
	{
		print("XPL Run Time: Readdata error (4)\n");
		
		#if ALLOW_THROW
			if (g_throw_on_disk_error)
				throw(e_diskerror);
		#endif

		exit(-1);
	}

	if (length & 0xFF)	/* fill in partial remainder as RTP did */
	{
		int i;
		
		if (issue_read_extended(the_device, (byte *) buf, sec + (length >> 8), 1))
		{
			print("XPL Run Time: Readdata error (5)\n");
			
			#if ALLOW_THROW
				if (g_throw_on_disk_error)
					throw(e_diskerror);
			#endif

			exit(-1);
		}
		
		for (i = 0; i < (length&0xFF); i++)
			_bufptr[(length & 0xFF) + i] = buf[i];
	}
}

void	readdata (fixed ms_sector, fixed ls_sector, fixed *_bufptr, fixed dir_size)
{
	issue_readdata(ms_sector, ls_sector, _bufptr, (uint32) (uint16) dir_size);
}

/* for extread, write:	info[0] = sector of external memory	*/
/*                      info[1] = word   of external memory	*/
/*                      info[2] = sectors to transfer		*/
/*                      info[3] = words   to transfer		*/

void	extread  (fixed ms_sector, fixed ls_sector, fixed *info)
{
	uint32 length = (((uint32) (uint16) info[2]) << 8) + ((uint32) (uint16) info[3]);
	
	_write_60(info[0] + shr(info[1],8));
	_write_61(info[1]);

	issue_readdata(ms_sector, ls_sector, &(ABLE_CONTEXT.d60[ABLE_CONTEXT._able_xmem_addr]), length);
}

void	extwrite (fixed ms_sector, fixed ls_sector, fixed *info)
{
	uint32 length = (((uint32) (uint16) info[2]) << 8) + ((uint32) (uint16) info[3]);
	
	_write_60(info[0] + shr(info[1],8));
	_write_61(info[1]);

	issue_writedata(ms_sector, ls_sector, &(ABLE_CONTEXT.d60[ABLE_CONTEXT._able_xmem_addr]), length);
}

fixed disk_check (fixed device)			/* check floppy disk							*/
{
	if (find_hfs_device(device))
		return (TRUE);
	else if (find_device(device))		/* if device there, we are ready (at least		*/
		return (TRUE);					/* for now, the only thing there is a winch!	*/
	else								/* also could check for write protect			*/
		return (FALSE);
}


// XPLRunTime_SetupSCSIMap()

// This routine looks at the InterChange setup settings, and sets up the XPL_scsi_code_map so
// that we know how to access the chosen device (image file, syncl scsi bus, mac scsi bus, etc.).

// Note: It does not interrogate the device or configure them into the device list.  That step
// is saved until the device is actually accessed.  It also does not set up the high memory area
// of the run time environment (see next routine).

void	XPLRunTime_SetupSCSIMap(interchange_settings *interchangeSettings,  int d24Avail, int t0Config, scsi_settings* t0Settings)
{
	zero_mem((byte *) &XPL_scsi_code_map, sizeof(XPL_scsi_code_map));					// init
	
	if (!g_scsi_device_data_base)														// no managers available
		return;

	if (((interchangeSettings->w0.bus_id     == SCSI_BUS_MENU_DISK_IMAGE   )			// if device is a disk image
	&&   (interchangeSettings->w0.image_file != 0						   ))			// and alias exists
	||  ( interchangeSettings->w0.bus_id     == SCSI_BUS_MENU_MAC_SCSI_PORT )			// or is mac scsi port
	||  ((interchangeSettings->w0.bus_id     == SCSI_BUS_MENU_D24           )			// or is device on board 0
	&&   (d24Avail                           != 0                           )))			// and we have the hardware available
	{
		XPL_scsi_code_map.board_list[0].target_list[ABLE_W0_DEFAULT_SCSI_ID].entry_avail    = true;
		XPL_scsi_code_map.board_list[0].target_list[ABLE_W0_DEFAULT_SCSI_ID].device_manager = &g_scsi_device_data_base[ABLE_W0_DEFAULT_SCSI_ID];
		XPL_scsi_code_map.board_list[0].target_list[ABLE_W0_DEFAULT_SCSI_ID].device_setup   = &interchangeSettings->w0;
		zero_mem((byte *) &g_scsi_device_data_base[ABLE_W0_DEFAULT_SCSI_ID], sizeof(g_scsi_device_data_base[ABLE_W0_DEFAULT_SCSI_ID]));

		if (interchangeSettings->w0.bus_id == SCSI_BUS_MENU_D24)
			XPL_scsi_code_map.board_list[0].target_list[ABLE_W0_DEFAULT_SCSI_ID].use_d24 = true;
		else
			XPL_scsi_code_map.board_list[0].target_list[ABLE_W0_DEFAULT_SCSI_ID].use_d24 = false;
	}

	if (((interchangeSettings->w1.bus_id     == SCSI_BUS_MENU_DISK_IMAGE   )			// if device is a disk image
	&&   (interchangeSettings->w1.image_file != 0						   ))			// and alias exists
	||  ( interchangeSettings->w1.bus_id     == SCSI_BUS_MENU_MAC_SCSI_PORT )			// or is mac scsi port
	||  ((interchangeSettings->w1.bus_id     == SCSI_BUS_MENU_D24           )			// or is device on board 0
	&&   (d24Avail							!= 0                            )))			// and we have the hardware available
	{
		XPL_scsi_code_map.board_list[0].target_list[ABLE_W1_DEFAULT_SCSI_ID].entry_avail    = true;
		XPL_scsi_code_map.board_list[0].target_list[ABLE_W1_DEFAULT_SCSI_ID].device_manager = &g_scsi_device_data_base[ABLE_W1_DEFAULT_SCSI_ID];
		XPL_scsi_code_map.board_list[0].target_list[ABLE_W1_DEFAULT_SCSI_ID].device_setup   = &interchangeSettings->w1;
		zero_mem((byte *) &g_scsi_device_data_base[ABLE_W1_DEFAULT_SCSI_ID], sizeof(g_scsi_device_data_base[ABLE_W1_DEFAULT_SCSI_ID]));

		if (interchangeSettings->w1.bus_id == SCSI_BUS_MENU_D24)
			XPL_scsi_code_map.board_list[0].target_list[ABLE_W1_DEFAULT_SCSI_ID].use_d24 = true;
		else
			XPL_scsi_code_map.board_list[0].target_list[ABLE_W1_DEFAULT_SCSI_ID].use_d24 = false;
	}

	if (((interchangeSettings->o0.bus_id     == SCSI_BUS_MENU_DISK_IMAGE   )			// if device is a disk image
	&&   (interchangeSettings->o0.image_file != 0						   ))			// and alias exists
	||  ( interchangeSettings->o0.bus_id     == SCSI_BUS_MENU_MAC_SCSI_PORT )			// or is mac scsi port
	||  ((interchangeSettings->o0.bus_id     == SCSI_BUS_MENU_D24           )			// or is device on board 0
	&&   (d24Avail							!= 0                            )))			// and we have the hardware available
	{
		XPL_scsi_code_map.board_list[0].target_list[ABLE_O0_DEFAULT_SCSI_ID].entry_avail    = true;
		XPL_scsi_code_map.board_list[0].target_list[ABLE_O0_DEFAULT_SCSI_ID].device_manager = &g_scsi_device_data_base[ABLE_O0_DEFAULT_SCSI_ID];
		XPL_scsi_code_map.board_list[0].target_list[ABLE_O0_DEFAULT_SCSI_ID].device_setup   = &interchangeSettings->o0;
		zero_mem((byte *) &g_scsi_device_data_base[ABLE_O0_DEFAULT_SCSI_ID], sizeof(g_scsi_device_data_base[ABLE_O0_DEFAULT_SCSI_ID]));

		if (interchangeSettings->o0.bus_id == SCSI_BUS_MENU_D24)
			XPL_scsi_code_map.board_list[0].target_list[ABLE_O0_DEFAULT_SCSI_ID].use_d24 = true;
		else
			XPL_scsi_code_map.board_list[0].target_list[ABLE_O0_DEFAULT_SCSI_ID].use_d24 = false;
	}

	if (((interchangeSettings->o1.bus_id     == SCSI_BUS_MENU_DISK_IMAGE   )			// if device is a disk image
	&&   (interchangeSettings->o1.image_file != 0						   ))			// and alias exists
	||  ( interchangeSettings->o1.bus_id     == SCSI_BUS_MENU_MAC_SCSI_PORT )			// or is mac scsi port
	||  ((interchangeSettings->o1.bus_id     == SCSI_BUS_MENU_D24           )			// or is device on board 0
	&&   (d24Avail							!= 0                            )))			// and we have the hardware available
	{
		XPL_scsi_code_map.board_list[0].target_list[ABLE_O1_DEFAULT_SCSI_ID].entry_avail    = true;
		XPL_scsi_code_map.board_list[0].target_list[ABLE_O1_DEFAULT_SCSI_ID].device_manager = &g_scsi_device_data_base[ABLE_O1_DEFAULT_SCSI_ID];
		XPL_scsi_code_map.board_list[0].target_list[ABLE_O1_DEFAULT_SCSI_ID].device_setup   = &interchangeSettings->o1;
		zero_mem((byte *) &g_scsi_device_data_base[ABLE_O1_DEFAULT_SCSI_ID], sizeof(g_scsi_device_data_base[ABLE_O1_DEFAULT_SCSI_ID]));

		if (interchangeSettings->o1.bus_id == SCSI_BUS_MENU_D24)
			XPL_scsi_code_map.board_list[0].target_list[ABLE_O1_DEFAULT_SCSI_ID].use_d24 = true;
		else
			XPL_scsi_code_map.board_list[0].target_list[ABLE_O1_DEFAULT_SCSI_ID].use_d24 = false;
	}

    // Configure SCSI tape
    if (t0Config == SCSI_BUS_MENU_D24) // SCSI tape
    {
        XPL_scsi_code_map.board_list[0].target_list[ABLE_T0_DEFAULT_SCSI_ID].entry_avail    = true;
        XPL_scsi_code_map.board_list[0].target_list[ABLE_T0_DEFAULT_SCSI_ID].device_manager = &g_scsi_device_data_base[ABLE_T0_DEFAULT_SCSI_ID];
        XPL_scsi_code_map.board_list[0].target_list[ABLE_T0_DEFAULT_SCSI_ID].device_setup   = t0Settings;
        zero_mem((byte *) &g_scsi_device_data_base[ABLE_T0_DEFAULT_SCSI_ID], sizeof(g_scsi_device_data_base[ABLE_T0_DEFAULT_SCSI_ID]));

        if (t0Config == SCSI_BUS_MENU_D24)
            XPL_scsi_code_map.board_list[0].target_list[ABLE_T0_DEFAULT_SCSI_ID].use_d24 = true;
        else
            XPL_scsi_code_map.board_list[0].target_list[ABLE_T0_DEFAULT_SCSI_ID].use_d24 = false;
    }
}


// XPLRunTime_ConfigureSCSIMap()

// This routine takes the SCSI Map (which previously has been set up by XPLRunTime_SetupSCSIMap) and enters all
// the legit devices into the run time environment's device list

// It is triggered by MONITOR shortly of MONITOR is loaded into memory via WINBOOT

void	XPLRunTime_ConfigureSCSIMap(fixed floppy_available, fixed d66_available, int t0Config)
{
    int         i;
	scsi_device *the_device;
	
	if (!g_able_context)								// make sure inited
		return;
	
    // Stuff configurable things
    i = c_contab + c_cmopt;                             // options word
    
    set_able_core(i, able_core(i) | o_d44);             // Mouse pesent
    
    if (d66_available)
        set_able_core(i, able_core(i) | o_d66);         // D66 pesent

    #if SYNC_USE_KERNEL
        if (sync_prefs_grab_one_pref(SYNC_PREF_REAL_TIME_MONO_VOICES))
            set_able_core(i, able_core(i) | o_pvoice);  // Mono voices
    #endif
    
    i = c_contab + c_strdev;                            // Add devices here - put W0 at first entry in storage area
    
	if (XPL_scsi_code_map.board_list[0].target_list[ABLE_W0_DEFAULT_SCSI_ID].entry_avail)
	{
		the_device = XPL_scsi_code_map.board_list[0].target_list[ABLE_W0_DEFAULT_SCSI_ID].device_manager;
	
		set_able_core(i + s_devtyp, 0x0101);						/* w0, scsi				*/
		set_able_core(i + s_seccyl, (fixed) the_device->fTotSec);	/* secs/cyl				*/
		set_able_core(i + s_totcyl, (fixed) the_device->fTotCyl);	/* cyls					*/
		set_able_core(i + s_devadr, ABLE_W0_DEFAULT_SCSI_ID	   );	/* target				*/
		
		i += 4;
	}

	if (XPL_scsi_code_map.board_list[0].target_list[ABLE_W1_DEFAULT_SCSI_ID].entry_avail)
	{
		the_device = XPL_scsi_code_map.board_list[0].target_list[ABLE_W1_DEFAULT_SCSI_ID].device_manager;
	
		set_able_core(i + s_devtyp, 0x0111);						/* w1, scsi				*/
		set_able_core(i + s_seccyl, (fixed) the_device->fTotSec);	/* secs/cyl				*/
		set_able_core(i + s_totcyl, (fixed) the_device->fTotCyl);	/* cyls					*/
		set_able_core(i + s_devadr, ABLE_W1_DEFAULT_SCSI_ID	   );	/* target				*/
		
		i += 4;
	}

	if (XPL_scsi_code_map.board_list[0].target_list[ABLE_O0_DEFAULT_SCSI_ID].entry_avail)
	{
		the_device = XPL_scsi_code_map.board_list[0].target_list[ABLE_O0_DEFAULT_SCSI_ID].device_manager;
	
		set_able_core(i + s_devtyp, 0x0104);						/* o0, scsi				*/
		set_able_core(i + s_seccyl, (fixed) the_device->fTotSec);	/* secs/cyl				*/
		set_able_core(i + s_totcyl, (fixed) the_device->fTotCyl);	/* cyls					*/
		set_able_core(i + s_devadr, ABLE_O0_DEFAULT_SCSI_ID	   );	/* target				*/
		
		i += 4;
	}

	if (XPL_scsi_code_map.board_list[0].target_list[ABLE_O1_DEFAULT_SCSI_ID].entry_avail)
	{
		the_device = XPL_scsi_code_map.board_list[0].target_list[ABLE_O1_DEFAULT_SCSI_ID].device_manager;
	
		set_able_core(i + s_devtyp, 0x0114);						/* o1, scsi				*/
		set_able_core(i + s_seccyl, (fixed) the_device->fTotSec);	/* secs/cyl				*/
		set_able_core(i + s_totcyl, (fixed) the_device->fTotCyl);	/* cyls					*/
		set_able_core(i + s_devadr, ABLE_O1_DEFAULT_SCSI_ID	   );	/* target				*/
		
		i += 4;
	}

	if (floppy_available)											// d 100 available
	{
		//dcl smin_config data public (shl("5", 8), 30, 80, shl(15, 8) or 0); /* super mini config */

		set_able_core(i + s_devtyp, 0x0500);						// f0
		set_able_core(i + s_seccyl,     30);						// secs/cyl				*/
		set_able_core(i + s_totcyl,     80);						// cyls					*/
		set_able_core(i + s_devadr, 0x0F00);
		
		i += 4;
	}
    
    // SCSI tape
    // do; /* 1: Kennedy configuration */
    //    tape_config (s#devtyp) = 0;   /* kennedy drive */
    //    tape_config (s#seccyl) = 464; /* blocks / track */
    //    tape_config (s#totcyl) = 4;   /* number of tracks */
    //    tape_config (s#devadr) = 0;   /* reserved */
    //    call assign_conf (dev, tape_config); /* Kennedy 1/4" cartridge */
    // end;
    // do; /* 2: SCSI configuration */
    //    tape_config (s#devtyp) = shl(1,8); /* SCSI drive */
    //    tape_config (s#seccyl) = 17024; /* blocks / track */
    //    tape_config (s#totcyl) = 1;   /* number of tracks */
    //    tape_config (s#devadr) = get_info(dev,0,i#scsi_adr); /* SCSI device address */
    //    call assign_conf (dev, tape_config); /* SCSI 1/2" reel */
    // end;
    
    // i = find_device (device); /* look up device */

    // if ((i <> 0) and ((core(i + s#devtyp) and "17") = 3)) then do; /* if a tape drive */
    //    tape_device = device; /* save last operation's device number */
    //    device = 8 + (shr(core(i + s#devtyp), 4) and "17"); /* map to actual device number */
    //    drive (device) = (device and 1); /* save drive number */
    //    if (core(i + s#devtyp) and "377") = system_device  then drive (0) = drive (device); /* remember it if it's the system device */
    //    if (core(i + s#devtyp) and "377") = current_device then drive (1) = drive (device); /* or the current device (this assumes another device won't be ENTERed while the tape is loaded!!!) */
    //    type (drive (device)) = (shr(core(i + s#devtyp), 8) and "3"); /* pick up drive type */
    //
    //    if type (drive (device)) = 0 /* if Kennedy */
    //    then call k#load;
    //    else call s#load (device); /* must be SCSI */
    //
    //    tape_position (drive (device)) = 0; /* assume it succeeded */
    // end; /* of tape drive */

    if (XPL_scsi_code_map.board_list[0].target_list[ABLE_T0_DEFAULT_SCSI_ID].entry_avail)
    {
        set_able_core(i + s_devtyp, 0x0103);                    /* SCSI drive       */
        set_able_core(i + s_seccyl, 17024);                     /* blocks / track   */
        set_able_core(i + s_totcyl, 1);                         /* number of tracks */
        set_able_core(i + s_devadr, ABLE_T0_DEFAULT_SCSI_ID);   /* target           */
        
        i += 4;
    }
    
    else if (t0Config == SCSI_BUS_MENU_D30){
        set_able_core(i + s_devtyp, 0x0003);                    /* kennedy drive    */
        set_able_core(i + s_seccyl, 464);                       /* blocks / track   */
        set_able_core(i + s_totcyl, 4);                         /* number of tracks */
        set_able_core(i + s_devadr, 0);                         /* reserved         */
        
        i += 4;
    }

	while (i < c_contab + c_strend)
		set_able_core(i++, -1);
}


// XPLRunTime_UpdateHiMemConfig()

void	XPLRunTime_UpdateHiMemConfig()
{
	int  i = c_contab + c_strdev;
	int  j = loc_ctab;

	if (able_core(loc_magic) != 12345)
		return;

	while (i < c_contab + c_strend)
	{
		set_able_core(j + (i - c_contab) - c_offset, able_core(i));
		i++;
	}
}


// XPLRunTime_InterrogateDevice()

// This routine performs an interrogation of a particular device to find out if it's available.  In two parts.

xpl_result   (*XPLRunTime_FileOpener) (xpl_file_ref inFileSpec, char permission, xpl_file_ref_num* refNum) = NULL;
xpl_result   (*XPLRunTime_FileCloser) (xpl_file_ref_num refNum)                                            = NULL;

MAC_SCSI_Target*	XPLRunTime_InterrogateDevice(MAC_SCSI_Target *the_target)
{
	scsi_device 	*the_device = the_target->device_manager;
	scsi_settings	*the_setup  = the_target->device_setup;

	if (!the_device || !the_setup)
		return (NULL);
		
	if ((the_device->fDeviceType == DEVICE_NOT_EXAMINED      )							// if device not yet examined
	||  (the_device->fDeviceType == DEVICE_DOES_NOT_EXIST    )							// or was not powered up last time we looked at it
	||  (the_device->fDeviceType == DEVICE_UNCOOPERATIVE_DISK)							// or had no media in it last time we checked...
	||  (the_device->fUnitAttention))													// or unit attention condition occured
	{
		if (the_setup->bus_id == SCSI_BUS_MENU_DISK_IMAGE)								// if is a disk image file
		{
			if (!the_target->ref_num)													// if image file not opened
			{
                // Here we open the disk image file based upon the information in the SCSI Setup
                // The SCSI Setup has been read in from disk.
                #if __LP64__
                    CSynclavierFileReference* fileRef = CSynclavierFileReference::CopyFromFSSpec(&the_setup->image_spec);
                
                    fileRef->Open(O_RDWR);
                
                    // Try for read only
                    if (fileRef->GetFile() == 0)
                        fileRef->Open(O_RDONLY);
                
                    if (fileRef->GetFile() == 0) {
                        fileRef->Release();
                        return NULL;
                    }
                
                    the_target->ref_num = fileRef->GetFile();
                
                    fileRef->Release();
                #else
                    OSErr FSstatus;
				
                    if (XPLRunTime_FileOpener)                                              // Open shared file if opening from InterChange
                        FSstatus = XPLRunTime_FileOpener(&the_setup->image_spec, fsRdWrShPerm, &the_target->ref_num);
                        
                    else
                    {
                        //	We need an FSRef for 64-bit Carbon File I/O routines
                        FSRef theFSRef;
                        FSstatus = FSpMakeFSRef(&the_setup->image_spec, &theFSRef);

                        if (FSstatus == noErr)
                        {
                            FSstatus = FSOpenFork
                            (	&theFSRef,				//	const FSRef *		ref,
                                0,						//	UniCharCount		forkNameLength,
                                NULL,					//	const UniChar *	forkName,			//	NULL implies the data fork
                                fsRdWrShPerm,			//	SInt8					permissions,
                                &the_target->ref_num	//	SInt16 *				forkRefNum
                            );
                        }
                    }
                    
                    if (FSstatus)
                    {
                        the_device->fDeviceType = DEVICE_DOES_NOT_EXIST;
                        return NULL;
                    }
                
                    the_device->fVRefNum = the_setup->image_spec.vRefNum;
                #endif
                
                the_device->fFRefNum = the_target->ref_num;

                SyncFSSpecRelease(&the_device->fFSSpec  );
                SyncFSSpecRetain (&the_setup->image_spec);
                
                the_device->fFSSpec = the_setup->image_spec;
 			}
			
			the_device->fDevicePort      = SIMULATED_PORT;
			the_device->fIdentifyMessage = IDENTIFY_NO_DISC;
			the_device->fTargetId        = the_setup->sim_id;
		}
		
		else if (the_setup->bus_id == SCSI_BUS_MENU_MAC_SCSI_PORT)
		{
			the_device->fDevicePort      = MAC_SCSI_PORT;
			the_device->fIdentifyMessage = IDENTIFY_NO_DISC;
			the_device->fTargetId        = the_setup->scsi_id;
		}
		
		else if (the_setup->bus_id == SCSI_BUS_MENU_D24)
		{
			the_device->fDevicePort      = D24_SCSI_PORT;
			the_device->fIdentifyMessage = IDENTIFY_NO_DISC;
			the_device->fTargetId        = the_setup->scsi_id;
		}
		
		else
		{
			the_device->fDeviceType = DEVICE_DOES_NOT_EXIST;
			return NULL;
		}
		
		// Publish device pointer for use by readdata/writedata
		if (the_setup->sim_id < MAX_NUM_DEVICES)
		{
			g_indexed_device [the_setup->sim_id] = the_device;
			g_indexed_setting[the_setup->sim_id] = the_setup;
		}
		
		interrogate_device(the_device);
		
		the_device->fUnitAttention = FALSE;
	}
	
	return (the_target);
}

void XPLRunTime_RemoveDevice(struct MAC_SCSI_Target *the_target) {
    scsi_device 	*the_device = the_target->device_manager;
    scsi_settings	*the_setup  = the_target->device_setup;
    
    if (!the_device || !the_setup)
        return;
    
    if (the_setup->sim_id < MAX_NUM_DEVICES)
    {
        g_indexed_device [the_setup->sim_id] = NULL;
        g_indexed_setting[the_setup->sim_id] = NULL;
    }
}

scsi_device*	XPLRunTime_LookUpDevicePointer(int theBoard, int theScsiID)
{
	MAC_SCSI_Target *the_target = NULL;

	// Find the MAC_SCSI_Target entry.
	if (((theBoard  >= 0) && (theBoard  < NUM_BOARDS_IN_BOARD_LIST  ))
	&&  ((theScsiID >= 0) && (theScsiID < NUM_TARGETS_IN_TARGET_LIST)))
		the_target = &XPL_scsi_code_map.board_list[theBoard].target_list[theScsiID];

	return (the_target ? the_target->device_manager : NULL);
}

scsi_settings*	XPLRunTime_LookUpDeviceSettings(int theBoard, int theScsiID)
{
	MAC_SCSI_Target *the_target = NULL;

	// Find the MAC_SCSI_Target entry.
	if (((theBoard  >= 0) && (theBoard  < NUM_BOARDS_IN_BOARD_LIST  ))
	&&  ((theScsiID >= 0) && (theScsiID < NUM_TARGETS_IN_TARGET_LIST)))
		the_target = &XPL_scsi_code_map.board_list[theBoard].target_list[theScsiID];

	return (the_target ? the_target->device_setup : NULL);
}

MAC_SCSI_Target*	XPLRunTime_ExamineDevice(int theBoard, int theScsiID)
{
	MAC_SCSI_Target *the_target = NULL;

	// Find the MAC_SCSI_Target entry.
	if (((theBoard  >= 0) && (theBoard  < NUM_BOARDS_IN_BOARD_LIST  ))
	&&  ((theScsiID >= 0) && (theScsiID < NUM_TARGETS_IN_TARGET_LIST)))
		the_target = &XPL_scsi_code_map.board_list[theBoard].target_list[theScsiID];

	else
		return (NULL);

	return (XPLRunTime_InterrogateDevice(the_target));
}


// XPLRunTime_CloseupSCSIMap()

// Handy routine to close any open image files and otherwise prepare for termination

void XPLRunTime_CloseupSCSIMap()
{
	int i,j;
	
	for (i=0; i<NUM_BOARDS_IN_BOARD_LIST; i++)
	{
		for (j=0; j<NUM_TARGETS_IN_TARGET_LIST; j++)
		{
			if (XPL_scsi_code_map.board_list[i].target_list[j].entry_avail)
			{
                if (XPL_scsi_code_map.board_list[i].target_list[j].device_manager)
                {
                    scsi_device& dm = *XPL_scsi_code_map.board_list[i].target_list[j].device_manager;
                    
                    if (dm.fFRefNum)
                    {
                        #if __LP64__
                            CSynclavierFileReference* fileRef = CSynclavierFileReference::CopyFromFSSpec(&dm.fFSSpec);
                            
                            fileRef->Close();
     
                            fileRef->Release();
                        #else
                            if (XPLRunTime_FileCloser)
                                XPLRunTime_FileCloser(XPL_scsi_code_map.board_list[i].target_list[j].ref_num);
                            else
                                FSCloseFork(XPL_scsi_code_map.board_list[i].target_list[j].ref_num);
                        #endif
                        dm.fFRefNum = 0;
                        
                        XPL_scsi_code_map.board_list[i].target_list[j].ref_num = 0;
                    }
                    
                    SyncFSSpecRelease(&dm.fFSSpec);
                    
                    // Close up segments
                    if (dm.fSegManager)
                    {
                        SyncMutexWaiter   waiter(SyncScsiSegmentizerMutex);

                        // Definitely won't go away while waiting for the mutex, but
                        // check anyways
                        if (dm.fSegManager) {
                            scsi_segmentizer& izer = * (scsi_segmentizer*) dm.fSegManager;
                            
                            for (int i=0; i<izer.fNumSegments; i++) {
                                scsi_segment& seg = izer.fSegList[i];
                                
                                if (seg.fSegFileRef)
                                    {seg.fSegFileRef->Release(); seg.fSegFileRef = NULL;}
                                
                                if (seg.fSegURLRef)
                                    {CFRelease(seg.fSegURLRef); seg.fSegURLRef = NULL;}
                                
                                if (seg.fSegStash) {
                                    if (--seg.fSegStash->stashRetains == 0)
                                        free(seg.fSegStash);
                                    
                                    seg.fSegStash = NULL;
                                }
                            }
                            
                            delete dm.fSegManager;
                        }
                        
                        dm.fSegManager = NULL;
                    }
                }
            
				XPL_scsi_code_map.board_list[i].target_list[j].entry_avail = 0;
			}
		}
	}
    
    // Close any BSD scsi files
    finalize_mac_scsi_port_scsi_chip();
}


// XPLRunTime_AssignHFSReaddataCode, XPLRunTime_FreeHFSReaddataCode

// Set up tables to access mac-resident image files other than W0, W1, O0, O1
// This routine is passed in a file ref num for an open file. It is also passed in
// a (FSSpec* or SyncFSSpec*) that identifies the file. It determines what
// XPL "Device Code" to use to read/write that image file

// Note: only used from InterChange; nost used SynclavierX

ufixed	XPLRunTime_AssignHFSReaddataCode (xpl_file_ref_num  inFileRefNum, xpl_file_ref inFileSpec)
{
	int i;
	
	if (inFileRefNum == 0)											// bogus
		return 0;
		
	// See if already in use
	for (i=ABLE_HFS_READDATA_CODE; i<256; i++)
		if (assigned_hfs_rnum[i] == inFileRefNum)
		{
			assigned_ref_count[i]++;
			return (i);
		}
		
	// Look for free
	for (i=ABLE_HFS_READDATA_CODE; i<256; i++)
		if (assigned_hfs_rnum[i] == 0)
			break;
			
	if (i >= 256)													// out of room
		return (0);

	// Create a scsi_device to access this device
	scsi_device* its_device = (scsi_device*) malloc(sizeof(scsi_device  ));
	xpl_file_ref its_spec   = (xpl_file_ref) malloc(sizeof(*its_spec    ));

	if (!its_device || !its_spec)
	{
		if (its_device) free (its_device);
		if (its_spec  ) free (its_spec  );
		
		return (0);
	}
	
	memset(its_device, 0, sizeof(*its_device));						// init scsi struct
	memset(its_spec,   0, sizeof(*its_spec  ));						// init spec struct
	
	its_device->fDevicePort      = SIMULATED_PORT;
	its_device->fDeviceType      = DEVICE_NOT_EXAMINED;
	its_device->fIdentifyMessage = IDENTIFY_NO_DISC;
	its_device->fTargetId        = 7;
	its_device->fFRefNum         = inFileRefNum;
    
    // The scsi_device holds a reference to the SyncFSSpec
    SyncFSSpecRetain (inFileSpec);
    SyncFSSpecRelease(&its_device->fFSSpec);
    
	its_device->fFSSpec = *inFileSpec;
	
    #if !__LP64__
        GetVRefNum(inFileRefNum, &its_device->fVRefNum);
    #endif
    
	interrogate_device(its_device);

    // The assigned_spec holds a reference to the SyncFSSpec
    SyncFSSpecRetain (inFileSpec);
    SyncFSSpecRelease(its_spec  );  // Won't do very much; it's all zeroes...

	*its_spec = *inFileSpec;
	
	assigned_hfs_rnum [i] = inFileRefNum;
	assigned_ref_count[i] = 1;
	assigned_device   [i] = its_device;
	assigned_spec     [i] = its_spec;
	
	return (i);
}

void XPLRunTime_FreeHFSReaddataCode (ufixed inReadDataCode)
{
	if (inReadDataCode < ABLE_HFS_READDATA_CODE || inReadDataCode >= 256)
		return;
		
	if (assigned_ref_count[inReadDataCode] == 0)
		return;

	assigned_ref_count[inReadDataCode]--;

	if (assigned_ref_count[inReadDataCode] == 0)
	{
		assigned_hfs_rnum[inReadDataCode] = 0;
		
		if (assigned_device[inReadDataCode])
		{
			free (assigned_device[inReadDataCode]);
			assigned_device[inReadDataCode] = NULL;
		}

		if (assigned_spec[inReadDataCode])
		{
            SyncFSSpecRelease(assigned_spec[inReadDataCode]);
			free (assigned_spec[inReadDataCode]);
			assigned_spec[inReadDataCode] = NULL;
		}
	}
}

// ---------------------------------------------------------------------------
//	¥ XPLRunTime_LatchHFSReaddataCode
// ---------------------------------------------------------------------------

// Given a readdata code that is allready activated, increment it's usage count
// to keep it open.

void XPLRunTime_LatchHFSReaddataCode (ufixed inReadDataCode)
{
	if (inReadDataCode < ABLE_HFS_READDATA_CODE || inReadDataCode >= 256)
		return;
		
	if (assigned_ref_count[inReadDataCode] == 0)
		return;

	assigned_ref_count[inReadDataCode]++;
}


// See if a file spec is in use as an added HFS device
ufixed XPLRunTime_LookForFSSpecInUse(const xpl_file_ref inFileSpec)
{
	int i;
	
	// See if in use
	for (i=ABLE_HFS_READDATA_CODE; i<256; i++)
	{
		if (assigned_spec[i])
            if (InterChangeLibEqualFSSpecs(inFileSpec, assigned_spec[i]))
                return i;
	}
	
	return (0);
}
