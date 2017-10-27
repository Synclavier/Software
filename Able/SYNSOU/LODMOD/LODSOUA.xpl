/* :SYNSOU:LODMOD:LODSOUA - D34, MAIN LOOP & routine to trigger cue synced to sequencer */

/*   02/12/92 - cj  - Reset sync mode on boot up, fixed trig/scrub */
/*   01/15/92 - cj  - New scrub/trig control                       */
/*   09/26/91 - cj  - Send envelope stuff to DSP during trig       */
/*   07/24/91 - cj  - Send editview xfade stuff to NUARCH          */
/*   06/25/91 - cj  - GPI improvements (a.k.a. NFL)                */
/*   03/20/91 - cj  - Sequenced bootload for better messages       */
/*   03/12/91 - cj  - Send config info over to DTD for DDSAD       */
/*   11/05/90 - cj  - Send Play.To information to DTD              */
/*   02/28/90 - cj  - redefined usage of LOD.LOADING for NU-ARCH   */
/*   04/26/89 - cj  - code to send event info to dtd in real time  */

/* $page - routines to communicate only with DSP dtd on D24 #3     */

reset.dsp.scsi:proc swapable;
	#if (0)
	dcl scsi.reset.time fixed;

		if inc.dtd=0 then return;
	
		call log.lod.error('Resetting SCSI/DSP port...');
	
		write(ScsiBus) = S$RST;                      /* issue reset       */
		scsi.reset.time=real.milliseconds;
	
		do while (real.milliseconds - scsi.reset.time) ilt 10;  
			interp_run_host_non_timer();
		end;
	
		write(ScsiBus) = 0;
		scsi.reset.time=real.milliseconds;
	
		do while (real.milliseconds-scsi.reset.time) ilt 200;
			call real.time.loop;
			interp_run_host_non_timer();
		end;
	#endif
end reset.dsp.scsi;

send.to.dsp.subroutine: proc(msgtype,out.len,msg.ptr) public swapable;   /* send message to dsp/lod */
   dcl (msgtype)         fixed;
   dcl (out.len)         fixed;
   dcl (msg.ptr)         fixed;
#if (0)
   dcl (new.ms.msb)      fixed;
   dcl (new.ms.lsb)      fixed;
   dcl (info.len)        fixed;

	return (-1);
	
		dcl (i,j,k,l,m)       fixed;
	
		if inc.dtd=0 then return (-1);
	
		/* procedure to adjust synclavier time base to DTD master clock */
	
		Adjust.Synclav.Timebase:proc (val);
			dcl val fixed;
	
			disable;                            /* make sure we are in internal */
			if  (Time.Base.Mode = 1)            /* sync and still playing.  Set */
			then do;                            /* global variable to be        */
				Synclav.Time.Base.Adjust = val;  /* processed by interrupt       */
				Time.Base.Mode           = 8;    /* routine                      */
			end;
			enable;
	
		end Adjust.Synclav.Timebase;
	
		send.again:;
	
		interp_set_scsi_id(interp_set_scsi_id_access_dtd, dsp.d24.#, dsp.lod.target.#);	
	
		write(ScsiSel)=S$SelectEnable \ Scsibits (dsp.d24.#);  /* select this d24 */
		write(ScsiBus) = 0;
		write(ScsiData)= 0;
	
		i=real.milliseconds;                          /* retry timer        */
		do while  ((read(ScsiBus )&S$BusMask )<>0)    /* check for bus bysy */
		or        ((read(ScsiData)&S$DataMask)<>0);   /* or reset needed    */
			if (real.milliseconds-i) igt 5000 then do; /* gonzo              */
				call reset.dsp.scsi;                    /* issue reset        */
				return (-1);                            /* not sent           */
			end;
			if run.syn<>0 then call real.time.loop;
			interp_run_host_non_timer();
			interp_set_scsi_id(interp_set_scsi_id_access_dtd, dsp.d24.#, dsp.lod.target.#);	
			write(ScsiSel)=S$SelectEnable \ Scsibits (dsp.d24.#);
			write(ScsiBus) = 0;
			write(ScsiData)= 0;
		end;
	
		interp_set_scsi_id(interp_set_scsi_id_access_dtd, dsp.d24.#, dsp.lod.target.#);	
		write(ScsiSel)  =S$SelectEnable \ Scsibits (dsp.d24.#);  /* select this d24 */
		write(ScsiData) =bits(dsp.lod.target.#);    /* single initiator   */
		write(ScsiBus)  =S$SEL;                     /* gnd SEL            */
	
		do i = 0 to 10; end;                        /* DSP responds quickly */
	
		if (read(ScsiBus)&S$BSY) = 0                /* if not found, check  */
		then do;
	
			i=real.milliseconds;                     /* retry timer        */
	
			do while (read(ScsiBus)&S$BSY)=0;        /* or not found       */
	
				if  ((dsp.running=0)                  /* if not running or  */
				and  (dsp.loading=0))                 /* loading            */
				or  ((real.milliseconds-i) igt 5000)  /* or dead for 5 sec  */
				then do;                              /* then give up       */
					write(ScsiBus) = 0;
					write(ScsiData)= 0;
					dsp.running    = 0;                   /* not running        */
					look.for.dsp   = 1;                   /* try to find one    */
					if msgtype<>1 then do;
						call log.lod.error('Timeout Error with DTD/DSP');
						New.Seq.Info = New.Seq.Info | 8;      /* new constants */
					end;
					return (-1);
				end;
				
				interp_run_host_non_timer();
			end;
		end;
	
		write(ScsiData)=0;                          /* release DATA       */
		write(ScsiBus )=0;                          /* release SEL        */
	
		do while (read(ScsiBus)&S$REQ)=0; end;      /* wait for his REQ   */	// won't take long...
	
		if msgtype=130 then do;                     /* send real time sync message  */
	
			write(ScsiWord) = 0;                     /* 3 bytes of pad plus */
			write(ScsiWord) = msgtype;               /* the message byte    */
	
			info.len = 0;                            /* assume no env info  */
	
			if (dsp.cue.len      <> 0)               /* set byte length     */
			or (env.control.bits <> 0)               /* of env info         */
			then info.len = 4 + dsp.cue.len + dsp.cue.len;
	
			if info.len = 0                          /* if no envelope info, send short message */
			then write(ScsiWord)=4+smpte.onoff+smpte.onoff;  /* byte length  */
			else write(ScsiWord)=6+info.len;         /* byte length          */
	
			/* wait for command phase to go away: */
	
			do while (read(ScsiBus)&S$C.D) <> 0; end;	// also won't take long
	
			disable;                                 /* send precise time    */
	
			i = 0;                                   /* init timeout timer   */
			do while (read(ScsiBus)&S$REQ)=0;        /* wait for his REQ     */
				i = i + 1;                            /* count                */
				if i = 0 then do;                     /* check limit          */
					enable;                            /* for our exit         */
					call reset.dsp.scsi;               /* issue reset          */
					dsp.running    = 0;                /* not running          */
					look.for.dsp   = 1;                /* try to find one      */
					New.Seq.Info = New.Seq.Info | 8;   /* new constants        */
					return (-1);                       /* not sent             */
				end;
			end;
	
			if ((dtd.is.scrubbing <> 0)              /* send current scrub   */
			or  (dtd.is.trigging  <> 0))
			then do;                                 /* position to DSP      */
				write(ScsiWord) = dtd.scrub.msb;      /* note this is a       */
				write(ScsiWord) = dtd.scrub.lsb;      /* sample # !!!!        */
	
				if smpte.onoff <> 0                   /* include empty smpte  */
				then write(ScsiWord) = 0;             /* word if needed       */
	
			end;
	
			else do;
				if   time.base.mode=7                 /* start time base      */
				then time.base.mode=1;                /* now that lod is up   */
				write(ScsiWord)=play.time.lsb;        /* to speed             */
				
				i = 0;
	
				load d16tim+i;                        /* get fraction into d16 */
	
				if smpte.onoff=0 then do;             /* non-smpte            */
					if res<0 then do;                  /* large phase error     */
						load (-res);                    /* means we are ahead    */
						mul samp.speed; mwait; div d16tim;
						write(ScsiWord)=play.time.acu-res;
					end;
					else do;
						mul samp.speed; mwait; div d16tim;
						write(ScsiWord)=play.time.acu+res;
					end;
				end;
				else do;                              /* smpte                */
					if res<0 then do;                  /* large phase error     */
						load (-res);                    /* means we are ahead    */
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
					write(ScsiWord)=SAMPLED.RATE;
				end;
			end;
	
			enable;
	
			if info.len <> 0                         /* send over fade-in/out    */
			then do;                                 /* info if any available    */
				if smpte.onoff = 0                    /* write empty SMPTE track  */
				then write(ScsiWord) = 0;             /* rate if needed           */
				write(ScsiWord) = env.control.bits;   /* write bits word          */
				write(ScsiWord) = env.num.of.envs;    /* write # of envelopes     */
				write(mam)=dsp.cue.ptr;               /* send over envelope info  */
				do i=1 to dsp.cue.len;                /* if needed                */
					write(ScsiWord)=read(mdi);
				end;
				env.control.bits = 0;                 /* toss info from our       */
				env.num.of.envs  = 0;                 /* buffer now that we       */
				dsp.cue.len      = 0;                 /* have sent it to DSP      */
			end;
		end;
	
		else do;                                    /* normal message     */
	
			write(ScsiWord) = 0;                     /* with 4 bytes of padding */
			write(ScsiWord) = msgtype;            
	
			if msgtype < 128
			then write(ScsiWord) = 0;                /* provide 6 bytes if no length field */
	
			else do;                                 /* data               */
				write(ScsiWord)=out.len;              /* length, bytes      */
				write(mam)=Msg.Ptr;                   /* get data           */
				do i=1 to shr(out.len+1,1);
					write(ScsiWord)=read(mdi);
				end;
			end;
		end;
	
		do while (read(ScsiBus)&S$REQ)=0; end;      /* wait for req       */
	
		i=read(ScsiByte);                           /* get reply          */
	
		if i>=128 then do;                          /* long message       */
	
			/* Handle real time scrolling info from the DTD:               */
	
			if i=230 then do;                        /* scan time base adjust */
				k=shr(read(ScsiWord),1);              /* get words to xfer     */
				j = (read(ScsiWord));                 /* get time base adj     */
				if lod.running = 0                    /* if we are only DTD    */
				then call Adjust.Synclav.Timebase(j);
				do j=4 to k by 2;
					l=read(ScsiWord);                  /* toss rest of info */
				end;
			end;
	
			else if i=231 then do;                   /* snarf off file name for bootload */
				Dsp.File.Name(0) = read(ScsiWord);   /* get length of file name in bytes */
				do j = 0 to Dsp.File.Name(0) - 1;    /* and get that many data bytes     */
					k = read(ScsiByte);
					if j < 64 then call pbyte(Dsp.File.Name, j, k&127);
				end;
				if Dsp.File.Name(0) IGT 64
				then Dsp.File.Name(0) = 64;
				if (dsp.loading&load.anything) = 0
				then dsp.loading = load.bootfile;
			end;
	
			else if i=128 then do;                   /* scan terminal msg  */
				j = read(ScsiWord);                   /* get byte len, including null, C string format */
				if j <> 0 then do;                    /* must be nonzero    */
					if j > 65 then do;                 /* if too long, limit */
						k = j - 64;                     /* get bytes to toss  */
						j = 64;                         /* get bytes to save  */
					end;
					else do;
						k = 1;                          /* toss 1 byte (null) */
						j = j - 1;                      /* save this many     */
					end;
					lod.message(0) = j;                /* save byte length   */
					do l = 0 to (j-1);                 /* get bytes          */
						call pbyte(lod.message, l, read(ScsiByte));
					end;
					if j then call pbyte(lod.message, j, 0); /* clean up */
					do l = 0 to (k-1);                 /* toss unneeded      */
						m = read(ScsiByte);
					end;
					call set.error(err.lod,lod.message);
				end;
			end;
	
			else do;                   /* toss erroneous message          */
				j=read(ScsiWord);       /* get byte length to toss         */
				do j=1 to shr(j+1,1);
					k=read(ScsiWord);
				end;
			end;
		end;
	
		else if i=4 then do;          /* from dtdds[: requesting new motion */
			new.motion = 1;
		end;
	
		else if i=8 then do;          /* 8 from nu-arch DTD              */
			if  ((dsp.loading&load.anything) = 0)
			and ( dsp.running                = 0)
			then dsp.loading = load.setrunning; /* then set running!      */
		end;
	
		return i;                     /* message type                    */

	#endif
end send.to.dsp.subroutine;

/* Check.For.SCSI.Reselect */

/* This routine is called to see if the LOD code is reselecting the */
/* synclavier at the current time.                                  */

/* pass the maximum size of your message array (or 0 if you don't   */
/* want to see the return message) and the message array.           */

/* This routine returns a true if we were reselected,  otherwise    */
/* it returns a false.   If we return a true,  then the returned    */
/* message will be in the array.   Word 0 will be the # of bytes    */
/* in the message,  followed by the message itself sitting at       */
/* word 1.                                                          */

Check.For.SCSI.Reselect: proc (maxlen, message) returns (fixed) PUBLIC swapable;
   dcl maxlen     fixed;   /* max length of message allowed */
   dcl message    array;   /* contents of message buffer    */
   dcl i          fixed;
   dcl j          fixed;
   dcl k          fixed;

   /* scan SCSI */
	interp_set_scsi_id(interp_set_scsi_id_access_dtd, lod.d24.#, lod.target.#);	
   
	write(ScsiSel) = S$SelectEnable \ Scsibits (lod.d24.#);  /* select this d24 */
   write(ScsiBus) = 0;   /* make sure we are not driving the */
   write(ScsiData)= 0;   /* bus                              */

   if  ((read(ScsiBus )&S$BusMask ) = S$SEL             )
   and ((read(ScsiData)&S$DataMask) = bits(syn.target.#))
   then do;

      /* ground busy to tell lod we are here */
      write(ScsiBus)=S$BSY;                    

      /* wait for selection to go away */
      do while ((read(ScsiBus)&S$BusMask)&S$Sel) <> 0;	// won't take long
      end;	

      /* get answer from lod */

      i = shr(read(ScsiWord),1);

      if maxlen<>0 then message(0) = shl(i,1);

      do j = 1 to i;
         k = read(ScsiWord);
         if j ILT maxlen
         then message(j) = k;
      end;

      /* clear bsy */
      write(ScsiBus)=0;

      return true;
   end;

   return false;

end Check.For.SCSI.Reselect;

/* $SUBTITLE - Send sequencer motion state record to LOD */

/* send.long.message */


/* send new info to Live Overdub Computer */
/* this routine is activated whenever     */
/* major stuff changes (start, stop,      */
/* track select change, etc.              */

send.long.message:proc public swapable;
dcl (i,j) fixed;
dcl AEE.Current.Project.Rate fixed external;

Send.Track.Info:proc(code);       /* send track head info     */
	dcl (code)   fixed;
	dcl (i,j,k)  fixed;

	write(mam) = Alt.Scsi.Ptr;     /* send track info to dtd   */
	write(md ) = code;             /* store code word          */

	do i = 0 to 254;
		write(mam) = TRK.HEAD;      /* point to trk head look up */
		write(mal) = i;             /* table for this track      */
		write(mam) = read(md);      /* point to thd (if one)     */
		write(mal) = code;          /* get desired entry         */
		j          = read(md);
		write(mam) = Alt.Scsi.Ptr;
		write(mal) = i+1;
		write(md ) = j;
	end;

	call Send.To.Lod.Subroutine(138,512,Alt.Scsi.Ptr);

	end Send.Track.Info;

	if inc.dtd=0 then return;

	if (time.base.mode <> 0)     /* clear dtd.is.scrubbing on any PLAY motion in case */
	then do;                     /* end of trig got lost...                           */
		dtd.is.scrubbing = 0;
		dtd.is.trigging  = 0;
	end;

	new.motion=0;                /* clear flag            */

	if new.cue.track.settings <> 0
	then do;
		new.cue.track.settings=0;

		/* send over track volume, Cue Routing  */

		if lod.running <> 0 then do;   /* only to DTD */
			call Send.Track.Info(THD.TVOL   ); 
			call Send.Track.Info(THD.CUE.OUT); 
		end;

	end;

	write(mam)=Alt.Scsi.Ptr;

				disable;
	/*  1 */	write(mdi) = play.time.msb;    /* look at play info     */
	/*  2 */	write(mdi) = play.time.lsb;
	/*  3 */	write(mdi) = play.time.acu;
	/*  4 */	write(mdi) = samp.speed;
				if (smpte.onoff   <>0)        /* smpte is on           */
				and (time.base.mode<>4)        /* not locked            */
	/*  5 */	then write(mdi) = shl(dtd.is.scrubbing,4) \ shl(dtd.is.trigging,5); /* no motion (except scrubbing) */
	/*  5 */	else write(mdi) = play\shl(recd,1)\shl(move,2)\shl(pnch,3)\shl(dtd.is.scrubbing,4)\shl(dtd.is.trigging,5);
				enable;
	/*  6 */	write(mdi) = LOD.PLAY.TRACKS;
	/*  7 */	write(mdi) = 0;                /* other play tracks    */
	/*  8 */	write(mdi) = 0;
	/*  9 */	write(mdi) = 0;
	/* 10 */	write(mdi) = LOD.RECD.TRACKS;
	/* 11 */	write(mdi) = 0;                /* other recd tracks    */
	/* 12 */	write(mdi) = 0;
	/* 13 */	write(mdi) = 0;
				if ((mark.button.msb<>0)\(mark.button.lsb ige zero.time))
				and (mark.button.disabled=0)
	/* 14 */	then write(mdi)=1;             /* mark button    active   */
	/* 14 */	else write(mdi)=0;             /* mark button in-active   */
	/* 15 */	write(mdi) = Mark.Button.Msb;  /* was first.play.time.msb */
	/* 16 */	write(mdi) = Mark.Button.Lsb;  /* was first.play.time.lsb */
	/* 17 */	write(mdi) = 0;                /* first.play.time.acu     */
	/* 18 */	write(mdi) = smpte.onoff;
	/* 19 */	write(mdi) = smpte.mode.rate;
	/* 20 */	write(mdi) = Lod.Punch.In.Tracks;
	/* 21 */	write(mdi) = 0;
	/* 22 */	write(mdi) = 0;
	/* 23 */	write(mdi) = 0;
	/* 24 */	write(mdi) = Delay.Note.And.Cue.Starts;
	/* 25 */	write(mdi) = Delay.Note.And.Cue.Time(lw#msb);
	/* 26 */	write(mdi) = Delay.Note.And.Cue.Time(lw#lsb);
	/* 27 */	write(mdi) = Play.Seq.To.A.Time;
	/* 28 */	write(mdi) = Seq.Play.To.Msb;
	/* 29 */	write(mdi) = Seq.Play.To.Lsb;
	/* 30 */	write(mdi) = AEE.Current.Project.Rate; /* for dsp option */
	/* 31 */	write(mdi) = 0;
	/* 32 */	write(mdi) = 0;

	#if (0)
		if dsp.running <> 0 then do;
			call Send.To.Dsp.Subroutine(131,64,Alt.Scsi.Ptr);	/* send motion info	*/
			dsp.update.time=real.milliseconds-lod.update.rate;	/* send update now	*/
		end;
	#endif
	
	 if lod.running <> 0 then do;
		 call Send.To.Lod.Subroutine(131,64,Alt.Scsi.Ptr);		/* send motion info	*/
		 lod.update.time=real.milliseconds-lod.update.rate;	/* send update now	*/
	 end;

end send.long.message;

/* $Page - Send object file/data file info to Direct-to-Disk */


dcl System.Info.Msb    fixed;    /* static variables used to load system */
dcl System.Info.Lsb    fixed;    /* info into lod in several pieces      */
dcl System.Info.Len    fixed;
dcl System.Info.Ctr    fixed;
dcl System.Info.Swap   fixed;

Send.System.Info.To.Lod: proc swapable;  /* called when lod.loading <> 0 */

   dcl lodfile  lit '''.LOD-7''';
   dcl ltabfile lit '''.LTAB-7''';
   dcl i           fixed;
   dcl j           fixed;
   dcl namebuf(48) fixed;
   dcl sectors     fixed;
   dcl words       fixed;

   if inc.dtd=0 then return;

   /* give priority to DSP to make error messages more intelligable:   */
   if dsp.loading <> 0
   then return;

   /* See if we should start sending over .lod-7 */

   if (lod.loading & load.lod7) <> 0
   then do;

      call COPY.STRING(lodfile,namebuf);
      i = file.search(namebuf, 2, -2, 0, 0); /* search for file on path and system devices */

      System.Info.Msb  = f#ms_sector;         /* SET FILE PARAMETERS ASIDE */
      System.Info.Lsb  = f#ls_sector;
      System.Info.Len  = f#ls_length;
      System.Info.Swap = 0;

      if i=0 then do;
         call log.lod.error('Multi-Track File ".LOD-7" missing');
         look.for.lod   = 0;
         lod.running    = 0;
         dtd.max.secs   = 0;
         lod.loading    = 0;
         return;
      end;

      lod.loading = load.senddata;          /* set up to send data over */

      /* see if object file contains a swap file */

      call ext.readdata(System.Info.Msb, System.Info.Lsb,
                        Alt.Scsi.Ptr, 0, 1, 0);

      write(mam) = Alt.Scsi.Ptr;           /* look at data read in  */
      write(mal) = 1;                      /* C#CONTAB              */
      write(mal) = read(md) + c#objlen;    /* get object length     */
      i          = shr(read(md) + 255,8);

      /* If file has swap code,  then send over only object code    */
      /* to loader rom.   Follow object by swap code immediately    */

      if i ILT System.Info.Len             /* if file contains more */
      then do;                             /* than object code...   */
         System.Info.Swap = System.Info.Len - i;
         System.Info.Len  = i;
         lod.loading = lod.loading \ load.sendswap;
      end;

      call log.lod.error('Loading Multi-Track Computer Memory...');

      write(mam)=Alt.Scsi.Ptr;
      write(mdi)=0;
      call Send.To.Lod.Subroutine(200,2,Alt.Scsi.Ptr);       /* send load address */

      System.Info.Ctr = 0;

      return;

   end;

   /* Check for the need to send over the frequency table */

   if (lod.loading & load.hzfile) <> 0
   then do;

      call COPY.STRING(ltabfile,namebuf);
      i = file.search(namebuf, 2, -2, 0, 0); /* search for file on path and system devices */

      System.Info.Msb  = f#ms_sector;         /* SET FILE PARAMETERS ASIDE */
      System.Info.Lsb  = f#ls_sector;
      System.Info.Len  = f#ls_length;
      System.Info.Swap = 0;

      if i=0 then do;
         call log.lod.error('Multi-Track File ".LTAB-7" missing');
         look.for.lod = 0;
         lod.running  = 0;
         dtd.max.secs = 0;
         lod.loading  = 0;
         return;
      end;

      /* set up to send over hz file */

      lod.loading = load.senddata \ load.dataishz \ load.setrunning;

      write(mam)=Alt.Scsi.Ptr;
      write(md )=0;
      call Send.To.Lod.Subroutine(210,2,Alt.Scsi.Ptr);  /* send load address */

      System.Info.Ctr = 0;

      return;

   end;

   /* Send over another sector if we have not loaded the */
   /* whole file                                         */

   if (lod.loading & load.senddata) <> 0
   then do;

      if System.Info.Ctr ILT System.Info.Len
      then do;
         call ext.readdata(System.Info.Msb, System.Info.Lsb,
                           Alt.Scsi.Ptr, 0, 1, 0);

         /* Send information from config area over to DTD to enable       */
         /* DDSAD configuration                                           */

         if  (System.Info.Ctr = 0)                     /* first sector    */
         and ((lod.loading & load.dataishz)  = 0)      /* not hz file     */
         and ((lod.loading & load.sendswap) <> 0)      /* able swap file  */
         then do;
            write(mam) = Alt.Scsi.Ptr;                 /* access data     */
            write(mal) = 1;                            /* it's config ptr */
            write(mal) = read(md) + c#offset;          /* pt to c#offset  */

            call copy.out(c#contab+c#offset, c#strdev-c#offset);
         end;

         if (lod.loading & load.dataishz) <> 0
         then call Send.To.Lod.Subroutine(211, 512, Alt.Scsi.Ptr); /* Hz. Table */
         else call Send.To.Lod.Subroutine(201, 512, Alt.Scsi.Ptr); /* Obj, Swap */

         call ADD16(1, loc(addr(System.Info.Msb)));   /* incr sector # */
         System.Info.Ctr = System.Info.Ctr + 1;       /* incr file pos */

         call real.time.loop;

         return;

      end;

      /* done with load of file */

      /* Send checksum/execute if end of object file load to original */
      /* DTD or to NU-ARCH DTD:                                       */

      if (lod.loading & load.dataishz) = 0
      then do;
         write(mam) = Alt.Scsi.Ptr;
         write(md ) = System.Info.Ctr;
         call Send.To.Lod.Subroutine(202,2,Alt.Scsi.Ptr);
         lod.target.# = 3;                 /* switch to id 3 after boot */
      end;

      /* Immediately send over swap file if loading original DTD */

      if (lod.loading & load.sendswap) <> 0
      then do;

         lod.loading = lod.loading xor load.sendswap;

         write(mam)=Alt.Scsi.Ptr;
         write(mdi)=0;
         call Send.To.Lod.Subroutine(203,2,Alt.Scsi.Ptr); /* prepare for swap file load */

         /* Set dsp sync mode appropriately if we are just rebooting */
         /* the DTD:                                                 */

         if (dtd.dsp.sync.mode <> 0)      /* if dsp is master        */
         then do;
            write(mam) = alt.scsi.ptr;    /* send message to DTD asking    */
            write(mdi) = 60;              /* it to sync to DSP.            */
            write(mdi) = dtd.dsp.sync.mode;
            call Send.To.Lod.Subroutine(144,4,Alt.Scsi.Ptr);
         end;

         System.Info.Len  = System.Info.Swap;
         System.Info.Swap = 0;
         System.Info.Ctr  = 0;

         return;

      end;

      /* clear bits */

      lod.loading = lod.loading and (not(load.senddata\load.sendswap\load.dataishz));
      lod.loading = lod.loading or  load.interrogate;

      /* do not return here - fall through to setrunning immediately */
      /* after sending the last part of the file over.               */

   end;

   /* Mark DTD as running when it is booted: */

   if (lod.loading & load.setrunning) <> 0
   then do;
      look.for.lod = 0;                 /* done with looking  */
      lod.running  = 1;                 /* now is running     */
      lod.loading  = load.interrogate;  /* done loading       */

      write(mam)=Alt.Scsi.Ptr;        /* send info for  */
      write(md) =ID.OF.THIS.VERSION;  /* software rev # */
      call Send.To.Lod.Subroutine(151,2,Alt.Scsi.Ptr);

      /* Enable for envelope information now if dsp is already running */

      if dsp.running <> 0 then do;
         write(mam) = Alt.Scsi.Ptr;    /* send info for  */
         write(mdi) = 57;
         call Send.To.Lod.Subroutine(144,2,Alt.Scsi.Ptr);
      end;

      if      (store.dtd.info=1) then call Send.To.Lod.Subroutine(12,0,Alt.Scsi.Ptr); /* song directory active */
      else if (store.dtd.info=2) then call Send.To.Lod.Subroutine(13,0,Alt.Scsi.Ptr); /* track directory active */

      new.motion=1;                   /* send motion info   */
      New.Seq.Info = New.Seq.Info | 8;/* new constants */

      /* set up track buttons for lod */

      i=0;
      do j=0 to 31;                   /* see if lod buttons there */
         write(mam)=tbut.ptr;         /* update track button table */
         write(mal)=j;
         if  ((read(md)&255)>=first.lod.track)  /* if live overdub tracks */
         and ((read(md)&255)< last.lod.track )  /* are already on button  */
         then i=1;                              /* panel then leave them  */
      end;                                      /* where they are         */

      if i=0 then do j=24 to 31;                /* else put them on       */
         write(mam)=tbut.ptr;                   /* buttons 25-32          */
         write(mal)=j;
         write(md)=(read(md)&"177400")\(first.lod.track+j-24);
      end;

      call display.track.buttons;
      new.dtd.info = new.dtd.info \ 2; /* new track information */
   end;

   if (lod.loading & load.interrogate) <> 0   /* follow file load by */
   then do;                                   /* a bunch of 'hellos' */
      lod.loading = shl(lod.loading,1);       /* to quickly find out */
      call send.to.lod(1,0);                  /* if he needs any     */
   end;                                       /* more files          */

end Send.System.Info.To.Lod;


/* Routine to bootload the DSP/DTD */

#if (0)
	dcl DSP.Info.Msb    fixed;    /* static variables used to load system */
	dcl DSP.Info.Lsb    fixed;    /* info into dtd/dsp in several pieces  */
	dcl DSP.Info.Len    fixed;
	dcl DSP.Info.Ctr    fixed;
#endif

Send.System.Info.To.Dsp: proc swapable;  /* called when dsp.loading <> 0 */
	#if (0)
		dcl i           fixed;
		dcl j           fixed;
		dcl namebuf(48) fixed;
		dcl sectors     fixed;
		dcl words       fixed;

		if inc.dtd=0 then return;
	
		/* See if we should get ready to send over a boot file to nu-arch */
	
		if (dsp.loading & load.bootfile) <> 0
		then do;
	
			i = file.search(Dsp.File.Name, 2, -2, 0, 0); /* search for file on path and system devices */
	
			Dsp.Info.Msb  = f#ms_sector;         /* SET FILE PARAMETERS ASIDE */
			Dsp.Info.Lsb  = f#ls_sector;
			Dsp.Info.Len  = f#ls_length;
			words         = f#words & 255;       /* get # of wrds in last sec */
			if words <> 0                        /* get 16 msb's of 24-bit    */
			then sectors = Dsp.Info.Len - 1;     /* # of words in file.       */
			else sectors = Dsp.Info.Len;
	
			if i=0 then do;
				call COPY.STRING('System file "',namebuf);
				call APPEND.TO.STR(namebuf,Dsp.File.Name);
				call APPEND.TO.STR(namebuf,'" is missing');
				call log.lod.error(namebuf);
				look.for.dsp = 0;
				dsp.running  = 0;
				dsp.loading  = 0;
				return;
			end;
	
			dsp.loading = load.senddata;
	
			call COPY.STRING('Loading system file "',namebuf);
			call APPEND.TO.STR(namebuf,Dsp.File.Name);
			call APPEND.TO.STR(namebuf,'"');
			call log.lod.error(namebuf);
	
			write(mam)=Alt.Scsi.Ptr;
			write(mdi)=0;
			write(mdi)=shr(sectors,7);
			write(mdi)=shl(sectors,9)\shl(words,1);
			call Send.To.Dsp.Subroutine(200,6,Alt.Scsi.Ptr);       /* send load address */
	
			Dsp.Info.Ctr = 0;
	
			return;
	
		end;
	
		/* Send over another sector if we have not loaded the */
		/* whole file                                         */
	
		if (dsp.loading & load.senddata) <> 0
		then do;
	
			if Dsp.Info.Ctr ILT Dsp.Info.Len
			then do;
				call ext.readdata(Dsp.Info.Msb, Dsp.Info.Lsb,
										Alt.Scsi.Ptr, 0, 1, 0);
	
				call Send.To.Dsp.Subroutine(201, 512, Alt.Scsi.Ptr); /* Object code */
	
				call ADD16(1, loc(addr(Dsp.Info.Msb)));   /* incr sector # */
				Dsp.Info.Ctr = Dsp.Info.Ctr + 1;          /* incr file pos */
	
				call real.time.loop;
	
				return;
	
			end;
	
			/* done with load of file */
	
			/* Send checksum/execute if end of object file load to DTD/DSP  */
	
			write(mam) = Alt.Scsi.Ptr;
			write(md ) = Dsp.Info.Ctr;
			call Send.To.Dsp.Subroutine(202,2,Alt.Scsi.Ptr);
	
			/* clear bits */
	
			dsp.loading = dsp.loading and (not(load.senddata\load.sendswap\load.dataishz));
			dsp.loading = dsp.loading or  load.interrogate;
	
			/* do not return here - fall through to setrunning immediately */
			/* after sending the last part of the file over.               */
	
			/* Special case - reset the DTD sync mode when we reboot the   */
			/* dsp addon:                                                  */
	
			if (dtd.dsp.sync.mode <> 0)         /* if dsp is master        */
			then do;
				if  (lod.running <> 0)           /* if lod is running       */
				or  (((lod.loading & load.senddata  ) <> 0)   /* or loading */
				and  ((lod.loading & load.sendswap  ) =  0))  /* swap or    */
				or  (( lod.loading & load.setrunning) <> 0 )  /* hz, or     */
				then do;                                      /* booted...  */
					write(mam) = alt.scsi.ptr;    /* send message to DTD asking    */
					write(mdi) = 60;              /* it to sync to DSP.            */
					write(mdi) = 0;
					call send.to.lod.subroutine(144,4,alt.scsi.ptr);
				end;
				dtd.dsp.sync.mode = 0;
			end;
		end;
	
		/* Mark DTD as running when it is booted: */
	
		if (dsp.loading & load.setrunning) <> 0
		then do;
			look.for.dsp = 0;                 /* done with looking  */
			dsp.running  = 1;                 /* now is running     */
			dsp.loading  = load.interrogate;  /* done loading       */
	
			write(mam)=Alt.Scsi.Ptr;          /* send info for  */
			write(md) =ID.OF.THIS.VERSION;    /* software rev # */
			call Send.To.Dsp.Subroutine(151,2,Alt.Scsi.Ptr);
	
			/* Enable for LOD envelope information now if it is already running */
	
			if lod.running <> 0 then do;
				write(mam) = Alt.Scsi.Ptr;    /* send info for  */
				write(mdi) = 57;
				call Send.To.Lod.Subroutine(144,2,Alt.Scsi.Ptr);
			end;
	
			new.motion=1;                     /* send motion info   */
			New.Seq.Info = New.Seq.Info | 8;  /* new constants */
	
		end;
	
		if (dsp.loading & load.interrogate) <> 0   /* follow file load by */
		then do;                                   /* a bunch of 'hellos' */
			dsp.loading = shl(dsp.loading,1);       /* to quickly find out */
			call send.to.dsp.subroutine(1,0,Alt.Scsi.Ptr);
		end;                                       /* more files          */

	#endif
end Send.System.Info.To.Dsp;

/* $page - allocate cues for direct to disk triggered playback */

allocate.cues:proc swapable;
   dcl (i,j,k,l,m,n,t)	fixed;
   dcl t2 (16)				fixed;
   dcl (sddd)				fixed;
   dcl (ptl )				fixed;

   sddd = dtd.cue#;                /* save dtd.cue# for screen code */

   if allocate.lod.cues = 2        /* see if all cue-list tracks    */
   then do;                        /* must be reallocated           */

      i = 0;                       /* if so,  loop over all timbres */

      do while i < par.numt;       /* loop over all timbers         */

         write(mam) = tim.head;    /* see if timbre head exists     */
         write(mal) = i;

         if read(md) <> 0          /* if timbre head exists, check  */
         then do;

            if tim.head.lookup(i, TIM.SYNTH.TYP+0) = TIM#CUE
            then do;
               call tim.head.store(i, tim.needs.cue.alloc, 1);
               j = tim.head.lookup(i, tim.kbdtab.ptr      +0);
               if j <> 0 then do;               /* if we have a kbd lookup */
                  write(mam)=tim.ptr + j - 1;   /* table (we had better!!) */
                  rpc 256;                      /* then zero it out        */
                  write(mdi) = 0;
               end;
            end;
         end;
         i = i + 1;
      end;
   end;

   allocate.lod.cues = 3;              /* set to 3 for loop     */
   i                 = 0;

   do while (i < par.numt)             /* loop for each timbre  */
   and      (dtd.max.secs <> 0)        /* as long as lod runs   */
   and      (allocate.lod.cues = 3);   /* and no restart needed */

      if tim.head.lookup(i,tim.needs.cue.alloc) <> 0
      then do;                                /* see if alloc needed */

         ptl = 0;

         next:;

         j=tim.head.lookup(i,tim.partial.pointers+ptl); /* get ptr to partial */
         k=num.params;                     /* rel ptr to first frame        */
         l=tim.head.lookup(i,tim.csem);    /* save csem to detect recall    */

         do while ( i                  < par.numt )  /* check for par.numt change      */
         and      ( dtd.max.secs       <>0        )  /* and the lod still runs         */
         and      ( allocate.lod.cues  = 3        )  /* and we don't have to restart   */
         and      ( l=tim.head.lookup(i,tim.csem) )  /* check for this timbre recalled */
         and      ( p.lookup(j       ) <>sup      )
         and      ( p.lookup(j+k     ) = mor      )  /* while there are more frames    */
         and      ((p.lookup(j+k+type) = cu.type )   /* of the correct type ,          */
         or        (p.lookup(j+k+type) = ev.type )); /* either cue name or event info  */

            t = p.lookup(j+k+type);           /* get record type handy   */

            if t = cu.type                    /* if cu name only         */
            then call p.lookup(j+k+cu.name);  /* then name is here       */
            else call p.lookup(j+k+ev.name);  /* else name is here       */

            do m=0 to 16;              /* get name   */
               t2(m)=read(mdi);
            end;

            dtd.cue# = 0;              /* assume no id is available   */

            if t2(0) <> 0 then do;     /* if name exists, allocate it */

               write(mam)=Alt.Scsi.Ptr;
               write(mdi)=0;           /* do not set up cue ptrs */

               do m=0 to 16;           /* send it                */
                  write(mdi)=t2(m);
               end;

               call Send.To.Lod.Subroutine(181,36,Alt.Scsi.Ptr);  /* get id# for this cue   */
            end;

            /* look up pointers again */
            /* in case things moved:  */

            j=tim.head.lookup(i,tim.partial.pointers+ptl);   /* look up pointer in case it moved */
            m=tim.head.lookup(i,tim.kbdtab.ptr      +ptl);   /* make sure we have a table        */

            if   ( i                  < par.numt )  /* check for par.numt change      */
            and  ( dtd.max.secs       <>0        )  /* and the lod still runs         */
            and  ( allocate.lod.cues  = 3        )  /* and we don't have to restart   */
            and  ( l=tim.head.lookup(i,tim.csem) )  /* check for this timbre recalled */
            and  ( p.lookup(j       ) <>sup      )
            and  ( p.lookup(j+k     ) = mor      )  /* while there are more frames    */
            and  ((p.lookup(j+k+type) = cu.type )   /* of the correct type ,          */
            or    (p.lookup(j+k+type) = ev.type ))  /* either cue name or event info  */
            then do;

               if (m <>0 )       /* still have look up table       */
               then do;
                  n=p.lookup(j+k+cu.key); /* actually cu.key or ev.key */
                  write(mam)=tim.ptr + m - 1;
                  write(mal)=n;
                  if t = cu.type             /* if this is simple cue  */
                  then write(md)=dtd.cue#;   /* name, then store id.   */
                  else write(md)=(-k);       /* else store pointer     */
               end;

               /* store id in partial area if event:                   */

               if t = ev.type
               then call p.store(j+k+ev.cue.id, dtd.cue#);

               k=k+p.lookup(j+k+clen); /* advance to next frame */

            end;

         end;

         if   (i                  < par.numt)    /* check for par.numt change      */
         and  (dtd.max.secs       <>0       )    /* and the lod still runs         */
         and  (allocate.lod.cues  = 3       )    /* and we don't have to restart   */
         and  (l=tim.head.lookup(i,tim.csem))    /* check for this timbre recalled */
         then do;
            ptl = ptl + 1;
            if ptl < num.partials then goto next;
            call tim.head.store(i,tim.needs.cue.alloc,0);
         end;
         else do;
            if   allocate.lod.cues = 3  /* if interrupted mid stream, start */
            then allocate.lod.cues = 1; /* over,  but may need to do ALL    */
         end;

      end;

      i=i+1;

   end;

   if   (i = par.numt)             
   and  (dtd.max.secs <> 0)
   and  (allocate.lod.cues = 3)
   then allocate.lod.cues = 0;

   dtd.cue# = sddd;

end allocate.cues;

/* $page - code and variables for d34 remote control interface     */

/* Examine D34 Inputs is called whenever there is a change in the  */
/* input state of a d34 input line.  This routine examines         */
/* the change, and sets a bit in one of the following public       */
/* variables if the change is significant (ie a button press)      */

dcl RemoteControlInputToggles   fixed public; /* bits for input toggles  */
dcl RemoteControlInputUpdates   fixed public; /* bits for input updates  */
dcl RemoteControlArmToggles     fixed public; /* bits for arm   toggles  */
dcl RemoteControlArmUpdates     fixed public; /* bits for arm   updates  */
dcl RemoteControlNewMotions     fixed public; /* bits for motion buttons */

dcl RemoteControlPunchIns       fixed public; /* tracks to punch in      */
dcl RemoteControlPunchOuts      fixed public; /* tracks to punch out     */

dcl debounce.time               lit '50'; /* debounce time, milliseconds */

dcl ignore.highs                fixed; /* for debouncing          */
dcl ignore.high.d34.bits        fixed;
dcl ignore.high.d35.bits        fixed;
dcl ignore.high.time            fixed;

dcl ignore.lows                 fixed; /* for debouncing          */
dcl ignore.low.d34.bits         fixed;
dcl ignore.low.d35.bits         fixed;
dcl ignore.low.time             fixed;

dcl new.d34.control.bits        fixed;  /* holds new values of bits scanned */
dcl new.d35.control.bits        fixed;  /* from d34/d35 input card          */

dcl active.d34.control.bits     fixed;  /* indicates software state         */
dcl active.d35.control.bits     fixed;  /* of associated input bits         */

dcl P.Screen.Active             fixed PUBLIC;
dcl Q.Screen.Active             fixed PUBLIC;
dcl Q.Screen.Ready              fixed PUBLIC;

DCL D34_CCC_BITS                FIXED EXTERNAL; /* D34 BITS LIT BY CCC (ON D34GPI) */
DCL D34_TRIGGER_BITS            FIXED EXTERNAL; /* D34 BITS LIT BY EXTERNAL TRIGGERS */


