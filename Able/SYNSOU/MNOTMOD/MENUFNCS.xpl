/*
   10/23/87 - lpg - added mouse insert/delete
   5/30/86 - adt & eg - updated for rel M
   5/20/86 - eg - made procedures and some variables pub.
   5/15/86 - "official" creation of release-M modules
*/

TOGGLE_ENTRY:procedure swapable;        /* toggles menu field when space bar typed */

  dcl I fixed;

  vector_mode;               /* erase current entry */
  call data_level (2);
  alpha_mode;
  call DISPLAY_ENTRY (ENTRY,OVERALL);

  if OVERALL then do case ENTRY;
    ;   /* Staves */
    ;   /* Measure# */
    ;   /* Time Signature */
    ;
    ;   /* Measures */
    ;   /* Start click */
    ;   /* Click Note */
    ;
  end;
  else do case ENTRY;
    ;   /* Track */
    ;
    do; /* Key */
      KEY = KEY + 1;
      if KEY > 21 then KEY = 1;
      else if KEY > 7 & KEY < 17 then KEY = 17;
      call DISPLAY_ENTRY (3,false);   /* remove accidental format on key sig toggle */
      if KEY_CODES(KEY) >= 0
      then PART_BITS = PART_BITS & ~P_FLAT;
      else PART_BITS = PART_BITS \  P_FLAT;
    end;
    do; /* Accidental format */
      if (PART_BITS & P_FLAT) = 0
      then PART_BITS = PART_BITS \  P_FLAT;
      else PART_BITS = PART_BITS & ~P_FLAT;
    end;
    do; /* Clef */
      CLEF = CLEF + 1;
      if CLEF = 7 then CLEF = 0;
    end;
    ;
    do; /* Resolution */
      I = 1;
      do while I <= RESOLUTION; I = shl(I,1); end;
      if I > 128 then I = 1;
      RESOLUTION = I;
    end;
    ; /* Dummy multiplier */
  end;

  if RESOLUTION < (CLICK_NOTE & LBYTE) /* Resolution < ClickNote = nonsense */
  then CLICK_NOTE = "H0100" \ RESOLUTION;

  call DISPLAY_ENTRY (ENTRY,OVERALL); /* Display formatted entry */
  if ~OVERALL then do;
    if ENTRY = 2 then call DISPLAY_ENTRY (3,false);
    call DUMP_PART_VECTOR (MENU_BASE,8+STAFF*8);
  end;
  call data_level (0);

  TOKEN(0)      = 0;
  FIELD_CLEARED = false;

end TOGGLE_ENTRY;

DEPOSIT_TOKEN:procedure swapable;

  dcl (I,J) fixed;

  if TOKEN(0) = 0 then do;
    if FIELD_CLEARED then do;
       vector_mode;
       call DATA_LEVEL(2);
       alpha_mode;
       call DISPLAY_ENTRY (ENTRY,OVERALL); /* restore entry if field had been cleared */
       call DATA_LEVEL(0);
    end;
    FIELD_CLEARED = false;
    return;
  end;

  call move_to (OLD_POS(X#),OLD_POS(Y#));
  call data_level(2);
  call pstring(TOKEN);  /* erase token */
  call data_level(0);

  /* Process entry */

  if OVERALL then do case ENTRY;
    do;      /* Staves */
      I = STRING_TO_FIXED (TOKEN);
      if 0 < I & I < 9 then do;
        I = I - 1;
        if STAFF = STAVES \ STAFF > I then STAFF = I;
        do J = STAVES + 1 to I; /* Default of new staves = last staff data */
          call COPY.EXT.MEM (MENU_BASE,8+STAVES*8+1,MENU_BASE,8+J*8+1,7);
        end;
        if      I >  2                 then HELP_MODE = false;
        else if I <= 2 & BEGINNER_MODE then HELP_MODE = true;
        STAVES             = I;
        full.seqed.refresh = true;
      end;
    end;
    do;      /* Measure# */
      I = STRING_TO_FIXED (TOKEN);
      if 0 <= I & I ~= nullint then do;
        START_CLICK = I * CLICKS_MEASURE + 1;
        POSITION = 0;
        full.seqed.refresh = true;
      end;
    end;
    do; /* Time Signature */
      I = FRACTION_TO_FIXED (TOKEN);
      if 0 < shr(I,8) & POWER (I & LBYTE) then do; TIME_SIG = I; full.seqed.refresh = true; end;
    end;
    do;      /* Pitch */
      I = STRING_TO_PITCH (TOKEN);
      if -12 <= I & I <= 72 then CUR_PITCH = I;
    end;
    do;      /* Measures */
      I = STRING_TO_FIXED (TOKEN);
      if 0 < I & I ~= nullint then do; MEASURES = I; full.seqed.refresh = true; end;
    end;
    do;      /* Start click */
      I = STRING_TO_FIXED (TOKEN);
      if 0 < I & I ~= nullint then do; START_CLICK = I; full.seqed.refresh = true; end;
    end;
    do; /* Click Note */
      I = FRACTION_TO_FIXED (TOKEN);
      if 0 < shr(I,8) & 0 < (I & LBYTE) then do; CLICK_NOTE = I; full.seqed.refresh = true; end;
    end;
    do; /* Duration */
      I = FRACTION_TO_FIXED (TOKEN);
      if 0 < shr(I,8) & 0 < (I & LBYTE) then CUR_DURATION = I;
    end;
  end;
  else do case ENTRY;
    do;      /* Track */
      I = STRING_TO_FIXED (TOKEN);
      if 1 <= I & I <= 200 then TRACK = I - 1;
    end;
    ;
    do; /* Key */
      if TOKEN(0) = 1 then call pbyte(TOKEN,1,32); /* Add a space to a 1 char key */
      if byte(TOKEN,1) = asc.f then call pbyte (TOKEN,1,lower.f); /* Allow EF or Ef */
      do I = 1 to 21;
        if TOKEN(1) = KEY_NAMES(I) then do;
          KEY = I;
          call data_level (2); alpha_mode;
          call DISPLAY_ENTRY (3,false); /* If key then update accform */
          if KEY_CODES(KEY) >= 0
          then PART_BITS = PART_BITS & ~P_FLAT;
          else PART_BITS = PART_BITS \  P_FLAT;
          call DISPLAY_ENTRY (3,false);
          call data_level (0);
        end;
      end;
    end;
    do; /* Accidental format */
      J = TOKEN(1) & LBYTE; if J > 96 then J = J - 32;
      if J = asc.f then PART_BITS = PART_BITS \  P_FLAT;
                   else PART_BITS = PART_BITS & ~P_FLAT;
    end;
    do; /* Clef */
      J = TOKEN(1) & LBYTE; if J > 96 then J = J - 32;
      do I = 0 to 6;
        if J = CLEF_NAMES(I) then CLEF = I;
      end;
    end;
    ;
    do;      /* Resolution */
      I = STRING_TO_FIXED (TOKEN);
      if 0 < I & I < 256 then RESOLUTION = I;
    end;
    ; /* Dummy multiplier */
/*
    do;      * Multiplier *
      I = STRING_TO_FIXED (TOKEN);
      if 1 <= I & I <= 99 then MULTIPLIER = I;
    end;
*/
  end;

  if RESOLUTION < (CLICK_NOTE & LBYTE) /* Resolution < ClickNote = nonsense */
  then CLICK_NOTE = "H0100" \ RESOLUTION;

  if OVERALL then call REFRESH_MAIN_MENU (ENTRY,ENTRY,true);
  else do;
    call DATA_LEVEL(2); alpha_mode;
    call DISPLAY_ENTRY (ENTRY,false); /* Display formatted entry */
    call DATA_LEVEL(0);
    call DUMP_PART_VECTOR (MENU_BASE,8+STAFF*8);
  end;

  TOKEN(0)      = 0;
  FIELD_CLEARED = false;

end DEPOSIT_TOKEN;

LOAD_TOKEN:procedure (C) returns (boolean) swapable;

  dcl C    fixed,
      MODE boolean; /* Set to map lower to upper */

  MODE = true;

  if TOKEN(0) = 0 then do; /* Clear field on 1st char */
    vector_mode; call data_level(2); alpha_mode;
    call DISPLAY_ENTRY (ENTRY,OVERALL);
    call data_level(0);
    FIELD_CLEARED = true;
  end;

  call move_to (OLD_POS(X#)+TOKEN(0)*ALPHA_WIDTH,OLD_POS(Y#)); alpha_mode;

  if C = del then do;
    if TOKEN(0) > 0 then do;
      TOKEN(0) = TOKEN(0) - 1;
      wchar(bs); 
      call data_level(2);
      wchar(byte(TOKEN,TOKEN(0)));
      call data_level(0);
      wchar(bs); 
    end;
    if TOKEN(0) = 0 then call DEPOSIT_TOKEN;
  end;
  else if sp <= C & C < del then do;
    if TOKEN(0) < OLD_POS(LENGTH#) then do;
      if MODE & lower.a <= C & C <= lower.z then C = C - 32; /* Map lower to upper */
      call pbyte (TOKEN,TOKEN(0),C); 
      call data_level(2);
      wchar(C);
      call data_level(0);
      TOKEN(0) = TOKEN(0) + 1;
    end;
  end;

  vector_mode;

end LOAD_TOKEN;

SEQED_MENU:procedure (mode,C,row,col,core_area) swapable;
  dcl mode      fixed;
  dcl c         fixed;  /* character/staff */
  dcl row       fixed;
  dcl col       fixed;
  dcl core_area array;

  dcl (I,J) fixed;

  if OVERALL then do;
    call viewport (0,SCREEN_RIGHT,0,SCREEN_TOP);
    call window   (0,SCREEN_RIGHT,-MENU_TOP,SCREEN_TOP-MENU_TOP);
  end;
  else do;
    call LOAD_PART_VECTOR (MENU_BASE,8+STAFF*8);
    call viewport (0,SCREEN_RIGHT,EDIT_BOT,EDIT_TOP);
    call window   (0,SCREEN_RIGHT,-Y_POS(STAFF)+EDIT_BOT,
                                  EDIT_TOP-Y_POS(STAFF));
  end;

  if mode = in.abss then do;      /* mouse click */
     call DEPOSIT_TOKEN;
     if c < 256 then do;          /* main or part menu */
        menu_row = c*4+row;       /* staff * 4 + row   (staff=0 for main menu) */
        menu_col = col;
     end;
     else do;                     /* music area select */
        SEQED_MODE = EDIT#;
        call MAP.XY.TO.NOTE(C&255,ROW,COL,CORE_AREA, #map_to_old);
        return;
     end;
  end;
  else if      C = u.arr then do;
    call DEPOSIT_TOKEN;
    if MENU_ROW = 0 then MENU_ROW = 101;
    else if MENU_ROW > 100 \ MENU_ROW <= STAVES*4 + 3 then do;
      MENU_ROW = MENU_ROW - 1;
      I        = true;
      if MENU_ROW < 100 then do while PLENS(shl(MENU_ROW&MASK2,1)\MENU_COL) = 0 & I;
        if (MENU_ROW & MASK2) = 0 then do;
          MENU_COL = ~MENU_COL & MASK1;
          I = false;
        end;
        else MENU_ROW = MENU_ROW - 1;
      end;
    end;
  end;
  else if C = d.arr then do;
    call DEPOSIT_TOKEN;
    if MENU_ROW = 101 then do;
      MENU_ROW = 0;
      MENU_COL = 0;
    end;
    else if MENU_ROW < STAVES*4 + 3 \ MENU_ROW >= 100 then do;
      MENU_ROW = MENU_ROW + 1;
      I = true;
      if MENU_ROW < 100 then do while PLENS(shl(MENU_ROW&MASK2,1)\MENU_COL) = 0 & I;
        if (MENU_ROW & MASK2) = 2 then do; /* Was 3 */
          MENU_COL = ~MENU_COL & MASK1;
          I = false;
        end;
        else MENU_ROW = MENU_ROW + 1;
      end;
    end;
  end;
  else if C = l.arr then do;
    if MENU_COL = 0 then do;
      if MENU_ROW < 100 
      then do;
         if (((START_CLICK-1) MOD CLICKS_MEASURE) != 0) full.seqed.refresh = true;
         if START_CLICK > (MEASURES - 1) * CLICKS_MEASURE
         then START_CLICK = START_CLICK - (MEASURES - 1) * CLICKS_MEASURE;
         else START_CLICK = 1;          /* start at beginning if can't do full scroll */
      end;
    end;
    else if (MENU_ROW >= 100 \ PLENS(shl(MENU_ROW&MASK2,1)\MENU_COL-1) > 0)
    then MENU_COL = MENU_COL - 1;
    call DEPOSIT_TOKEN;      /* do this last to catch start_click update */
  end;
  else if C = r.arr then do;
    call DEPOSIT_TOKEN;
    if MENU_ROW < 100 then do;
      if MENU_COL = 1 \ PLENS(shl(MENU_ROW&MASK2,1)\MENU_COL+1) = 0
      then do;
        SEQED_MODE = EDIT#;
        POSITION   = 0;
      end;
      else MENU_COL = MENU_COL + 1;
    end;
    else if MENU_COL < 3 then MENU_COL = MENU_COL + 1;
  end;

  else if C = asc.min then do;
    call DEPOSIT_TOKEN;
    if MENU_ROW < 100 & MENU_ROW - 4 >= 0 then MENU_ROW = MENU_ROW - 4;
  end;
  else if C = asc.equ then do;
    call DEPOSIT_TOKEN;
    if MENU_ROW < 100 & MENU_ROW + 4 <= STAVES*4 + 3 then MENU_ROW = MENU_ROW + 4;
  end;

  else if C = cret then call DEPOSIT_TOKEN;
  else if C = tab then do;
    call DEPOSIT_TOKEN;
    SEQED_MODE = EDIT#;
  end;

  else if C = sp then call TOGGLE_ENTRY;
  else call LOAD_TOKEN (C);

  if MENU_ROW >= 100 then do;
    OVERALL    = true;
    ENTRY      = shl(MENU_ROW-100,2) \ MENU_COL;
  end;
  else do;
    OVERALL    = false;
    ENTRY      = shl(MENU_ROW & MASK2,1) \ MENU_COL;
    STAFF      = shr(MENU_ROW,2);
  end;

  NEW_MAX_KEY_LENGTH = 0;
  NEW_MAX_BLOCKS     = 0;
  do I = 0 to STAVES;
    call LOAD_PART_VECTOR (MENU_BASE,8+I*8);
    J = RESOLUTION * MULTIPLIER;
    if NEW_MAX_BLOCKS < J then NEW_MAX_BLOCKS = J;
    J = KEY_CODES(KEY);
    if NEW_MAX_KEY_LENGTH < abs(J) then NEW_MAX_KEY_LENGTH = abs(J);
  end;
  NEW_MAX_KEY_LENGTH = NEW_MAX_KEY_LENGTH * 8; /* Set max space for key sigs */
  CLICKS_MEASURE = (shr(TIME_SIG,8) * (CLICK_NOTE & LBYTE)) /
                   ((TIME_SIG & LBYTE) * shr(CLICK_NOTE,8));
  if CLICKS_MEASURE = 0 then CLICKS_MEASURE = 1; /* Fixup illegal clicknoteXtimesig */
  CLICKS = MEASURES * CLICKS_MEASURE;

  call LOAD_PART_VECTOR (MENU_BASE,8+STAFF*8);
  call SETUP_POSITION(core_area); transparent_mode;

end SEQED_MENU;
