/*	:SYNSOU:LODMOD:LOD-CUE1  $TITLE  Routines for managing Cues on the Direct-to-Disk

Modified:
2002/08/25 - TY  - Modified wrap_24_hours() to be a no-op if (SAMP.SPEED ile 20)
2001/02/13 - TY  - Replaced the inneffective 24-hour wrap code
1991/11/18 - PF  - More quick update support
1991/07/29 - cj  - set new motion on project rate change so add-on gets it
1989/12/13 - MWH - Don't allow cue sync time to ever be "negative"
*/

/*	Cue Storage Areas:	*/

/*	# of reels allocated -	1 = cue build ptr		*/
/*									4 = clipboards			*/
/*									8 = user reels			*/
/*									3 = edit scratch		*/
/*									1 = copy of cur cue	*/
/*									1 = audition cue		*/

/*	NOTE: see literall "#.Of.DTD.Reels.To.Allocate"	*/

dcl Cue.Build.Ptr			fixed public;	/*	holds base to all allocated cue areas	*/

dcl Scratch.Cue.Ptr1		fixed public;	/*	work cue record buffer #1	*/
dcl Scratch.Cue.Ptr2		fixed public;	/*	work cue record buffer #2	*/
dcl Scratch.Cue.Ptr3		fixed public;	/*	work cue record buffer #3	*/
dcl Preview.Cue.Ptr		fixed public;	/*	for previewing an edit operation	*/

dcl Saved.Cur.Cue.Ptr	fixed public;	/*	copy of current cue; used when "audition during edit" function is available	*/

dcl Cue.Clip.Ptr			fixed public;	/*	pointer to clipboard in use	*/

dcl cue.clip.reel			fixed public;	/*	holds current clipboard reel index 0-11	*/
dcl previous.clip.reel	fixed public;	/*	holds previous clipboard reel index, 0 - 11	*/

/*	Variables to hold info about current cue:	*/

dcl Current.Cue.Reel				fixed public;	/*	holds current reel code, 0 - 11	*/
dcl Current.Cue.Ptr				fixed public;	/*	pointer that corresponds to current.cue.reel	*/

dcl Current.Cue.Protect			fixed public;
dcl Current.Cue.Drive.Bits		fixed public;
dcl Current.Cue.Mod				fixed public;
dcl Current.Cue.Audition		fixed public;
dcl Current.Sync.Locked			fixed public;
dcl Current.Cue.Rate				fixed public;
dcl Current.Cue.Sync.Mode		fixed public;

dcl Pending.Edit					fixed public;
dcl Previewing.Edit				fixed public;
dcl Scrub.Scan.Mode				fixed public;	/*	0 = scrub/point mode, 1 = scan/segment mode	*/

/*	the following static variables hold the actual times that	*/
/*	apply to the cue.  the displayed values are sometimes			*/
/*	quite different (duration vs. abs time, for example)			*/

/*	Note: all cue times are with respect to the current cue		*/
/*	mapping.																		*/

//	Understanding the use of the following seven time variables:
//		All seven are in real milliseconds scaled by SAMP.SPEED and include the zero.time offset
//		The first five are relative to the start of the Direct-to-Disk
//		The last two are relative to the start of the current sequence
dcl Cue.In.Time			(1)	fixed public;	/*	cue  in  time	*/
dcl Cue.Out.Time			(1)	fixed public;	/*	cue  out time	*/
dcl Cue.Edit.In.Time		(1)	fixed public;	/*	edit in  time	*/
dcl Cue.Edit.Out.Time	(1)	fixed public;	/*	edit out time	*/

dcl Cue.Offs.Time			(1)	fixed public;	/*	offset time		*/
//	Note: for now, Cue.Offs.Time and Cue.Edit.In.Time are latched together	*/

dcl Cue.Sync.Time			(1)	fixed public;	/*	sync time (corresponding to display) - Not stored in cue record	*/
dcl Cue.Trig.Time			(1)	fixed public;	/*	time to trigger cue		*/

/*	The Cue.Time.Display.Offset is added to most cue times for display	*/
/*	purposes.  It will (generally) map the cue in/out times so that		*/
/*	they are shown with respect to the start of the current project:		*/

dcl Cue.Time.Display.Offset	(1)	fixed PUBLIC;

dcl AEE.Time.Format						fixed PUBLIC;
dcl AEE.Shuttle.Anchor.Time	(1)	fixed PUBLIC;
dcl AEE.Shuttle.In.Time			(1)	fixed PUBLIC;
dcl AEE.Shuttle.Out.Time		(1)	fixed PUBLIC;
dcl AEE.Fine.In.Time				(1)	fixed PUBLIC;
dcl AEE.Fine.Out.Time			(1)	fixed PUBLIC;
dcl AEE.Cue.Blocking.Going.On			fixed public;
dcl Most.Recent.AEE.Activity			fixed public;

/*	Literals for AEE time format:	*/

dcl O#MSEC.Sec.Msec			lit '0';	/*	literals for O#MSEC.Format	*/
dcl O#MSEC.Beats				lit '1';	/*		 and O#MSEC.Disp.Format	*/
dcl O#MSEC.Measures.Beats	lit '2';
dcl O#MSEC.SMPTE				lit '3';
dcl O#MSEC.Feet.Frames		lit '4';
dcl O#MSEC.Minutes.Seconds	lit '5';

/*	$page - Routines to manage Cue Edit scrub bars						*/

/*	Scroll bars - The operating system manages a coarse and a fine	*/
/*	scrub bar that may be linked to the current cue. The following	*/
/*	routines perform these functions.										*/

/*	Set.DTD.Fine.Scrub.Bar:														*/

/*	This routine is called whenever the position and or times of	*/
/*	the coarse scroll bar is changed, or if the anchor time			*/
/*	changes.  It resets the fine bar centered around the new			*/
/*	anchor point (or the old anchor point if it did not change)		*/
/*	so the user is ready to scrub.											*/

/*	This routine returns a 1 if the times of the fine bar have		*/
/*	actually changed, else it returns a 0									*/

/*	Scrubbing.Scale.Table defines resolution of the cue scrubbing	*/
/*	movement.  They should match the switch settings of the AEE.	*/
/*	They currently provide .5, 2.0, and 5.0 seconds on the fine		*/
/*	bar:																				*/

dcl Scrub.Scale.Table data public (5, 20, 50);

Set.DTD.Fine.Scrub.Bar: proc (time,force) public swapable;
	dcl time			array;	/*	time to center fine bar around (anchor time)	*/
	dcl force		fixed;	/*	force change even if new time is on the		*/
									/*	fine bar, for example when changing the		*/
									/*	fine bar scale											*/
	dcl i				fixed;
	dcl in	(1)	fixed;
	dcl out	(1)	fixed;
	dcl tmp	(1)	fixed;

	/*	If the new time already exists on the fine bar, leave the	*/
	/*	fine bar settings alone.  Allows better waveshape displays.	*/

	if  (COM32(Time, AEE.Fine.In.Time )	=	lw#ilt)	/*	time is off			*/
	or  (COM32(Time, AEE.Fine.Out.Time)	>=	lw#ieq)	/*	scale					*/
	or  (force									<>	0		)	/*	or scale change	*/
	then do;

		call COPY32(AEE.Fine.In.Time,in);		/*	save range of fine bar	*/
		call COPY32(AEE.Fine.Out.Time,out);

		i = GID(Saved.AEE.Shuttle.Fine.Switch) + Fine.Switch.Default;
		i = scrub.scale.table(i);					/*	get .1 seconds in bar	*/
		load i; mul 50;								/*	get 1/2 sbar dist			*/
		bitmsb = ures; bitlsb = res;				/*	get milliseconds			*/
		call ratio_multiply(samp.speed,1000);	/*	scale for speed			*/
		call COPY32(loc(addr(bitmsb)),tmp);
		call ADD16(zero.time,tmp);
		if COM32(tmp,time) = lw#igt
		then call STR32(0,zero.time,AEE.Fine.In.Time);
		else do;
			call SUB32(time,tmp,AEE.Fine.In.Time);
			call ADD16(zero.time,AEE.Fine.In.Time);
		end;
		call SUB16(zero.time,tmp);
		call ADD32(AEE.Fine.In.Time,tmp,AEE.Fine.Out.Time);
		call ADD32(AEE.Fine.Out.Time,tmp,AEE.Fine.Out.Time);

		/*	If the times of the fine bar have changed, return a 1:	*/

		if (COM32(AEE.Fine.In.Time,in)	<> lw#ieq	/*	A time has changed	*/
		OR  COM32(AEE.Fine.Out.Time,out)	<> lw#ieq)
		then do;

			/*	Create a system event when the fine bar endpoints		*/
			/*	change:																*/

			new.dtd.info = new.dtd.info \ 1024;

			return 1;
		end;
	end;

	/*	Return 0 if the times did not change:	*/

	return 0;

end Set.DTD.Fine.Scrub.Bar;


/*	The operating system keeps track of 1 time called the Anchor	*/
/*	time.  This is normally the time that is indicated by the		*/
/*	"thumb box" of the coarse and fine scroll bars.						*/

Set.DTD.Scrub.Anchor.Point: proc (time) public swapable;
	dcl time	array;		/*	pass new anchor time							*/

	/*	Create a system event if the anchor time changes				*/

	if COM32(time, AEE.Shuttle.Anchor.Time) <> lw#ieq
	then new.dtd.info = new.dtd.info \ 512;

	/*	Store the new anchor time:												*/

	call COPY32(time, AEE.Shuttle.Anchor.Time);

	/*	Save as Current DTD time for take buttons as well:				*/

	call COPY32(time, Cur.DTD.Ms);

	/*	Prepare the fine bar for scrubbing at this anchor time:		*/

	call Set.DTD.Fine.Scrub.Bar(time,0);

end Set.DTD.Scrub.Anchor.Point;

/*	$page - send cue times or send project times to scrub bars				*/

/*	The following two routines are used to send either the current cue	*/
/*	times or the current project times to the coarse scroll bar.			*/

/*	Basically, Send.Cue.Times.To.DTD.Scrub.Bars is called whenever a		*/
/*	new current cue is called up.  If the coarse bar is in "CUE",			*/
/*	then it is set up to match the range of the cue and the anchor			*/
/*	time is set to the cue in time.													*/

/*	Send.Project.Times.To.DTD.Scrub.Bars is called whenever a new			*/
/*	project is called up.  If the coarse bar is in "PROJ", then				*/
/*	it is set up to scrub over the current project.								*/

Send.Cue.Times.To.DTD.Scrub.Bars:proc public swapable;
	dcl code		fixed;
	dcl i			fixed;
	dcl in(1)	fixed;
	dcl out(1)	fixed;
	dcl tmp(1)	fixed;

	/*	compute expanded times - allow 10 % overlap so start of cue			*/
	/*	can be adjusted																	*/

	call SUB32(Cue.Out.Time, Cue.In.Time, tmp);	/*	compute cue length	*/
	call SHR32(tmp,3);									/*	get 1/8 of cue			*/
	if COM32(tmp,Cue.In.Time) = lw#igt
	then call STR32(0,0,in);
	else call SUB32(Cue.In.Time,tmp,in);
	if COM16(zero.time,in) = lw#igt
	then call STR32(0,zero.time,in);
	call ADD32(Cue.Out.time,tmp,out);

	/*	copy times to coarse shuttle if it is	*/
	/*	locked to cue									*/

	i = GID(Saved.AEE.Shuttle.Coarse.Switch) + Coarse.Switch.Default;

	/*	if switch setting is cue, but cue variables are un-initialize		*/
	/*	(ie during startup) then sho project with coarse bar					*/

	if  (i=CS.Cue.Setting)
	and ((COM16(zero.time,Cue.In.Time )   = lw#igt)
	or   (COM16(zero.time,Cue.Out.Time)   = lw#igt)
	or   (COM32(Cue.In.Time,Cue.Out.Time) = lw#ieq))
	then i=CS.Proj.Setting;

	if i = CS.Cue.Setting		/*	coarse bar linked to cue	*/
	then do;
		call COPY32(In,AEE.Shuttle.In.Time);
		call COPY32(Out,AEE.Shuttle.Out.Time);
		call Set.DTD.Scrub.Anchor.Point(Cue.In.Time);

		/*	Force an update of the fine bar whenever a cue is called up		*/
		/*	since the audio waveform under the fine bar may change even		*/

		/*	if the times represented by it did not change:						*/

		call Set.DTD.Fine.Scrub.Bar(Cue.In.Time, 1);

		/*	Create a system event when the region of audio covered by the	*/
		/*	coarse scroll bar changes:													*/

		new.dtd.info = new.dtd.info \ 2048;

	end;

	/*	Else if coarse bar is not linked to the cue (ie HOLD or PROJ),		*/
	/*	just set the anchor to the cue in time:									*/

	else call Set.DTD.Scrub.Anchor.Point(Cue.In.Time);

end Send.Cue.Times.To.DTD.Scrub.Bars;

/*	Send project times to shuttle will send the project in/out times	*/
/*	to the shuttle bars if that is where they are linked					*/

Send.Project.Times.To.DTD.Scrub.Bars:proc (SetAnchor) public swapable;
	dcl SetAnchor	fixed;	/*	1 = set anchor time to project start	*/
	dcl i				fixed;
	dcl in(1)		fixed;
	dcl out(1)		fixed;

	i = GID(Saved.AEE.Shuttle.Coarse.Switch) + Coarse.Switch.Default;	/*	get switch setting	*/

	/*	if switch setting is cue, but cue variables are un-initialize	*/
	/*	(ie during startup) then sho project with coarse bar				*/

	if  (i=CS.Cue.Setting)
	and ((COM16(zero.time,Cue.In.Time )   = lw#igt)
	or   (COM16(zero.time,Cue.Out.Time)   = lw#igt)
	or   (COM32(Cue.In.Time,Cue.Out.Time) = lw#ieq))
	then i=CS.Proj.Setting;

	if i = CS.Proj.Setting	/*	coarse bar linked to proj	*/
	then do;

		call COPY32(AEE.Shuttle.In.Time,In);	/*	Save range of coarse bar	*/
		call COPY32(AEE.Shuttle.Out.Time,Out);

		/*	set shuttle in time to start of project	*/

		Call DTD.Sample.#.To.Msecs(AEE.Current.Project.Base, CF#Time, SAMP.SPEED,
											AEE.Current.Project.Rate, zero.zero, AEE.Shuttle.In.Time);
		Call DTD.Sample.#.To.Msecs(AEE.Current.Project.End,  CF#Time, SAMP.SPEED,
											AEE.Current.Project.Rate, zero.zero, AEE.Shuttle.Out.Time);

		/*	When first going to screen, set the anchor time to reflect	*/
		/*	the start of the current project:									*/

		if SetAnchor then do;
			call Set.DTD.Scrub.Anchor.Point(AEE.Shuttle.In.Time);
			call Set.DTD.Fine.Scrub.Bar(AEE.Shuttle.In.Time, 1);
		end;

		/*	Else just leave the current anchor in place:						*/

		else call Set.DTD.Fine.Scrub.Bar(AEE.Shuttle.Anchor.Time, 1);

		/*	Create a system event when the region of audio covered by the	*/
		/*	coarse scroll bar changes:													*/

		new.dtd.info = new.dtd.info \ 2048;

	end;

end Send.Project.Times.To.DTD.Scrub.Bars;

/*	$page - Routines to set & unset cue modified:	*/

Set.Cue.Modified:proc public swapable;
	if Current.Cue.Reel = 0
	then Current.Cue.Mod = True;
end Set.Cue.Modified;

UnSet.Cue.Modified:proc public swapable;
	if Current.Cue.Reel = 0
	then Current.Cue.Mod = False;
end UnSet.Cue.Modified;

/*	$page - compute cue trigger time	*/

/*	this routine computes the trigger time for the cue, based upon		*/
/*	the displayed sync time and the setting of the cue.offset.switch	*/
/*	and the offset time or length of the cue									*/

/*	the displayed sync time is the controlling time.  the trig time	*/
/*	is the time at which the start of the cue must be triggered			*/
/*	so that the desired synchronization (ie pre, offs, post) is			*/
/*	achieved																				*/

/*	Returns 0 if error (ie would have to trigger cue before start		*/
/*	of sequence to achieve desired sync)										*/

COMPUTE.CUE.TRIGGER.TIME:proc PUBLIC swapable;
	dcl temp (1)	fixed;

	do case (Current.Cue.Sync.Mode);						/*	branch on type			*/
		call STR32(0,0,temp);								/*	pre:	no offset		*/
		call SUB32(Cue.Offs.Time,Cue.In.Time,temp);	/*	offs:	sync offset		*/
		call SUB32(Cue.Out.Time, Cue.In.Time,temp);	/*	post:	sync cue end	*/
	end;

	call ADD16(zero.time,temp);							/*	for compare		*/

	/*	return an error code if we would need to trigger the cue			*/
	/*	before the start of the sequence to achieve the correct			*/
	/*	synchronization:																*/

	if COM32(Cue.Sync.Time,temp) = lw#ilt	//	<<== THIS SHOULD NEVER HAPPEN
	then do;

		/*	create an event if we are changing the cue trigger time		*/

		if COM16(zero.time, Cue.Trig.Time) <> lw#ieq
		then new.dtd.info = new.dtd.info \ 256;

		call STR32(0,zero.time,Cue.Trig.Time);
		return 0;	/*	DISPLAY.NOTICE (NT#SyncTimeError,ATTR#Reverse,NulStr,0,0,0);	*/
	end;

	/*	else compute the time to trigger the cue so that the				*/
	/*	desired synchronization is achieved:									*/

	call SUB16(zero.time,temp);
	call SUB32(Cue.Sync.Time,temp,temp);

	/*	create an event if we are changing the cue trigger time			*/

	if COM32(temp, Cue.Trig.Time) <> lw#ieq
	then new.dtd.info = new.dtd.info \ 256;

	call COPY32(temp,Cue.Trig.Time);

	return 1;

end COMPUTE.CUE.TRIGGER.TIME;

/*	Compute Cue Display Globals checks two things about the global		*/
/*	variables for the current cue:												*/

/*		1.	It makes sure all the global variables contains valid			*/
/*			data (ie no values before zero.time)								*/
/*		2.	It sees if the cue is in the current project and sets			*/
/*			up Cue.Time.Display.Offset accordingly.							*/

Compute.Cue.Display.Globals: proc swapable;
	dcl temp		 (1)	fixed;
	dcl basetime (1)	fixed;

	zcheck:proc(arr);
		dcl arr	array;

		if   (arr(0) ieq 0		  )
		and  (arr(1) ilt zero.time)
		then  arr(1)  =  zero.time;
	end zcheck;

	call zcheck(Cue.In.Time);
	call zcheck(Cue.Out.Time);
	call zcheck(Cue.Offs.Time);
	call zcheck(Cue.Edit.In.Time);
	call zcheck(Cue.Edit.Out.Time);
	call zcheck(Cue.Sync.Time);
	call zcheck(Cue.Trig.Time);

	/*	compute time display offset so times will generally appear		*/
	/*	relative to current project in most cases								*/

	call COMPUTE.ACTUAL.DTD.IN.SAMPLE.#(Current.Cue.Ptr);
	write(mam) = Current.Cue.Ptr;		/*	get actual sample # that goes	*/
	write(mal) = Cue.In.S#.Msb;		/*	with first audio					*/
	call COPY.IN(addr(temp(0)),2);

	/*	show times with respect to current mapping if current cue is	*/
	/*	before current project														*/

	if COM32(temp,AEE.Current.Project.Base) = lw#ilt
	then call STR32(0,0,Cue.Time.Display.Offset);

	/*	otherwise compute display offset that will yield time displays	*/
	/*	that are relative to start of current project						*/

	/*	description of offset: Cue.Time.Display.Offset is the sum		*/
	/*	of the following four numbers:											*/
	/*			1.	32 Bit Msec Time of Cue.In.S# rel to start of disk		*/
	/*			2.	- 32 Bit Msec Time of cur proj base rel to start		*/
	/*				of disk																*/
	/*			3.	- Cue.In.Time (32 Bit Msec Time of cue in point			*/
	/*				rel to start of map												*/
	/*			4.	zero.time															*/

	/*	If this number is added to a cue time (32 Bit Msec rel to		*/
	/*	start of map) it will yield the 32 Bit Msec time rel to the		*/
	/*	start of cur proj																*/

	else do;
		Call DTD.Sample.#.To.Msecs(AEE.Current.Project.Base, CF#Time, SAMP.SPEED,	/*	get msec from start of disk to	*/
											AEE.Current.Project.Rate, zero.zero, BaseTime);	/*	project base							*/
		Call DTD.Sample.#.To.Msecs(Temp, CF#Time, SAMP.SPEED,
											AEE.Current.Project.Rate, zero.zero, Temp);		/*	first audio segment					*/
		Call SUB32(Temp, BaseTime, Cue.Time.Display.Offset);								/*	this will be time to use for		*/
		Call SUB32(Cue.Time.Display.Offset, Cue.In.Time, Cue.Time.Display.Offset);	/*	cue.in.time.  Add offset to it	*/
		call ADD16(zero.time, Cue.Time.Display.Offset);
	end;

end Compute.Cue.Display.Globals;

/*	$page - Routine to select new current reel			*/

/*	This routine is called to switch the current reel	*/

SET.DTD.CURRENT.REEL:proc (reel) public swapable;
	dcl reel	fixed;

	if Current.Cue.Reel <> reel then		/*	create a system event	*/
	new.dtd.info = new.dtd.info \ 128;	/*	if the reel changes		*/

	Current.Cue.Reel = reel;

	if reel = 12 then
		Current.Cue.Ptr = Cue.Build.Ptr + (5*(shr(Max.Cue.Record.Len+255,8)));
	else
		Current.Cue.Ptr = Cue.Build.Ptr + ((reel+6)*(shr(Max.Cue.Record.Len+255,8)));

end SET.DTD.CURRENT.REEL;

/*	$page - routines to create/erase cue audition track	*/

dcl Audition.Cue.Id	fixed;

ERASE.CUE.AUDITION.TRACK: proc public swapable;
	dcl i					fixed;
	dcl tname(17-1)	fixed;
	dcl s					fixed;

	/*	do not create extraneous events when erasing or changing	*/
	/*	the cue audition track since this information is not		*/
	/*	available outside the operating system:						*/

	s = new.seq.info;
	call ERASE.TRACK(Cue.Audition.Track);	/*	zap track + timbre	*/
	new.seq.info = s;

	call COPY.STRING('.AUDIT.',tname);

	i = LOCATE.DTD.CUE(tname);					/*	zap cue def				*/
	do while i <> 0;
		call DELETE.DTD.CUE(i);
		i =  LOCATE.DTD.CUE(tname);
	end;

	Audition.Cue.Id = 0;

end ERASE.CUE.AUDITION.TRACK;

/*	place current cue on audition track	*/

PLACE.CUE.ON.AUDITION.TRACK:proc public swapable;
	dcl tname(17-1)			fixed;
	dcl i							fixed;
	dcl j							fixed;
	dcl new.name.word.len	fixed;
	dcl cue.length				fixed;
	dcl cue.seg.ptr			fixed;
	dcl prior.info				fixed;

	dcl LOAD.SEQ.GLOBALS				proc(fixed)	external;
	dcl BACKUP.TO.PRIOR.REAL.NOTE	proc			external;
	dcl ISA.NORMAL.OR.ILP.NOTE		proc			external;
	dcl ADVANCE.TO.NEXT.REAL.NOTE	proc			external;
	dcl REMOVE.NOTE.RECORD			proc			external;
	dcl (#ANYR,#ANYF)					fixed			external;

	call COPY.STRING('.AUDIT.',tname);	/*	get cue name	*/

	/*	copy information from the current reel to	*/
	/*	a temporary buffer so we can stuff the		*/
	/*	.AUDIT. name into it								*/

	/*	first look up some basic cue information	*/

	write(mam)	= Current.Cue.Ptr;		/*	get length and seg ptr	*/
	write(mal)	= CUE.RLEN;					/*	info for the current		*/
	cue.length	= read(md);					/*	cue							*/
	write(mal)	= CUE.LIST.PTR;
	cue.seg.ptr	= read(md);

	/*	copy basic cue info (before name) into place:	*/

	call COPY.EXT.MEM(Current.Cue.Ptr,0,Cue.Build.Ptr,0,CUE.NAME);

	/*	copy new name into place	*/

	new.name.word.len = 1+shr(tname(0)+1,1);

	write(mam) = Cue.Build.Ptr;
	write(mal) = CUE.NAME;
	do i=0 to new.name.word.len-1;		/*	substitute default	*/
		write(mdi)=tname(i);					/*	name						*/
	end;
	write(mdi) = 0;							/*	and no caption			*/

	if cue.seg.ptr <> 0 then do;			/*	copy segment list		*/
		call COPY.EXT.MEM(Current.Cue.Ptr,cue.seg.ptr,
								Cue.Build.Ptr,CUE.NAME+new.name.word.len+1,
								cue.length - cue.seg.ptr);
		write(mam) = Cue.Build.Ptr;
		write(mal) = CUE.LIST.PTR;
		write(md ) = CUE.NAME + new.name.word.len + 1;
		write(mal) = CUE.RLEN;
		write(md ) = CUE.NAME + new.name.word.len + 1 + (cue.length - cue.seg.ptr);
	end;
	else do;
		write(mam) = Cue.Build.Ptr;
		write(mal) = CUE.RLEN;
		write(md ) = CUE.NAME + new.name.word.len;
	end;

	if Audition.Cue.Id = 0 then do;
		Audition.Cue.Id = DEFINE.ENTIRE.DTD.CUE(Cue.Build.Ptr);

		/*	If could not save cue - bomb out here	*/

		if Audition.Cue.Id < 0 then do;
			Audition.Cue.Id = 0;
			new.seq.info = prior.info;
			return;
		end;

	end;
	else do;
		call Replace.Entire.DTD.Cue(Audition.Cue.Id,Cue.Build.Ptr);
	end;

	/*	quickly delete prior note if there is one	*/
	/*	on the track										*/

	prior.info = new.seq.info;							/*	do not generate events when	*/
																/*	changing the audition track	*/

	if LOAD.SEQ.GLOBALS(Cue.Audition.Track) then do;
		do while #anyr<>0;								/*	back up to start	*/
			call BACKUP.TO.PRIOR.REAL.NOTE;			/*	of track				*/
		end;
		if #anyf<>0 then do;								/*	if any notes		*/
			if ISA.NORMAL.OR.ILP.NOTE=0				/*	find first normal	*/
			then call ADVANCE.TO.NEXT.REAL.NOTE;	/*	note					*/
			if #anyf<>0										/*	if one found,		*/
			then call REMOVE.NOTE.RECORD;				/*	delete it			*/
		end;
	end;

	if Audition.Cue.Id > 0 then do;
		call PLACE.CUE.IN.SEQUENCE (Audition.Cue.Id,Cue.Audition.Track,Cue.Trig.Time,0);
	end;

	/*	clear bit in new.seq.info, since we really do not have	*/
	/*	to update the sequence panel after setting up the			*/
	/*	audition cue															*/

	new.seq.info = prior.info;

end PLACE.CUE.ON.AUDITION.TRACK;

/*	$page - change audition mode											*/

ASSIGN.NEW.DTD.AUDITION: proc (val) public swapable;
	dcl val	fixed;	/*	pass new value to assign to current.cue.audition	*/

	if (val<0) or (val>1)					/*	poo poo on you too	*/
	then return;

	Current.Cue.Audition = val;			/*	assign a value			*/

	/*	Create a system event whenever the audition mode changes:	*/

	new.dtd.info = new.dtd.info | 4096;

	/*	clear/set audition track as needed:	*/

	call ERASE.CUE.AUDITION.TRACK;		/*	get rid of possible old .audit. cues this way	*/

	if Current.Cue.Audition <> 0			/*	makes for much speedier audition updates		*/
	then call PLACE.CUE.ON.AUDITION.TRACK;
end ASSIGN.NEW.DTD.AUDITION;

wrap_24_hours: proc (time) public swapable;
	dcl time	fixed array;
	/*	Get 24 hours of milliseconds in the current mode	*/
	//	The smpte mode is actually irrelevant since 24 hours in any mode will yield
	//	the same number of milliseconds.  Only SAMP.SPEED has an effect.  -TY 2001/02/12
	//	compute round(24*60*60*1000 * 1000/SAMP.SPEED)

	//	NOTE: this computation will overflow if SAMP.SPEED is 20 or less
	if (SAMP.SPEED ile 20)	return;	//	don't alter time if the result will be bogus
//	if (SAMP.SPEED == 0)	return;		//	guard against division by zero error

	bitmsb =  1318;
	bitlsb = 23552;
	ratio_multiply(1000,SAMP.SPEED);
	call ADD16(2*zero.time, bitmsbarray);	//	must account for both negative and positive zero.time offsets
	/*	Add 24 hours to the time so it will be positive	*/
	call ADD32(time, bitmsbarray, time);
end wrap_24_hours;

/*	$page - update current cue record	*/

/*	update current cue record is called to update the saved copy	*/
/*	of the cue (in ext memory) for changes in any time					*/
/*	parameter (in, out, offset, edit in, edit out, sync), the		*/
/*	drive bits, the sync mode, or the protect indicator				*/

/*	it does not update the NAME or CAPTION fields						*/
/*	it also does not update the cue fade in/fade out fields			*/

/*	Note: all times are times with respect to the current cue		*/
/*	mapping																			*/

UPDATE.CURRENT.CUE.RECORD: proc public swapable;
	dcl cue.record	(CUE.NAME-1)	fixed;
	dcl temp			(4)				fixed;

	consub: proc(in,out);
		dcl in	array;
		dcl out	array;

		call Msec.To.DTD.Sample.#(In, CF#Time, SAMP.SPEED, AEE.Current.Project.Rate, zero.zero, Out);
	end consub;

	//	Begin by computing the new Cue.Trig.Time based upon Cue.Sync.Time and our sync mode.
	//	Since Cue.Sync.Time might be negative, we must first apply a 24-hour wrap if this is the case.
	if (Cue.Sync.Time(0) < 0) wrap_24_hours(Cue.Sync.Time);
	Compute.Cue.Trigger.Time();	//	this also performs a 24-hour wrap on Cue.Sync.Time if it's negative

	/*	Now update the current cue record in external memory:			*/

	write(mam) = Current.Cue.Ptr;
	call COPY.IN(addr(cue.record(0)),CUE.NAME);

	/*	Copy current in/out times into cue.record	*/
	call Consub(Cue.In.Time,		 loc(addr(cue.record(CUE.S.MSB		 ))));
	call Consub(Cue.Out.Time,		 loc(addr(cue.record(CUE.E.MSB		 ))));
	call Consub(Cue.Offs.Time,		 loc(addr(cue.record(CUE.OFF.MSB		 ))));	/*	abs time in dtd	*/
	call Consub(Cue.Edit.In.Time,	 loc(addr(cue.record(CUE.EDIT.IN.MSB ))));
	call Consub(Cue.Edit.Out.Time, loc(addr(cue.record(CUE.EDIT.OUT.MSB))));

	/*	Copy over sync times	*/
	call MSEC.TO.SMPTE(Cue.Trig.Time, cf#time, SAMP.SPEED, SM.MODE, loc(addr(SM.HRS)), temp);
	call SMPTE.TO.SBITS (temp, SM.MODE, loc(addr(cue.record(CUE.SMPT.MSB))));

	cue.record(CUE.SMPT.MODE) = SM.MODE;

	cue.record(CUE.TRKS) = Current.Cue.Drive.Bits;

	cue.record(CUE.BITS) = ((Bit0		 ) & (Current.Cue.Protect))
								\ ((Bit1\Bit2) & (shl(Current.Cue.Sync.Mode,1)))
								\   Bit3		/*	absolute cue.smpt.msb	*/
								\ (cue.record(CUE.BITS) & (not(Bit0\Bit1\Bit2\Bit3)));

	write(mam) = Current.Cue.Ptr;
	call COPY.OUT(addr(cue.record(0)),CUE.NAME);

	/*	Compute correct display offset now that we have assigned the right	*/
	/*	cue in time:																			*/

	//	Compute global variables needed for displays
	call Compute.Cue.Display.Globals;	/*	set display offset, etc.			*/

	/*	create a new audition cue whenever the current cue changes:				*/
	if Current.Cue.Audition <> 0		/*	and audition cue							*/
	then call PLACE.CUE.ON.AUDITION.TRACK;

	/*	create a system event when the current cue changes		*/
	new.dtd.info = new.dtd.info \ 256;

end UPDATE.CURRENT.CUE.RECORD;

/*	$page - deposit cue and shuttle displays						*/

/*	Deposit.A.New.Current.Cue is called whenever a new cue	*/
/*	has been called up to the Current.Cue.Ptr, or if the		*/
/*	Current.Cue.Ptr is changed.  This routine looks up			*/
/*	the information about the cue (from external memory) and	*/
/*	stores this information in some global variables so that	*/
/*	it may be easily processed and displayed.						*/

/*	current cue to the user.  it reads the information from	*/
/*	the current cue buffer (Current.Cue.Ptr) into some			*/
/*	global static variables.  It then deposits these			*/
/*	items into both the Cue Editor and Shuttle Panels			*/

/*	This routine also creates an AUDITION cue to match the	*/
/*	displayed values, if desired										*/

Deposit.A.New.Current.Cue: proc (Reset.Sbar) PUBLIC swapable;
	dcl Reset.Sbar	fixed;	/*	pass 1 to reset sbar if linked to cue	*/
									/*	(we don't want to do that after edits)	*/

	dcl cue.record	(CUE.NAME-1)	fixed;
	dcl temp			(4)				fixed;
	dcl rlen								fixed;
	dcl nlen								fixed;
	dcl clen								fixed;

	consub: proc (Arr);
		dcl Arr	array;

		Call DTD.Sample.#.To.Msecs(Arr, CF#Time, SAMP.SPEED, AEE.Current.Project.Rate, zero.zero, Arr);
	end consub;

	/*	look up record length, name length (wrds), comment length (wrds)	*/

	write(mam) = Current.Cue.Ptr;
	write(mal) = CUE.RLEN;
	rlen		  = read(md);
	write(mal) = CUE.NAME;
	nlen		  = shr(read(md)+3,1);	/*	# of wrds in cue name	*/
	if rlen igt CUE.NAME+nlen
	then do;
		write(mal) = CUE.NAME+nlen;
		clen	  = read(md);				/*	get comment len, bytes	*/
	end;
	else clen  = 0;						/*	no comment in record		*/

	/*	read in basic cue info up to just	*/
	/*	before name:								*/

	write(mal) = 0;
	call COPY.IN(addr(cue.record(0)),CUE.NAME);

	if Current.Sync.Locked = 0 then do;	/*	if sync time is not locked	*/
		Current.Cue.Sync.Mode = shr(cue.record(CUE.BITS),1)&3;
	end;

	/*	read in/out/offset/edit times into static	*/
	/*	variables											*/

	call COPY32(loc(addr(cue.record(CUE.S.MSB))),Cue.In.Time);
	call COPY32(loc(addr(cue.record(CUE.E.MSB))),Cue.Out.Time);
	call COPY32(loc(addr(cue.record(CUE.OFF.MSB))),Cue.Offs.Time);
	call COPY32(loc(addr(cue.record(CUE.EDIT.IN.MSB))),Cue.Edit.In.Time);
	call COPY32(loc(addr(cue.record(CUE.EDIT.OUT.MSB))),Cue.Edit.Out.Time);

	//	Convert all cue times to msecs
	Call Consub(Cue.In.Time);
	Call Consub(Cue.Out.Time);
	Call Consub(Cue.Offs.Time);
	Call Consub(Cue.Edit.In.Time);
	Call Consub(Cue.Edit.Out.Time);

	if COM32(Cue.Offs.Time,Cue.In.Time) = lw#ilt				/*	limit offset time		*/
	then call COPY32(Cue.In.Time,Cue.Offs.Time);				/*	to be in cue			*/
	if COM32(Cue.Offs.Time,Cue.Out.Time) = lw#igt
	then call COPY32(Cue.Out.Time,Cue.Offs.Time);

	if COM32(Cue.Edit.In.Time,Cue.In.Time) = lw#ilt			/*	limit edit time		*/
	then call COPY32(Cue.In.Time,Cue.Edit.In.Time);			/*	to be in cue			*/
	if COM32(Cue.Edit.In.Time,Cue.Out.Time) = lw#igt
	then call COPY32(Cue.Out.Time,Cue.Edit.In.Time);

	if COM32(Cue.Edit.Out.Time,Cue.In.Time) != lw#igt		/*	limit edit outtime	*/
	then call COPY32(Cue.Out.Time,Cue.Edit.Out.Time);		/*	to be in cue			*/
	if COM32(Cue.Edit.Out.Time,Cue.Out.Time) = lw#igt
	then call COPY32(Cue.Out.Time,Cue.Edit.Out.Time);

	if COM32(Cue.Edit.Out.Time,Cue.Edit.In.Time) = lw#ilt	/*	make sure edit out	*/
	then call COPY32(Cue.Edit.In.Time,Cue.Edit.Out.Time);	/*	is > edit in			*/

	/***** for now, keep edit in time and offset time equal *****/
	call COPY32(Cue.Edit.In.Time, Cue.Offs.Time);
	/************************************************************/

	if Current.Sync.Locked = 0 then do;				/*	recall sync time		*/
		if ((cue.record(CUE.BITS) & bit3) == 0)	/*	if relative sync		*/
		//	ÉÉÉRÉEÉLÉAÉTÉIÉVÉEÉÉÉSÉYÉNÉCÉÉÉ	(Apparently never used?)
		SBITS.TO.MSEC(loc(addr(cue.record(CUE.SMPT.MSB))), cf#time, SAMP.SPEED, cue.record(CUE.SMPT.MODE), Cue.Trig.Time);
		else
		//	ÉÉÉAÉBÉSÉOÉLÉUÉTÉEÉÉÉSÉYÉNÉCÉÉÉ	(Apparently always the case?)
		{
			call SBITS.TO.SMPTE(loc(addr(cue.record(CUE.SMPT.MSB))), cue.record(CUE.SMPT.MODE), temp);
			call SMPTE.TO.MSEC(temp, cf#time, SAMP.SPEED, SM.MODE, loc(addr(SM.HRS)), Cue.Trig.Time);
			/*	but might be before start of sequence				*/
			/*	If negative, add 24 hours so it's POSITIVE!		*/
			//	Note: Cue.Trig.Time will either be >= zero.time or <= -zero.time
			if (Cue.Trig.Time(0) < 0) wrap_24_hours(Cue.Trig.Time);
		}

		do case (Current.Cue.Sync.Mode);				/*	branch on type			*/
			STR32(0,0,temp);								/*	pre - no offset		*/
			SUB32(Cue.Offs.Time,Cue.In.Time,temp);	/*	show offset				*/
			SUB32(Cue.Out.Time, Cue.In.Time,temp);	/*	show cue end			*/
		end;
		call ADD32(Cue.Trig.Time,temp,Cue.Sync.Time);
	end;

	Current.Cue.Drive.Bits = Cue.Record(CUE.TRKS);
	Current.Cue.Protect	  = Cue.Record(CUE.BITS)&Bit0;

	/*	important - re-store information back out in external memory			*/
	/*	here to match any round-off that might have occurred.  Also				*/
	/*	generate a system event to reflect the new cue has been called up.	*/

	call UPDATE.CURRENT.CUE.RECORD;	/*	store new info in external memory	*/

	/*	When calling up a cue, set up the coarse scroll bar for					*/
	/*	scrubbing through this cue (provide coarse bar is in "CUE" mode)		*/

	if Reset.Sbar
	then call Send.Cue.Times.To.DTD.Scrub.Bars;

	/*	Also create system events to invalidate & update the coarse and		*/
	/*	fine bar wavewhape displays, since the underlying audio has				*/
	/*	likely changed:																		*/

	new.dtd.info = new.dtd.info \ (1024\2048);

end Deposit.A.New.Current.Cue;

/*	$page - SET.UP.AEE.DTD.GLOBALS												*/

/*	This routine is called whenever the CURRENT PROJECT NUMBER,			*/
/*	CURRENT PROJECT NAME, CURRENT PROJECT START TIME, CURRENT PROJECT	*/
/*	END TIME, or CURRENT PROJECT SAMPLING RATE may have changed.		*/

/*	It gets the actual information for these items from the DTD and	*/
/*	stores them in some global variables:										*/

SET.UP.AEE.DTD.GLOBALS: proc public swapable;
	dcl (i,s,e)							fixed;
	dcl rec(CUE.HIST.REC.LEN-1)	fixed;

	if DTD.MAX.SECS <> 0 then do;

		call FETCH.DTD.SYSTEM.INFO;	/*	make sure dtd.max.msb,lsb get up to date	*/

		/*	Get info for current song.  Old software will handle	*/
		/*	first 10 songs only:												*/

		if lod.version >= 10
		then call FETCH.DTD.SONG.DIRECTORY(-1);			/*	-1	= current song	*/
		else call FETCH.DTD.SONG.DIRECTORY(dtd.song#);	/*		= 0 - 9			*/

		write(mam) = Scsi.Ptr;
		AEE.Current.Project = read(mdi)-1;
		do i = 1 to 12;
			AEE.Current.Project.Name(i) = read(mdi);
		end;
		AEE.Current.Project.Name(0) = 24;
		write(mal) = 13;
		s			  = read(mdi);
		e			  = read(mdi);
		AEE.Current.Project.Rate = read(mdi);

		new.motion = 1;				/*	tell dsp add-on about new rate	*/

		/*	compute base sample # for	*/
		/*	this project					*/
		/*	Note: this computation		*/
		/*	must match the lod softw	*/
		/*	exactly							*/

		call STR32(0, S, Loc(Addr(BITMSB)));
		call ratio_multiply(50000,256);				/*	get sector # to start	*/
		call unround;										/*	truncate down				*/
		call COPY32(Loc(Addr(BITMSB)), AEE.Current.Project.Base);
		call SHL32(AEE.Current.Project.Base, 8);	/*	get sample #				*/

		call STR32(0, E, Loc(Addr(BITMSB)));
		call ratio_multiply(50000,256);				/*	get sector # to start	*/
		call unround;										/*	truncate down				*/
		call COPY32(Loc(Addr(BITMSB)), AEE.Current.Project.End);
		call SHL32(AEE.Current.Project.End, 8);	/*	get sample #				*/

		call BUILD.PROJ.HIST.REC(dtd.song#,3,rec);	/*	mark the current project as changed in cue history	*/
		call ADD.CUE.TO.HIST(rec);

		/*	Re-deposit all new cue times since the current project	*/
		/*	rate may have changed:												*/

	end;

	/*	Simulate sensible info if DTD is not up yet	*/

	else do;
		AEE.Current.Project.Rate = 500;
	end;

	call Deposit.A.New.Current.Cue(0);

end SET.UP.AEE.DTD.GLOBALS;

/*	$page - routines for cue name and caption storage:								*/

/*	Routines to store a new cue name and cue caption in current cue:			*/

/*	This routine is passed a cue name in the NED string format.  It			*/
/*	stores the new name in the current cue record.									*/

/*	A zero is returned if the new name will not fit in memory.  Otherwise	*/
/*	a one is returned:																		*/

ASSIGN.NEW.DTD.CUE.NAME:proc (name) public swapable;
	dcl name						array;
	dcl new.name.word.len	fixed;
	dcl cue.length				fixed;
	dcl cue.seg.ptr			fixed;
	dcl cue.name.word.len	fixed;

	/*	Limit length in a rather brute force way:	*/

	if name(0) IGT 32 then name(0) = 32;

	new.name.word.len = 1+shr(name(0)+1,1);

	write(mam)	= Current.Cue.Ptr;
	write(mal)	= CUE.RLEN;
	cue.length	= read(md);
	write(mal)	= CUE.LIST.PTR;
	cue.seg.ptr	= read(md);
	write(mal)	= CUE.NAME;
	cue.name.word.len = 1+shr(read(md)+1,1);

	if new.name.word.len <> cue.name.word.len
	then do;

		if  (new.name.word.len igt cue.name.word.len)
		and (cue.length + (new.name.word.len - cue.name.word.len)) igt MAX.CUE.RECORD.LEN
		then return 0;

		call COPY.EXT.MEM(Current.Cue.Ptr,CUE.NAME+cue.name.word.len,
								Current.Cue.Ptr,CUE.NAME+new.name.word.len,
								cue.length - (CUE.NAME+cue.name.word.len));
		cue.length	= cue.length  + (new.name.word.len - cue.name.word.len);
		if cue.seg.ptr <> 0 then
		cue.seg.ptr	= cue.seg.ptr + (new.name.word.len - cue.name.word.len);
		write(mam)	= Current.Cue.Ptr;
		write(mal)	= CUE.RLEN;
		write(md )	= cue.length;
		write(mal)	= CUE.LIST.PTR;
		write(md )	= cue.seg.ptr;
	end;

	write(mam) = Current.Cue.Ptr;
	write(mal) = CUE.NAME;
	call COPY.OUT(addr(name(0)),new.name.word.len);

	/*	Create a system event that shows the new current cue name:	*/

	new.dtd.info = new.dtd.info \ 256;

	return 1;

end ASSIGN.NEW.DTD.CUE.NAME;

//	Similar routine for caption.
//	Returns 0 if it won't fit in memory, otherwise returns 1.

ASSIGN.NEW.DTD.CUE.CAPTION:proc (capt) public swapable;
	dcl capt						array;
	dcl new.capt.word.len	fixed;
	dcl cue.length				fixed;
	dcl cue.seg.ptr			fixed;
	dcl cue.capt.word.len	fixed;
	dcl cue.name.word.len	fixed;

	/*	Limit length in a rather brute force way:	*/

	if capt(0) IGT 128 then capt(0) = 128;

	new.capt.word.len = 1+shr(capt(0)+1,1);

	write(mam)	= Current.Cue.Ptr;
	write(mal)	= CUE.RLEN;
	cue.length	= read(md);
	write(mal)	= CUE.LIST.PTR;
	cue.seg.ptr	= read(md);
	write(mal)	= CUE.NAME;
	cue.name.word.len = 1+shr(read(md)+1,1);
	if cue.length igt	CUE.NAME+cue.name.word.len
	then do;
		write(mal)	=	CUE.NAME+cue.name.word.len;
		cue.capt.word.len = 1+shr(read(md)+1,1);
	end;
	else cue.capt.word.len = 0;

	if new.capt.word.len <> cue.capt.word.len
	then do;

		if  (new.capt.word.len igt cue.capt.word.len)
		and (cue.length + (new.capt.word.len - cue.capt.word.len)) igt MAX.CUE.RECORD.LEN
		then return 0;

		call COPY.EXT.MEM(Current.Cue.Ptr,CUE.NAME+cue.name.word.len+cue.capt.word.len,
								Current.Cue.Ptr,CUE.NAME+cue.name.word.len+new.capt.word.len,
								cue.length - (CUE.NAME+cue.name.word.len+cue.capt.word.len));
		cue.length	= cue.length  + (new.capt.word.len - cue.capt.word.len);
		if cue.seg.ptr <> 0 then
		cue.seg.ptr	= cue.seg.ptr + (new.capt.word.len - cue.capt.word.len);
		write(mam)	= Current.Cue.Ptr;
		write(mal)	= CUE.RLEN;
		write(md )	= cue.length;
		write(mal)	= CUE.LIST.PTR;
		write(md )	= cue.seg.ptr;
	end;

	write(mam) = Current.Cue.Ptr;
	write(mal) = CUE.NAME+cue.name.word.len;
	call COPY.OUT(addr(capt(0)),new.capt.word.len);

	/*	Create a system event that shows the new current cue caption:	*/

	new.dtd.info = new.dtd.info \ 256;

	return 1;

end ASSIGN.NEW.DTD.CUE.CAPTION;
