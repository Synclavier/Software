/*
   11/13/87 - lpg - modified mouse add to advance cursor (imitate lower-case key)
   10/23/87 - lpg - added mouse insert/delete
   5/30/86 - adt & eg - updated for rel M
   5/20/86 - eg - made procedures and some variables pub.
   5/15/86 - "official" creation of release-M modules
*/

RANGE_PITCH:procedure (C) returns (fixed) swapable;

  dcl (C,I)    fixed,
      KEY_NO   fixed,
      OCT      fixed,
      (YPOS,Y) fixed;

  dcl SHARP_POS fixed data (L0,L0,S0,S0,L1,S1,S1,L2,L2,S2,S2,L3),
      FLAT_POS  fixed data (L0,S0,S0,L1,L1,S1,L2,L2,S2,S2,L3,L3),
      SHR_NOTES fixed data (0,5,0,7,2,9,4,11),
      FLT_NOTES fixed data (0,11,4,9,2,7,0,5),
      KEY_NOS   fixed data (9,11,0,2,4,5,7);

  KEY_NO = KEY_NOS(C - asc.a);
  if CUR_ACCD > 0 then KEY_NO = KEY_NO + (CUR_ACCD - 2);
  else do;
    I = KEY_CODES(KEY);
    if      I < 0 then do I = 1 to -I;
      if KEY_NO = FLT_NOTES(I) then KEY_NO = KEY_NO - 1;
    end;
    else if I > 0 then do I = 1 to  I;
      if KEY_NO = SHR_NOTES(I) then KEY_NO = KEY_NO + 1;
    end;
  end;

  if KEY_NO = -1 then KEY_NO = 11;
  if KEY_NO = 12 then KEY_NO = 0;

  if (PART_BITS & P_FLAT) ~= 0 
  then YPOS = FLAT_POS(KEY_NO);
  else YPOS = SHARP_POS(KEY_NO);

  YPOS = YPOS + CLEF_TABLE(CLEF); /* Adjust note to clef */
  OCT  = 0;
  Y    = ((YPOS + 21 * (-3)) / 3 - 30) * 4;
  do while Y <= RANGE_CENTER - 16;
    OCT = OCT + 1;
    Y   = ((YPOS + 21 * (OCT-3)) / 3 - 30) * 4;
  end;
  KEY_NO   = KEY_NO - 12 + OCT*12;
  CUR_ACCD = 0;
  RANGE_CENTER = Y;
  if RANGE_CENTER > MAX_RANGE then RANGE_CENTER = MAX_RANGE;
  if RANGE_CENTER < MIN_RANGE then RANGE_CENTER = MIN_RANGE;

  return (KEY_NO);

end RANGE_PITCH;

EDIT_FUNCTION:procedure (C,CORE_AREA) swapable;
  dcl core_area  array;
  dcl callarg(1) fixed;
  dcl p          fixed;

  dcl C     fixed, 		/* Command character */
      (I,J) fixed,
      ST(1) fixed, 		/* Time of current position */
      T(1)  fixed, 		/* Time temp */
      SECT  fixed, 		/* Note pointer MSB */
      WORD  fixed, 		/* Note pointer LSB */
      PTCH  fixed,
      DUR   fixed,
      SHIFT boolean; 	/* Set if C is an upper case letter */

   FIND_NOTE:procedure (SECT,WORD,TRACK) returns (boolean);

     /* Must be called with #'s set and track present */

     dcl SECT  fixed,
         WORD  fixed,
         TRACK fixed,
         T(1)  fixed;

     T(1) = EARLIEST_ST(1); 		/* We will scan in abs time for efficiency */
     T(0) = EARLIEST_ST(0);
     call UNCONVERT_TIME (T,0); 	/* T = lower boundary of screen window */

     /* Scan backward until prior to earliest note */
     do while #ANYR & (#NMSB igt T(1) \ (#NMSB = T(1) & #NLSB ige T(0)));
       call BACKUP.TO.PRIOR.NOTE;
     end;
     /* Scan forward until we find note */
     do while #SEC ~= SECT \ #WRD ~= WORD;
       if ~#ANYF then return (false);
       call ADVANCE.TO.NEXT.NOTE;
     end;
     call LOOK.UP.NOTE.INFO;
     return (true);

   end FIND_NOTE;

  if RECD <> 0 then return;   								/* no editing while recording */

  if (NEW.ABLE.SEQ.INFO & 6) <> 0   					   /* new notes or titles - scan off info before tossing character */
  then do;                         							/* this allows for speedy entry of edit characters */
     NEW.ABLE.SEQ.INFO = NEW.ABLE.SEQ.INFO and (not(6));  	/* names and notes only */
     NOABORT = 1;
     call SEQED.REFRESH(CORE_AREA);
     NOABORT = 0;
  end;

  if STAFF > 0 then do;
    write(SECT$) = NEW_SECT;      /**/
    write(WORD$) = 7 + STAFF;
    P            = read(DATA$);
  end;
  else P = 16;
  call LOAD_PART_VECTOR (NEW_SECT,P);  /**/
  call FIND_POSITION(core_area);

  if (NEW.ABLE.SEQ.INFO \ NEW.SEQ.INFO)<> 0 then return; /* must be a new sequence or something */

  BLOCKS = NEW_POS(BLOCKS#); /* get valid block count for this cursor */

  if lower.a <= C & C <= lower.z then do;
    C     = C - 32;
    SHIFT = false;
  end;
  else SHIFT = true;

  TRACK_PRESENT = LOAD.SEQ.GLOBALS (NUM.KBD.TRACKS+TRACK);

  if CHORD_LENGTH > 0 then do; /* Calculate various statistics about this note */

    SECT = CORE_AREA(CHORD_INDEX * 3 - 1 + 1); /* Note pointer */
    WORD = CORE_AREA(CHORD_INDEX * 3 - 1 + 2);

    if SECT ~= 0 then do;

      if ~TRACK_PRESENT then return;
      call FIND_NOTE (SECT,WORD,TRACK);

      PTCH = (#W2 & MASK6) + 12;
      if #W1 then do;
        if (#W4 & S_OCTDN) ~= 0 then PTCH = PTCH - 12;
        if (#W4 & S_OCTUP) ~= 0 then PTCH = PTCH + 12;
      end;

      T(1)  = #NMSB;
      T(0)  = #NLSB;
      DUR   = SIEVE (BLOCKS,TICS,CONVERT_TIME(T,GET_DURATION));

      ST(1) = START_TIME(1);
      ST(0) = START_TIME(0);
      call ADD32 (ST,SIEVE(TICS,BLOCKS,POSITION));

    end;

  end;
  else SECT = 0;

/* DEBUG
  if C = sp then do;
    transparent_mode;
    call cpos(20,40); call pstring('ST1: '); call pnum(ST(1),0); call pstring('  ');
    call cpos(21,40); call pstring('ST0: '); call pnum(ST(0),0); call pstring('  ');
    T(1) = #NMSB;
    T(0) = #NLSB;
    call CONVERT_TIME(T,0);
    call cpos(22,40); call pstring('AT1: '); call pnum(T(1),0);  call pstring('  ');
    call cpos(23,40); call pstring('AT0: '); call pnum(T(0),0);  call pstring('  ');

    call cpos(20,60); call pstring('PTC: '); call pnum(PTCH,0); call pstring('  ');
    call cpos(21,60); call pstring('DUR: '); call pnum(DUR ,0); call pstring('  ');
    call cpos(22,60); call pstring('PTS: '); call pnum(SECT,0); call pstring('  ');
    call cpos(23,60); call pstring('PTW: '); call pnum(WORD,0); call pstring('  ');
  end;
*/
  if C = del & SECT ~= 0 then do; /* Delete a note */

    if (CORE_AREA(CHORD_INDEX*3 - 1) & N_CTIE) ~= 0 then do;

      I = RESOLUTION;
      do J = 1 to (shr(CORE_AREA(1),5) & MASK3);
        I = I / 2;
      end;
      if (CORE_AREA(CHORD_INDEX*3 - 1) & N_DOT) ~= 0 then I = I + I/2;
      DUR = DUR - I;

      T(1) = ST(1); T(0) = ST(0);

      callarg(0) = 0;   /* msb */
      callarg(1) = UNCONVERT_TIME(T,SIEVE(TICS,BLOCKS,DUR));

      call CHANGE.NOTE.DURATION (callarg,0);

    end;
    else call REMOVE.NOTE.RECORD;

  end;

  else if C = asc.p & SECT ~= 0 & PTCH < 84 then do; /* Pitch up */
    PTCH = PTCH + 1;
    call CHANGE.NOTE.PITCH (PTCH);
  end;
  else if C = scolon & SECT ~= 0 & PTCH > 0 then do; /* Pitch down */
    PTCH = PTCH - 1;
    call CHANGE.NOTE.PITCH (PTCH);
  end;

  else if (C = asc.lbr \ C = asc.rbr) & SECT ~= 0 then do; /* Duration shorter */
    if C = asc.lbr then if DUR > 1 then DUR = DUR - 1; else;
    else DUR = DUR + 1;
    T(1) = ST(1); T(0) = ST(0);
    callarg(0) = 0;   /* msb */
    callarg(1) = UNCONVERT_TIME(T,SIEVE(TICS,BLOCKS,DUR));
    call CHANGE.NOTE.DURATION (callarg,0);
  end;

  else if (C = lbrace \ C = rbrace \ C = asc.j) & SECT ~= 0 &
          (CORE_AREA(CHORD_INDEX*3 - 1) & N_CTIE) = 0
  then do; /* Adjust start time */

    EDIT_SETUP = true;
    if      C = lbrace & POSITION > 0          then POSITION = POSITION - 1; /* Start time backward */
    else if C = rbrace & POSITION < BLOCKS - 1 then POSITION = POSITION + 1; /* Start time forward */
    else EDIT_SETUP = false;

    T(1) = START_TIME(1);
    T(0) = START_TIME(0);
    call ADD32 (T,SIEVE(TICS,BLOCKS,POSITION));
    call UNCONVERT_TIME (T,0);

    callarg(0) = T(1);   /* MSB */
    callarg(1) = T(0);   /* LSB */
    call CHANGE.NOTE.START (callarg,0);
  end;

  else if C = asc.q \ C = asc.sharp then CUR_ACCD = 3; /* Sharp - raise next pitch by 1/2 step */
  else if C = asc.s                 then CUR_ACCD = 2; /* Natural */
  else if C = asc.z                 then CUR_ACCD = 1; /* Flat - lower next pitch by 1/2 step */

  else if asc.0 <= C & C <= asc.9 then do case C - asc.0; /* Set current duration */
    ;                       /* 0 */
    CUR_DURATION = "H0101"; /* 1 */
    CUR_DURATION = "H0102"; /* 2 */
    CUR_DURATION = "H0120"; /* 3 */
    CUR_DURATION = "H0104"; /* 4 */
    ;                       /* 5 */
    CUR_DURATION = "H0110"; /* 6 */
    ;                       /* 7 */
    CUR_DURATION = "H0108"; /* 8 */
    ;                       /* 9 */
  end;
  else if C = dot then do; /* Dotted note - increase dur by 50% */

    I = shr(CUR_DURATION,8);
    J = CUR_DURATION & LBYTE;
    if I then do;
      I = shl(I,1);
      J = shl(J,1);
      if J > 255 then return;
    end;
    I = I + shr(I,1);
    if I > 255 then return;
    CUR_DURATION = shl(I,8) \ J;

  end;
  else if (asc.a <= C & C <= asc.g) \ C = sp \ C = asc.r then do; /* Set current pitch and/or enter note */

    DUR = (shr(CUR_DURATION,8) * RESOLUTION) / (CUR_DURATION & LBYTE);
    J   = POSITION;

    if C = asc.r \ C = sp \ SHIFT = 0 then do;
      POSITION = POSITION + DUR;
      if POSITION >= BLOCKS then do;
         if MEASURES > 1 
         then do;
            if (((START_CLICK-1) MOD CLICKS_MEASURE) != 0) full.seqed.refresh = true;
            START_CLICK = START_CLICK + (MEASURES - 1) * CLICKS_MEASURE;
            POSITION = POSITION - (MEASURES - 1) * BLOCKS/MEASURES;
         end;
         else do;
            START_CLICK = START_CLICK + CLICKS_MEASURE;  /* case for one measure only */
            POSITION = POSITION - BLOCKS/MEASURES;
         end;
      end;
      EDIT_SETUP = true;
      if C = asc.r then return; /* "Rest" just advances cursor */
    end;

    DUR = SIEVE(TICS,BLOCKS,DUR);
    if DUR = 0 then DUR = 1;
    if C ~= sp then CUR_PITCH = RANGE_PITCH (C);

    T(1) = START_TIME(1);
    T(0) = START_TIME(0);
    call ADD32 (T,SIEVE(TICS,BLOCKS,J));
    DUR = UNCONVERT_TIME (T,DUR);

    REC(0) = 0;
    REC(1) = DUR;
    REC(2) = CUR_PITCH + 12;
    REC(3) = 0;
    REC(4) = RTE.MAX;
    REC(5) = 0;

    /* Add ERROR CHECKING */
    if ~TRACK_PRESENT then do; /* Set up a track if there isn't one yet */
      call ALLOCATE.TRK.HEADER (NUM.KBD.TRACKS+TRACK); /* Create a new track header */
      call LOAD.SEQ.GLOBALS (NUM.KBD.TRACKS+TRACK);    /* Load #'s */
      call COPY.TIMBRE (0,NUM.KBD.TRACKS+TRACK);       /* Give it the keyboard timbre */
      call LOAD.SEQ.GLOBALS (NUM.KBD.TRACKS+TRACK);    /* Load #'s */
    end;
    call BUILD.NOTE.RECORD (REC);        /* Assemble a correct note record in #W's */
    call INSERT.NOTE.RECORD (T(1),T(0)); /* Insert it into track */

  end;

end EDIT_FUNCTION;

SEQED_EDIT:procedure (mode,C,row,col,core_area,action) swapable;  /* digest characters in edit mode */
  dcl mode  fixed;          /* in.char, in.abss */
  dcl C     fixed;          /* character/staff  */
  dcl row   fixed;          /* row              */
  dcl col   fixed;
  dcl core_area array;
  dcl action    fixed;      /* indicated delete/position/add */

  dcl (P) fixed,
      SET_Y boolean;

  if STAFF > 0 then do;
    write(SECT$) = NEW_SECT;      /**/
    write(WORD$) = 7 + STAFF;
    P            = read(DATA$);
  end;
  else P = 16;
  call LOAD_PART_VECTOR (NEW_SECT,P);  /**/
  call FIND_POSITION(core_area);

  EDIT_SETUP = false;
  SET_Y      = false;

  if mode = in.abss then do;      /* mouse click */
     if c < 256 then do;          /* main or part menu */
        SEQED_MODE = MENU#;       /* back to menu      */
        menu_row = c*4+row;       /* staff * 4 + row   (staff=0 for main menu) */
        menu_col = col;
        EDIT_SETUP = true;
        if MENU_ROW >= 100 then do;
          OVERALL    = true;
          ENTRY      = shl(MENU_ROW-100,2) \ MENU_COL;
        end;
        else do;
          OVERALL    = false;
          ENTRY      = shl(MENU_ROW & MASK2,1) \ MENU_COL;
          STAFF      = shr(MENU_ROW,2);
        end;
     end;
     else do;                     /* music area select */
        if action = #action_delete
        then do;
           call MAP.XY.TO.NOTE(C&255,ROW,COL,CORE_AREA,#map_to_old);
           call EDIT_FUNCTION(del, core_area);              /* imitate the delete key press */
        end;
        else if action = #action_none
        then do;
           call MAP.XY.TO.NOTE(C&255,ROW,COL,CORE_AREA,#map_to_old);
        end;
        else if action = #action_add
        then do;
           call MAP.XY.TO.NOTE(C&255,ROW,COL,CORE_AREA,#map_to_new);
           do case CLEF;
              CALL EDIT_FUNCTION(lower.a + (((range_center/4) + 1) mod 7), core_area);  /* G Clef */
              CALL EDIT_FUNCTION(lower.a + (((range_center/4) + 2) mod 7), core_area);  /* Alto clef */
              CALL EDIT_FUNCTION(lower.a + (((range_center/4) - 1) mod 7), core_area);  /* Tenor Clef */
              CALL EDIT_FUNCTION(lower.a + (((range_center/4) + 3) mod 7), core_area);  /* Bass Clef */
              CALL EDIT_FUNCTION(lower.a + (((range_center/4) + 1) mod 7), core_area);  /* Percussion Clef */
              CALL EDIT_FUNCTION(lower.a + (((range_center/4) + 1) mod 7), core_area);  /* Single Perc. Clef */
              CALL EDIT_FUNCTION(lower.a + (((range_center/4) + 1) mod 7), core_area);  /* G Clef */
           end;
        end;
        return;
     end;
  end;
  else if  C = u.arr then do;
    if CHORD_INDEX > 1 then do;
      CHORD_INDEX = CHORD_INDEX - 1;
      SET_Y       = true;
    end;
    else if RANGE_CENTER < RANGE_TOP then do;
      RANGE_CENTER = RANGE_CENTER + 7*VERTICAL_UNIT; /* Up an octave */
      if RANGE_CENTER > RANGE_TOP then RANGE_CENTER = RANGE_TOP;
    end;
    else if STAFF > 0 then do;    /* move up to next staff */
      STAFF        = STAFF - 1;
      RANGE_CENTER = RANGE_BOT;
      CHORD_INDEX  = 31; /* Set to some no. larger than largest acctual value */
      EDIT_SETUP        = true;
    end;
    else do;                      /* move up to main menu */
      SEQED_MODE = MENU#;
      OVERALL    = true;
      MENU_ROW   = 101;
      P = 0;
      do while OCOLS(P+1) < OLD_POS(X#); P = P + 1; end;
      if OLD_POS(X#) - OCOLS(P) < OCOLS(P+1) - OLD_POS(X#)
      then MENU_COL = P;
      else MENU_COL = P + 1;
      ENTRY = shl(MENU_ROW-100,2) \ MENU_COL;
      EDIT_SETUP = true;
    end;
  end;
  else if C = d.arr then do;
    if CHORD_INDEX < CHORD_LENGTH then do;
      CHORD_INDEX = CHORD_INDEX + 1;
      SET_Y       = true;
    end;
    else if RANGE_CENTER > RANGE_BOT then do;
      RANGE_CENTER = RANGE_CENTER - 7*VERTICAL_UNIT; /* Down an octave */
      if RANGE_CENTER < RANGE_BOT then RANGE_CENTER = RANGE_BOT;
    end;
    else if STAFF < STAVES then do;  /* move down one staff */
      STAFF        = STAFF + 1;
      RANGE_CENTER = RANGE_TOP;
      EDIT_SETUP        = true;
    end;
  end;
  else if C = l.arr then do;
    if POSITION = 0 then do;
      SEQED_MODE = MENU#;
      if OVERALL then do;
        MENU_COL = 0;
        MENU_ROW = 0;
        OVERALL  = false;
      end;
    end;
    else POSITION = POSITION - 1;
    EDIT_SETUP = true;
  end;
  else if C = r.arr then do;
    if POSITION = OLD_POS(BLOCKS#) - 1 
    then do;
       if ((MEASURES > 1) && (((START_CLICK-1) MOD CLICKS_MEASURE) != 0)) full.seqed.refresh = true;
       if MEASURES > 1
       then START_CLICK = START_CLICK + (MEASURES -1) * CLICKS_MEASURE;
       else START_CLICK = START_CLICK + CLICKS_MEASURE;  /* case for one measure only */
    end;
    else POSITION = POSITION + 1;
    EDIT_SETUP = true;
  end;
  else if C = asc.min then do;
    if STAFF > 0 then do;
      STAFF = STAFF - 1;
      EDIT_SETUP = true;
    end;
  end;
  else if C = asc.equ then do;
    if STAFF < STAVES then do;
      STAFF = STAFF + 1;
      EDIT_SETUP = true;
    end;
  end;
  else if C = tab then do;
    SEQED_MODE = MENU#;
    EDIT_SETUP      = true;
  end;
  else call EDIT_FUNCTION (C,CORE_AREA);

  if EDIT_SETUP then do;
    if STAFF > 0 then do;
      write(SECT$) = NEW_SECT;    /**/
      write(WORD$) = 7 + STAFF;
      P            = read(DATA$);
    end;
    else P = 16;
    call LOAD_PART_VECTOR (NEW_SECT,P);  /**/
    call SETUP_POSITION(core_area);
  end;
  else do;
    if SET_Y then do;
      NEW_POS(Y#)  = ((CORE_AREA(CHORD_INDEX*3 - 1) & MASK6) - 30) * VERTICAL_UNIT;
      RANGE_CENTER = NEW_POS(Y#);
      if RANGE_CENTER > MAX_RANGE then RANGE_CENTER = MAX_RANGE;
      if RANGE_CENTER < MIN_RANGE then RANGE_CENTER = MIN_RANGE;
    end;
    NEW_POS(RANGE#) = RANGE_CENTER;
  end;

end SEQED_EDIT;
