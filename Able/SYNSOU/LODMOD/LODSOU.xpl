/* :SYNSOU:LODMOD:LODSOU  $TITLE  DIRECT TO DISK MODULE

   Modified:
   08/04/00 - cj  - changed RPC usage to 32 vs. 256 for better interrupt response time
   04/27/94 - pf  - move LOD to SCSI ID 3
   02/27/94 - pf  - use new DTDSCSICONNECT to connect with DTD (no IDENTIFY)
   02/12/94 - pf  - use SCSICONNECT to connect with DTD
   02/12/94 - pf  - insert -XPL:SCSIROUT for SCSICONNECT
   02/27/92 - cj  - Accepted string in C format from DSP option
   01/15/92 - cj  - Added DTD/DSP sync mode control, new scrub/trig control
   12/06/91 - PF  - Split LOD-DTD into 2 files
   11/18/91 - PF  - More quick update support
   09/26/91 - cj  - Send env info to DSP during TRIG
   08/09/91 - cj  - Send editview xfade stuff to NUARCH
   08/09/91 - cj  - fixed length bug with dsp time base adjust message
   08/08/91 - cj  - Fix "can't scrub poly with no DTD bug"
   06/26/91 - cj  - initialize d34 during startup for better GPI performance
   01/21/91 - cj  - handled better timeout for NU-ARCH DTD
   11/04/90 - CJ  - ADDED SOME EXTERNALS
   10/15/90 - cj  - fixed "click" problem with DTD audition
   07/23/90 - cj  - Created version easily switched between 2.4 and 3.0.
                    Loads both a DSP and a DTD using two scsi ports.
   02/08/90 - cj  - add Scsi Protocol format for 53C90 and Protocol message
                    for file name boot.  Added message 231 for file name.
                    Redefined usage of lod.loading.  Send out file length
                    (in bytes) in 200 message.
   01/25/90 - MWH - Fix "300 msec delay starting cues" bug (from EditView)
   10/13/89 - cj  - set new.dtd.info & 32 when dtd signs on
   05/02/89 - TSS - Merged PLAY.REAL.TIME.CUE() routine
   04/26/89 - cj  - code to send event info to dtd in real time
                  - moved code to lodsoua
   04/20/89 - cj  - added new cue allocation for event triggering
   04/04/89 - cj  - added Check.For.SCSI.Reselect, command 179 for scsi to
                    poly transfer. Send.To.Lod.Subroutine public.
   03/13/89 - MWH - Declare event whenever number of DTD tracks changes
   02/01/89 - CJ  - bug fix with NEW SEQ INFO when DTD turns OFF
   12/20/88 - cj  - changed use of ddt.xfer.mode for Multi-Track DDT
                  - passed DDT config info from lod to synclav (ddt.config,
                    ddt.avail.trks)
   01/17/89 - MWH - Declare event when DTD goes up or down
   12/16/88 - MWH - Remove PROTOCOL DCL file (no longer needed)
   12/10/88 - cj  - fixed 'first cue will not trigger when start is pressed
                    twice' bug by adding send.all.cue.info
   12/02/88 - CJ  - fixed smpte/dtd 'wow & flutter' but (smpte.rate.accum)
   10/19/88 - TSS - insert MOUSRDCL
   10/19/88 - TSS - took protocol scanning out of MAIN.LOOP
   10/15/88 - cj  - different usage for new.sequencer.forward.motion
                    since poly notes are started up now.
   10/13/88 - SJS - added DSP message code
   09/25/88 - tss - Put code to scan protocol packets in MAIN.LOOP
   07/26/88 - cj  - fixed bug in allocate.lod.cues
   06/30/88 - TSS - Changed INC.D34.REMOTE.CONTROL.CODE literal to D34GPI.THERE run-time variable
   06/15/88 - cj  - finished up bounce and digital transfer
   05/17/88 - cj  - add dtd.xfer.mode
   04/19/88 - CJ  - Added code to Load over Swap file
   04/15/88 - CJ  - Send live track routing to dtd
   03/18/88 - MWH - Allow general transfer to external memory
   03/12/88 - cj  - added dtd.num.voices in initialization message
   03/11/88 - TS  - KEEP LOD CODE FROM POLLING D24 #3 (0-3) IF DSP BOARD COMPILED IN
   12/12/87 - CJ  - ADDED SPECIAL REMOTE I/O CODE
   11/12/87 - MWH - Make timer comparisons unsigned (taken from CJ's sources)
   
*/

MODULE LODMOD;

DCL ID.OF.THIS.VERSION LIT '8';  /* REV # OF THIS SYNCLAVIER SOFTWARE */

INSERT ':SYNLITS:COMLITS';   /* ALL MODULES MUST INSERT COMLITS    */
INSERT ':SYNLITS:GLOBLITS';  /* PRACTICALLY ALL WILL WANT GLOBLITS */
INSERT ':-XPL:SCSIROUT';     /* FOR SCSICONNECT */
INSERT ':-XPL:INTRPRTR';

INSERT ':SYNLITS:THDLITS';
INSERT ':SYNLITS:TIMLITS';
INSERT ':SYNLITS:PRMLITS';
INSERT ':SYNLITS:FCODLITS';

INSERT ':SYNAUXS:ERRLITS';   /* LITERAL FOR ERR.LOD                */
INSERT ':SYNAUXS:LODLITS';   /* LITERAL DEFINITIONS                */
INSERT ':SYNAUXS:DSPLITS';   /* LITERAL DEFS FOR DSP STUFF         */
INSERT ':SYNAUXS:LEDITLIT';  /* LITERAL FOR REAL EDIT ROUTINES     */
INSERT ':SYNAUXS:DTDLITS';

INSERT ':SYNMODS:GLOBDCL';   /* FOR MAM/MAL                        */
INSERT ':SYNMODS:GETVDCL';   /* FOR NEW DTD INFO                   */
INSERT ':SYNMODS:GETDCL';    /* FOR GET.ITEM.ENABLED               */
INSERT ':SYNMODS:XMEMDCL';   /* FOR EXT READDATA                   */
INSERT ':SYNMODS:FILEDCL';   /* FOR FILE.SEARCH                    */
INSERT ':SYNMODS:TTYDCL';    /* FOR RUN.SYN                        */
INSERT ':SYNMODS:MATHDCL';   /* FOR ADD16                          */
INSERT ':SYNMODS:TFORMDCL';  /* FOR COPY.STRING                    */
INSERT ':SYNMODS:MOUSRDCL';  /* FOR MOUSE.PORT.IS.D50              */
INSERT ':SYNMODS:SEQDCL';    /* FOR EVENT STUFF                    */
INSERT ':SYNMODS:GIDDCL';    /* FOR GID/PID VARIABLES              */
INSERT ':SYNMODS:POLYDCL';   /* FOR SCSI POLY READ/WRITE           */
INSERT ':-XPL:CATSWAP';      /* FOR MS_SECTOR (ETC)                */

dcl real.time.loop               proc         external;
dcl display.track.buttons        proc         external;
dcl erase.track                  proc (fixed) external;
dcl smpte.onoff                  fixed external;
dcl time.base.mode               fixed external;
dcl new.sequencer.forward.motion fixed external;
dcl play.time.msb                fixed external;
dcl play.time.lsb                fixed external;
dcl play.time.acu                fixed external;
dcl samp.speed                   fixed external;
dcl click.track.mode             fixed external;
dcl sm.mode                      fixed external;
dcl sm.hrs                       fixed external;
dcl d16tim                       fixed external;
dcl smpte.track.rate             fixed external;
dcl smpte.rate.accum             fixed external;
dcl smpte.mode.rate              fixed external;
dcl SAMPLED.RATE                 fixed external;
dcl new.motion                   fixed external;
dcl new.cue.track.settings       fixed external;
dcl lod.play.tracks              fixed external;
dcl lod.recd.tracks              fixed external;
dcl mark.button.msb              fixed external;
dcl mark.button.lsb              fixed external;
dcl mark.button.disabled         fixed external;
dcl first.play.time.msb          fixed external;
dcl first.play.time.lsb          fixed external;
dcl first.play.time.acu          fixed external;
dcl tbut.ptr                     fixed external;
dcl par.numt                     fixed external;
dcl trk.head.lookup              proc (fixed,fixed) external;
dcl tim.head.lookup              proc (fixed,fixed) external;
dcl trk.head.store               proc (fixed,fixed,fixed) external;
dcl tim.head.store               proc (fixed,fixed,fixed) external;
dcl p.lookup                     proc (fixed)        external;
dcl p.store                      proc (fixed, fixed) external;
dcl tim.ptr                      fixed external;
dcl bas.ptr                      fixed external;
dcl trk.head                     fixed external;
dcl tim.head                     fixed external;
DCL NEWKEY         					FIXED EXTERNAL;

DCL (DELAY.NOTE.AND.CUE.STARTS)	FIXED EXTERNAL;
DCL (DELAY.NOTE.AND.CUE.TIME)		ARRAY EXTERNAL;
DCL (PLAY.SEQ.TO.A.TIME)			FIXED EXTERNAL;
DCL (SEQ.PLAY.TO.MSB)				FIXED EXTERNAL;
DCL (SEQ.PLAY.TO.LSB)				FIXED EXTERNAL;
dcl Zero.Zero							data external;	//	from :SYNMODS:SMGRDCL

/* For 2.0 need these routines:

   DCL REMAP.WITH.LIVE.CLICK  PROC (FIXED,FIXED,FIXED,FIXED,FIXED) EXTERNAL;
   DCL REMAPPED.DUR.MSB       FIXED EXTERNAL;

*/

/* For 3.0 need these routines: */

   dcl Map.Sequence.Time.To.Real.Time         proc(array, array)        external;
   dcl Map.Sequence.Duration.To.Real.Duration proc(array, array, array) external;
   dcl Map.Real.Time.To.Sequence.Time         proc(array, array)        external;
   dcl Map.Real.Duration.To.Sequence.Duration proc(array, array, array) external;

/* public variables: */

dcl (dtd.retries   )    fixed public;  /* counts times dtdconnect fails     */
dcl (d34gpi.there  )    fixed public;  /* true if d34 gpi card in system    */
dcl (scsi.ptr      )    fixed public;  /* external memory buffer pointer    */
dcl (lod.cue.ptr   )    fixed public;  /* external cue trigger buffer       */
dcl (lod.cue.len   )    fixed public;  /* words of data in lod.cue.ptr      */
dcl (dsp.cue.ptr   )    fixed;         /* external envelope data buffer     */
dcl (dsp.cue.len   )    fixed;         /* words of data in dsp.cue.ptr      */
dcl (env.control.bits)  fixed;         /* control bits for envelope data    */
dcl (env.num.of.envs )  fixed;         /* number of envelope descriptors    */
dcl (dtd.is.scrubbing)  fixed public;  /* keep track of scrub position      */
dcl (dtd.is.trigging)   fixed public;  /* keep track of trig  position      */
dcl (dtd.scrub.msb )    fixed public;
dcl (dtd.scrub.lsb )    fixed public;
dcl (look.for.lod  )    fixed public;  /* set to 1 to poll for lod          */
dcl (check.reset   )    fixed;
dcl (lod.update.time)   fixed public;  /* time of last update               */
dcl (lod.version   )    fixed public;  /* lod version #                     */

dcl (lod.d24.#     )    fixed public;  /* d24 # where we find l.o.d         */
dcl (dsp.d24.#     )    fixed;         /* d24 # where DSP/DTD resides       */
dcl (lod.running   )    fixed public;  /* set true if lod there & running   */
dcl (syn.target.#  )    lit '6';       /* target # of us                    */
dcl (lod.target.#  )    fixed public;  /* target # lod software responds to */
dcl (dsp.lod.target.#)  lit '6';       /* works old way when talking to dsp */

dcl (look.for.dsp  )    fixed;
dcl (dsp.running   )    fixed public;  /* set true if DSP is running        */
dcl (dsp.update.time)   fixed;         /* time of last DSP update           */

dcl (dtd.song#     )    fixed public;  /* song row for dtd song display     */
dcl (dtd.track#    )    fixed public;  /* track row for dtd track display   */
dcl (dtd.max.secs  )    fixed public;  /* max recording time, seconds       */
dcl (dtd.max.tracks)    fixed public;  /* max recording track #             */
dcl (dtd.max.inputs)    fixed public;  /* max input #                       */
dcl (dtd.num.voices)    fixed public;  /* # of dtd outputs available        */
dcl (dtd.tks.per.drive) fixed public;  /* number of tracks per drive        */
dcl (dtd.xfer.mode )    fixed public;  /* bits for current ddt mode         */
dcl (dtd.avail.trks)    fixed public;  /* bits for available tracks         */
dcl (ddt.config    )    fixed public;  /* lower 4 bits:  0 = none           */
                                       /*                1 = routable       */
                                       /*                2 = multi          */
                                       /*                4 = DDSYN          */
                                       /* next  4 bits:  0 = mutsu only     */
                                       /*                1 = universal ddt  */
                                       /* next  4 bits:  current format     */
                                       /* upper 4 bits:  current sync       */
dcl (ddt.avail.trks)    fixed public;  /* tracks that have DDT available    */
dcl (dtd.dsp.sync.mode) fixed public;  /* 0 = seq, 1 = follow DSP           */

dcl (dtd.num.cues)      fixed public;  /* number of cues defined            */
dcl (dtd.cp.cues)       fixed public;  /* number of cues in current song    */
dcl (dtd.current.cue)   fixed public;  /* allocation # of cur cue           */
dcl (dtd.max.msb)       fixed public;  /* syncl time of dtd song end        */
dcl (dtd.max.lsb)       fixed public;
dcl (allocate.lod.cues) fixed public;  /* holds code to look up cue id's    */
dcl (stop.info.cues)    fixed public;  /* set true to stop info note trig   */
dcl (lod.xfer.len)      fixed public;  /* words transferred to poly         */
dcl (lod.mam)           fixed public;  /* where to transfer to xmem         */
dcl (send.all.cue.info) fixed public;  /* set to resend all cue info        */

dcl (Synclav.Time.Base.Adjust) fixed public; /* used to modulate synclav    */
                                             /* time base                   */
/* NOTE: the cur.dtd.ms is always a DTD disk time (not offset in any        */
/* way).  When scrubbing,  it holds the value of the scrub destination      */
/* time (not the current scrubbing audio point),  since this is generally   */
/* what you want.   When triggering a cue,  it holds the disk time we       */
/* are currently triggering at.                                             */

dcl Cur.DTD.Ms          lit 'loc(addr(cur.dtd.ms.msb))';

dcl (cur.dtd.ms.msb)    fixed public;  /* time for scrolling                */
dcl (cur.dtd.ms.lsb)    fixed public;  /* time for scrolling                */
dcl (cur.dtd.playing)   fixed public;  /* true if dtd is playing            */

dcl (dtd.cue#      )    fixed public;  /* cue # returned from lod           */

dcl (store.dtd.info)    fixed public;  /* 1=store song,  2=store track      */
dcl (scsibits)          data  public (256,512,1024,2048);		// bits for selecting scsi board

dcl Lod.Punch.In.Tracks fixed public;  /* for button panel recording        */

dcl Alt.Scsi.Ptr        fixed;         /* to send to lod without trashing   */
                                       /* info in scsi ptr                  */

/* display error message regarding */
/* live overdub                    */

dcl lod.message (32)  fixed public;  /* holds error message        */

log.lod.error:proc(mess) public; /* log error message for/from LOD         */
   dcl mess array;               /* leave NON-SWAP so string is in ext mem */
   dcl i    fixed;

   if inc.dtd=0 then return;

   i=mess(0);
   if i>64 then i=64;
   do i=0 to shr(i+1,1);
      lod.message(i)=mess(i);
   end;
   call set.error(err.lod,lod.message);
end log.lod.error;

/* $page  -  ABORT.SCSI */


/* abort.scsi:

      Called from real time loop when software wants to do input/output from
      scsi.   It terminates any live overdub scsi i/o that is currently
      going on. */


dcl (scsi.busy      )      fixed;
dcl (scsi.reset.time)      fixed;  /*       timer for reset                       */

#if (inc.mono == 0)
abort.scsi:proc public swapable;
   dcl (i,j) fixed;
   if inc.dtd=0 then return;

   do case (scsi.busy);

      do;                               			/* case 0 - no action needed  			*/
      end;

      do;                               			/* case 1 - lod is selected   			*/
        write(ScsiData)=0;             			/* remove DATA6               			*/
         
			do i=0 to 99;                  			/* wait for possible busy     			*/
				interp_usleep(1); 						// Delay for PPC
         end;                           			/* REQ could be down here     			*/
         
			write(ScsiBus )=S$SEL\S$ATN;   			/* leave SEL,  gnd ATN        			*/
         
			/* DTD might have seen selection just as we removed it.  If it did so, it     */
			/* has grounded S$BSY and is waiting for S$Sel to be released (see talksouf). */
			/* After it sees S$Sel released, it checks atn and aborts if set				   */
			
			do while (read(ScsiBus)&S$BSY)<>0;
				write(ScsiBus )=S$SEL\S$ATN\S$BSY;  /* assert S$BSY to hold on to buss		*/
         	write(ScsiBus )=S$ATN\S$BSY;			/* remove S$SEL, leave S$ATN and S$BSY */	// must assert S$BSY as well to keep hold of buss
				do i=0 to 99;               			/* hopefully LOD will see it  			*/	// hopefully LOD will see !S$Sel at this point
				   interp_usleep(1); 					// Delay for PPC
				end;
		      
				interp_run_host_non_timer();        /* chill							 				*/	// provide reasonable window even on Mac
         	
				write(ScsiBus )=S$SEL\S$ATN\S$BSY;  /* assert S$Sel to hold on to buss	   */
				write(ScsiBus )=S$SEL\S$ATN;			/* remove S$BSY,leave S$ATN & S$SEL		*/	// re-assert S$Sel to keep posession of bus
				do i=0 to 99;               			/* hopefully LOD will see it  			*/	// wait for our release of S$BSY to clear the bus
					interp_usleep(1); 					// Delay for PPC
				end;
         end;
        
		   write(ScsiBus )=0;             			/* release SEL and ATN        			*/
      end;

      do;                               			/* case 2 - reset pulse       			*/
         do while (real.milliseconds-scsi.reset.time) ilt 10;  
		      interp_run_host_non_timer();
         end;
         write(ScsiBus) = 0;
         scsi.reset.time=real.milliseconds;
         do while (real.milliseconds-scsi.reset.time) ilt 2000;
		      interp_run_host_non_timer();
         end;
      end;

      do;                               			/* case 3 - end of reset      			*/
         do while (real.milliseconds-scsi.reset.time) ilt 2000;
		      interp_run_host_non_timer();
         end;
      end;

   end;
   
	scsi.busy=0;

end abort.scsi;
#endif

#if (inc.mono == 1)
   dcl Abort.Scsi proc external;
#endif

/* $page - RESET.LOD.SCSI */


/* reset.lod.scsi - issue reset to scsi port */

reset.lod.scsi:proc swapable;

   if inc.dtd=0 then return;

   call log.lod.error('Resetting SCSI/LOD port...');

   scsi.busy=2;                                 /* reseting          */
   write(ScsiBus) = S$RST;                      /* issue reset       */
   scsi.reset.time=real.milliseconds;

   // Reset for 25 milliseconds
   do while (real.milliseconds - scsi.reset.time) ilt 25;
      call real.time.loop;
      if scsi.busy=0 then return;               /* reset completed by main loop */
		interp_run_host_non_timer();
   end;

   // Wait for up to 2 seconds for the bus to become free
   do while (((real.milliseconds - scsi.reset.time) ilt 2000)
   &&        (((read(ScsiBus) & (S$BusMask XOR S$RST)) != 0) || ((read(ScsiData) & S$DataMask) != 0)));
      call real.time.loop;
      if scsi.busy=0 then return;               /* reset completed by main loop */
      interp_run_host_non_timer();
   end;

   scsi.busy=3;
   write(ScsiBus) = 0;
   scsi.reset.time=real.milliseconds;

   do while (real.milliseconds-scsi.reset.time) ilt 2000;
      call real.time.loop;
      if scsi.busy=0 then return;               /* reset completed by main loop */
      interp_run_host_non_timer();
   end;

   scsi.busy=0;

end reset.lod.scsi;


/* $page  -  SEND.TO.LOD.SUBROUTINE */


/* send.to.lod.subroutine - procedure to send a message to lod computer */

/* routine is passed:

   1. a message type.   see Live Overdub printout for message
      protocols (1='hello', 4='reboot', 200-202='load')

   2. a byte length. 

   3. a pointer to a message in external memory (usually Scsi.Ptr) */


/* routine sends message to live overdub.   will return if
   live overdub has gone off the air */
   
/* returns message acknowledgement code received. */

/* error messages for terminal are spun off here. */

dcl DSP.File.Name (32) fixed;    /* holds name of file to boot into DSP  */
dcl lod.loading        fixed;    /* nonzero if loading code & tables     */
dcl dsp.loading        fixed;    /* nonzero if loading DSP               */

/* lod.loading holds bits that control the transfer of files over the    */
/* SCSI Protocol.  They are as follows:                                  */

dcl load.lod7        lit ' 1';   /* set up to load .lod-7                */
dcl load.hzfile      lit ' 2';   /* set up to load .ltab-7               */
dcl load.bootfile    lit ' 4';   /* set up to load Dsp.File.Name         */
dcl load.senddata    lit ' 8';   /* send data from file                  */
dcl load.sendswap    lit '16';   /* send ABLE swap file info             */
dcl load.dataishz    lit '32';   /* set if data being sent is HZ file    */
dcl load.setrunning  lit '64';   /* mark DTD running after file load     */
                                 /* higher bits are used to re-enter     */
                                 /* send.system.info.to.lod quickly      */
                                 /* to speed up the bootload process     */

dcl load.anything    lit '(load.lod7\load.hzfile\load.bootfile\load.senddata\load.sendswap\load.dataishz\load.setrunning)';
dcl load.interrogate lit '"177400"';

dcl Build.Proj.Hist.Rec proc (fixed,fixed,array) recursive;
dcl Add.Cue.To.Hist     proc (array)             recursive;

/* some assembler lits we will need */
   dcl loa0  lit '"200"';
   dcl and0  lit '"204"';
   dcl or0   lit '"230"';
   dcl tand0 lit '"244"';
   dcl rtnz  lit '"335"';
   dcl rtze  lit '"331"';
   dcl rtra  lit '"330"';
   dcl bsor0 lit '"260"';

/* $subtitle Connect to Direct-to-Disk over the SCSI bus */

/*
   DTDCONNECT

   This procedure is used to connect the Synclavier to the DTD. It calls
   ARBITRATE and then SELECT; no IDENTIFY as the DTD boot rom can't.
*/

DTDConnect: proc(D24ID,Initiator,Target,Lun) returns (fixed) public swapable;
   dcl D24ID         fixed; /* D24 id number */
   dcl Initiator     fixed; /* SCSI Initiator ID number */
   dcl Target        fixed; /* SCSI Target ID number */
   dcl Lun           fixed; /* SCSI Logical Unit number */
   dcl ID            fixed; /* Bit encoded ID */
   dcl i             fixed;
   dcl saveR0        fixed;
   dcl interrupts    fixed;
   dcl Bit data (1, 2, 4, 8, 16, 32, 64, 128); /* Binary to Bit mapping */
   
	interp_set_scsi_id(interp_set_scsi_id_access_dtd, D24ID, Target);
   
	/*** Select the D24 SCSI Host Adapter ***/
   i = shl(Bit(D24ID), 8);
   write(ScsiSel) = S$SelectEnable or i;   /* Enable D24 */
   write(ScsiBus)  = 0;                    /* Clear scsi bus */
   write(ScsiData) = 0;                    

   /*** Arbitration Phase ***/
   ID = Bit (Initiator);                   /* Get Initiator ID */

   write("313") = 0;                       /* clear our win flag */
   i = real.milliseconds;                  /* set timer (500 ms) loop takes about 8 us when bus is bsy */
   disable;                                /* disable interrupts to ensure proper scsi timings */

   // Arbitration is meaningless now. We don't really support multiple computers on the
   // same SCSI bus any more.
   do while ((read("313")=0) and ((real.milliseconds - i) < 200));
      saveR0       = read("300");          /* save r0 */

      write(loa0 ) = read("25");           /* latch scsi bus */
      write(or0  ) = read("25");           /* or it in again */
      write(tand0) = (S$BusFree);          /* test for the bus free bits */ 	// 2 instructions
      write(rtnz ) = 17;                   /* bus not free */
      write("25" ) = S$BSY;                /* assert bsy before a bus set delay 1.8 us */
      write("24" ) = "100";                /* assert id6 - hard coded for now */ // 2 insructions ScsiData
      write(atnv ) = 0;                    /* wait an arb delay from assertion of bsy (2.2 us) */
      write(atnv ) = 0;
      write(atnv ) = 0;
      write("300") = read("24");           /* look at data bits ScsiData */   
      write(tand0) = "200";                /* mask off all but bit id7 */	// 2 instructions...
      write(rtze ) = 4;                    /* zero: we won */
      write("25" ) = 0;                    /* we lost: release bus and data lines */
      write("24" ) = 0;                    // ScsiData
      write("313") = 2;                    /* set lost flag */
      write(rtra ) = 3;                    /* we lost: exit loop */
      write("25" ) = (S$BSY or S$SEL);     /* we won: assert SEL now */	// 2 instructions
      write("313") = 1;                    /* set flag so we know we won */
      write(atnv ) = 0;                    /* marker */

      write("300") = saveR0;               /* restore r0 */
     
	   if (read("313") = 0) then do;			 /* lost arbitration */
         enable;
         
	      interp_set_scsi_done();		      // Done with scsi command
         
         if (run.syn <> 0) then do;			 /* so chill for a while */
            call real.time.loop;
			end;

			if (newkey == 0 || run.syn == 0)	 // Exit from DTask and otherwise chill if no keypresses pending.  If new note is pending
				interp_run_host_non_timer();   // return immediately to real.time.loop to start it.  Newkey will be zero virtually always.

			interp_set_scsi_id(interp_set_scsi_id_access_dtd, D24ID, Target);	
			write(ScsiSel)=S$SelectEnable \ Scsibits (D24ID);  
			write(ScsiBus)  = 0;
			write(ScsiData) = 0;                    
			write("313")    = 0;           	/* restore "313" */

         disable;
      end;
   end;

   if (read("313") = 1) then do;           /* we won arbitration 	*/
      write(ScsiData) = 0;                 /* cease arbitration 	*/

      /*** Selection Phase ***/
      write(ScsiData) = (ID or Bit (Target)); /* Set Initiator and Target IDs on data bus */
      write(ScsiBus) = (S$SEL);               /* Release BSY; assert SEL 		*/
   end;
   else if (read("313") = 0) then do;      /* no bus free phase 	*/
      write(ScsiBus)  = 0;                 /* Cease arbitration 	*/
      write(ScsiData) = 0;
      enable;
      return (S$BadBusState);              /* Go no further 		*/
   end;
   else if (read("313") = 2) then do;      /* we lost 				*/
      write(ScsiBus)  = 0;                 /* Cease arbitration 	*/
      write(ScsiData) = 0;
      enable;
      return (S$ArbFailed);                /* We lost arbitration */
   end;

   enable;

   return(S$GoodConnect);              /* Good connection made */

end DTDConnect;

send.to.lod.subroutine: proc(msgtype,out.len,msg.ptr) PUBLIC swapable;   /* send message to lod */
   dcl (msgtype)         fixed;
   dcl (out.len)         fixed;
   dcl (msg.ptr)         fixed;
   dcl (new.ms.msb)      fixed;
   dcl (new.ms.lsb)      fixed;
   
   dcl (i,j,k,l,m)       fixed;
   dcl (target)          fixed;
   dcl rec(cue.hist.rec.len-1) fixed;
	dcl (ever.had.dtd)    fixed static;

   /* procedure to adjust synclavier time base to DTD master clock */

   Adjust.Synclav.Timebase:proc (val);
      dcl val fixed;

      if val = 0 then return;             /* zero val means no adjust     */

      disable;                            /* make sure we are in internal */
      if  (Time.Base.Mode = 1)            /* sync and still playing.  Set */
      then do;                            /* global variable to be        */
         Synclav.Time.Base.Adjust = val;  /* processed by interrupt       */
         Time.Base.Mode           = 8;    /* routine                      */
			interpret_set_timebase_mode(TIME.BASE.MODE);
      end;
      enable;

   end Adjust.Synclav.Timebase;

   /* Procedure to scan envelope infomration and forward it to the DSP:   */

   Read.Envelope.Information: proc(max);
      dcl max fixed;          /* pass # of bytes of env info remaining    */
      dcl i   fixed;
      dcl j   fixed;

      i = read(ScsiWord);                        /* get control bits      */
      j = read(ScsiWord);                        /* get # of envelopes    */

      if (i&1) <> 0 then do;                     /* reset if needed       */
         env.control.bits = 0;                   /* toss old information  */
         env.num.of.envs  = 0;                   /* if we should          */
         dsp.cue.len      = 0;
      end;

      /* Scan and merge envelope data if it will fit in dsp.cue.ptr:      */

      if (dsp.cue.len + shr(max-4,1)) <= lod.cue.max
      then do;
         env.control.bits = env.control.bits \ i;   /* or control bits    */
         env.num.of.envs  = env.num.of.envs  + j;   /* add # of envs      */
         
         write(mam) = dsp.cue.ptr + shr(dsp.cue.len, 8);
         write(mal) = dsp.cue.len;

         do i = 4 to max-2 by 2;                    /* append data        */
            write(mdi) = read(ScsiWord);
         end;

         dsp.cue.len = dsp.cue.len + shr(max-4, 1);
      end;

      /* else ignore data if it will not fit:                             */
      else do;
         do i=4 to max-2 by 2;
            j=read(ScsiWord);            /* toss rest of info */
         end;
      end;
   end Read.Envelope.Information;

   if (inc.dtd=0) or (dtd.retries>5) then return (-1);

   target = lod.target.#;
   send.again:;

   i = DTDConnect(lod.d24.#, S.Initiator, target,0);

   if  (i <> S$GoodConnect) {                    	   /* if no connection made (e.g. could not arbitrate) */
		
		interp_set_scsi_done();				               // Done with scsi command
      
      return(-1);
   }
   
   scsi.busy=1;          										/* set scsi.busy so abort.scsi knows what is happening */

   // SEL and the target/initiator bits are set.
   // Wait for the DTD to set BSY
   i = real.milliseconds;
   do while (read(ScsiBus)&S$BSY) == 0;					/* wait for response */
      if (interp_D24_requested(0) == 1) {
            write(ScsiData) = 0;                      /* Release data bus 	 */
				abort.scsi();									   /* clean up           */
				
				// This will return to main loop and clear D24_requested
		      interp_set_scsi_done();				         // Done with scsi command
	         
				return (-1);										/* done					 */
      }

      if ((real.milliseconds-i) igt 20)					/* after 20 milliseconds, do extra checks */
		{
         if  (((lod.running ==   0)                  	/* if not running or  */
         and   (lod.loading ==   0))                 	/* loading            */
			or   (msgtype      == 130 ))					   /* or simple sync msg */
			{
            write(ScsiData) = 0;                      /* Release data bus 	 */
				abort.scsi();									   /* clean up           */
				
				if (target <> 6) then do;						/* try ID 6 in case	 */
					target = 6;										/* A1 boot button		 */
					goto send.again;								/* was pressed			 */
				end;
			
				if  ((lod.running == 0)                   /* if not running or  */
				and  (lod.loading == 0))                  /* loading            */
				{														/* retry search if	 */
					if (ever.had.dtd == 0)						/* count retries towards failure if never saw a DTD				  */
						dtd.retries = dtd.retries + 1;      /* count succesive fails so we eventually stop looking for a DTD */
				}
				
				else													/* else if we did see one once, never give up looking for one    */
					ever.had.dtd = true;
		      
		      interp_set_scsi_done();				         // Done with scsi command
				
				return (-1);										/* done					 */
			}
			
			/* if we had been talking to a DTD but it is now not responding, wait quite a while to */
			/* hope it recovers.  If it is totally gone (e.g. powered down) give up after			   */
			/* a while and try searching for it again.															*/
			
         if ((real.milliseconds-i) igt 30000)  			/* if dead for 30 sec */
         {                              					/* then give up       */
            write(ScsiData) = 0;                      /* Release data bus 	 */
				abort.scsi();									   /* clean up           */
            
				lod.running    = 0;                   		/* not running        */
				dtd.max.secs   = 0;
				look.for.lod   = 1;                   		/* try to find one    */
				
				if msgtype<>1 then do;
					call log.lod.error('Timeout Error in Multi-Track');
					New.Seq.Info = New.Seq.Info | 8;      /* new constants */
				end;
                  
		      interp_set_scsi_done();				         // Done with scsi command
				
				return (-1);
         }
      }
      
		if run.syn<>0 then call real.time.loop;
		
      if scsi.busy=0 then goto send.again;     			/* aborted by main l  */

		if (((read(ScsiBus)&S$BSY) == 0)				  		/* still no response: allow mac to run.  We will return within 5 msecs due to deferred task */
		&&  ((newkey == 0 || run.syn == 0)))	 			// Exit from DTask and otherwise chill if no keypresses pending.  If new note is pending
			interp_run_host_non_timer();                 // return immediately to real.time.loop to start it.  Newkey will be zero virtually always.
   end;

   lod.target.# = target;
  
   dtd.retries     = 0;                        /* clear counter      */
   write(ScsiData) = 0;                    
   write(ScsiBus ) = 0;                        /* Selection successful - Release SEL; DTD is holding BSY */

   /* Wait here for REQ so we can see if the DTD is the new or       */
   /* old variety:                                                   */

   i=real.milliseconds;                        /* retry timer        */

	// Normally, S$REQ is set by the time we get here.  It only would not be set if LOD
	// has just seen our selection, has asserted S$BSY, but has not asserted S$REQ yet.
   do while (read(ScsiBus)&S$REQ)=0;           /* wait for his REQ   */
      if (real.milliseconds-i) igt 100         /* no req for 100 ms? */
      then do;   
         call abort.scsi;                      /* abort ourselves    */
         goto send.again;  
      end;

      if run.syn<>0 then call real.time.loop;
		
		if (newkey == 0 || run.syn == 0)         // Exit from DTask and otherwise chill if no keypresses pending.  If new note is pending
			interp_run_host_non_timer();          // return immediately to real.time.loop to start it.  Newkey will be zero virtually always.

      if scsi.busy=0 then goto send.again;     /* aborted by main l  */
   end;

   if (read(ScsiBus) and S$SigMask) = S$Status /* See if status phase has been entered */
   then do;
      if (read(ScsiWord) <> S$GoodConnect) then do;
         // Wait for bus free
         do while (read(ScsiBus)&S$BSY)<>0; end;
         scsi.busy=0;
		   interp_set_scsi_done();				    // Done with scsi command
  			return(-1);
      end;
   end;

   if msgtype=130 then do;                     /* send real time sync message  */

      if (read(ScsiBus)&S$C.D) = 0             /* if no command phase ... */
      then write(ScsiByte)=msgtype;            /* send to original DTD    */
      else do;                                 /* else send to NU-ARCH    */
         write(ScsiWord) = 0;                  /* with 4 bytes of padding */
         write(ScsiWord) = msgtype;            
      end;

      disable;                                 /* send precise time    */

      if lod.cue.len=0 then do;                /* no cues - send time  */
         write(ScsiWord)=4+smpte.onoff+smpte.onoff;
      end;
      else do;
         write(ScsiWord)=8+shl(lod.cue.len,1); /* send cues as well    */
      end;

      if   time.base.mode=7                    /* start time base      */
      then do;
			time.base.mode=1;                  	  /* now that lod is up   */
			interpret_set_timebase_mode(TIME.BASE.MODE);
		end;

      write(ScsiWord)=play.time.lsb;           /* to speed             */

      // The original D16 timer, when set to microseconds, would count
      // from -1000 up to 0 to create the 1 msec  interrupt. More or less.
      // In response to the D16 interrupt the processor read the d16 timer
      // to figure out how many microseoncds into the next millisecond we
      // were, and then reset the D16 to a minus number that would trigger
      // the next millisecond at the right time.
      //
      // in Measure.D16, in the interpreter case we just set up the D16
      // as if it is counting at a 100 khz raqte
      // D16TIM	= 500;
      // D16THERE	= 499;
      // D16HALF	= SHR(D16TIM,1);
      // D16PHASE	= D16HALF;

      // That is, in the interpreter we model 500 D16 ticks per millisecond.
      // This is the only point in the real time software where we try to model
      // a time point between the millisecond interrupts.

      // March 16, 2017 - Measured assert_wait_deadline. Task was typically woken up .2 msec late.
      // Did not find a constraint policy that fixed that.

		if (interp_is_running != 0)
         load interp_get_d16_time(D16TIM);      /* get fraction into next d16 interval */

      else
		{
			i=read(d16);                          /* get current d16      */
			if i>=(-1) then do;
				write(d16)=1;                      /* reset d16 timer since interrupt request was cleared by read */
				i=(-1);
			end;
         
         load d16tim+i;                         /* get fraction into d16 */
		}

      // For example, i == -250 means we are half way through the current millisecond
      // d16tim+i then == +250

      // What we are trying to do here is compute a precise time for the DTD
      // as to where we are.

      // we start with play.time.acu. that increments by samp.speed every millsecond.
      // so we compute the prices play.time.acu that goes with the current instant.

      if smpte.onoff=0 then do;                /* non-smpte            */
         if res<0 then do;                     /* large phase error     */
            load (-res);                       /* means we are ahead    */
            mul samp.speed; mwait; div d16tim;
            write(ScsiWord)=play.time.acu-res;
         end;
         else do;
            mul samp.speed; mwait; div d16tim;
            write(ScsiWord)=play.time.acu+res;
         end;
      end;
      else do;                                 /* smpte                */
         if res<0 then do;                     /* large phase error     */
            load (-res);                       /* means we are ahead    */
            mul samp.speed; mwait; div d16tim;
            load res;
            mul smpte.track.rate; mwait; div smpte.mode.rate;
            i = (-res);
         end;
         else do;
            mul samp.speed; mwait; div d16tim;
            load res;
            mul smpte.track.rate; mwait; div smpte.mode.rate;
            i = res;
         end;
         load smpte.rate.accum; 
         mul samp.speed; mwait; div smpte.mode.rate;
         write(ScsiWord)=play.time.acu + i + res;
         write(ScsiWord)=SAMPLED.RATE;				// Send raw SMPTE bit rate to DTD.
      end;
      enable;

      /* now send direct to disk cues */

      if lod.cue.len<>0 then do;                   /* send cue data      */
         if smpte.onoff=0 then write(ScsiWord)=0;  /* fill in smpte time */
         write(ScsiWord)=0;                        /* space to word 4    */
         write(mam)=lod.cue.ptr;                   /* send info          */
         do i=1 to lod.cue.len;
            write(ScsiWord)=read(mdi);
         end;
         lod.cue.len=0;
      end;
   end;

   else do;                                    /* normal message     */

      if (read(ScsiBus)&S$C.D) = 0             /* if no command phase ... */
      then write(ScsiByte) = msgtype;          /* send to original DTD    */
      else do;                                 /* else send to NU-ARCH    */
         write(ScsiWord) = 0;                  /* with 4 bytes of padding */
         write(ScsiWord) = msgtype;            
         if msgtype < 128
         then write(ScsiWord) = 0;             /* provide 6 bytes if no length field */
      end;

      if msgtype>=128 then do;                 /* data               */
         write(ScsiWord)=out.len;              /* length, bytes      */
         write(mam)=Msg.Ptr;                   /* get data           */
         do i=1 to shr(out.len+1,1);
            write(ScsiWord)=read(mdi);
         end;

         if msgtype=179 then do;               /* basic data to poly */
            write(mam)=Msg.Ptr;
            write(mal)=2;                      /* pt to # of wrds    */
            i=read(mdi);
            do while i ige 32;
               rpc 32;
               write(ScsiWord)=read(mdi);
               i=i-32;
					interp_check_interrupts();
            end;
            if i<>0 then do;
               rpc i;
               write(ScsiWord)=read(mdi);
               i=0;
            end;
         end;
         else if msgtype=196 then do;          /* send poly data     */
            write(mam)=Msg.Ptr;
            write(mal)=1;                      /* pt to # of wrds    */
            i=read(md);
            do while i ige 32;
               rpc 32;
               write(ScsiWord)=read(psd);
               i=i-32;
					interp_check_interrupts();
            end;
            if i<>0 then do;
               rpc i;
               write(ScsiWord)=read(psd);
               i=0;
            end;
         end;
         else if msgtype = 199 then do;			/* if DSP message */
            write(mam)=Msg.Ptr;
            if  (read(mdi)=1)
            and (read(mdi)=0)
            then do;										/* send sector if continuation word = 0 */
               i = read(mdi);							/* get  code length */
               rpc i;
               write(ScsiWord) = read(mdi);
            end;   										/* of if continuation = 0 */
         end;      										/* of if DSP message */
      end;
   end;

   // Wait for REQ with return result
   i = read(ScsiBus);
   
   // Wait for !ack from write to D27
   while ((i & (S$ACK|S$BSY|S$ATN|S$RST)) == (S$ACK|S$BSY))
      i = read(ScsiBus);
   
   // Wait for req, watching for disconnect or reset
   while ((i & (S$REQ|S$BSY|S$ATN|S$RST)) != (S$REQ|S$BSY)) {
      if ((i & (S$BSY|S$ATN|S$RST)) != (S$BSY)) {
         scsi.busy=0;
         interp_set_scsi_done();				    // Done with scsi command
         return(-1);
      }
      i = read(ScsiBus);
   }

   // We need a short delay (10) microseconds after writing to the D27
   // before we read the D26 or D27. I belive this is because the state machine
   // is still busy doing the posted write to D27. A 10 microsecond delay
   // here fixed all the DTD crashing. 3/7/2015 C. Jones with tremendous
   // help from Mitch Marcoulier.
   interp_usleep(10);
   
   i=read(ScsiByte);                           /* get reply          */

   if i>=128 then do;                          /* long message       */
      if  ((i=220)&(store.dtd.info=1))         /* scan off dtd info  */
      or  ((i=221)&(store.dtd.info=2))
      then do;
         j=read(ScsiWord);                     /* get message len    */
         if i=220 then dtd.song# =read(ScsiWord);
         else          dtd.track#=read(ScsiWord);
         write(mam)=trd.ptr;                   /* store in trd area  */
         do k=2 to shr(j+1,1);                 /* get other info     */
            write(mdi)=read(ScsiWord);
         end;

         /* Create a system event when a new track directory or a    */
         /* new song directory is stored:                            */

         if (i=220)                            /* song directory     */
         then new.dtd.info = new.dtd.info \ 1; /* new song directory */
         else new.dtd.info = new.dtd.info \ 4; /* new track dir      */
      end;
      else if (i=222) or (i=225) then do;      /* lod init info      */
         k=read(ScsiWord);                     /* read length        */
         j=read(ScsiWord);                     /* get dtd.max.secs   */
         if j <> dtd.max.secs then do;         /* has it changed?    */
            dtd.max.secs = j;                  /* if so, save it     */
            new.seq.info = new.seq.info | 8;   /* new constants      */
            new.dtd.info = new.dtd.info | 32;  /* new sample rate    */
         end;
         dtd.max.tracks=read(ScsiWord);
         dtd.max.inputs=read(ScsiWord);
         if i=225 then do;                     /* get expanded info */
            dtd.avail.trks=read(ScsiWord);
            if k >=24 then do;                 /* cue info sent     */
               dtd.num.cues      = read(ScsiWord);
               dtd.cp.cues       = read(ScsiWord);
               dtd.current.cue   = read(ScsiWord);
               dtd.max.msb       = read(ScsiWord);
               dtd.max.lsb       = read(ScsiWord);
               lod.version       = read(ScsiWord);
               dtd.num.voices    = read(ScsiWord);
               dtd.xfer.mode     = read(ScsiWord);
               ddt.config        = read(ScsiWord);
               ddt.avail.trks    = read(ScsiWord);
               dtd.dsp.sync.mode = read(ScsiWord);
					dtd.tks.per.drive = read(ScsiWord);
               do j=33 to k by 2;
                  l=read(ScsiWord);            /* toss rest of info */
               end;
               if lod.version < 6              /* turn into bits for early software */
               then dtd.xfer.mode = bits(dtd.xfer.mode);
					if (dtd.tks.per.drive == 0)
						dtd.tks.per.drive = 4;
            end;
            else do j=10 to k by 2;
               l=read(ScsiWord);               /* toss rest of info */
            end;
         end;
         else do;
            dtd.avail.trks=bits(dtd.max.tracks)-1;
         end;

         /* Create a system event when a new initialization record   */
         /* is received:                                             */

         new.dtd.info = new.dtd.info \ 1;      /* new info  rcvd     */

         if   dtd.num.voices < dtd.max.tracks  /* set num voices     */
         then dtd.num.voices = dtd.max.tracks; /* for old LOD's      */
      end;

      /* Handle real time scrolling info from the DTD:               */

      else if i=226 then do;                   /* play time from lod */
         j=read(ScsiWord);                     /* get length handy   */
         new.ms.msb = read(ScsiWord);          /* play time msb      */
         new.ms.lsb = read(ScsiWord);
         if j >= 6 then do;                    /* get motion  word   */
            cur.dtd.playing = read(ScsiWord); 
         end;
         if j >= 8 then do;                    /* adjust if needed   */
            call Adjust.Synclav.Timebase(read(ScsiWord));
         end;
         if j >= 10 then do;                   /* env info           */
            call Read.Envelope.Information(j-8);
         end;

         /* Create a system event if the current dtd time changes:   */

         if  ((cur.dtd.ms.msb <> new.ms.msb)   /* if different       */
         or   (cur.dtd.ms.lsb <> new.ms.lsb))
         then do;
            cur.dtd.ms.msb = new.ms.msb;       /* create event for   */
            cur.dtd.ms.lsb = new.ms.lsb;       /* dtd scrolling      */
            new.dtd.info   = new.dtd.info \ 8; /* messages.          */
         end;
      end;

      else if i=227 then do;                   /* cue id#/record     */
         k=read(ScsiWord);                     /* read and toss len  */
         dtd.cue# = read(ScsiWord);            /* get cue #          */

         write(mam)=Msg.Ptr;                   /* store cue in scsi ptr */
         do j=4 to k by 2;                     /* get cue info       */
            write(mdi)=read(ScsiWord);         /* if any to be ret'd */
				interp_check_interrupts();
         end;
      end;
      else if (i=228)&(store.dtd.info=3)
      then do;                                 /* data for poly      */
         k=shr(read(ScsiWord),1);              /* get words to xfer  */
         j=k;
         do while j ige 32;
            rpc 32;
            write(psd)=read(ScsiWord);
            j=j-32;
				interp_check_interrupts();
         end;
         if j<>0 then do;
            rpc j;
            write(psd)=read(ScsiWord);
            j=0;
         end;
         lod.xfer.len = lod.xfer.len + k; /* accumulate words transferred */
      end;
      else if (i=229)&(store.dtd.info=4)
      then do;                                 /* for external mem   */
         k=shr(read(ScsiWord),1);              /* get words to xfer  */
         j=k;
         write(mam)=lod.mam;
         do while j ige 32;
            rpc 32;
            write(mdi)=read(ScsiWord);
            j=j-32;
				interp_check_interrupts();
         end;
         if j<>0 then do;
            rpc j;
            write(mdi)=read(ScsiWord);
            j=0;
         end;
         lod.xfer.len = lod.xfer.len + k; /* accumulate words transferred */
      end;

      else if i=230 then do;                   /* scan time base adjust */
         j=read(ScsiWord);                     /* get byte length       */
         call Adjust.Synclav.Timebase(read(ScsiWord));
         if j >= 4 then do;                    /* get envelope info     */
            call Read.Envelope.Information(j-2);
         end;
      end;

      /* For 231 message,  means we are talking to a DSP/DTD.  Switch */
      /* over to using that code:                                     */

      else if i=231 then do;                  /* snarf off file name for bootload */
         Dsp.File.Name(0) = read(ScsiWord);   /* get length of file name in bytes */
         do j = 0 to Dsp.File.Name(0) - 1;    /* and get that many data bytes     */
            k = read(ScsiByte);
            if j < 64 then call pbyte(Dsp.File.Name, j, k&127);
         end;

         if Dsp.File.Name(0) IGT 64
         then Dsp.File.Name(0) = 64;

         look.for.lod = 0;         /* cancel all activity for DTD  */
         dtd.max.secs = 0;
         lod.running  = 0;
         lod.loading  = 0;

         look.for.dsp = 1;         /* activate DTD/DSP software    */
         dsp.d24.#    = lod.d24.#;

         dsp.loading = load.bootfile;
      end;

      /* 232 message has current DTD scrub position:               */

      else if i=232 then do;                   /* scrub pos from lod */
         j=read(ScsiWord);                     /* get length handy   */
         dtd.scrub.msb = read(ScsiWord);       /* scrub sample #     */
         dtd.scrub.lsb = read(ScsiWord);       /* msb, lsb           */
         if j >= 6 then do;                    /* get motion         */
            k = read(ScsiWord);                /* get trig\loop      */
            if  (dsp.running      <> 0)        /* if dsp running     */
            and (k                =  0)        /* and dtd is idle    */
            and ((dtd.is.scrubbing <> 0)       /* but was scrubbing  */
            or   (dtd.is.trigging  <> 0))      /* or trigging        */
            then do;
               new.motion = 1;
               dtd.is.scrubbing = 0;
               dtd.is.trigging  = 0;
            end;
         end;
         if j >= 12 then do;
            new.ms.msb = read(ScsiWord);          /* play time msb      */
            new.ms.lsb = read(ScsiWord);
            cur.dtd.playing = read(ScsiWord); 
            if  ((cur.dtd.ms.msb <> new.ms.msb)   /* if different       */
            or   (cur.dtd.ms.lsb <> new.ms.lsb))
            then do;
               cur.dtd.ms.msb = new.ms.msb;       /* create event for   */
               cur.dtd.ms.lsb = new.ms.lsb;       /* dtd scrolling      */
               new.dtd.info   = new.dtd.info \ 8; /* messages.          */
            end;
         end;
         do k=14 to j by 2;
            l=read(ScsiWord);                  /* toss rest of info  */
         end;
      end;

      else if i=128 then do;                   /* scan terminal msg  */
         lod.message(0)=read(ScsiWord);        /* get length, bytes  */
         if lod.message(0)>64 then do;         /* limit to 32 words  */
            do j=1 to 32;                      /* length in bytes    */
               lod.message(j)=read(ScsiWord);
            end;
            do j=33 to shr(lod.message(0)+1,1);
               k=read(ScsiWord);
            end;
            lod.message(0)=64;
         end;
         else do j=1 to shr(lod.message(0)+1,1);
            lod.message(j)=read(ScsiWord);
         end;
         call set.error(err.lod,lod.message);
      end;
      else do;                   					/* toss erroneous message          */
         j=read(ScsiWord);       					/* get byte length to toss         */
         do j=1 to shr(j+1,1);
            k=read(ScsiWord);
         end;
      end;
   end;

   else if i=2 then do;          					/* 2 from ABLE loader              */
      if (lod.loading&load.anything) = 0
      then do;                   					/* then send .lod-7 file           */
         lod.loading  = load.lod7;
         if lod.running <> 0     					/* if had been running but we are  */
         then do;                					/* now rebooting then means some   */
            lod.running  = 0;    					/* one pressed the boot button.    */
            dtd.max.secs = 0;
            look.for.lod = 1;
            New.Seq.Info = New.Seq.Info | 8;
         end;
      end;
   end;

   else if i=3 then do;          					/* 3 from ABLE .lod-7              */
      if  ((lod.loading&load.anything) = 0)
      and ( lod.running                = 0)
      then lod.loading = load.hzfile; 				/* then send .lod-7 file      */
   end;

   else if i=4 then do;          					/* from lod: requesting new motion */
      new.motion             = 1;
      new.cue.track.settings = 1;
   end;

   else if i=5 then do;          					/* change in punch in/out          */
      new.dtd.info = new.dtd.info \ 16;
   end;

   /* Code 6 is set whenever a tape load/backup command is          */
   /* finished,  or whenever a new DTD project is called up.        */

   else if i=6 then do;          /* command completed               */
      new.dtd.info = new.dtd.info \ 32;
      allocate.lod.cues = 2;     /* re-allocate all cues here       */
   end;

   else if i=7 then do;          /* send cue play info              */
      if new.sequencer.forward.motion = 0     /* if not busy doing  */
      then new.sequencer.forward.motion = 2;  /* notes, start cues  */
      else send.all.cue.info = 1;             /* else restart later */
   end;

   /* Also might see 8 from DTD/DSP                                 */

   else if i=8 then do;         /* 8 from nu-arch DTD               */
      look.for.lod = 0;         /* cancel all activity for DTD      */
      dtd.max.secs = 0;
      lod.running  = 0;
      lod.loading  = 0;

      look.for.dsp = 1;         /* activate DTD/DSP software        */
      dsp.d24.#    = lod.d24.#; /* on this d24                      */

      dsp.loading  = load.setrunning;
   end;

   // Wait for bus free
   do while (read(ScsiBus)&S$BSY)<>0; end;
   
   scsi.busy=0;

	interp_set_scsi_done();		   // Done with scsi command
   
   return i;                     /* message type                    */

end send.to.lod.subroutine;


/* for compatibility with older code - pass Scsi.Ptr as */
/* default argument to Send.To.Lod.Subroutine           */

send.to.lod: proc(msgtype,out.len) public swapable;   /* send message to lod */
   dcl msgtype    fixed;
   dcl out.len    fixed;

   return Send.To.Lod.Subroutine(msgtype, out.len, Scsi.Ptr);
end send.to.lod;

/* $page - insert files LOD routines for AEE, Remote Box, and Events */

insert ':synsou:lodmod:lodsoua';
insert ':synsou:lodmod:lodsoub';
insert ':synsou:lodmod:lod-dtd1';
insert ':synsou:lodmod:lod-dtd2';
insert ':synsou:lodmod:lod-cue1';
insert ':synsou:lodmod:lod-cue2';
insert ':synsou:lodmod:lod-cue3';
insert ':synsou:lodmod:lod-cue4';
insert ':synsou:lodmod:lod-edt1';
insert ':synsou:lodmod:lod-edt2';
insert ':synsou:lodmod:lod-edt3';
insert ':synsou:lodmod:lod-edt4';

lodinit:proc swapable;
   dcl i fixed;

   if (read("51")&"40000")<>0 then do; 			/* scsi board exists        */

      /* If we have a SCSI board,  set variables to search for     */
      /* regular DTD:                                              */

      lod.d24.#    = 0;                			/* start here               */
      /* look for live overdub    */
      look.for.lod = ((interp_real_time_prefs(SYNCLAVIER_PREF_POLL_DTD) & SYNCLAVIER_PREF_POLL_DTD) != 0);  								
      lod.target.# = 3;                			/* start with new ID of 3, if dtd doesn't respond we will try old ID of 6 in send.to.lod.subroutine */
      lod.update.time = real.milliseconds - lod.update.rate; /* look NOW */

      // Check for reset first time only
      check.reset = Scsibits(0) | Scsibits(1) | Scsibits(2) | Scsibits(3);
   end;

   if (inc.d34.remote.control.code<>0)    /* force inclusion of d34 code */
   or ((read("51")&"100000")<>0) then do; /* look to expanded reg for d34 id bit */
      if (inc.d34.remote.control.code<>0) /* force inclusion of d34 code */
      or ((read("57")&8)<>0) then do;     /* d34 gpi interface exists */
         d34gpi.there = ((interp_real_time_prefs(SYNCLAVIER_PREF_POLL_D34GPI) & SYNCLAVIER_PREF_POLL_D34GPI) != 0);
      end;
   end;

   call Initialize.Remote.Control.Items;  /* init d34 hardware        */

   /* initialize ext memory area to empty cue */

   init.cue:proc(ptr);         /* initialize so all times are */
      dcl ptr fixed;           /* zeros, name is null         */
      write(mam) = ptr;        /* string                      */
      write(mal) = CUE.RLEN;
      write(md ) = CUE.NAME+1;
      write(mal) = cue.fin;
      write(md ) = 1;
      write(mal) = cue.fout;
      write(md ) = 1;
   end init.cue;

   Alt.Scsi.Ptr     = alloc.examount(1);               /* for cue alloc, seq info      */
   scsi.ptr         = alloc.examount(shr(Max.Cue.Record.Len+255,8));
   lod.cue.ptr      = alloc.examount(lod.cue.max/256); /* external cue trigger buffer  */
   dsp.cue.ptr      = alloc.examount(lod.cue.max/256); /* external envelope data buffer */

   cue.build.ptr    = alloc.examount(#.Of.DTD.Reels.To.Allocate * shr(Max.Cue.Record.Len+255,8));
   Scratch.Cue.Ptr1 = cue.build.ptr + 1*shr(Max.Cue.Record.Len+255,8);
   Scratch.Cue.Ptr2 = cue.build.ptr + 2*shr(Max.Cue.Record.Len+255,8);
   Scratch.Cue.Ptr3 = cue.build.ptr + 3*shr(Max.Cue.Record.Len+255,8);
/* Saved.Cur.Cue.Ptr= cue.build.ptr + 4*shr(Max.Cue.Record.Len+255,8); */
   Preview.Cue.Ptr  = cue.build.ptr + 5*shr(Max.Cue.Record.Len+255,8);

   /* Saved.Cur.Cue.Ptr represents CUE in clip/reel table */
   Saved.Cur.Cue.Ptr= cue.build.ptr + 6*shr(Max.Cue.Record.Len+255,8);

   cue.clip.ptr     = cue.build.ptr + 7*shr(Max.Cue.Record.Len+255,8);
   cue.clip.reel    = 1; /* initialize to select basic clipboard #1 */

   do i = 0 to #.Of.DTD.Reels.To.Allocate -1 ;
      call init.cue(Cue.Build.Ptr + i*(shr(Max.Cue.Record.Len+255,8)));
   end;

   call SET.DTD.CURRENT.REEL(0);  /* set current.cue.ptr for "CUE" */

   call PID(Cue.Place.Track, Num.Kbd.Tracks);

   AEE.Current.Project.Rate = 500;   /* init to 50.0 khz until DTD comes on line */

end lodinit;

call lodinit;

end lodmod;
