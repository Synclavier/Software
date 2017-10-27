/*
   11/2/87  - lpg - fixed enharmonic problems with B#/Cb, E#/Fb
   10/23/87 - lpg - added mouse insert/delete
   5/30/86 - adt & eg - updated to work with rel M
   5/20/86 - eg - made procedures and some variables pub.
   5/15/86 - "official" creation of release-M modules
*/

ADD32:procedure (A,N); /* A = A + N (Won't wrap) */

  dcl A fixed array,
      N fixed;

  A(LSB) = A(LSB) + N;
  if A(LSB) ilt N then do;
    if A(MSB) = -1 then A(LSB) = -1;
                   else A(MSB) = A(MSB) + 1;
  end;

end ADD32;

SUB32:procedure (A,N); /* A = A - N (Won't wrap) */

  dcl A fixed array,
      N fixed;

  N = A(LSB) - N;
  if N igt A(LSB) then do;
    if A(MSB) = 0 then N      = 0;
                  else A(MSB) = A(MSB) - 1;
  end;
  A(LSB) = N;

end SUB32;

POWER:procedure (X) swapable; /* Return TRUE iff 0 < X <= 64 and X = 2^K */

  dcl (X,I,B) fixed;

  B = 1;
  do I = 0 to 6;
    if X = B then return (true);
    B = shl(B,1);
  end;
  return (false);

end POWER;

CONVERT_TIME:procedure (T,D);

  dcl T fixed array, /* Incoming synclavier time */
      D fixed;       /* Incoming synclavier duration */

  dcl XMEM_SECT fixed, /* Temps to hold xmem regs */
      XMEM_WORD fixed;

  /* Convert from real sequence time to simulated metronome = 1000 time */
  if CLICK.TRACK.MODE ige 4 then do;
    XMEM_SECT = read(SECT$);
    XMEM_WORD = read(WORD$);
    call REMAP.WITH.LIVE.CLICK (T(1),T(0),0,D,1);
    T(1) = REMAPPED.TIME.MSB;
    T(0) = REMAPPED.TIME.LSB;
    D    = REMAPPED.DUR.LSB;
    if REMAPPED.DUR.MSB ~= 0 then D = -1;
    write(SECT$) = XMEM_SECT;
    write(WORD$) = XMEM_WORD;
  end;

  call SUB32 (T,ZERO.TIME);

  return (D);

end CONVERT_TIME;

UNCONVERT_TIME:procedure (T,D);

  dcl T fixed array, /* Incoming beat time */
      D fixed;       /* Incoming beat duration */

  dcl XMEM_SECT fixed, /* Temps to hold xmem regs */
      XMEM_WORD fixed;

  call ADD32 (T,ZERO.TIME);

  /* Convert from real sequence time to simulated metronome = 1000 time */
  if CLICK.TRACK.MODE ige 4 then do;
    XMEM_SECT = read(SECT$);
    XMEM_WORD = read(WORD$);
    call REMAP.WITH.LIVE.CLICK (T(1),T(0),0,D,0);
    T(1) = REMAPPED.TIME.MSB;
    T(0) = REMAPPED.TIME.LSB;
    D    = REMAPPED.DUR.LSB;
    if REMAPPED.DUR.MSB ~= 0 then D = -1;
    write(SECT$) = XMEM_SECT;
    write(WORD$) = XMEM_WORD;
  end;
  return (D);

end UNCONVERT_TIME;

SIEVE:procedure (BASE,GRID,POSN) returns (fixed);

	/* This does not swap.  It could but it's small and it's called a lot */

	dcl BASE  fixed,
		 GRID  fixed,
		 POSN  fixed;

	if GRID then do;
		GRID = shl(GRID,1);
		POSN = shl(POSN,1);
	end;

	write(5)   = BASE;
	write(6)   = POSN;
	write(NOP) = read(5);
	write(7)   = GRID;
	return read(5) + (read(4) ige shr(GRID,1));

end SIEVE;

GET_DURATION:procedure returns (fixed) swapable;

  dcl (DURATION,I) fixed;

  DURATION = (shr(#W1,1) & "006000") \ shr(#W2,6);
  do I = 1 to (rot(#W1,3) & MASK2);
    write(5) = DURATION;
    write(6) = 4;
    if read(4) ~= 0 then do;
      DURATION = -1; I = 3;
    end;
    else DURATION = read(5);
  end;
  return (DURATION);

end GET_DURATION;

STRING_TO_FIXED:procedure (BUFF) returns (fixed) swapable;

  dcl BUFF  fixed array,
      (C,I) fixed,
      (M,S) fixed;

  M = 1; I = BUFF(0) - 1; S = 0;
  do while I >= 0;
    C = byte(BUFF,I);
    if asc.0 <= C & C <= asc.9 then do;
      S = S + (C - asc.0) * M;
      M = M * 10;
    end;
    else if I = 0 & C = asc.min then S = -S;
    else if I > 0 \ C ~= asc.plus then return (nullint);
    I = I - 1;
  end;

  return (S);

end STRING_TO_FIXED;

STRING_TO_PITCH:procedure (BUFF) returns (fixed) swapable;

  dcl BUFF fixed array,
      (KEY,C) fixed,                       /* Key number */
      KEYTAB  fixed data (9,11,0,2,4,5,7); /* KeyTable */

  C = byte(BUFF,0) - asc.a;                /* Change the character into a number */
  if C < 0 \ 6 < C then return (nullint);  /* Make sure the number is within bounds */
  KEY = KEYTAB(C); if BUFF(0) = 1 then return (KEY + 24); /* Use number as an index to the keytable to determine the key # */ 
                                                          /* If Pitch, return the code */

  C = byte(BUFF,1);    /* Determine if the key is a sharp or a flat */
  if C = asc.f then do; /* If a flat */
    KEY = KEY - 1;      /* Reduce key by a semitone */
    if BUFF(0) = 2 then return (KEY + 24); /* If no octave, return the keycode */
    C = byte(BUFF,2);   /* Get the specified octave number */
  end;
  else if C = asc.sharp then do;   	/* If a sharp */
    KEY = KEY + 1;            		/* Increase key by a semitone */
    if BUFF(0) = 2 then return (KEY + 24); /* If no octave number is specified, return the key code */
    C = byte(BUFF,2);         		/* Get the specified octave number */
  end;

  if asc.0 <= C & C <= asc.7 then return (KEY + 12 * (C - asc.1)); /* Return the key number offset by the octave */
  return (nullint);                                                /* Otherwise return a null value */

end STRING_TO_PITCH;

FRACTION_TO_FIXED:procedure (BUFF) returns (fixed) swapable;

  dcl BUFF    fixed array,
      TEMP(2) fixed,
      (S,N)   fixed,
      (I,C)   fixed;

  TEMP(0) = 0;
  I       = 0;
  N       = 0;
  S       = 1;

  do while S > 0;

    C = byte(BUFF,I);

    if C = slash & S = 1 then do;
      N       = STRING_TO_FIXED (TEMP);
      TEMP(0) = 0;
      S       = 2;
    end;
    else if asc.0 <= C & C <= asc.9 then do;
      if TEMP(0) = 3 then return (0);
      call pbyte (TEMP,TEMP(0),C); TEMP(0) = TEMP(0) + 1;
    end;
    else if C ~= sp then return (0);

    I = I + 1;
    if I = BUFF(0) then S = 0;

  end;

  return (shl(N,8) \ STRING_TO_FIXED (TEMP));

end FRACTION_TO_FIXED;

FREE:procedure (P) swapable; /* Free a record in core */

  dcl P pointer;

  core(FREE_TAIL) = P;
  core(P)         = null;
  FREE_TAIL       = P;

end FREE;

ALLOCATE:procedure returns (pointer) swapable; /* Allocate a record in core */

  dcl P pointer;

  if FREE_HEAD = null then do; /* Allocate a new record */
    if LIST_BASE + 8 igt LIST_TOP then return (null); /* Out of room in core area */
    P         = LIST_BASE;
    LIST_BASE = LIST_BASE + 8;
  end;
  else do;
    P         = FREE_HEAD;
    FREE_HEAD = core(FREE_HEAD);
    if FREE_HEAD = null then FREE_TAIL = addr(FREE_HEAD);
  end;

  write(R0)   = P;             /* Destination = P       */
  write(R13)  = addr(REC(1));  /* Source       = REC(1) */

  write(MR0I) = null;          /* core(P+0)   = null    */
  write(MR0I) = read(MR13I);   /* core(P+1)   = REC(1) */
  write(MR0I) = read(MR13I);   /* core(P+2)   = REC(2) */
  write(MR0I) = read(MR13I);   /* core(P+3)   = REC(3) */
  write(MR0I) = read(MR13I);   /* core(P+4)   = REC(4) */
  write(MR0I) = read(MR13I);   /* core(P+5)   = REC(5) */
  write(MR0I) = read(MR13I);   /* core(P+6)   = REC(6) */
  write(MR0I) = read(MR13I);   /* core(P+7)   = REC(7) */

  return (P);

end ALLOCATE;

LOAD_PART_VECTOR:procedure (SECT,WORD) swapable;

  dcl SECT fixed,
      WORD fixed;

  /* Leaves xmem at 1st word after header (1st word in list) */

  write(SECT$) = SECT + shr(WORD,8);
  write(WORD$) = WORD;

                                          TRACK          = read(DATI$) & LBYTE;
  MULTIPLIER        = shr(read(DATA$),8); RESOLUTION     = read(DATI$) & LBYTE;
  CLEF              = shr(read(DATA$),8); KEY            = read(DATI$) & LBYTE;
  MEASRS            = shr(read(DATA$),8); MAX_KEY_LENGTH = read(DATI$) & LBYTE;
                                          CLICNOTE       = read(DATI$);
                                          CLICS          = read(DATI$);
                                          MAX_BLOCKS     = read(DATI$);
                                          PART_BITS      = read(DATI$);

end LOAD_PART_VECTOR;

DUMP_PART_VECTOR:procedure (SECT,WORD) swapable;

  dcl SECT fixed,
      WORD fixed;

  /* Leaves xmem at 1st word after header (1st word in list) */

  write(SECT$) = SECT + shr(WORD,8);
  write(WORD$) = WORD;

  write(DATI$) = TRACK;
  write(DATI$) = shl(MULTIPLIER,8) \ RESOLUTION;
  write(DATI$) = shl(CLEF      ,8) \ KEY;
  write(DATI$) = shl(MEASRS    ,8) \ MAX_KEY_LENGTH;
  write(DATI$) = CLICNOTE;
  write(DATI$) = CLICS;
  write(DATI$) = MAX_BLOCKS;
  write(DATI$) = PART_BITS;

end DUMP_PART_VECTOR;

KEY_NOTE:procedure (KEY_NO) returns (fixed) swapable;

  dcl KEY_NO      fixed,
      ACC_IND     fixed,
      OCTAVE      fixed,
      ADJ_YPOS    fixed,
      (ACC,NOTE)  fixed,
      (YPOS,I,K)  fixed,
      ACC_SET     boolean;

   dcl NOTE_POS    fixed data (L0,0,S0,0,L1,S1,0,L2,0,S2,0,L3),
       SHARP_NOTES fixed data (0,S1,L0,L2,S0,S2,L1,L3),
       FLAT_NOTES  fixed data (0,L3,L1,S2,S0,L2,L0,S1);

  ACC  = NATURAL#;
  NOTE = KEY_NO;
  YPOS = NOTE_POS(NOTE mod 12);
  K = KEY_CODES(KEY);

  if (YPOS = 0)
  or ((YPOS = L3) and ((K = -6) or (K = -7))) /* Bnat with Cb in key sig */
  or ((YPOS = L1) and (K = -7))               /* Enat with Fb in key sig */
  or ((YPOS = S1) and ((K = 6) or (K = 7)))   /* Fnat with E# in key sig */
  or ((YPOS = L0) and (K = 7))                /* Cnat with B# in key sig */
  then do;
    if (PART_BITS & P_FLAT) ~= 0 then do;
      NOTE = NOTE + 1; ACC = ACC + 1;
    end;
    else do;
      NOTE = NOTE - 1; ACC = ACC - 1;
    end;
    YPOS = NOTE_POS(NOTE mod 12);
  end;
  OCTAVE   = NOTE / 12;
  ADJ_YPOS = YPOS + (21 * (OCTAVE - 3)); /* Adjust note to octave */

  ACC_IND = 0; ACC_SET = false;
  do I = 1 to ACC_LIST(0);
    if ADJ_YPOS = (ACC_LIST(I) & LBYTE) then do;
      if ACC = shr(ACC_LIST(I),8) & ~ACC_SET then ACC = 0;
      else ACC_LIST(I) = shl(ACC,8) \ (ADJ_YPOS & LBYTE);
      ACC_SET = true; ACC_IND = I;
    end;
  end;

  if ~ACC_SET then do;
    if K < 0 then do I = 1 to -K;
      if YPOS = FLAT_NOTES(I) then do;
        if ACC = FLAT# then ACC = 0; ACC_SET = true;
      end;
    end;
    else if K > 0 then do I = 1 to K;
      if YPOS = SHARP_NOTES(I) then do;
        if ACC = SHARP# then ACC = 0; ACC_SET = true;
      end;
    end;
  end;
  if ~ACC_SET & ACC = NATURAL# then ACC = 0;

  if ACC ~= 0 & ACC_IND = 0 then do;
    ACC_LIST(0) = ACC_LIST(0) + 1;
    ACC_LIST(ACC_LIST(0)) = shl(ACC,8) \ (ADJ_YPOS & LBYTE);
  end;

  ADJ_YPOS = ADJ_YPOS + CLEF_TABLE(CLEF); /* Adjust note to clef */
  if ADJ_YPOS < 0 then ADJ_YPOS = 0; if ADJ_YPOS > 255 then ADJ_YPOS = 255;
  return (shl(ACC,6) \ (ADJ_YPOS / 3));

end KEY_NOTE;

DRAW_CURSOR:procedure swapable;

  call viewport (0,SCREEN_RIGHT,EDIT_BOT,EDIT_TOP);
  call window (0,SCREEN_RIGHT,-Y_POS(OLD_POS(STAFF#))+EDIT_BOT,
                              EDIT_TOP-Y_POS(OLD_POS(STAFF#)));

  do case OLD_POS(TYPE#);
    do; /* Main menu */
      call viewport (0,SCREEN_RIGHT,0,SCREEN_TOP);
      call window   (0,SCREEN_RIGHT,-MENU_TOP,SCREEN_TOP-MENU_TOP);
      call MENU_CURSOR (OLD_POS(X#),OLD_POS(Y#),OLD_POS(LENGTH#));
    end;
    call MENU_CURSOR (OLD_POS(X#),OLD_POS(Y#),OLD_POS(LENGTH#)); /* Part menu */
    call EDIT_CURSOR (OLD_POS(X#),OLD_POS(Y#),OLD_POS(RANGE#)); /* Edit mode */
  end;

end DRAW_CURSOR;

/* find_position extracts a data record from the NEW_SECT */
/* storage area.   this contains a 3-word record for      */
/* every note of a chord.   CHORD_LENGTH is set           */

FIND_POSITION:procedure (core_area) returns (pointer) swapable;
  dcl core_area array;

  dcl (P,EP) fixed,
      POSN   fixed,
      WLEN   fixed;

  write(SECT$) = NEW_SECT;
  if STAFF = 0 then P = 16 + 16;
  else do;
    write(WORD$) = 7 + STAFF;
    P            = read(DATA$) + 16;
  end;
  write(WORD$) = 8 + STAFF;
  EP           = read(DATA$);

  CHORD_LENGTH = 0;

  do while P ~= EP;

    write(SECT$) = NEW_SECT + shr(P,8);
    write(WORD$) = P;

    POSN = shr(read(DATA$),6);
    WLEN = read(DATI$) & MASK6;

    if POSN > POSITION then return (false);
    else if POSN = POSITION then do;
      if (read(DATA$) & P_REST) ~= 0 \ WLEN = 2 then return (false);
      write(SECT$) = NEW_SECT + shr(P,8);
      write(WORD$) = P;
      call copy.in(addr(core_area(0)),wlen);
      CHORD_LENGTH = (WLEN-2) / 3;
      return (true);
    end;

    P = P + WLEN;

 end;

 return (false);

end FIND_POSITION;

SETUP_POSITION:procedure (core_area) swapable;
  dcl core_area array;

  NEW_POS(STAFF#)  = STAFF;
  BLOCKS           = (shr(CLICK_NOTE,8) * RESOLUTION * MULTIPLIER * CLICKS) / (CLICK_NOTE & LBYTE);
  NEW_POS(BLOCKS#) = BLOCKS;
  
  if (OLD_POS(BLOCKS#) == 0)
    POSITION = 0;
  else
    POSITION = SIEVE(NEW_POS(BLOCKS#),OLD_POS(BLOCKS#),POSITION);

  if      POSITION <  0                then POSITION = 0;
  else if POSITION >= NEW_POS(BLOCKS#) then POSITION = NEW_POS(BLOCKS#) - 1;
  NEW_POS(POSITION#) = POSITION;
  MEASURE = (START_CLICK - 1 + (CLICKS * POSITION) / NEW_POS(BLOCKS#)) /
            CLICKS_MEASURE + 1;

  if SEQED_MODE = MENU# then do;
    if OVERALL then do;
      NEW_POS(TYPE#)     = MAIN_MENU#;
      NEW_POS(X#)        = OCOLS(MENU_COL);
      NEW_POS(Y#)        = OROWS(MENU_ROW-100);
      NEW_POS(LENGTH#)   = OLENS(ENTRY);
    end;
    else do;
      MENU_ROW           = MENU_ROW & MASK2; /* Remove staff no. from menu row */
      NEW_POS(TYPE#)     = PART_MENU#;
      NEW_POS(X#)        = PCOLS(MENU_COL);
      NEW_POS(Y#)        = PROWS(MENU_ROW);
      NEW_POS(LENGTH#)   = PLENS(shl(MENU_ROW,1)\MENU_COL);
      MENU_ROW           = shl(STAFF,2) \ MENU_ROW;
    end;
  end;
  else do; /* EDIT */
    NOTE_LEFT   = EDIT_LEFT + 24 + NEW_MAX_KEY_LENGTH; /* Allow room for clef & keysig */
    EDIT_FIELD  = NOTE_RIGHT - NOTE_LEFT + 1;
    NEW_POS(TYPE#) = NOTE#;
    NEW_POS(X#) = NOTE_LEFT +
                  SIEVE(NOTE_RIGHT - NOTE_LEFT + 1,
                       (shr(CLICK_NOTE,8) * NEW_MAX_BLOCKS * CLICKS) / 
                       (CLICK_NOTE & LBYTE),1) / 2 +
                  SIEVE(EDIT_FIELD,NEW_POS(BLOCKS#),POSITION);
    if FIND_POSITION(core_area) then do;
      if      CHORD_INDEX < 1            then CHORD_INDEX = 1;
      else if CHORD_INDEX > CHORD_LENGTH then CHORD_INDEX = CHORD_LENGTH;
      RANGE_CENTER = ((CORE_AREA(CHORD_INDEX*3-1) & MASK6) - 30) * VERTICAL_UNIT;
      if RANGE_CENTER > MAX_RANGE then RANGE_CENTER = MAX_RANGE;
      if RANGE_CENTER < MIN_RANGE then RANGE_CENTER = MIN_RANGE;
      NEW_POS(Y#)  = RANGE_CENTER;
    end;
    else do;
      NEW_POS(Y#) = nullint;
      CHORD_INDEX = 0;
    end;
  end;

  NEW_POS(RANGE#) = RANGE_CENTER;

end SETUP_POSITION;

MAP.XY.TO.NOTE:PROC(ST,ROW,COL,CORE_AREA,TARGET) SWAPABLE;
   DCL ST        FIXED;    /* PASS STAFF */
   DCL ROW       FIXED;    /* PASS ROW   */
   DCL COL       FIXED;    /* PASS COL   */
   DCL CORE_AREA ARRAY;
   DCL TARGET    FIXED;    /* #map_to_old = find closest note, #map_to_new = return loc for new note */
   DCL ROUNDING  FIXED;

   dcl NBITS fixed;
   dcl Y     fixed;
   dcl OLDY  fixed;
   DCL P     FIXED;
   dcl K     fixed;

   STAFF        = ST;     /* new staff         */
   if STAFF > 0 then do;
     write(SECT$) = NEW_SECT;    /**/
     write(WORD$) = 7 + STAFF;
     P            = read(DATA$);
   end;
   else P = 16;

   call LOAD_PART_VECTOR (NEW_SECT,P);  /**/

   NEW_POS(STAFF#)  = STAFF;
   BLOCKS           = (shr(CLICK_NOTE,8) * RESOLUTION * MULTIPLIER * CLICKS) / (CLICK_NOTE & LBYTE);
   NEW_POS(BLOCKS#) = BLOCKS;

   rounding = (note_right-note_left)/shl(blocks,1);

   IF COL < NOTE_LEFT + ROUNDING
   THEN POSITION = 0;
   ELSE POSITION = SIEVE(BLOCKS,
                         EDIT_FIELD,
                         COL-NOTE_LEFT-ROUNDING);

   if      POSITION <  0      then POSITION = 0;
   else if POSITION >= BLOCKS then POSITION = BLOCKS-1;

   NEW_POS(POSITION#) = POSITION;
   MEASURE = (START_CLICK - 1 + (CLICKS * POSITION) / NEW_POS(BLOCKS#)) /
              CLICKS_MEASURE + 1;

   NEW_POS(TYPE#) = NOTE#;
   NEW_POS(X#) = NOTE_LEFT +
                 SIEVE(NOTE_RIGHT - NOTE_LEFT + 1,
                      (shr(CLICK_NOTE,8) * NEW_MAX_BLOCKS * CLICKS) / 
                      (CLICK_NOTE & LBYTE),1) / 2 +
                 SIEVE(EDIT_FIELD,NEW_POS(BLOCKS#),POSITION);

   RANGE_CENTER  = (((row + (vertical_unit/2)) / vertical_unit) * vertical_unit);  /* signed multiply/divide */

   if target = #map_to_old then do;
      if FIND_POSITION(core_area) then do; /* notes exists here */

         /* find nearest note */

         chord_index = 1;
         NBITS = core_area(2);             /* get pos if lowest note    */
         oldy  = ((NBITS & MASK6) - 30) * VERTICAL_UNIT;

         do K = 2 to CHORD_LENGTH ;    /* (LPG removed by 3) check other notes for nearest */
            NBITS = core_area(k*3-1);      /* get bits for next note        */
            Y     = ((NBITS & MASK6) - 30) * VERTICAL_UNIT;
            if abs(y-range_center) < abs(oldy-range_center)
            then do;
               oldy        = y;
               chord_index = k;
            end;
         end;

         range_center = oldy;
         NEW_POS(Y#)  = RANGE_CENTER;
      end;
      else do;
         NEW_POS(Y#) = nullint;
         CHORD_INDEX = 0;
      end;
   end;  /* if target = map to old */
   else if target = #map_to_new then do;
      new_pos(y#) = RANGE_CENTER;
      chord_index = 0;          /*???? */
   end;

   if RANGE_CENTER > MAX_RANGE then RANGE_CENTER = MAX_RANGE;
   if RANGE_CENTER < MIN_RANGE then RANGE_CENTER = MIN_RANGE;
   NEW_POS(RANGE#) = RANGE_CENTER;

END MAP.XY.TO.NOTE;


dcl NOABORT fixed;   /* set to disable abort */

ABORT_REFRESH:procedure (unusedarg) returns (boolean); /* True if anything changed or char typed */

  /* THIS PROCEDURE SHOULD NOT SWAP! */

  dcl (unusedarg) fixed;
                        
  dcl I fixed STATIC; /* static init (0) */

  if ABORTING then return (true);

  I = I + 1;
  if PLAY \ I = 10 then do; /* Adjust frequency of rsl calls here */
    call CHECK.NEXT.EVENT;  /* done to keep mouse going           */
    I = 0;
  end;

  if PLAY then call CHECK.WINDOW.BOUNDARIES;

  if NOABORT <> 0 then return (false);

  if  (num.of.d50.in.chars<>0)                  /* if user has typed a character */
  then do;                                      /* then force exit from this     */
    new.ABLE.seq.info = new.ABLE.seq.info \ 4;  /* scan and force a re-entry     */
  end;

	if (((new.seq.info | new.ABLE.seq.info) & MNOT_SEQEVENTS_OF_INTERST) != 0)
  then do;                              				/* sequencer significantlyh changed while plotting it */
    ABORTING             = true;
    return (true);
  end;

  return (false);

end ABORT_REFRESH;

INITIALIZE.SEQED:procedure (core_area) swapable;
  dcl core_area array;

  dcl I fixed;

   /* Menu positioning tables (NO LENGTHS MAY EXCEED 20 UNLESS "TOKEN" LEN IS CHANGED */
   /* (These 3 sets of tables must be exactly the same length and in the same order) */

   dcl MENU_TABLE_LENGTH# lit '33';

   dcl OROWV fixed data (-14,-28),
       MCOLV fixed data ( 8,160,312,464,0),    /* Label colomns */
       OCOLV fixed data (88,248,384,544,2048), /* data colomns (the 2048 is handy for finding nearest col in editfncs) */
       OLENV fixed data (1,5,5,3,5,5,5,5), /* R1C1,R1C2,R1C3,R1C4,R2C1,etc... */
       PROWV fixed data (16,2,-12,-26),
       PCOLV fixed data (32,64),
       PLENV fixed data (3,0,2,1,1,0,3,0); /* R1C1,R1C2,R2C1,R2C2,etc... */
                   /*    T   K A C   R M  */

   /* For the MG600 */
   dcl OROWM fixed data (-22,-44),
       MCOLM fixed data (16, 236,474,700,0),    /* Label columns */
       OCOLM fixed data (146,379,591,830,2048), /* data columns (the 2048 is handy for finding nearest col in editfncs) */
       OLENM fixed data (1,5,5,3,5,5,5,5), /* R1C1,R1C2,R1C3,R1C4,R2C1,etc... */
       PROWM fixed data (24,2,-20,-42),
       PCOLM fixed data (52,104),
       PLENM fixed data (3,0,2,1,1,0,3,0); /* R1C1,R1C2,R2C1,R2C2,etc... */
                   /*    T   K A C   R M  */

  if mg600 then do;
    ALPHA_WIDTH        = 13;
    ALPHA_HIGHT        = 19;
    SCREEN_BOT         = 0;
    SCREEN_TOP         = 779;
    SCREEN_LEFT        = 0;
    SCREEN_RIGHT       = 1023;
    SEQED.MOUSE.HOLE.X = 985;
    SEQED.MOUSE.HOLE.Y = SCREEN_TOP - 32;
    MENU_TOP           = SCREEN_TOP;
    HELP_SIZE          = 277;
    MENU_SIZE          = 52;
    MENU_DIVIDE        = 730;
    EDIT_BOT           = 0;
    EDIT_LEFT          = SCREEN_LEFT + 130;
    EDIT_RIGHT         = SCREEN_RIGHT;
    NOTE_LEFT          = EDIT_LEFT;
    NOTE_RIGHT         = EDIT_RIGHT - 10;
    do I = 0 to MENU_TABLE_LENGTH#;
      OROWS(I) = OROWM(I);
    end;
  end;
  else if vt640 then do;
    ALPHA_WIDTH  = 8;
    ALPHA_HIGHT  = 14;
    SCREEN_BOT   = 0;
    SCREEN_TOP   = 479;
    SCREEN_LEFT  = 0;
    SCREEN_RIGHT = 639;
    SEQED.MOUSE.HOLE.X = 607;
    SEQED.MOUSE.HOLE.Y = SCREEN_TOP - 30;
    MENU_TOP           = SCREEN_TOP;
    HELP_SIZE    = 192;
    MENU_SIZE    = 35;
    MENU_DIVIDE  = 427;
    EDIT_BOT     = SCREEN_BOT;
    EDIT_LEFT    = SCREEN_LEFT + 75;
    EDIT_RIGHT   = SCREEN_RIGHT;
    NOTE_LEFT    = 140; /* ? */
    NOTE_RIGHT   = 631;
    do I = 0 to MENU_TABLE_LENGTH#;
      OROWS(I) = OROWV(I);
    end;
  end;

  TCUR_ROW      = 0;
  TCUR_COL      = 77;

  SEQED_MODE    = MENU#;
  HELP_MODE     = true;
  BEGINNER_MODE = true;

  STAFF         = 0;
  ENTRY         = 0;
  CHORD_INDEX   = 0;
  OVERALL       = true;
  MOVE_DOWN     = false;
  MENU_REFRESH  = false;

  MENU_ROW      = 100;
  MENU_COL      = 0;

  CLICKS         = 8;
  CLICKS_MEASURE = 4;
  CLICK_NOTE     = "H0104";
  TIME_SIG       = "H0404";
  START_CLICK    = 1;
  STAVES         = 0;
  MEASURES       = 2;

  NEW_MAX_BLOCKS     = 16;
  NEW_MAX_KEY_LENGTH = 0;

  RANGE_CENTER       = 8;
  CUR_PITCH          = 24;
  CUR_DURATION       = "H0104";
  CUR_ACCD           = 0;

  MULTIPLIER        = 1;
  RESOLUTION        = 16;
  CLEF              = 0;
  KEY               = 1;
  MEASRS            = 2;
  MAX_KEY_LENGTH    = 0;
  CLICNOTE          = "H0104";
  CLICS             = 8;
  MAX_BLOCKS        = 16;
  PART_BITS         = 0;

  do I = 0 to 7;
    TRACK = I;
    call DUMP_PART_VECTOR (MENU_BASE,8+I*8);
  end;

  call SETUP_POSITION(core_area);
  call blockmove (NEW_POS,OLD_POS,8);

end INITIALIZE.SEQED;

SEQED.TIME.CURSOR:procedure swapable;

  /* Make sure this always preserves registers */

  dcl T(1) fixed, /* Temp time */
      CB   fixed; /* Current block */

  T(1) = SAMPLED.ACTUAL.PLAY.TIME.MSB;
  T(0) = SAMPLED.ACTUAL.PLAY.TIME.LSB;

  new.ABLE.seq.info = new.ABLE.seq.info & (not(32));

  call CONVERT_TIME (T,0);

  /* Find new cursor position */

  /* What about PRESCAN??? */
  CB = T(LSB) - START_TIME(LSB);     /* Get tics into screen */
  CB = SIEVE(TC_MAX_BLOCKS,TICS,CB); /* Get block we're on */
  TIME_CUR_POS = TC_NOTE_FIRST + SIEVE(TC_EDIT_FIELD,TC_MAX_BLOCKS,CB); /* Get X position */

  /**************************/       /* use local variables here */
  dcl CURSOR.BEGIN(1)  fixed;
  dcl CURSOR.END  (1)  fixed;
  /**************************/

  CURSOR.BEGIN(MSB) = START_TIME(MSB);
  CURSOR.BEGIN(LSB) = START_TIME(LSB);
  call ADD32 (CURSOR.BEGIN,SIEVE(TICS,TC_MAX_BLOCKS,CB));
  call UNCONVERT_TIME (CURSOR.BEGIN,0);

  Cursor.Scroll.Begin(0) = CURSOR.BEGIN(MSB);
  Cursor.Scroll.Begin(1) = CURSOR.BEGIN(LSB);

  CURSOR.END(MSB) = START_TIME(MSB);
  CURSOR.END(LSB) = START_TIME(LSB);
  call ADD32 (CURSOR.END,SIEVE(TICS,TC_MAX_BLOCKS,CB+1)-1);
  call UNCONVERT_TIME (CURSOR.END,0);

  Cursor.Scroll.End(0) = CURSOR.END(MSB);
  Cursor.Scroll.End(1) = CURSOR.END(LSB);

  if OLD_TIME_CUR_POS ~= 0 then call TIME_CURSOR (TIME_CUR_POS);

end SEQED.TIME.CURSOR;
