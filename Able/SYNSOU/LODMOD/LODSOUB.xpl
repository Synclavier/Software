/* LODSOUB - MAIN LOOP & routine to trigger cue synced to sequencer */

/*   01/15/92 - cj  - New scrub/trig control                        */
/*   09/26/91 - cj  - Send envelope stuff to DSP during trig        */
/*   07/24/91 - cj  - Send editview xfade stuff to NUARCH           */
/*   06/25/91 - cj  - Created this file from lodsoua                */

/* Subroutine to set motion tallies and record  */
/* tallies only                                 */

Set.DTD.Motion.Tallies:proc PUBLIC swapable;
   dcl Armed.for.Protocol.DTD.Recording fixed external;
   dcl DTD.Ready.Enabled                fixed external;
   dcl Protocol.Recording.State         fixed external;
   dcl Protocol.Recording.Drives        fixed external;
   dcl Record.Group                     fixed external;
   dcl Record.Recording.State           fixed external;
   dcl Record.Recording.Bits            fixed external;
   dcl MOVE.SPEED                       fixed external;
   dcl j fixed;

   /* begin with check for end of play.  Must clear Lod.Punch.In.Tracks */
   /* when smpte drops out (etc)                                        */

   if (PLAY = 0) or (MOVE <> 0)    /* end of play or start of move - clear */
   then do;                        /* lod punch in tracks                  */
      if Lod.Punch.In.Tracks <> 0  /* basically, tell user we have stopped */
      then do;                     /* recording when smpte drops out       */
         Lod.Punch.In.Tracks = 0;
         New.Motion          = 1;
      end;
   end;

   /* Set tally lines for motion control */
   /* and record lines                   */

   j = 0;

   /* set motion tallies:                                         */

   IF  (PLAY<>0)
   AND (MOVE=0) THEN j = j \ (M$Play*256);    /* START LIGHT ON,  UNLESS FF/REW */
   IF (MOVE<>0) THEN DO;                      /* FF OR REW */
      IF MOVE.SPEED<0 
      THEN           j = j \ (M$Rew*256);     /* REWIND */
      ELSE           j = j \ (M$FF *256);     /* FAST FORWARD   */
   END;

   /* Now set the record tally lights (or clear the record flip-flops) */

   /* Begin by seeing if we are doing protocol DTD recording:          */

   if  (Armed.for.Protocol.DTD.Recording <> 0)  /* if armed for protocol recording */
   and (DTD.Ready.Enabled                <> 0)  /* and DTd is ready                */
   and (Protocol.Recording.State         <> 0)  /* and armed or recording          */
   then do;
      j = j \ 256 \ (Protocol.Recording.Drives & 255);
   end;

   /* Also see if we are doing AEE DTD recording:            */

   else if  (Q.Screen.Active        <> 0)  /* if AEE running */
   and      (Record.Group           <> 0)  /* rec pan there  */
   and      (DTD.Ready.Enabled      <> 0)  /* readied        */
   and      (Record.Recording.State <> 0)  /* recording      */
   and      (Record.Recording.Bits  <> 0)  /* on tracks      */
   then do;
      j = j \ 256 \ (Record.Recording.Bits & 255);
   end;

   /* Also see if we are doing punch in recording on the     */
   /* P screen or the Q screen:                              */

   else if ((P.Screen.Active     <> 0)       /* on P page   */
   or       (Q.Screen.Active     <> 0))      /* or Q page   */
   and     ( Lod.Punch.In.Tracks <> 0 )      /* recording   */
   then do;
      j = j \ 256 \ (Lod.Punch.In.Tracks & 255);
   end;

   /* Else if not doing any recording,  keep the flip-flops clear    */
   /* so that we can detect punch-ins:                               */

   else if (RemoteControlPunchIns  = 0)
   and     (RemoteControlPunchOuts = 0)
   and     ((RemoteControlNewMotions & M$MasterRecordIn ) = 0)
   and     ((RemoteControlNewMotions & M$MasterRecordOut) = 0)
   then do;
      if (inc.d34.remote.control.code<>0) /* force inclusion of d34 code   */
      or (d34gpi.there               <>0) /* remote control interface card in system */
      then write("34") = (d34_trigger_bits | "100000" | j); /* clear flip flops  */
   end;

   if (inc.d34.remote.control.code<>0) /* force inclusion of d34 code   */
   or (d34gpi.there               <>0) /* remote control interface card in system */
   then do;
      write("34") = (d34_trigger_bits | j);
      d34_ccc_bits = j;                /* remember holding these bits   */
   end;

end Set.DTD.Motion.Tallies;

/* $page - set dtd tally lines:                 */

/* this routine is called to quickly set the    */
/* tally lines coming out of the d34 control    */
/* interface.                                   */

/* It sets the tally lines to reflect the       */
/* current state of the machine                 */

Set.DTD.Tally.Lines: proc (Modes, Statuses) public swapable;
   dcl Modes              array;      /* pass current mode     for each track */
   dcl Statuses           array;      /* pass current statuses for each track */
   dcl DTD.Ready.Enabled  fixed external;

   dcl (i,j)     fixed;

   /* set tally lights quickly.  begin by computing tally display */
   /* for input/arm lines                                         */

   j = 0;

   if (Q.Screen.Active <> 0)   /* set tallies on either screen    */
   or (P.Screen.Active <> 0)
   then do i = 0 to 7;          /* only 8 tracks                 */

      if  (modes (i) = 1)                       /* 1 = input mode */
      and ((dtd.avail.trks&bits(i))<>0)         /* trk is avail   */
      then j = j\bits(i);        

      if  (  statuses          (i) = 1 )  /* if track is ready      */
      and ((dtd.avail.trks&bits(i))<>0 )  /* trk is avail           */
      and ((DTD.Ready.Enabled    <> 0)    /* and dtd enabled        */
      or   (P.Screen.Active      <> 0))   /* or P page  */
      then j = j\bits(i+8);     
   end;

   if (inc.d34.remote.control.code<>0) /* force inclusion of d34 code   */
   or (d34gpi.there<>0) then do;       /* remote control interface card in system */
      write("35") = j;
   end;

end Set.DTD.Tally.Lines;

/* Routine to initialize d34 control signals.  This routine is called       */
/* when we enter the P page and enter the Q page                            */

Initialize.Remote.Control.Items:proc public swapable;

   if (inc.d34.remote.control.code<>0) /* force inclusion of d34 code   */
   or (d34gpi.there<>0) then do;      /* remote control interface card in system */
      write("34") = (d34_trigger_bits | "100000"); /* pulse sr flip flop reset line  */
      write("34") = (d34_trigger_bits | "000000"); /*                                */
      write("35") = "000000";                      /* turn off all lights            */

      d34_ccc_bits = 0;               /* not holding anything */
      active.d34.control.bits   = 0;  /* clear our variables so we get  */
      active.d35.control.bits   = 0;  /* told about anything held now   */

      RemoteControlInputToggles = 0;  /* clear any button activity that */
      RemoteControlInputUpdates = 0;  /* clear any button activity that */
      RemoteControlArmToggles   = 0;  /* occurred while screen was      */
      RemoteControlArmUpdates   = 0;  /* occurred while screen was      */
      RemoteControlNewMotions   = 0;  /* inactive                       */
      RemoteControlPunchIns     = 0;
      RemoteControlPunchOuts    = 0;

      call Set.DTD.Motion.Tallies;    /* but light motion tallies now   */
   end;                               /* in case we just entered screen */

end Initialize.Remote.Control.Items;

/* Routine to process whatever remote control inputs we can process     */
/* from within check.next.event and run.syn.loop:                       */

Process.D34.Inputs: proc swapable;
   dcl modes    (15)       fixed;
   dcl statuses (15)       fixed;
   dcl info.rcvd           fixed;
   dcl new.motion.tallies  fixed;
   dcl new.info.tallies    fixed;
   dcl i                   fixed;

   dcl CONTINUE.SEQUENCER               proc                       external;
   dcl STOP.SEQUENCER                   proc                       external;
   dcl FAST.FORWARD.SEQUENCER           proc                       external;
   dcl REWIND.SEQUENCER                 proc                       external;
   dcl PLAY.INITIALIZE                  proc                       external;
   dcl STOP.DTD.PLAY.STATE              proc                       external;
   dcl Set.Sequencer.Mark.Start.Point   proc (fixed, fixed, fixed) external;
   dcl Record.Ready.Bits                fixed                      external;
   dcl DTD.PLAY.STATE                   fixed external;
   dcl SEND.FOR.DTD.TRACK.MODES         proc(array, array)         external;
   dcl SET.DTD.DRIVE.MODE               proc(fixed, fixed)         external;
   dcl SET.DTD.DRIVE.STATUS             proc(fixed, fixed)         external;
   dcl Record.Group                     fixed                      external;
   dcl DTD.Ready.Enabled                fixed                      external;

   dcl RT.MANUAL      lit '0';
   dcl RM.PUNCH.IN    lit '1';

   dcl Record.Mode    fixed external;
   dcl Record.Trigger fixed external;

   /* Check for new motion toggles  */
   /* Check in special order to get */
   /* useful results if quick       */
   /* presses                       */

   if (RemoteControlNewMotions <> 0)
   then do;

      if (RemoteControlNewMotions & M$STOP) <> 0   /* check for STOP   */
      then do;
         if Play <> 0                              /* stop seq         */
         then call STOP.SEQUENCER;

         else if DTD.Play.State <> 0               /* or dtd           */
         then call STOP.DTD.PLAY.STATE;

         RemoteControlNewMotions = RemoteControlNewMotions xor M$Stop;
      end;

      if (RemoteControlNewMotions & M$LOCATE) <> 0 /* check for LOCATE */
      then do;
         call STOP.SEQUENCER;            /* stop ff/rew motion */
         if Mark.Button.Disabled <> 0
         then call PLAY.INITIALIZE;
         else call SET.SEQUENCER.MARK.START.POINT(Mark.Button.Msb, Mark.Button.Lsb, 2);
         RemoteControlNewMotions = RemoteControlNewMotions xor M$Locate;
      end;

      if (RemoteControlNewMotions & M$Play) <> 0     /* check for PLAY     */
      then do;
         call CONTINUE.SEQUENCER;       /* continue sequencer */
         RemoteControlNewMotions = RemoteControlNewMotions xor M$Play;
      end;

      if (RemoteControlNewMotions & M$FF) <> 0   /* check for FF   */
      then do;
         call FAST.FORWARD.SEQUENCER;
         RemoteControlNewMotions = RemoteControlNewMotions xor M$FF;
      end;

      if (RemoteControlNewMotions & M$REW) <> 0   /* check for REW  */
      then do;
         call REWIND.SEQUENCER;
         RemoteControlNewMotions = RemoteControlNewMotions xor M$REW;
      end;

      new.motion.tallies = 1;
   end;


   /* Disable most functions if the DTD is not ready or if not on   */
   /* a DTD screen:                                                 */

   if (DTD.MAX.SECS = 0)
   or ((P.Screen.Active = 0) & (Q.Screen.Active = 0))
   then do;
      RemoteControlInputToggles = 0;
      RemoteControlInputUpdates = 0;
      RemoteControlArmToggles   = 0;
      RemoteControlArmUpdates   = 0;
      RemoteControlNewMotions   = 0;
      RemoteControlPunchIns     = 0;
      RemoteControlPunchOuts    = 0;
   end;

   /* Handle punch-in style recording here from both the P screen and */
   /* the Q screen:                                                   */

   if  ( P.Screen.Active   <> 0 )
   or  ((Q.Screen.Active   <> 0)
   and  (Record.Group      <> 0)
   and  (DTD.Ready.Enabled <> 0)
   and  (Record.Trigger    =  RT.MANUAL)
   and  (Record.Mode       =  RM.PUNCH.IN))
   then do;

      if (RemoteControlNewMotions & M$MasterRecordIn) <> 0   /* check for master record */
      then do;

         if (PLAY <> 0) & (Move = 0) then do;              /* only recognize while playing */
            Lod.Punch.In.Tracks = Record.Ready.Bits;
            New.Motion          = 1;        /* tell lod about it            */
            new.motion.tallies  = 1;
         end;

         RemoteControlNewMotions = RemoteControlNewMotions xor M$MasterRecordIn;
         RemoteControlPunchIns = 0;
      end;

      if (RemoteControlPunchIns <> 0)         /* check for indiv punch in */
      then do;

         RemoteControlPunchIns = RemoteControlPunchIns & DTD.AVAIL.TRKS & Record.Ready.Bits;

         do i = 0 to 15;

            if (RemoteControlPunchIns & bits(i)) <> 0
            then do;

               if (PLAY <> 0) & (move = 0) then do;  /* only recognize while playing */
                  Lod.Punch.In.Tracks = Lod.Punch.In.Tracks \ bits(i);
                  New.Motion          = 1;           /* tell lod about it            */
                  new.motion.tallies  = 1;
               end;

               RemoteControlPunchIns = RemoteControlPunchIns xor bits(i);
            end;
         end;
      end;

      if (RemoteControlNewMotions & M$MasterRecordOut) <> 0   /* check for master punch out */
      then do;
         Lod.Punch.In.Tracks = 0;           /* punch out on all tracks */
         New.Motion          = 1;           /* tell lod about it       */
         new.motion.tallies  = 1;

         RemoteControlNewMotions = RemoteControlNewMotions xor M$MasterRecordOut;
         RemoteControlPunchOuts = 0;
      end;

      if (RemoteControlPunchOuts <> 0)         /* check for indiv punch in */
      then do;

         RemoteControlPunchOuts = RemoteControlPunchOuts & DTD.AVAIL.TRKS;

         do i = 0 to 15;

            if (RemoteControlPunchOuts & bits(i)) <> 0
            then do;

               if PLAY <> 0 then do;              /* only recognize while playing */
                  Lod.Punch.In.Tracks = Lod.Punch.In.Tracks & (not(bits(i)));
                  New.Motion          = 1;        /* tell lod about it            */
                  new.motion.tallies  = 1;
               end;

               RemoteControlPunchOuts = RemoteControlPunchOuts xor bits(i);
            end;
         end;
      end;
   end;

   /* Handle changing/setting input/auto from all screens        */

   if (RemoteControlNewMotions & M$MasterInput) <> 0   /* check for master input function */
   then do;

      if info.rcvd = 0            /* send for modes only once        */
      then info.rcvd = SEND.FOR.DTD.TRACK.MODES (modes,statuses);

      if info.rcvd <> 0 then do;
         do i = 0 to 15;        /* check each track.  toggle mode  */
            if  (modes(i)                   <> 1)  /* if not in input       */
            and ((dtd.avail.trks & bits(i)) <> 0)  /* and trk is avail      */
            then do;
               RemoteControlInputUpdates = RemoteControlInputUpdates \ bits(i);
               modes(i)                  = 1;
            end;
         end;
         if DTD.Max.Secs <> 0 then do;   /* send all input command */
            write(mam) = Scsi.Ptr;       /* to perform function    */
            write(md ) = 17;             /* quickly                */
            call Send.To.Lod(144,2);
         end;
         new.info.tallies = 1;
      end;

      RemoteControlNewMotions = RemoteControlNewMotions xor M$MasterInput;

   end;

   if (RemoteControlNewMotions & M$MasterOutput) <> 0   /* check for master output function */
   then do;
      if info.rcvd = 0            /* send for modes only once        */
      then info.rcvd = SEND.FOR.DTD.TRACK.MODES (modes,statuses);

      if info.rcvd <> 0 
      then do;
         do i = 0 to 15;        /* check each track.  toggle mode  */
            if  (modes(i)                   <> 2)  /* if not in auto        */
            and ((dtd.avail.trks & bits(i)) <> 0)  /* and trk is avail      */
            then do;
               RemoteControlInputUpdates = RemoteControlInputUpdates \ bits(i);
               modes(i)                  = 2;
            end;
         end;
         if DTD.Max.Secs <> 0 then do;   /* send all auto  command */
            write(mam) = Scsi.Ptr;       /* to perform function    */
            write(md ) = 18;             /* quickly                */
            call Send.To.Lod(144,2);
         end;
         new.info.tallies = 1;
      end;

      RemoteControlNewMotions = RemoteControlNewMotions xor M$MasterOutput;

   end;
      
   /* handle toggles of input/repro mode,  or safe ready status: */

   if  (RemoteControlInputToggles <> 0)
   or  (RemoteControlArmToggles   <> 0)
   then do;

      if info.rcvd = 0            /* send for modes only once        */
      then info.rcvd = SEND.FOR.DTD.TRACK.MODES (modes,statuses);

      if info.rcvd = 0
      then do;
         RemoteControlInputToggles = 0;
         RemoteControlArmToggles   = 0;
      end;

      else do i = 0 to 15;        /* process each track, change mode */

         /* change input */

         if  (RemoteControlInputToggles&bits(i)) <> 0
         then do;
            if modes(i) = 1       /* if in input mode, set to AUTO so record switching happens */
            then do;
               call SET.DTD.DRIVE.MODE (2, bits(i));
               modes(i) = 2;
            end;
            else do;              /* else set to input mode          */
               call SET.DTD.DRIVE.MODE (1, bits(i));
               modes(i) = 1;
            end;
            RemoteControlInputUpdates = RemoteControlInputUpdates \ bits(i);
            RemoteControlInputToggles = RemoteControlInputToggles xor bits(i);
            new.info.tallies = 1;
         end;

         /* change safe/ready */

         if  (RemoteControlArmToggles&bits(i)) <> 0
         then do;
            call SET.DTD.DRIVE.STATUS  ((statuses(i)&1) xor 1, bits(i));

            RemoteControlArmUpdates = RemoteControlArmUpdates \ bits(i);
            RemoteControlArmToggles = RemoteControlArmToggles xor bits(i);
            new.info.tallies = 1;
         end;
      end;

   end;

   /* Set output tally lines if any changes were made:           */

   if new.motion.tallies <> 0
   then call Set.DTD.Motion.Tallies;

   if new.info.tallies <> 0
   then do;
      call SEND.FOR.DTD.TRACK.MODES (modes,statuses);  /* get correct info */
      call Set.DTD.Tally.Lines(modes, statuses);
   end;

   /* Set bit in new.seq.info for any items that cannot be       */
   /* processed here:                                            */

   if (RemoteControlInputToggles <> 0)
   or (RemoteControlInputUpdates <> 0)
   or (RemoteControlArmToggles   <> 0)
   or (RemoteControlArmUpdates   <> 0)
   or (RemoteControlNewMotions   <> 0)
   or (RemoteControlPunchIns     <> 0)
   or (RemoteControlPunchOuts    <> 0)
   then New.Seq.Info = New.Seq.Info \ 1024;

end Process.D34.Inputs;

Examine.D34.Inputs  :proc  swapable;
   dcl i fixed;

   /* begin by looking for button presses on */
   /* the appropriate input lines            */

   i = new.d35.control.bits              /* look for presses of these  */
     & (not(active.d35.control.bits));   /* input bits                 */

   RemoteControlInputToggles = RemoteControlInputToggles \ (i & 255);
   RemoteControlArmToggles   = RemoteControlArmToggles   \ (shr (i,8));

   i = shr(new.d34.control.bits              /* look for presses of these  */
     & (not(active.d34.control.bits)),8);    /* input bits                 */

   /* if pressing play while holding stop, then */
   /* map to locate button                      */

   if  ((i & M$Play) <> 0)                   /* play button pressed        */
   and ((new.d34.control.bits&(M$Stop * 256)) <> 0)
   then do;
      i = i xor (M$Play \ M$Locate);
   end;

   RemoteControlNewMotions   = RemoteControlNewMotions \ i;

   /* check release of record buttons so we can clear the */
   /* flop flops if we are not recording                  */

   if ((active.d34.control.bits  )
   &   (not(new.d34.control.bits))
   &   (shl(M$MasterRecordIn,8  ))) <> 0
   then RemoteControlNewMotions = RemoteControlNewMotions \ M$RecordRelease;

   i = new.d34.control.bits              /* get tracks to punch in on */
     & (not(active.d34.control.bits))
     & 255;
   RemoteControlPunchIns   = RemoteControlPunchIns \ i;

   i = (not(new.d34.control.bits))       /* and to punch out on       */
     & active.d34.control.bits
     & 255;
   RemoteControlPunchOuts   = RemoteControlPunchOuts \ i;

   /* acknowledge receipt of these commands to main loop software */
   /* by setting bits in active.control.bits variables.   It is   */
   /* now up to us to perform the function                        */

   active.d34.control.bits = new.d34.control.bits;
   active.d35.control.bits = new.d35.control.bits;

   /* Look further if input command is detected */

   if (RemoteControlInputToggles <> 0)
   or (RemoteControlInputUpdates <> 0)
   or (RemoteControlArmToggles   <> 0)
   or (RemoteControlArmUpdates   <> 0)
   or (RemoteControlNewMotions   <> 0)
   or (RemoteControlPunchIns     <> 0)
   or (RemoteControlPunchOuts    <> 0)
   then call Process.D34.Inputs;

end Examine.D34.Inputs;

dcl SUPPRESS_LOD_SCAN fixed public;

/* the following routines handle real scsi transactions */

get.scsi.command.byte: proc PUBLIC;
   dcl (i) fixed;

   // Assert REQ
   write(ScsiBus)=S$Command \ S$REQ;

   // Wait for ACK
   do while ((read(ScsiBus)&(S$ACK\S$ATN\S$RST))) = 0;
   end;

   // Get the data
   i=read(ScsiData)&S$DataMask;

   // Remove REQ
   write(ScsiBus)=S$Command;

   // Wait for !ack
   do while ((read(ScsiBus)&(S$ACK\S$ATN\S$RST))) = S$ACK;
   end;

   return i;

end get.scsi.command.byte;

get.scsi.command.word: proc PUBLIC;

   dcl (i) fixed;

   i=get.scsi.command.byte;

   return shl(i,8)\get.scsi.command.byte;

end get.scsi.command.word;

get.scsi.data.byte: proc PUBLIC;
   dcl (i) fixed;

   write(ScsiBus)=S$DataOut \ S$REQ;

   do while ((read(ScsiBus)&(S$ACK\S$ATN\S$RST))) = 0;
   end;

   i=read(ScsiData)&S$DataMask;

   write(ScsiBus)=S$DataOut;

   do while ((read(ScsiBus)&(S$ACK\S$ATN\S$RST))) = S$ACK;
   end;

   return i;

end get.scsi.data.byte;

send.scsi.data.byte: proc(b) PUBLIC;
   dcl (i,b) fixed;

   write(ScsiData)=b;
   write(ScsiBus)=S$DataIn \ S$REQ;

   do while ((read(ScsiBus)&(S$ACK\S$ATN\S$RST))) = 0;
   end;

   write(ScsiBus)=S$DataIn;

   do while ((read(ScsiBus)&(S$ACK\S$ATN\S$RST))) = S$ACK;
   end;

   write(ScsiData)=0;

end send.scsi.data.byte;

send.scsi.data.word: proc (b) PUBLIC;
   dcl b fixed;

   call send.scsi.data.byte(shr(b,8));
   call send.scsi.data.byte(b&255);

end send.scsi.data.word;

finish.processing.scsi: proc (status) PUBLIC;
   dcl status fixed;

   write(ScsiBus) = S$Status;       /* status phase */
   write(ScsiData) = status&255;
   write(ScsiBus) = S$Status | S$Req;
   do while (read(ScsiBus) & S$Ack) = 0;end;
   write(ScsiBus) = S$Status;                   /* release REQ */
   do while (read(ScsiBus) & S$Ack) <> 0;end;   /* wait for ACK to go false */

   write(ScsiBus) = S$MessIn;       /* message phase */
   write(ScsiData) = 0;             /* command complete */
   write(ScsiBus) = S$MessIn | S$Req;
   do while (read(ScsiBus) & S$Ack) = 0;end;
   write(ScsiBus) = S$Messin;                   /* release REQ */
   do while (read(ScsiBus) & S$Ack) <> 0;end;   /* wait for ACK to go false */

   write(ScsiData) = 0;                 /* release data lines */
   write(ScsiBus)=0;                    /* free the buss */

end finish.processing.scsi;

/* $page - process an actual scsi msg */

dcl scsi.comm.msb fixed public;
dcl scsi.comm.lsb fixed public;
dcl scsi.comm.len fixed public;
dcl scsi.comm.prt fixed public;

dcl Update.Laser.Index   proc         returns (boolean) external; /* tries to bring index file up to date with Optical disk */
DCL ADD.FILE.TO.CACHE    proc(FIXED ARRAY) returns (boolean) external; /* add file to cache */
DCL SETUP.CACHE          proc(fixed) EXTERNAL; /* set up a cache of all sound files on winchester disk */
dcl O$EntryCount         fixed external; /* no. directory entries on optical disk */

dcl scsi.poly.bin        fixed;
dcl scsi.poly.sec(1)     fixed;
dcl scsi.poly.wrd        fixed;

dcl LastSenseKey         fixed;
LastSenseKey=0;

process.scsi.msg: proc PUBLIC;
   dcl (i,j)           fixed;
   dcl scsi.command    fixed;
   dcl scsi.in.msg     fixed;
   dcl last.scsi.in.msg fixed;
   dcl lun             fixed;
   dcl data.buf(255)   fixed;
   dcl (msb,lsb)       fixed;
   dcl trk             fixed;
   dcl len             fixed;
   dcl status          fixed;
   dcl temp.base(1)    fixed;
   dcl saver0          fixed;
   dcl name.buf(127)   fixed;

   dcl vendor data   ('DSGITILA');
   dcl product data  ('yScnalivre      ');
   dcl revision data ('.5 0');

   scsi.in.msg = 0;                         /* init message byte */
   status = 0;                              /* init status byte */

   do while ((read(Scsibus)&S$SEL)<>0);     /* SEL release                 */
   end;

   if ((read(Scsibus)&S$ATN)<>0)            /* see if attention set        */
   then do;
   end;

   if ((read(Scsibus)&S$RST)<>0)            /* see if abort desired        */
   then do;
      write(ScsiBus)=0;                     /* done                        */
      return;
   end;

   write(ScsiBus)=S$Command;                /* start command phase */

   scsi.command = get.scsi.command.byte;

   if scsi.command = "H08" then do;          /* SCSI Receive Command */
      lun = shr(get.scsi.command.byte,5);    /* read in lun byte */
      i = get.scsi.command.byte;             /* skip mmsb len byte */
      len = get.scsi.command.word;           /* read lsb len only */
      i = get.scsi.command.byte;             /* read in control byte */

      write(ScsiBus)=S$DataIn;               /* go to data in phase */

      if last.scsi.in.msg = 3 then do;
         if (scsi.poly.bin < polynums) then do;
            call set.cur.bin(scsi.poly.bin);
            call copy32(scsi.poly.sec,temp.base);
            temp.base(lw#msb) = temp.base(lw#msb) \ shl(scsi.poly.bin,base#bin_p);
            call psmread(temp.base,0,scsi.poly.wrd);
            do i = 0 to len - 1;
/* ???????????
               write("313") = read(psd);
               insert ':sendword';
  sendword is lost, now using send.scsi.data.word - pf 12/1/94
?????????????? */
               call send.scsi.data.word(read(psd));
            end;
            call add16(len/256,scsi.poly.sec);
            scsi.poly.wrd = scsi.poly.wrd + (len mod 256);
            if (scsi.poly.wrd > 255) then do;
               call add16(1,scsi.poly.sec);
               scsi.poly.wrd = scsi.poly.wrd & 255;
            end;
         end;
         else status = S$CheckCondition;
      end;
      else if last.scsi.in.msg = 4 then do;
         if (scsi.poly.bin < polynums) then do;
            call set.cur.bin(scsi.poly.bin);
            call send.scsi.data.word(polynums);
            call send.scsi.data.word(psfirst.msb);
            call send.scsi.data.word(psfirst.lsb);
            call send.scsi.data.word(pslast.msb);
            call send.scsi.data.word(pslast.lsb);
            call send.scsi.data.word(psmax.msb);
            call send.scsi.data.word(psmax.lsb);
            call send.scsi.data.word(psfree.msb);
            call send.scsi.data.word(psfree.lsb);
         end;
         else status = S$CheckCondition;
      end;
      else status = S$CheckCondition;
   end;
   else if scsi.command = "H0A" then do;     /* SCSI Send Command */

      lun = shr(get.scsi.command.byte,5);    /* read in lun byte */
      i = get.scsi.command.byte;             /* skip msb len bytes */
      len = get.scsi.command.word;           /* read len (in bytes) */
      i = get.scsi.command.byte;             /* read in control byte */

      if (len > 256) then len = 256;         /* limit to our buffer size */

      write(ScsiBus) = S$DataOut;            /* switch to data out phase */
      
      do i = 0 to (len-1);
         data.buf(i) = get.scsi.data.byte;   /* read in command data bytes */
      end;

      scsi.in.msg = data.buf(0);             /* get our command type */
      
      if scsi.in.msg = 1
      then do;
         /* process commands below (after scsi bus is released) */
      end;
   end;
   else if scsi.command = "H12" then do;     /* SCSI Inquiry Command */
      lun = shr(get.scsi.command.byte,5);    /* read in lun byte */
      j = get.scsi.command.word;             /* skip reserved */
      i = get.scsi.command.byte;             /* get alloc length */
      j = get.scsi.command.byte;             /* skip control byte */

      data.buf( 0) = shl(3 ,8) or 0;          /* device type/qualifier */
      data.buf( 1) = shl(1 ,8) or 0;          /* ANSI approved/reserved */
      data.buf( 2) = shl(31,8) or 0;          /* addition length/unused */
      data.buf( 3) = shl(0 ,8) or 0;          /* unused/features */
      data.buf( 4) = vendor  (1);             /* vendor: 'Synclav' */
      data.buf( 5) = vendor  (2);
      data.buf( 6) = vendor  (3);
      data.buf( 7) = vendor  (4);
      data.buf( 8) = product (1);             /* product: 'Direct-to-Disk' */
      data.buf( 9) = product (2);
      data.buf(10) = product (3);
      data.buf(11) = product (4);
      data.buf(12) = product (5);
      data.buf(13) = product (6);
      data.buf(14) = product (7);
      data.buf(15) = product (8);
      data.buf(16) = revision(1);             /* revision: 7.0 */
      data.buf(17) = revision(2);

      if (i <> 0) then do;                    /* non-zero alloc length passed in */
         write(ScsiBus)=S$DataIn;             /* go to data in phase */
         j = 0;
         do while (j < i);                    /* send as many bytes as caller asked for */
            call send.scsi.data.byte(shr(data.buf(shr(j,1)),8));
            j = j + 1;
            if (j < i) then do;
               call send.scsi.data.byte(data.buf(shr(j,1)) & 255);
               j = j + 1;
            end;
         end;
      end;
   end;
   else if scsi.command = "H03" then do;     /* SCSI Request Sense */
      lun = shr(get.scsi.command.byte,5);    /* read in lun byte */
      j = get.scsi.command.word;             /* skip reserved */
      i = get.scsi.command.byte;             /* get alloc length */
      j = get.scsi.command.byte;             /* skip control byte */

      if i = 0 then i = 4;                   /* default 'scsi spec' */

      do j=0 to 255;data.buf(j)=0;end;

      data.buf(0) = "H70";
      data.buf(1) = 0;
      data.buf(2) = LastSenseKey;
      data.buf(3) = 0;

      write(ScsiBus)=S$DataIn;             /* go to data in phase */
      j = 0;
      do while (j < i);                    /* send as many bytes as caller asked for */
         call send.scsi.data.byte(data.buf(j));
         j = j + 1;
      end;
      LastSenseKey=0;
   end;
   else if scsi.command = "H00" then do;     /* SCSI Test Unit Ready */
      lun = shr(get.scsi.command.byte,5);    /* read in lun byte */
      j = get.scsi.command.word;             /* skip reserved */
      i = get.scsi.command.byte;             /* get alloc length */
      j = get.scsi.command.byte;             /* skip control byte */
   end;
   else do;
      LastSenseKey=5;
      status = S$CheckCondition;
   end;

   call finish.processing.scsi(status);

   if (scsi.in.msg = 1) then do;              /* add a file to cache */
      name.buf(0) = data.buf(1);              /* get length byte */
      do i = 0 to (name.buf(0) - 1);          /* build tree name in name.buf */
         call pbyte(name.buf,i,data.buf(2+i));
      end;
      if not ADD.FILE.TO.CACHE(name.buf)      /* try to add file to cache */
      then call SETUP.CACHE(false);           /* reset sound file cache */
   end;
   else if (scsi.in.msg = 2) then do;         /* update optical index */
      i = shl(data.buf(1),8) | data.buf(2);   /* read number of files added */
      O$EntryCount = O$EntryCount + i;        /* must increment this to get index to rebuild */
      call Update.Laser.Index;
   end;
   else if (scsi.in.msg = 3) then do;         /* set poly pointers */
      scsi.poly.bin = data.buf(1);
      scsi.poly.sec(0) = shl(data.buf(2),8) | data.buf(3);
      scsi.poly.sec(1) = shl(data.buf(4),8) | data.buf(5);
      scsi.poly.wrd    = data.buf(6);
   end;
   else if (scsi.in.msg = 4) then do;         /* set poly pointers */
      scsi.poly.bin = data.buf(1);
   end;

   last.scsi.in.msg = scsi.in.msg;

end process.scsi.msg;

MAIN.LOOP:PROC PUBLIC;                      /* TO SCAN LOD & MAIN LOOP */
   dcl (i) fixed;

	IF (0)                                 // ANALYZE RESPONSE
	{
		DCL KBD.MILLISECONDS	FIXED EXTERNAL;
		DCL (A,B) FIXED STATIC;
		A = A+1;
		IF (A == 1000)
		{
			SEND 'MAIN.LOOP ', KBD.MILLISECONDS - B;
			A = 0;
			B = KBD.MILLISECONDS;
		}
	}

	// Look for real time connection from Macintosh (e.g. S/Link)
	if (inc.slink) {
      if ((read("51") and ScsiBoard) != 0)
      {
         interp_set_scsi_id(interp_set_scsi_id_poll_host, 0, 7);
      
         disable;
      
         write(ScsiSel) = S$SelectEnable \ Scsibits (0);  /* Select d24 #0 */
         if  ((read(ScsiBus )&S$BusMask )=(S$SEL         ))
         or  ((read(ScsiBus )&S$BusMask )=(S$SEL or S$ATN))
         then do;
            
            /* check for selection from real scsi initiator (hard coded as ID 7  */
            
            if ((read(ScsiData)&S$DataMask)=(bits(syn.target.#) \ "H80"))
            then do;
               write(ScsiBus)=S$BSY;                    /* assert bsy   */
            
               enable;
            
               call process.scsi.msg;                   /* now read msg */
            end;
         end;
      
         enable;
      }
	}
	
   call real.time.loop;                     /* perform real time stuff */

   if  (inc.poly<>0)                        /* then check lod          */
   and (inc.mono =0)
   and (inc.dtd <>0)
   then do;

      /* Send new motion message to both lod and dtd/dsp: */

      if (new.motion<>0)             /* must send play time    */
      then do;
         if (lod.running <> 0)
         or (dsp.running <> 0)
         then call send.long.message;       /* send complete message  */
         else new.motion = 0;
      end;

      /* Check for loading of/looking for lod */

      if  lod.running = 0                   /* lod is not running: check */
      then do;                              /* for software/table load   */

         if (real.milliseconds - lod.update.time) ige lod.update.rate
         then do;

            lod.update.time=real.milliseconds;    /* set next time now */

            if (lod.loading <> 0)                 /* check for load of */
            then do;                              /* code or table     */
               call Send.System.Info.To.Lod;
               lod.update.time = real.milliseconds - lod.update.rate;
            end;

            else if (look.for.lod<>0) then do;  /* look for one           */

               if  (play              = 0) /* look for lod if not playing (preserve computer time during play if no lod) */
               and (suppress_lod_scan = 0) /* and not suppressed for STM */
               then do;
						interp_set_scsi_id(interp_set_scsi_id_access_dtd, lod.d24.#, lod.target.#);
                  write(ScsiSel) = S$SelectEnable \ Scsibits (lod.d24.#);  /* select this d24 */

                  if (read(ScsiSel) & Scsibits(lod.d24.#))=0 then do;      /* board does not exist */
                     lod.d24.#=(lod.d24.#+1)&1;                            /* try other d24        */
                  end;

                  // else check for reset
                  else if ((check.reset & Scsibits(lod.d24.#)) != 0) then do;
                     check.reset = check.reset XOR Scsibits(lod.d24.#);
                     
                     write(ScsiBus)  = 0;                            /* Clear scsi bus */
                     write(ScsiData) = 0;                    
         
                     if (((read(ScsiBus)&S$SigMask) != 0) || ((read(ScsiData)&0xFF) != 0))
                        reset.lod.scsi();
                  end;
                  
                  else do;
                     i = send.to.lod(1,0);    /* try to send 'HELLO'  */
                     if i < 0
                     then lod.d24.#=(lod.d24.#+1)&1;  /* try other d24      */
                  end;
               end;
            end;
         end;
      end;

      /* else if lod is up and running,  check to see */
      /* if we should send him some info              */

      else do;                               /* check for new motion  */
         
         if (lod.loading <> 0)                 /* check for load of   */
         then do;                              /* nu-arch files       */
            call Send.System.Info.To.Lod;
         end;

         else if (real.milliseconds - lod.update.time) ige lod.update.rate
         then do;                            /* send synchronize message */
            lod.update.time = real.milliseconds-(lod.update.rate-200);  /* approx 5 times/second */
            if suppress_lod_scan = 0         /* check for suppressed     */
            then do;                         /* for mono sampling...     */
               call send.to.lod(130,4);      /* send sync message to     */
					
					#if (0)
						if  (dsp.running <> 0)        /* lod.  Also send it to    */
						and (dsp.loading  = 0)        /* the DSP here if it is    */
						then do;                      /* running                  */
							dsp.update.time = real.milliseconds-(lod.update.rate-200);  /* approx 5 times/second */
							call send.to.dsp.subroutine(130,4,Alt.Scsi.Ptr);
							if ((dtd.is.scrubbing <> 0) /* send scrub position      */
							or  (dtd.is.trigging  <> 0))
							then do;                    /* to add-on every 50 msec  */
								lod.update.time = real.milliseconds-(lod.update.rate-50);
								dsp.update.time = real.milliseconds-(lod.update.rate-50);
							end;
						end;
					#endif
					
            end;
         end;

         if allocate.lod.cues<>0 then do;     /* allocate them       */
            if dtd.max.secs<>0                /* call when init      */
            then call allocate.cues;          /* is completed        */
         end;

         if stop.info.cues <> 0 then do;
            if dtd.max.secs<>0                
            then call Send.To.Lod(65,0);
            stop.info.cues = 0;
         end;

         /* $PAGE - scan inputs for DTD remote controls */

         if (inc.d34.remote.control.code <> 0) /* force inclusion of d34 code */
         or (d34gpi.there                <> 0) /* or d34 gpi board in system */
         then begin;                           /* scan d34 remote interface */
            dcl prior.d34    fixed;
            dcl prior.d35    fixed;
            dcl new.d34      fixed;
            dcl new.d35      fixed;
            dcl new.d34.bits fixed;
            dcl new.d35.bits fixed;

            /* save prior bits to perform button debounce function */
            /* quickly                                             */

            prior.d34 = new.d34;
            prior.d35 = new.d35;

            /* scan new buton values from inputs                   */

            new.d34 = read("34");
            new.d35 = read("35");

            /* Button debounce algorithm:                          */
            /*    1) ignore new highs as needed                    */
            /*    2) compute new highs                             */
            /*    3) set variables to ignore new lows              */
            /*    4) ignore lows as needed                         */
            /*    5) compute new lows                              */
            /*    6) set variables to ignore new highs             */

            if (new.d34      <> prior.d34)  /* if any change now   */
            or (new.d35      <> prior.d35)  /* or any recent       */
            or (ignore.highs <> 0        )  /* change, ...         */
            or (ignore.lows  <> 0        )
            then do;

/* 1) */       if ignore.highs <> 0 then do;
                  if (real.milliseconds - ignore.high.time) igt debounce.time
                  then do;
                     ignore.highs         = 0;
                     ignore.high.d34.bits = 0;
                     ignore.high.d35.bits = 0;
                  end;
                  else do;
                     new.d34 = new.d34 & (not(ignore.high.d34.bits));
                     new.d35 = new.d35 & (not(ignore.high.d35.bits));
                  end;
               end;

/* 2) */       new.d34.bits = new.d34 & (not(prior.d34)); /* get new highs */
               new.d35.bits = new.d35 & (not(prior.d35));

/* 3) */       if (new.d34.bits\new.d35.bits) <> 0        /* any new highs */
               then do;
                  ignore.lows = 1;
                  ignore.low.time = real.milliseconds;
                  ignore.low.d34.bits = ignore.low.d34.bits \ new.d34.bits;
                  ignore.low.d35.bits = ignore.low.d35.bits \ new.d35.bits;
               end;

/* 4) */       if ignore.lows <> 0 then do;
                  if (real.milliseconds - ignore.low.time) igt debounce.time
                  then do;
                     ignore.lows         = 0;
                     ignore.low.d34.bits = 0;
                     ignore.low.d35.bits = 0;
                  end;
                  else do;
                     new.d34 = new.d34 \ ignore.low.d34.bits;
                     new.d35 = new.d35 \ ignore.low.d35.bits;
                  end;
               end;

/* 5) */       new.d34.bits = prior.d34 & (not(new.d34)); /* get new lows */
               new.d35.bits = prior.d35 & (not(new.d35));

/* 6) */       if (new.d34.bits\new.d35.bits) <> 0        /* any new lows */
               then do;
                  ignore.highs = 1;
                  ignore.high.time = real.milliseconds;
                  ignore.high.d34.bits = ignore.high.d34.bits \ new.d34.bits;
                  ignore.high.d35.bits = ignore.high.d35.bits \ new.d35.bits;
               end;

               new.d34.control.bits = new.d34;
               new.d35.control.bits = new.d35;

               if (new.d34.control.bits <> active.d34.control.bits)
               or (new.d35.control.bits <> active.d35.control.bits)
               then do;
                  call Examine.D34.Inputs;

                  if  (new.motion  <> 0)    /* send new motion message immediately */
                  and (lod.running <> 0)    /* to accomplish punch-in quickly */
                  then call send.long.message;
               end;
            end;
         end;
      end;

      /* Similar code for DSP */
		#if (0)
	      if  dsp.running = 0                   		 /* lod is not running: check */
	      then do;                              		 /* for software/table load   */

	         if (real.milliseconds - dsp.update.time) ige lod.update.rate
	         then do;

	            dsp.update.time=real.milliseconds;    /* set next time now */

	            if (dsp.loading <> 0)                 /* check for load of */
	            then do;                              /* code or table     */
	               call Send.System.Info.To.Dsp;
	               dsp.update.time = real.milliseconds - lod.update.rate;
	            end;

	            else if (look.for.dsp<>0) then do;  /* look for one           */
	               if  (play              = 0) /* look for lod if not playing (preserve computer time during play if no lod) */
	               and (suppress_lod_scan = 0) /* and not suppressed for STM */
	               then call send.to.dsp.subroutine(1,0,Alt.Scsi.Ptr);    /* try to send 'HELLO'  */
	            end;
	         end;
	      end;

	      /* else if dtd/dsp is up and running,  check to see */
	      /* if we should send him some info                  */

	      else do;
	         if (dsp.loading <> 0)                 /* check for load of   */
	         then do;                              /* nu-arch files       */
	            call Send.System.Info.To.Dsp;
	         end;

	         /* Send out sync message to DSP here if LOD is not running.  */
	         /* Else send it out above so that EditView Xfade information */
	         /* gets sent out quickly.                                    */

	         else if (lod.running = 0)
	         and     ((real.milliseconds - dsp.update.time) ige lod.update.rate)
	         then do;                            /* send synchronize message */
	            dsp.update.time = real.milliseconds-(lod.update.rate-200);  /* approx 5 times/second */
	            if suppress_lod_scan = 0
	            then call send.to.dsp.subroutine(130,4,Alt.Scsi.Ptr);
	         end;
      	end;
      #endif

   end;
	
END MAIN.LOOP;

REBOOT.LOD:PROC PUBLIC SWAPABLE;

   if inc.dtd=0 then return;

   if lod.running<>0 then do;
      call Send.To.Lod.Subroutine(4,0,Alt.Scsi.Ptr);  
   end;
END REBOOT.LOD;

/* $page - store event information in sequence.                        */

/* Store real time event info is sued to store a record of information */
/* to send to the LOD in real time.   This record is used to trigger a */
/* cue or an event.  */

/* note: this is a NON-SWAP routine because it is used as we are       */
/* scanning the sequencer.                                             */

Store.Real.Time.Event.Info: proc (CueId, Time, Bitinfo, 
                                  #Wptr, Trkinfo, Eventinfo) PUBLIC;
   dcl CueId               fixed;   /* pass ID of cue,  or underlying cue */
   dcl Time                array;   /* sequencer time to trigger cue at   */
   dcl Bitinfo             fixed;   /* bits for cue.exists field          */
   dcl #Wptr               array;   /* pointer to #W1, #W2, #W3, #W4      */
   dcl Trkinfo             fixed;   /* 32768 + synclavier track # if cue  */
                                    /* is to be triggered with track      */
                                    /* volume and routing, else 0         */
   dcl Eventinfo           array;   /* pass loc(0) for cues,  else pass   */
                                    /* pointer to EV.IN.MSB - EV.SPARE5   */

   /* make sure there is room in BOTH the external memory buffer,  and    */
   /* in the LOD's Scsi.In.Buf:                                           */

   if lod.cue.len < (lod.cue.max - 32) then do;
      write(mam)=lod.cue.ptr + shr(lod.cue.len, 8);
      write(mal)=lod.cue.len;

      /* write out basic info for all cue triggers or events:             */

      write(mdi) = CueId;     /* write id# */
      write(mdi) = Time(0);   /* msb time  */
      write(mdi) = Time(1);   /* lsb time  */
      write(mdi) = Bitinfo;   /* bits      */
      write(mdi) = #Wptr(1);  /* #W2       */
      write(mdi) = #Wptr(2);  /* #W3       */
      write(mdi) = #Wptr(3);  /* #W4       */

      /* write out event-specific info if any:                            */

      if  (addr(Eventinfo(0)) <> 0)           /* if pointer is <> 0       */
      and (lod.version        >= 8)           /* and event software avail */
      then do;                                /* then send event info.    */
         write(mdi)=Trkinfo \ 16384;          /* trkinfo + "Event" bit    */
         write("300")=addr(Eventinfo(0));
         rpc (EV.MARK.MSB - EV.IN.MSB);       /* 4 words - in & out time  */
         write(mdi)=read("360");
         write("300")=addr(Eventinfo(EV.FADE.IN - EV.IN.MSB));
         rpc (EV.NAME - EV.FADE.IN);          /* 8 words - fades & spares */
         write(mdi)=read("360");
         lod.cue.len = lod.cue.len + (8 + (ev.mark.msb - ev.in.msb )
                                        + (ev.name     - ev.fade.in));
      end;
      else do;
         write(mdi)=Trkinfo;                  /* if no event info, just   */
         lod.cue.len = lod.cue.len + 8;       /* send possible track info */
      end;
   end;

end Store.Real.Time.Event.Info;


/* Manually.Trigger.Event.Or.Cue provides a convenient mechanism for */
/* triggering a cue or an event if you are not in very much of a     */
/* hurry.                                                            */

Manually.Trigger.Event.Or.Cue: proc (CueId,  CueTime, Bitinfo,
                                     Volinfo, Paninfo, Outinfo, Priinfo,
                                     Trackinfo, Eventinfo) public swapable;
   dcl Cueid        fixed;   /* pass ID of cue           */
   dcl CueTime      array;   /* time to trigger cue at   */
   dcl Bitinfo      fixed;   /* cue.exists bits          */
   dcl Volinfo      fixed;   /* volume info, 0 - rte.max */
   dcl Paninfo      fixed;   /* pan info (unknown fmt)   */
   dcl Outinfo      fixed;   /* output info              */
   dcl Priinfo      fixed;   /* priority info            */
   dcl Trackinfo    fixed;   /* 32768 + track#           */
   dcl Eventinfo    array;   /* event info               */

   DCL (W1,W2,W3,W4) FIXED;

   W1 = 1;                                        /* create a mock   */
   W2 = SHL(PRIINFO&"37",6)\SHL(OUTINFO&"37",11); /* note record     */
   W3 = VOLINFO;
   W4 = SHL(PANINFO,8);

   call Store.Real.Time.Event.Info (CueId,         /* cue id         */
                                    CueTime,       /* seq time       */
                                    Bitinfo,       /* cue.exists     */
                                    loc(addr(W1)), /* #W1 - #W4      */
                                    Trackinfo,     /* no track info  */
                                    Eventinfo);    /* no event info  */

   /* set timer to send cue stack over right now: */

   lod.update.time = real.milliseconds-lod.update.rate;

end Manually.Trigger.Event.Or.Cue;
