/*
   10/28/87 - lpg - Updated title and help screen, fixed timbre name blink
   5/30/86 - adt & eg - updated to work with rel M
   5/20/86 - eg - made procedures and some variables pub.
   5/15/86 - "official" creation of release-M modules
*/

DISPLAY_ENTRY:procedure (ENTRY,OVERALL) swapable;

  dcl ENTRY   fixed,
      TEMP(1) fixed,
      OVERALL boolean;

  if OVERALL then do;

    call move_to (OCOLS(ENTRY & MASK2),OROWS(shr(ENTRY,2))); alpha_mode;

    do case ENTRY;

      call pnum(STAVES+1,0);        /* Staves */
      call pnum(MEASURE-1,0);       /* Measure */
      do;                           /* TimeSig */
        call pnum(shr(TIME_SIG,8),0); wchar(slash); call pnum(TIME_SIG&LBYTE,0);
      end;
      call PRINT_PITCH(CUR_PITCH);
      call pnum(MEASURES,0);        /* Measure Spread */
      call pnum(START_CLICK,0);     /* IniClik */
      do;                           /* ClikNot */
        call pnum(shr(CLICK_NOTE,8),0); wchar(slash); call pnum(CLICK_NOTE&LBYTE,0);
      end;
      call PRINT_DURATION(CUR_DURATION);

    end;

  end;
  else do;

    call move_to (PCOLS(ENTRY & MASK1),PROWS(shr(ENTRY,1))); alpha_mode;

    do case ENTRY;

      call pnum(TRACK+1,0); /* Track */
      ;
      do; /* Key */
        TEMP(0) = 2;
        TEMP(1) = KEY_NAMES(KEY);
        call pstring(TEMP);
      end;
      if (PART_BITS & P_FLAT) ~= 0 then call pc (asc.f    );
                                   else call pc (asc.sharp);
      wchar(CLEF_NAMES(CLEF)); /* Clef */
      ;
      call pnum(RESOLUTION,0); /* Resolution */
      ; /* Dummy Multiplier */

    end;

  end;

end DISPLAY_ENTRY;

PART_FRAME:procedure swapable;

  call move_to (0,PROWS(0)); alpha_mode;
  call ps ('Trk'); call pcr;
  call ps ('Key'); call pcr;
  call ps ('Clf'); call pcr;
  call ps ('Res');
  call move_to (PCOLS(1)-ALPHA_WIDTH-4,PROWS(1)); alpha_mode;
  call pc (asc.min);
end PART_FRAME;

MENU_FRAME:procedure swapable;

  dcl HOLD_STAFF fixed,
      (I,J,Y)    fixed;

  call clear.term;

  if HELP_MODE then do;

    MENU_TOP   = SCREEN_TOP - HELP_SIZE;
    EDIT_TOP   = MENU_TOP - MENU_SIZE;
    INSTR_MODE = -1;
    TCUR_COL   = 79;
    if MG600 then TCUR_ROW = 9;
             else TCUR_ROW = 10;

    call PRINT.SCREEN.FRAME (7,'MUSIC NOTATION DISPLAY - 3/14/89');
    call PRINT_INSTRUCTIONS;

  end;
  else do;
    MENU_TOP = SCREEN_TOP;
    EDIT_TOP = MENU_TOP - MENU_SIZE;
    TCUR_ROW = 0;
    TCUR_COL = 76;
  end;
  /* call cpos (TCUR_ROW,TCUR_COL); */

  if D44_PRESENT /* If device available plot the mouse hole */
  then call plot.mouse.hole (seqed.mouse.hole.x,seqed.mouse.hole.y);

  call viewport (0,SCREEN_RIGHT,0,SCREEN_TOP);
  call window   (0,SCREEN_RIGHT,0,SCREEN_TOP);

  vector_mode;
  call data_level(2);
  call move_to (SCREEN_LEFT,MENU_TOP);    call plot (SCREEN_RIGHT,MENU_TOP);
  call move_to (EDIT_LEFT-1,EDIT_BOT);    call plot (EDIT_LEFT-1,EDIT_TOP);
  call move_to (SCREEN_LEFT,EDIT_TOP+1);  call plot (SCREEN_LEFT,MENU_TOP);
  call move_to (SCREEN_RIGHT,EDIT_TOP+1); call plot (SCREEN_RIGHT,MENU_TOP);
  call move_to (SCREEN_LEFT,EDIT_TOP+1);  call plot (SCREEN_RIGHT,EDIT_TOP+1);
  call window   (0,SCREEN_RIGHT,-MENU_TOP,SCREEN_TOP-MENU_TOP);
  call move_to(MCOLS(0),OROWS(0)); call pstring('Staves:');
  call move_to(MCOLS(0),OROWS(1)); call pstring('Measures:');
  call move_to(MCOLS(1),OROWS(0)); call pstring('Measure#:');
  call move_to(MCOLS(1),OROWS(1)); call pstring('StrtClick:');
  call move_to(MCOLS(2),OROWS(0)); call pstring('TimeSig:');
  call move_to(MCOLS(2),OROWS(1)); call pstring('Click:');
  call move_to(MCOLS(3),OROWS(0)); call pstring('Pitch:');
  call move_to(MCOLS(3),OROWS(1)); call pstring('Duration:');

  /* calculate vertical position for each staff */

  J = (EDIT_TOP + 1 - EDIT_BOT) / (STAVES + 1);
  Y = EDIT_TOP - J/2;
  do I = 0 to STAVES;
    Y_POS(I) = Y;
    Y        = Y - J;
  end;

  do I = 0 to STAVES;
    call window (0,SCREEN_RIGHT,-Y_POS(I),SCREEN_TOP-Y_POS(I));
    call PART_FRAME;
  end;

  TOKEN(0)      = 0;     /* Necessary? */
  FIELD_CLEARED = false; /* Necessary? */
  STAFF_REFRESH = -1;    /* Set all bits => Draw all staves (only used if FULL case fails due to ABORT) */
  MENU_REFRESH  = true;

  call data_level(0);

end MENU_FRAME;

COMPARE:procedure (NS,NW,OS,OW,LEN) returns (boolean) swapable;

  dcl (NS,NW) fixed,
      (OS,OW) fixed,
      (LEN,I) fixed;

  write(mam) = NS + shr(nw,8);
  write(mal) = nw;
  call copy.in(addr(misc.buf(0)),len);

  write(SECT$) = OS + shr(OW,8);
  write(WORD$) = OW;
  write(R13)   = addr(MISC.BUF(0));

  do I = 1 to LEN;
    if read(DATI$) ~= read(MR13I) then return (true);
  end;
  return (false);

end COMPARE;

POINTER_COMPARE:procedure (NS,NW,OS,OW,LEN) returns (boolean) swapable;

  dcl (NS,NW) fixed,
      (OS,OW) fixed,
      (LEN,I) fixed;

  write(mam) = ns + shr(nw,8);
  write(mal) = nw;
  call copy.in(addr(misc.buf(0)),len);

  write(SECT$) = OS + shr(OW,8);
  write(WORD$) = OW;
  write(R13)   = addr(MISC.BUF(0));

  if read(DATI$) ~= read(MR13I) then return (true); /* Position record */
  if read(DATI$) ~= read(MR13I) then return (true);
  do I = 3 to LEN by 3;
    if read(DATI$) ~= read(MR13I) then return (true); /* 1st word of record */
    write(DATI$) = read(MR13I); /* Sequence pointer sector */
    write(DATI$) = read(MR13I); /* Sequence pointer word */
  end;
  return (false);

end POINTER_COMPARE;

COMPARE_INT:procedure (P,Q,LEN) returns (boolean) swapable; /* Returns true if matrix P ~= Q */

  dcl (P,Q)   fixed array,
      (LEN,I) fixed;

  do I = 0 to LEN-1;
    if P(I) ~= Q(I) then return (true);
  end;
  return (false);

end COMPARE_INT;

DELETE_POSITION:procedure (OP,LEN) swapable;

  dcl (OP,P)   fixed,
      (I,LEN)  fixed;

  write(SECT$) = OLD_SECT;
  do I = RL_STAFF to 7; /* Update old map pointers */
    write(WORD$) = 8 + I;
    write(DATA$) = read(DATA$) - LEN;
  end;
  P            = read(DATA$);
  write(WORD$) = 0;
  write(DATA$) = (read(DATA$) & UBYTE) \ shr(P+255,8); /* New length of old map in sects */
  call COPY.EXT.MEM (OLD_SECT,OP+LEN,OLD_SECT,OP,P-OP); /* Slide latter half of old map down */

end DELETE_POSITION;

INSERT_POSITION:procedure (NP,OP,LEN) swapable;

  dcl (NP,OP)  fixed,
      LEN      fixed,
      (I,P)    fixed;

  if OP + LEN >= ((NEW_SECT - OLD_SECT) * 256) then return; /* Out of room */
  write(SECT$) = OLD_SECT;
  do I = RL_STAFF to 7; /* Update old map pointers */
    write(WORD$) = 8 + I;
    write(DATA$) = read(DATA$) + LEN;
  end;
  P            = read(DATA$);
  write(WORD$) = 0;
  write(DATA$) = (read(DATA$) & UBYTE) \ shr(P+255,8); /* New length of old map in sects */
  call COPY.EXT.MEM (OLD_SECT,OP,OLD_SECT,OP+LEN,P - (OP+LEN)); /* Slide latter half of old map up */
  call COPY.EXT.MEM (NEW_SECT,NP,OLD_SECT,OP,LEN); /* Copy position from new to old */

end INSERT_POSITION;

REFRESH_MAIN_MENU:procedure (ITEM1,ITEM2,LOADING) swapable;

  dcl (item1,item2) fixed;   /* pass starting and ending item */

  dcl ITEM    fixed,
      PREV    fixed,
      CURR    fixed,
      LOADING boolean;

  SWITCH:procedure (N,ITEM) returns (fixed);

    dcl (N,ITEM,V) fixed;

    do case ITEM;
      do; V = STAVES;       STAVES = N;       end;
      do; V = MEASURE;      MEASURE = N;      end;
      do; V = TIME_SIG;     TIME_SIG = N;     end;
      do; V = CUR_PITCH;    CUR_PITCH = N;    end;
      do; V = MEASURES;     MEASURES = N;     end;
      do; V = START_CLICK;  START_CLICK = N;  end;
      do; V = CLICK_NOTE;   CLICK_NOTE = N;   end;
      do; V = CUR_DURATION; CUR_DURATION = N; end;
    end;

    return (V);

  end SWITCH;

  do item=item1 to item2;          /* cut down on swapping */

     write(SECT$) = MENU_BASE;
     write(WORD$) = ITEM;
     PREV         = read(DATA$);
     CURR         = SWITCH (PREV,ITEM);

     if PREV ~= CURR \ LOADING then do;
       vector_mode;
       call data_level (2); alpha_mode;
       if ~LOADING then do; /* Erase old entry */
         call DISPLAY_ENTRY (ITEM,true);
         write(SECT$) = MENU_BASE;
         write(WORD$) = ITEM;
       end;
       write(DATA$) = CURR;
       call SWITCH (CURR,ITEM);
       call DISPLAY_ENTRY (ITEM,true);
       call data_level (0);
     end;
  end;

end REFRESH_MAIN_MENU;

REFRESH_LIST:procedure (NP,NEP,OP,OEP) swapable;

  dcl (NP,OP)     pointer,
      (NEP,OEP)   pointer,
      (NPOS,OPOS) fixed,
      (NLEN,OLEN) fixed,
      (NADV,OADV) boolean,
      (NUPD,OUPD) boolean,
      (REFR,REFC) boolean;

  NADV = NP ~= NEP;
  OADV = OP ~= OEP;
  NUPD = false; OUPD = false;
  NPOS = 0;     OPOS = 0;
  NLEN = 16;    OLEN = 16;
  REFR = 0;     REFC = false;

  do while NP ~= NEP \ OP ~= OEP;

    /* Note ploting and insertion */

    if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF &
       ((NUPD & OLD_POS(POSITION#) = NPOS) \ 
        (OUPD & OLD_POS(POSITION#) = OPOS))
    then do; /* Get rid of edit cursor before updating pos */
      REFC                = true;
      EDIT_CURSOR_REFRESH = true;
    end;
    else REFC = false;

    if OUPD then do; /* Erase old position and remove it from old list */
      call data_level (1);
      call PLOT_POSITION (OLD_SECT,OP);
      call DELETE_POSITION (OP,OLEN);
      OEP  = OEP - OLEN;
      OLEN = 0;
      REFR = REFR \ "1";
    end;
    if NUPD then do; /* Draw new position and insert it into old list */
      call data_level (0);
      call PLOT_POSITION (NEW_SECT,NP);
      call INSERT_POSITION (NP,OP,NLEN);
      OP   = OP  + NLEN;
      OEP  = OEP + NLEN;
      REFR = REFR \ "2";
    end;

    if ABORT_REFRESH(1) then return (REFR);

    /* Pointer advance */

    if NADV then do; /* Advance new pointer */
      NP           = NP + NLEN;
      write(SECT$) = NEW_SECT + shr(NP,8);
      write(WORD$) = NP;
      NPOS         = shr(read(DATA$),6);
      NLEN         = read(DATA$) & MASK6;
    end;
    if OADV then do; /* Advance old pointer */
      OP           = OP + OLEN;
      write(SECT$) = OLD_SECT + shr(OP,8);
      write(WORD$) = OP;
      OPOS         = shr(read(DATA$),6);
      OLEN         = read(DATA$) & MASK6;
    end;

    /* Interleave logic */

    NADV = false; OADV = false;
    NUPD = false; OUPD = false;
    if      NP = NEP \ (OPOS < NPOS & OP ~= OEP) then do; /* Old without new */
      OADV = true; OUPD = true;
    end;
    else if OP = OEP \ (NPOS < OPOS & NP ~= NEP) then do; /* New without old */
      NADV = true; NUPD = true;
    end;
    else if NPOS = OPOS then do; /* Both present - compare */
      if NLEN ~= OLEN \ POINTER_COMPARE (NEW_SECT,NP,OLD_SECT,OP,NLEN)
      then do;
        NUPD = true; OUPD = true;
      end;
      NADV = true; OADV = true;
    end;

  end;

  return (REFR);

end REFRESH_LIST;

REFRESH_INITIALIZE:procedure swapable;

  dcl I fixed;

  if CLICK.TRACK.MODE igt 3 then CLICK_RATE = 1000;	/* Use surrogate click track */
                          else CLICK_RATE = SAMP.CLICK.RATE;	/* Use metronome */
  TICS = CLICK_RATE * CLICKS;

  TC_MAX_BLOCKS  = (shr(CLICK_NOTE,8) * NEW_MAX_BLOCKS * CLICKS) / (CLICK_NOTE & LBYTE);
  NOTE_LEFT      = EDIT_LEFT + 24 + NEW_MAX_KEY_LENGTH;	/* Allow room for clef & keysig */
  TC_EDIT_FIELD  = NOTE_RIGHT - NOTE_LEFT + 1;
  TC_NOTE_FIRST  = NOTE_LEFT + SIEVE(TC_EDIT_FIELD,TC_MAX_BLOCKS,1)/2;	/* Center notes in edit field */

  /* Check play time and calculate a new starting click if necessary */

  if PLAY_TIME(1) ~= SAMPLED.ACTUAL.PLAY.TIME.MSB \
     PLAY_TIME(0) ~= SAMPLED.ACTUAL.PLAY.TIME.LSB
  then do; /* Reset window position by setting START_CLICK based on play.time */

    PLAY_TIME(1) = SAMPLED.ACTUAL.PLAY.TIME.MSB;
    PLAY_TIME(0) = SAMPLED.ACTUAL.PLAY.TIME.LSB;	/* - PRESCAN ??? */
    call CONVERT_TIME (PLAY_TIME,0);

    /* Find floor(t/jump_tics) * jump_clicks */
    write(5) = PLAY_TIME(0);
    write(4) = PLAY_TIME(1);
    write(7) = CLICK_RATE;
    I        = read(5);	/* Click we're in */
    if I < START_CLICK - 1 \ START_CLICK - 1 + CLICKS <= I
    then do;
      if (((START_CLICK-1) MOD CLICKS_MEASURE) != 0) full.seqed.refresh = true;
      START_CLICK = (I / CLICKS) * CLICKS + 1;
    end;

    PLAY_TIME(1) = SAMPLED.ACTUAL.PLAY.TIME.MSB;
    PLAY_TIME(0) = SAMPLED.ACTUAL.PLAY.TIME.LSB;
    MEASURE = (START_CLICK - 1 + (CLICKS * POSITION) / BLOCKS) /
              CLICKS_MEASURE + 1;

  end;

  /* Find integer_start_time (start_click * click_rate) and convert to synclavier time */
  write(5)      = START_CLICK - 1;
  write(6)      = CLICK_RATE;
  START_TIME(1) = read(4);
  START_TIME(0) = read(5);

  END_TIME(1) = START_TIME(1);
  END_TIME(0) = START_TIME(0);
  call ADD32 (END_TIME,TICS);

  /**************************/       /* use local variables here */
  dcl SCREEN.BEGIN(1)  fixed;
  dcl SCREEN.END  (1)  fixed;
  /**************************/

  SCREEN.BEGIN(1) = START_TIME(1);
  SCREEN.BEGIN(0) = START_TIME(0);
  call UNCONVERT_TIME (SCREEN.BEGIN,0);

  Screen.Scroll.Begin(0) = SCREEN.BEGIN(MSB);
  Screen.Scroll.Begin(1) = SCREEN.BEGIN(LSB);

  SCREEN.END(1)   = END_TIME(1);
  SCREEN.END(0)   = END_TIME(0);
  call UNCONVERT_TIME (SCREEN.END,0);

  Screen.Scroll.End(0)   = SCREEN.END(MSB);
  Screen.Scroll.End(1)   = SCREEN.END(LSB);

  ABORTING = false;

end REFRESH_INITIALIZE;

SEQED.REFRESH:procedure (core_area) swapable;
  dcl core_area array;

  dcl (NSP,OSP)     pointer,
      (NEP,OEP)     pointer,
      REF_PART_MENU boolean,
      REF_CLEF      boolean,
      REF_TNAME     boolean,
      FULL          boolean,
      (I,J)         fixed;

  dcl first         fixed static;

  if first=0 then do;
     call initialize.seqed(CORE_AREA);
     first=1;
  end;

  retry:;

  FULL                 = full.seqed.refresh;
  full.seqed.refresh   = false;

  vector_mode;

  /* Step 1 - Refresh cursor and check for full refresh */

  /* On full refresh - clear screen, draw menu frame, and create an empty old note map */
  if FULL then do;
    call MENU_FRAME; /* Clear screen and draw menus */
    OLD_TIME_CUR_POS = -1; /* Forces TIME_CURSOR to plot cursor for first time */
    write(SECT$) = OLD_SECT; /* Set sect length to 1 */
    write(DATA$) = 1;
    call EXTSET (OLD_SECT,8,8,16); /* Set all part list pointers for empty lists */
  end;

  /* Refresh cursor if necessary */
  if COMPARE_INT(NEW_POS,OLD_POS,8) \ FULL then do;
    call data_level (2); 
    if ~FULL then do; /* Erase old cursor */
      call DRAW_CURSOR;
    end;
    call blockmove (NEW_POS,OLD_POS,8); /* Update cursor position vector and draw new cursor */
    call DRAW_CURSOR; /* Plot new cursor */
    call data_level (0);
  end;

  call REFRESH_INITIALIZE; /* Must happen before the 1st ABORT_REFRESH */

  // Restart if full refresh needed
  if full.seqed.refresh then do; /* start over again on new sequence */
    goto retry;
  end;

  call SEQED.TIME.CURSOR;  /* Must happen after REFRESH_INITIALIZE so that TICS is set */
  if ABORT_REFRESH(0) then do; /* Flip-out test - calls RunSynLoop and SEQED.TIME.CURSOR */
    transparent_mode;
    return;
  end;

  /* Draw instructions */
  if HELP_MODE & ~FULL then call PRINT_INSTRUCTIONS;
  if ABORT_REFRESH(0) then do; /* Give up on new sequence */
    transparent_mode;
    return;
  end;

  /* Redraw items in main menu area */

  call viewport (0,SCREEN_RIGHT,0,SCREEN_TOP);
  call window   (0,SCREEN_RIGHT,-MENU_TOP,SCREEN_TOP-MENU_TOP);

  call REFRESH_MAIN_MENU (0,7,MENU_REFRESH);

  MENU_REFRESH = false;
  if ABORT_REFRESH(0) then do; /* start over again on new sequence */
    transparent_mode;
    return;
  end;

  /* Step 2 - Build new note map */

  call BUILD_NOTE_MAP(core_area);

  if (NOABORT) \ (ABORTING) \ (ABORT_REFRESH(1)) then do; /* Give up on new sequence or character typed here */
    transparent_mode;
    return;
  end;

  /* Step 3 - Compare note maps and refresh as needed */

  do RL_STAFF = 0 to STAVES;

    write(SECT$) = NEW_SECT;
    write(WORD$) = 7 + RL_STAFF;
    NSP          = read(DATI$); if RL_STAFF = 0 then NSP = 16;
    NEP          = read(DATA$);
    write(SECT$) = OLD_SECT;
    write(WORD$) = 7 + RL_STAFF;
    OSP          = read(DATI$); if RL_STAFF = 0 then OSP = 16;
    OEP          = read(DATA$);

    REF_PART_MENU = false;
    REF_CLEF      = false;
    REF_TNAME     = false;
    EDIT_CURSOR_REFRESH = false;

    if OSP = OEP then do; /* If list is empty, give it a part menu area */
      call INSERT_POSITION (NSP,OSP,16);
      OEP           = OEP + 16;
      REF_PART_MENU = true;
      REF_CLEF      = true;
      REF_TNAME     = true;
    end;
    else if COMPARE (NEW_SECT,NSP,OLD_SECT,OSP,8) then do; /* If part menus are different, erase old line and update part menu area */

      call data_level (1);
      call LOAD_PART_VECTOR (NEW_SECT,NSP); I = CLEF;
      call LOAD_PART_VECTOR (OLD_SECT,OSP);
      call PLOT_INITIALIZE (RL_STAFF);
      call PLOT_CLEFKEY; REF_CLEF = true;
      if I ~= CLEF & I = SINGLE_CLEF# then do;
        call TIME_CURSOR (0);
        call STAFF_LINES (false);
        call TIME_CURSOR (TIME_CUR_POS);
      end;

      if REFRESH_LIST (null,null,OSP,OEP) /* Erase the list */
      then STAFF_REFRESH = STAFF_REFRESH \ shl(1,RL_STAFF); /* Staff got religious (wholy) and needs refresh */
      if ABORTING then do; /* Give up if char in input buffer or something changed */
        transparent_mode;
        return;
      end;

      call LOAD_PART_VECTOR (MENU_BASE,8+RL_STAFF*8);
      call data_level (2);
      do I = 0 to 6; /* Erase part menu */
        call DISPLAY_ENTRY (I,false);
        call CHECK.NEXT.EVENT;
      end;
      REF_PART_MENU = true;

      write(SECT$) = OLD_SECT;
      write(WORD$) = 7 + RL_STAFF;
      OSP          = read(DATI$); if RL_STAFF = 0 then OSP = 16;
      OEP          = read(DATA$);
      call COPY.EXT.MEM (NEW_SECT,NSP,OLD_SECT,OSP,8); /* Copy new part menu data to old map */

    end;

    call LOAD_PART_VECTOR (NEW_SECT,NSP);
    call PLOT_INITIALIZE (RL_STAFF);

    if ~FULL & COMPARE (NEW_SECT,NSP+8,OLD_SECT,OSP+8,8) then do; /* If timbre names are different, update */
      vector_mode;
      call data_level (1); alpha_mode;
      call PLOT_TIMBRE_NAME (OSP+8);	/* erase old timbre name */
      call COPY.EXT.MEM (NEW_SECT,NSP+8,OLD_SECT,OSP+8,8);
      REF_TNAME = true;
    end;

    vector_mode;
    call data_level (2); alpha_mode;
    if REF_PART_MENU then do I = 0 to 6;  /* Fill part menu */
      call DISPLAY_ENTRY (I,false);
      call CHECK.NEXT.EVENT;
    end;
    call data_level (0);

    if FULL then do;
      if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF
      then call DRAW_CURSOR;
      call TIME_CURSOR (0);
      call STAFF_LINES (CLEF = SINGLE_CLEF#);
      call PLOT_CLEFKEY; REF_CLEF = false;
      STAFF_REFRESH = STAFF_REFRESH & ~shl(1,RL_STAFF);
      call TIME_CURSOR (TIME_CUR_POS);
      if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF
      then call DRAW_CURSOR;
    end;

    I = REFRESH_LIST (NSP,NEP,OSP,OEP); /* Compare and update the list and screen */
    if I then STAFF_REFRESH = STAFF_REFRESH \ shl(1,RL_STAFF); /* Staff needs refresh */

    call data_level (0);
    if REF_CLEF  then call PLOT_CLEFKEY;

    if ABORTING then do; /* Give up if char in input buffer or something changed */
      transparent_mode;
      return;
    end;

    I = shl(1,RL_STAFF);
    if ~FULL & (STAFF_REFRESH & I) ~= 0 then do; /* Refresh staff lines if they were damaged */
      if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF
      then call DRAW_CURSOR;
      call TIME_CURSOR (0);
      call PLOT_TIES (NSP+16,NEP);
      call STAFF_LINES (CLEF = SINGLE_CLEF#);
      STAFF_REFRESH = STAFF_REFRESH & ~I;
      REF_TNAME     = true;
      call TIME_CURSOR (TIME_CUR_POS);
      if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF
      then call DRAW_CURSOR;
    end;

    if REF_TNAME then do;
		 vector_mode; call DATA_LEVEL(0);
       alpha_mode;
       call PLOT_TIMBRE_NAME (OSP+8);
    end;

    if EDIT_CURSOR_REFRESH & SEQED_MODE = EDIT# then do; /* Refresh edit cursor again if note map under it has changed */
      call SETUP_POSITION(core_area);
      if COMPARE_INT(NEW_POS,OLD_POS,8) then do;
        call data_level (1);
        call DRAW_CURSOR;
        call blockmove (NEW_POS,OLD_POS,8); /* Update cursor position vector and draw new cursor */
        call data_level (0);
        call DRAW_CURSOR; /* Plot new cursor */
      end;
    end;

  end;

  call DATA_LEVEL(0);
  transparent_mode; 
  call pos.seqed.cursor; /* Position VT100 cursor */

end SEQED.REFRESH;
