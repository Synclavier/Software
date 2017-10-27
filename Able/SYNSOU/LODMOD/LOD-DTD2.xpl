/* LOD-DTD2  $TITLE  LOD Routines for Remote Box, AEE, and Event Processing

   Modified:
   12/06/91 - PF  - Split this file off from LOD-DTD

*/

/* Place Cue in Sequence - routine to place a complex      */
/* cue event in the sequencer.                             */

/* error conditions from place.cue.in.sequence match those */
/* returned from add.cue.to.list                           */

/* Code to place complex cue event from simple cue id:     */

PLACE.CUE.IN.SEQUENCE: proc (cueid,track,time,code) returns (fixed) public swapable;
   dcl cueid   fixed;
   dcl track   fixed;  /* absolute track # */
   dcl time    array;
   dcl code    fixed;  /* 0 = time is real time,  1 = time is sequence time */

   dcl event(event.record.size-1) fixed;
   dcl i       fixed;
   dcl nlen    fixed;
   dcl rlen    fixed;
   dcl clen    fixed;

   if DTD.MAX.SECS = 0 then return (-1);

   /* get default event to set up values correctly: */

   call Get.Default.Event(event, event.type.event); /* get defaults  */

   /* set up event record for place: */

   event(event.track#    ) = track;      /* on this track            */

   if code <> 0
   then call COPY32(time, loc(addr(event(event.time.msb))));
   else call Map.Real.Time.To.Sequence.Time(time,
                                            loc(addr(event(event.time.msb))));

   event(event.time.count) = 0;          /* use count of 0           */

   if FETCH.ENTIRE.DTD.CUE (cueid, Scsi.Ptr) = 0  /* get cue         */
   then return (-1);

   write(mam) = Scsi.Ptr;
   write(mal) = CUE.NAME;
   call COPY.IN(addr(event(event.cue.name)),Event.Cue.Max.Words);

   event(event.cue.id) = cueid;

   write(mam) = Scsi.Ptr;                 /* get first 256 words of  */
   call COPY.IN(addr(MISC.BUF(0)), 256);  /* cue for in time         */

   call SUB32(loc(addr(misc.buf(CUE.S.MSB))),
              loc(addr(misc.buf(EVENT.IN.S#.MSB))),
              loc(addr(event(event.in.msb))));
   call SUB32(loc(addr(misc.buf(CUE.E.MSB))),
              loc(addr(misc.buf(EVENT.IN.S#.MSB))),
              loc(addr(event(event.out.msb))));
   call SUB32(loc(addr(misc.buf(CUE.OFF.MSB))),
              loc(addr(misc.buf(EVENT.IN.S#.MSB))),
              loc(addr(event(event.mark.msb))));

   event(event.fade.in)  = misc.buf(CUE.FIN);
   event(event.fade.out) = misc.buf(CUE.FOUT);

   /* get caption */

   event(event.caption) = 0;     /* remove possible old caption */

   rlen       = misc.buf(CUE.RLEN);
   nlen       = shr(misc.buf(CUE.NAME)+3,1);   /* # of wrds in cue name  */
   if rlen igt CUE.NAME+nlen
   then do;
      clen  = misc.buf(CUE.NAME+nlen);       /* caption length, bytes */
      if clen <> 0 then do;
         write(mam) = Scsi.Ptr;
         write(mal) = CUE.NAME+nlen;
         call COPY.IN(addr(event(event.caption)),shr(clen+3,1));
      end;
   end;

   return Place.Event.In.Sequence(event);    /* put in sequence       */

end PLACE.CUE.IN.SEQUENCE;

/* $page - routines for cue directory access */

FETCH.NEXT.ALPHA.DTD.CUE: proc returns (fixed) public swapable;
   if DTD.MAX.SECS <> 0 then do;
      DTD.Cue# = 0;
      call Send.To.Lod(24,0);
      return DTD.Cue#;
   end;
end FETCH.NEXT.ALPHA.DTD.CUE;

FETCH.PREVIOUS.ALPHA.DTD.CUE: proc returns (fixed) public swapable;
   if DTD.MAX.SECS <> 0 then do;
      DTD.Cue# = 0;
      call Send.To.Lod(25,0);
      return DTD.Cue#;
   end;
end FETCH.PREVIOUS.ALPHA.DTD.CUE;

FETCH.NEXT.TIME.DTD.CUE: proc returns (fixed) public swapable;
   if DTD.MAX.SECS <> 0 then do;
      DTD.Cue# = 0;
      call Send.To.Lod(26,0);
      return DTD.Cue#;
   end;
end FETCH.NEXT.TIME.DTD.CUE;

FETCH.PREVIOUS.TIME.DTD.CUE: proc returns (fixed) public swapable;
   if DTD.MAX.SECS <> 0 then do;
      DTD.Cue# = 0;
      call Send.To.Lod(27,0);
      return DTD.Cue#;
   end;
end FETCH.PREVIOUS.TIME.DTD.CUE;

FETCH.NEXT.SYNC.DTD.CUE: proc returns (fixed) public swapable;
   if DTD.MAX.SECS <> 0 then do;
      DTD.Cue# = 0;
      call Send.To.Lod(31,0);
      return DTD.Cue#;
   end;
end FETCH.NEXT.SYNC.DTD.CUE;

FETCH.PREVIOUS.SYNC.DTD.CUE: proc returns (fixed) public swapable;
   if DTD.MAX.SECS <> 0 then do;
      DTD.Cue# = 0;
      call Send.To.Lod(32,0);
      return DTD.Cue#;
   end;
end FETCH.PREVIOUS.SYNC.DTD.CUE;

FETCH.SYNC.DTD.CUE: proc (sync.time) returns (fixed) public swapable;
   dcl sync.time array;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = sync.time(0);
      write(mdi) = sync.time(1);
      DTD.Cue# = 0;
      call Send.To.Lod(190,4);
      return DTD.Cue#;
   end;
end FETCH.SYNC.DTD.CUE;

/* for fetch.time.dtd.cue: */

/* times passed are 32-bit milliseconds with respect */
/* to the current project                            */

/* this routine is used to find the cue for the      */
/* record panel RETAKE function.   It is passed a    */
/* 32-bit millisecond time with respect to the       */
/* start of the current project                      */

FETCH.TIME.DTD.CUE: proc (intime) returns (fixed) public swapable;
   dcl intime array;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = intime(0);
      write(mdi) = intime(1);
      DTD.Cue# = 0;
      call Send.To.Lod(186,4);
      return DTD.Cue#;
   end;
end FETCH.TIME.DTD.CUE;

/* Fetch cue using absolute sample # as sort argument */
FETCH.ABS.TIME.DTD.CUE: proc (intime) returns (fixed) public swapable;
   dcl intime array;              /* **** SAMPLE #, NOT MILLISECONDS ***** */

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = intime(0);
      write(mdi) = intime(1);
      write(mdi) = 1;
      rpc 29;
      write(mdi) = 0;
      DTD.Cue# = 0;
      call Send.To.Lod(186,64);
      return DTD.Cue#;
   end;
end FETCH.ABS.TIME.DTD.CUE;

FETCH.NTH.ALPHA.DTD.CUE: proc (n) returns (fixed) public swapable;
   dcl n fixed;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;      
      write(mdi) = n;
      DTD.Cue# = 0;
      call Send.To.Lod(187,2);     /* Locate Nth alpha cue command */
      return DTD.Cue#;
   end;
   else return 0;
end FETCH.NTH.ALPHA.DTD.CUE;

FETCH.NTH.TIME.DTD.CUE: proc (n) returns (fixed) public swapable;
   dcl n fixed;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;      
      write(mdi) = n;
      DTD.Cue# = 0;
      call Send.To.Lod(188,2);     /* Locate Nth alpha cue command */
      return DTD.Cue#;
   end;
   else return 0;
end FETCH.NTH.TIME.DTD.CUE;

FETCH.NTH.SYNC.DTD.CUE: proc (n) returns (fixed) public swapable;
   dcl n fixed;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;      
      write(mdi) = n;
      DTD.Cue# = 0;
      call Send.To.Lod(191,2);     /* Locate Nth alpha cue command */
      return DTD.Cue#;
   end;
   else return 0;
end FETCH.NTH.SYNC.DTD.CUE;

FETCH.DTD.CUE.INDEX: proc (index) returns (fixed) public swapable;
   dcl index fixed;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = 29;
      call Send.To.Lod(144,2);
      write(mam) = SCSI.Ptr;
      write(mal) = index;
      return read(mdi);
   end;
end FETCH.DTD.CUE.INDEX;

FETCH.DTD.SYSTEM.INFO: proc public swapable;
   if DTD.MAX.SECS <> 0 then do;
      call Send.To.Lod(29,0);
   end;
end FETCH.DTD.SYSTEM.INFO;

SET.DTD.SCROLL.RANGE: proc (range) public swapable;
   dcl range fixed;  /* 0 - Whole disk  1 - Current project */

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = 28;
      write(mdi) = range;
      call Send.To.Lod(144,4);
   end;
end SET.DTD.SCROLL.RANGE;

/* This routine is used to control the status of the READY lights on the */
/* meter bridge.   If the state is set to 1, then any READY tracks will  */
/* show there ready light.   If the state is set to 0,  then any         */
/* ready tracks will not show there ready lights (means a TRACK might    */
/* be ready,  but the AEE/DTD/REMOTE BOX is not in a mode when it is     */
/* about to record.                                                      */

dcl DTD.Ready.Enabled     fixed   public;  /* keep track of state        */

SET.DTD.READY.LIGHT: proc (state) public swapable;
   dcl state fixed;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = 32;
      write(mdi) = state & 1;
      call Send.To.Lod(144,4);
      DTD.Ready.Enabled = state & 1;

      /* Create a system event when the RECORD READY status changes */
      /* for the DTD                                                */

      new.dtd.info = new.dtd.info \ 16;

   end;
end SET.DTD.READY.LIGHT;

FETCH.DTD.AUTO.ALLOCATE.TIME: proc (trks, stime, etime) public swapable;
   dcl trks   fixed;   /* pass track bits.  0 = use dtd ready tracks */
   dcl stime  array;   /* disk time to start recording returned here */
   dcl etime  array;   /* disk time to end recording returned here   */

   if (simulate_dtd) {
      stime(0) = 0;
      stime(1) = 0;
      etime(0) = 0;
      etime(1) = 50000;
      return;
   }

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = 30;
      write(mdi) = trks;
      write(mdi) = 0;
      write(mdi) = 0;
      write(mdi) = 0;
      write(mdi) = 0;
      write(mdi) = 0;
      write(mdi) = 0;

      DTD.Cue# = 0;

      call Send.To.Lod(144,16);

      if DTD.Cue# <> 0 then do;
         write(mam) = SCSI.Ptr;
         stime(0)    = read(mdi);
         stime(1)    = read(mdi);
         etime(0)    = read(mdi);
         etime(1)    = read(mdi);
      end;
   end;
end FETCH.DTD.AUTO.ALLOCATE.TIME;

/* for record dtd synced to seq:                     */

/* times passed are 32-bit milliseconds with respect */
/* to the current project                            */

RECORD.DTD.SYNCED.TO.SYNCLAV: proc (seq.time,disk.start.time,disk.stop.time,usetrkinfo,type,rehearse,trackbits,audiobits) public swapable;
   dcl seq.time         array;  /* pass 0,0 if immediate recording */
   dcl disk.start.time  array;
   dcl disk.stop.time   array;
   dcl usetrkinfo       fixed;  /* 0 = use ready tracks.  1 = use passed trackbits */
   dcl type             fixed;  /* 0 = manual.  1 = seq/punch in  2 = seq/auto allocate */
   dcl rehearse         fixed;  /* 0 = do it    1 = rehearse                            */
   dcl trackbits        fixed;  /* ready tracks to record on */
   dcl audiobits        fixed;  /* audio tracks to here      */

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;

      write(mdi) = type;
      write(mdi) = seq.time(0);  /* Sequencer start time */
      write(mdi) = seq.time(1);
      /* 3 */
      write(mdi) = 1;  /* destination time is disk time (not cue) */
      /* 4 */
      write(mdi) = 1;  /* for now, always construct cross fade    */
      /* 5 */
      write(mdi) = disk.start.time(0);
      write(mdi) = disk.start.time(1);
      /* 7 */
      write(mdi) = disk.stop.time(0);
      write(mdi) = disk.stop.time(1);
      write(mdi) = rehearse;
      write(mdi) = usetrkinfo;
      write(mdi) = trackbits;
      write(mdi) = audiobits;
      write(mdi) = 0;
      write(mdi) = 0;
      write(mdi) = 0;

      call Send.To.Lod(135,32);

      /* Create a system event when we think the armed state may */
      /* change:                                                 */

      new.dtd.info = new.dtd.info \ 16;

   end;
end RECORD.DTD.SYNCED.TO.SYNCLAV;

/* fetch.dtd.drive status sets up a two word array indicating the */
/* state of the direct to disk                                    */
/*                                                                */
/*   array(0) = 0 if not recording                                */
/*              1 if armed                                        */
/*              2 if recording (but not armed)                    */
/*              3 if recording and armed                          */
/*   array(1) = tracks we are recording on at the moment          */

FETCH.DTD.DRIVE.STATUS: proc (return.bits) public swapable;
   dcl return.bits  array;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = 31;
      call Send.To.Lod(144,2);  /* Fetch drive status */

      return.bits(0) = 0;
      write(mam) = SCSI.Ptr;
      if read(mdi) <> 0 then return.bits(0) = 1;                   /* armed */
      if read(mdi) <> 0 then return.bits(0) = (return.bits(0)\2);  /* recording */
      return.bits(1) = read(mdi);
   end;
   else do;
      return.bits(0) = 0;
      return.bits(1) = 0;
   end;
end FETCH.DTD.DRIVE.STATUS;

/* fetch sync point from auto-allocate recording            */

/* returns disk time = syncl time of disk (ie start of cue) */
/*         trig time = syncl time when we punched in        */

/* This routine is called once ALLOCATE MODE RECORDING      */
/* has started (it might already be over, but the           */
/* values are not valid until it has at least started       */
/* actually recording.  It returns A: where on the disk     */
/* the recording actually started (as a sequence time),     */
/* and where the sequence was at that time.                 */

/* It is used to set the sync field for a DTD cue after     */
/* doing allocate mode recording.                           */

FETCH.DTD.SYNC.POINT:proc (disktime,trigtime) public swapable;    /* convert to direct disk addresses */
   dcl (disktime) array;
   dcl (trigtime) array;

   if DTD.MAX.SECS <> 0 then do;       /* get disk time (ie start of allocated cue) */
      write(mam)=scsi.ptr;             /* and trig time (ie seq time we punched in at)  */
      write(md )=35;
      call send.to.lod(144,2);               
      write(mam)=scsi.ptr;
      disktime(0) = read(mdi);
      disktime(1) = read(mdi);
      trigtime(0) = read(mdi);
      trigtime(1) = read(mdi);
   end;
end FETCH.DTD.SYNC.POINT;


/* Set DTD Sync Point is only used when BLOCKING     */
/* cues during ALLOCATE MODE RECORDING.   Once the   */
/* recording has started,  a cue can be defined      */
/* before the recording has actually finished.       */
/* The sync points would have to be changed for the  */
/* next cue to be defined correctly.                 */

/* for set.dtd.sync.point                            */

/* times passed are 32-bit milliseconds with respect */
/* to the current project                            */

/* the time that is passed is the disk time of the   */
/* next cue that is being defined                    */

SET.DTD.SYNC.POINT: proc (newdisktime) public swapable; /* update sync point for next cue definition */
   dcl (newdisktime) array;

   if DTD.MAX.SECS <> 0 then do;
      write(mam)=scsi.ptr;      
      write(mdi)=36;
      write(mdi)=newdisktime(0);
      write(md )=newdisktime(1);
      call send.to.lod(144,6);
   end;
end SET.DTD.SYNC.POINT;


/* Create DTD auto allocate cue creates a simple cue and returns it */
/* in misc.buf.   It is normally used after allocate mode           */
/* recording,  but is really more general than the name implies.    */
/* The newly created cue (without a name or a caption) is           */
/* returned in misc.buf                                             */

CREATE.DTD.AUTO.ALLOCATE.CUE: proc(start.time, stop.time, drive.bits, sync.time) public swapable;
   dcl start.time   array;
   dcl stop.time    array;
   dcl drive.bits   fixed;
   dcl sync.time    array;
   dcl i            fixed;
   dcl in     (1)   fixed;
   dcl out    (1)   fixed;
   dcl sync   (5)   fixed;

   call Msec.To.DTD.Sample.#(Start.Time, CF#Time, SAMP.SPEED, AEE.Current.Project.Rate, AEE.Current.Project.Base, In);
   call Msec.To.DTD.Sample.#(Stop.Time , CF#Time, SAMP.SPEED, AEE.Current.Project.Rate, AEE.Current.Project.Base, Out);

   write("313") = addr(misc.buf(0)); /* set MR13 to beginning of misc.buf */
   rpc 256;
   write("373") = 0;                 /* fill array with zeros      */
   
   misc.buf(CUE.RLEN ) = CUE.NAME+1; /* set name as if null string */

   misc.buf(CUE.S.MSB) = In(0);
   misc.buf(CUE.S.LSB) = In(1);

   misc.buf(CUE.E.MSB) = Out(0);
   misc.buf(CUE.E.LSB) = Out(1);

   misc.buf(CUE.OFF.MSB) = In(0);
   misc.buf(CUE.OFF.LSB) = In(1);

   misc.buf(CUE.EDIT.IN.MSB) = In(0);
   misc.buf(CUE.EDIT.IN.LSB) = In(1);

   misc.buf(CUE.EDIT.OUT.MSB) = Out(0);
   misc.buf(CUE.EDIT.OUT.LSB) = Out(1);

   misc.buf(CUE.IN.S#.MSB) = In(0);
   misc.buf(CUE.IN.S#.LSB) = In(1);

   misc.buf(EVENT.IN.S#.MSB) = In(0);   /* set event relative time when */
   misc.buf(EVENT.IN.S#.LSB) = In(1);   /* recorded cue is saved.       */
   misc.buf(CUE.BITS       ) = misc.buf(CUE.BITS) \ bit4; /* defined    */

   misc.buf(CUE.FIN)          = 1;
   misc.buf(CUE.FOUT)         = 1;

   misc.buf(CUE.TRKS)         = drive.bits;

   /* Compute absolute SBITS of sync time for the sync field:           */

   call MSEC.TO.SMPTE(sync.time, cf#time, SAMP.SPEED, SM.MODE, loc(addr(SM.HRS)), sync);
   call SMPTE.TO.SBITS (sync, SM.MODE, loc(addr(misc.buf(CUE.SMPT.MSB))));
   misc.buf(CUE.SMPT.MODE) = SM.MODE;
   misc.buf(CUE.BITS     ) = misc.buf(CUE.BITS) \ bit3;  /* absolute smpte sync time */
end CREATE.DTD.AUTO.ALLOCATE.CUE;

/* for erase dtd drive area:                         */

/* times passed are 32-bit milliseconds with respect */
/* to the current project                            */

ERASE.DTD.DRIVE.AREA: proc (sel.bits,dstart,dend) public swapable;
   dcl sel.bits  fixed;   /* bits for which tracks  */
   dcl dstart    array;   /* start time in cur proj */
   dcl dend      array;   /* end time in cur proj or beyond (limited by eof by lod) */

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;

      write(mdi) = 6;
      write(mdi) = sel.bits;
      write(mdi) = 0;
      write(mdi) = 0;
      write(mdi) = 0;

      write(mdi) = dstart(0);  /* Start time */
      write(mdi) = dstart(1);

      write(mdi) = dend(0);    /* End   time */
      write(mdi) = dend(1);

      call Send.To.Lod(144,18);
   end;
end ERASE.DTD.DRIVE.AREA;

/* LOAD.DTD.TEMP.CUE is called to load an entire cue/reel into */
/* a temporary buffer in the DTD.   From this temporary buffer */
/* sections of the cue can be triggered                        */

/* NOTE:  IT MUST ALSO COPY THE CUE TO SCSI.PTR (for stm       */
/* transfer)                                                   */

LOAD.DTD.TEMP.CUE: proc (Cue.Ptr) PUBLIC swapable; 
   dcl cue.ptr fixed;    /* sector of ext memory */
   dcl i       fixed;   

   write(mam) = Cue.Ptr;
   write(mal) = CUE.RLEN;
   i          = read(md);

   if DTD.MAX.SECS <> 0 then do;
      call COPY.EXT.MEM(Cue.ptr,0,Scsi.ptr,0,i);
      call COMPUTE.ACTUAL.DTD.IN.SAMPLE.#(Scsi.Ptr);
      call Send.To.Lod(137,shl(i,1));
   end;  

end LOAD.DTD.TEMP.CUE;

/* Fetch.dtd.song.directory is used to fetch information for 10 entries */
/* in the song directory.   It is passed a song # (0 - 40) that tells   */
/* where in the song directory to start getting the information from.   */

/* if top.song is (-1),  then this command gets the information for     */
/* the current.song plus 9 songs after that (random numbers above       */
/* song 50).                                                            */

FETCH.DTD.SONG.DIRECTORY: proc (top.song) public swapable;
   dcl top.song fixed;

   if DTD.MAX.SECS <> 0 then do;
      write(mam) = SCSI.Ptr;
      write(mdi) = top.song;
      call Send.To.Lod(189,2);
   end;
end FETCH.DTD.SONG.DIRECTORY;

/* Routine to set the safe/ready status of a */
/* set of DTD tracks:                        */

SET.DTD.DRIVE.STATUS: proc (status,drive.bits) public swapable;
   dcl status     fixed;
   dcl drive.bits fixed;
   dcl i          fixed;

   /* STATUS:  0 - Safe
               1 - Ready
               2 - Lock
               3 - Unlock
   */

   if LOD.Running <> 0 then do;
      call Send.To.Lod(13,0);  /* Select track info */
      do i = 0 to 15;
         if (drive.bits&bits(i))<>0 then do;  /* Bit set */

            write(mam) = SCSI.Ptr;     /* select this track */
            write(mdi) = i;
            call Send.To.Lod(143,2);

            do case status;
               do;  /* Safe */
                  write(mam) = SCSI.Ptr;
                  write(mdi) = 0;
                  call Send.To.Lod(153,2);
               end;

               do;  /* Ready */
                  write(mam) = SCSI.Ptr;
                  write(mdi) = 1;
                  call Send.To.Lod(153,2);
               end;

               do;  /* Lock */
                  write(mam) = SCSI.Ptr;
                  write(mdi) = 15;
                  call Send.To.Lod(144,2);  
               end;

               do;  /* Unlock */
                  write(mam) = SCSI.Ptr;
                  write(mdi) = 14;
                  call Send.To.Lod(144,2);  
               end;
            end;

            /* Create a system event when the safe/ready status */
            /* of a track changes:                              */

            /* NOTE: change is reported in NEW SEQ INFO!!       */

            new.seq.info = new.seq.info \ 4096;

         end;
      end;
      if store.dtd.info = 1                      /* if sitting on O page */
      then call send.to.lod(12,0);               /* Select song dir      */
   end;
end SET.DTD.DRIVE.STATUS;


/* Routine to set the repro/input/auto/cue playback mode of a set */
/* of tracks:                                                     */

SET.DTD.DRIVE.MODE: proc (mode,drive.bits) public swapable;
   dcl mode        fixed;
   dcl drive.bits  fixed;
   dcl i           fixed;

   /* MODE:  0 - Repro
             1 - Input
             2 - Auto
             3 - Cue Playback
   */

   if LOD.Running <> 0 then do;

      call Send.To.Lod(13,0);  /* Select track info */

      do i = 0 to 15;
         if (drive.bits&bits(i))<>0 then do;  /* Bit set */

            write(mam) = SCSI.Ptr;    /* select a track  */
            write(mdi) = i;
            call Send.To.Lod(143,2);

            write(mam) = SCSI.Ptr;
            write(mdi) = mode;        /* Set to passed mode */
            call Send.To.Lod(154,2);
         end;

         /* Create a system event when the safe/ready status */
         /* of a track changes:                              */

         /* NOTE: change is reported in NEW SEQ INFO!!       */

         new.seq.info = new.seq.info \ 16384;

      end;
      if store.dtd.info = 1                      /* if sitting on O page */
      then call send.to.lod(12,0);               /* Select song dir      */
   end;
end SET.DTD.DRIVE.MODE;

/* Routine to assign a misc parameter to a particular DTD track: */

SET.DTD.DRIVE.PARAMETER: proc (drive,parm,value) public swapable;
   dcl drive   fixed;
   dcl parm    fixed;
   dcl value   fixed;

   /* Provide a sneak path to the Audio Event Editor so it does */
   /* not have to update every line of the Project Panel        */
   /* when one item is changed from the protocol:               */

   dcl Display.New.Project.Info fixed external;

   /* PARM: 0 - Input channel  (0=None 1=1A 2=1B 3=1C 4=1D 5=2A 6=2B etc.)
            1 - Gain * 10
            2 - Volume * 10
            3 - Pan (-50 to 50)
            4 - Crossfade
            5 - Doa routing
            6 - DDT routing
   */
            
   if LOD.Running <> 0 then do;

      Display.New.Project.Info = Display.New.Project.Info | bits(drive);

      call Send.To.Lod(13,0);   /* Select track info */
      write(mam) = SCSI.Ptr;
      write(mdi) = drive;
      call Send.To.Lod(143,2);  /* Set track # */
      write(mam) = SCSI.Ptr;
      write(mdi) = value;
      do case parm;
         call Send.To.Lod(145,2);  /* Input channel */
         call Send.To.Lod(147,2);  /* Gain          */
         call Send.To.Lod(148,2);  /* Volume        */
         call Send.To.Lod(149,2);  /* Pan           */
         call Send.To.Lod(150,2);  /* Crossfade     */
         call Send.To.Lod(155,2);  /* Doa routing   */
         call Send.To.Lod(156,2);  /* DDT routing   */
      end;
      if store.dtd.info = 1                      /* if sitting on O page */
      then call send.to.lod(12,0);               /* Select song dir      */

      /* Create a system event when a track parameter changes: */

      new.dtd.info = new.dtd.info \ 16384;

   end;
end SET.DTD.DRIVE.PARAMETER;


/* $page - Snarf Track Directory:                       */

/* Snarf Track Directory gets a copy of the DTD's       */
/* track directory and puts it in SCSI.PTR;             */

Snarf.Track.Directory: proc public swapable;

   write(mam) = scsi.ptr;    /* first initialize the data area */
   rpc 256;                  /* to all zeroes in case no one   */
   write(mdi) = 0;           /* is home.                       */

   if DTD.MAX.SECS = 0       /* if no DTD                      */
   then return;              /* then no go.                    */

   write(mam) = SCSI.Ptr;    /* get track directory            */
   write(mdi) = 39;          /* into scsi.ptr                  */
   call Send.To.Lod(144,2);

end Snarf.Track.Directory;

/* $page - send for dtd track modes                     */

/* send for dtd track modes looks up the input mode for each of the */
/* dtd tracks.   the current input mode is returned in the passed   */
/* array for each of the tracks.   The current track status (safe   */
/* or ready) is returned in the second array                        */

/* mode codes: (upper half)            */
/*       0 = repro                     */
/*       1 = input                     */
/*       2 = auto                      */
/*       3 = cue playback              */
/*                                     */
/* status codes: (lower half)          */
/*       0 = safe                      */
/*       1 = ready                     */

SEND.FOR.DTD.TRACK.MODES:proc (modes,statuses) PUBLIC swapable;
   dcl modes    array;
   dcl statuses array;
   dcl i        fixed;

   if LOD.Running = 0                      /* if no lod running, no trks */
   then return 0;                          /* are in input               */

   call Snarf.Track.Directory;             /* get copy of tdir           */

   do i = 0 to 15;                         /* check 16 tracks            */
      write(mam)   = SCSI.Ptr;
      write(mal)   = i*DTD.Track.Directory.Len+DTD.Track.Directory.Status;
      statuses(i)  = read(md) & 255;
      modes   (i)  = shr(read(md),8);
   end;

   return 1;
   
end SEND.FOR.DTD.TRACK.MODES;

/* Routines for current project selection: */

dcl SET.UP.AEE.DTD.GLOBALS proc recursive;

ASK.FOR.DTD.CURRENT.PROJECT: proc (project) public swapable;
   dcl project  fixed;

   if DTD.MAX.SECS <> 0 then do;

      write(mam) = SCSI.Ptr;
      write(mdi) = project;
      call Send.To.Lod(152,2);

      /* Set DTD.SONG# here in case we are using an old lod and      */
      /* we are approximating the first 10 projects:                 */

      dtd.song# = project mod 10;

      /* Set up global information right away - even through we will */
      /* probably see an event shortly that will re-compute          */
      /* everything again after all the error checking is complete.  */

      call SET.UP.AEE.DTD.GLOBALS;

   end;

end ASK.FOR.DTD.CURRENT.PROJECT;

SELECT.DTD.CURRENT.PROJECT: proc (project) public swapable;
   dcl project  fixed;

   if DTD.MAX.SECS <> 0 then do;

      call ASK.FOR.DTD.CURRENT.PROJECT(project);

      /* wait for project select to finish: */

      DTD.Cue# = 0;
      do while DTD.Cue# = 0;					// Wait for project to be selected.  Run lower level code (if run.syn...)
	  		chill.out.and.run.syn();			// also run mac.  allow return at DTask level.
         call Send.To.Lod(30,0);
      end;

      call SET.UP.AEE.DTD.GLOBALS;

      return true;
   end;
   return false;
end SELECT.DTD.CURRENT.PROJECT;

