/* $title List Plotting Routines */

/*
   3/14/89 - adt - fixed chord loop bug (xmem ptrs trashed)
   5/30/86 - adt & eg - updated for rel M
   5/20/86 - eg - made procedures and some variables pub.
   5/15/86 - "official" creation of release-M modules
*/


/* plots all notes at a given position - passed pointer to old_sect */

PLOT_POSITION:procedure (SECT,WORD) returns (fixed) swapable;

  dcl SECT           fixed,
      WORD           fixed,
      WIDTH          fixed,   /* Note width */
      POSN           fixed,
      WLEN           fixed,
      BITS           fixed,
      ACCD           fixed,
      NBITS          fixed,
      (VAL,STLN)     fixed,   /* Note value, stem length */
      (NX,SX,BX)     fixed,   /* Note X position, Stem X position, Voice X position */
      (SY1,SY2)      fixed,
      (X,Y,DIR)      fixed,   /* Y position, Stem direction */
      (B,I,J,K,T)    fixed,
      REF_TIME_CUR   boolean,
      REF_EDIT_CUR   boolean;

  write(SECT$) = SECT + shr(WORD,8);
  write(WORD$) = WORD;
  WORD         = WORD + 2;

  POSN = shr(read(DATA$),6);
  WLEN = read(DATI$) & MASK6;
  BITS = read(DATI$);

  VAL  = shr(BITS,5) & MASK3; /* Note value */
  STLN = BITS & MASK5; /* Stem length */
  DIR  = 1; if (BITS & P_DIR) ~= 0 then DIR = -1;
  X    = NOTE_FIRST + SIEVE(EDIT_FIELD,BLOCKS,POSN);
  if MG600 then SX = X + 4*DIR;
           else SX = X + 3*DIR;

  REF_TIME_CUR = false;
  if X - 10 <= TIME_CUR_POS & TIME_CUR_POS <= X + 10 then do;
    call TIME_CURSOR (0);
    REF_TIME_CUR = true;
  end;
  REF_EDIT_CUR = false;
  if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF &
     X - 10 <= OLD_POS(X#) & OLD_POS(X#) <= X + 10 then do;
    call DRAW_CURSOR;
    REF_EDIT_CUR = true;
  end;

  if (BITS & P_BAR) ~= 0 then do;
    if POSN = BLOCKS - 1 then call BAR (EDIT_RIGHT);
    else do;
      I = (OVER_BLOCKS * (POSN+1)) / BLOCKS;
      call BAR (NOTE_FIRST + (SIEVE(EDIT_FIELD,OVER_BLOCKS,I-1) +
                              SIEVE(EDIT_FIELD,OVER_BLOCKS,I)) / 2);
    end;
  end;

  do K = 5 to WLEN by 3;

    write(SECT$) = SECT + shr(WORD,8);
    write(WORD$) = WORD;
    WORD         = WORD + 3;

    NBITS = read(DATI$);
    ACCD  = shr(NBITS,6) & MASK2;
    Y     = ((NBITS & MASK6) - 30) * VERTICAL_UNIT;

    if (NBITS & N_REST) ~= 0 then do; /* This note is actually a rest */
      if VAL = WHOLE# then do; /* Center whole rests in measure */
        // Handle case of partial first measure
        if ((POSN == 0) && ((START_CLICK-1) MOD CLICKS_MEASURE != 0))
          B = POSN + BLOCKS_MEAS - ((((START_CLICK-1) MOD CLICKS_MEASURE) * BLOCKS_MEAS) / CLICKS_MEASURE) - 1; /* Position of 1st block in next measure */

        // Handle case of full first measure
        else
          B = POSN + BLOCKS_MEAS - 1; /* Position of 1st block in next measure */

        I = (X + NOTE_FIRST + SIEVE(EDIT_FIELD,BLOCKS,B)) / 2;
      end;
      else I = X;

      call REST (VAL,0,I,Y); /* Plot the rest */
    end;

    else do; /* Note */
/*
      if (REC(D5) & B_NOFF) ~= 0 then NX = BX + WIDTH*DIR;
                                 else NX = BX;
*/
      NX = X;
      call NOTE (VAL,NX,Y); /* Draw the note */
      call LEDGER (NX,Y,6); /* Draw the ledger line (LEDGER called redundantly) */

      if K = 5    then SY1 = Y;
      if K = WLEN then SY2 = Y;

      if STLN > 0 & ((K=5&DIR=1) \ (K=WLEN&DIR=-1)) then do; /* Plot stem (chord span stuff will go away when beams are put in) */
        /*
        OBEAM(VOICE)   = shr(REC(D3),12);       
        CBEAM(VOICE)   = shr(REC(D3),8) & MASK4;
        */
        I  = VAL - QUARTER#;
        if (BITS & P_FLAG) = 0 then I = 0;
        call STEM (SX,Y,STLN*VERTICAL_UNIT*DIR,I); /* Draw a stem with flags */

        /* BEAMCODE goes here */

      end; /* End of stemming block */

      I = X - 10; /* - shr(REC(D4),12) * 8;     * Calculate accidental offset */
      do case ACCD; /* Plot the appropriate accidental */
        ;
        call SHARP       (I,Y);           /* 211 - Sharp */
        call NATURAL     (I,Y);           /* 212 - Natural */
        call FLAT        (I,Y);           /* 213 - Flat */
      end;

      if (NBITS & N_OTIE) ~= 0 then do; /* Open a tie */
        B = shr(RESOLUTION * MULTIPLIER,VAL);
        if (NBITS & N_DOT) ~= 0 then B = B + shr(B,1);
        B = POSN + B;
        if B >= BLOCKS then J = EDIT_RIGHT; /* X2 point */
                       else J = NOTE_FIRST + SIEVE(EDIT_FIELD,BLOCKS,B);
        if X-10 <= TIME_CUR_POS & TIME_CUR_POS <= J+10 then do;
          call TIME_CURSOR (0);
          REF_TIME_CUR = true;
        end;
        if ~REF_EDIT_CUR & SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF &
            X-10 <= OLD_POS(X#) & OLD_POS(X#) <= J+10 then do;
          call DRAW_CURSOR;
          REF_EDIT_CUR = true;
        end;
        call SLUR (X+4,Y+4*(-DIR),J-4,Y+4*(-DIR),DIR=1);
      end;

      call CHECK.NEXT.EVENT;   /* keep mouse cursor going */

    end; /* End of note record case */

    if (NBITS & N_DOT) ~= 0 then do; /* Plot a dot for a dotted note/rest */
      if (Y & MASK3) = 0 /* Y mod 8 */
      then call SPOT (X+8,Y+VERTICAL_UNIT); /* Draw a dot (Dots do not go on lines) */
      else call SPOT (X+8,Y);
    end;

  end; /* End of chord loop */

  if WLEN > 5 then do; /* This joins the notes of a chord, beaming should remove this (?) */
    call move_to (SX,SY1); call plot (SX,SY2);
  end;

  if REF_TIME_CUR ~= 0 then call TIME_CURSOR (TIME_CUR_POS);
  if REF_EDIT_CUR ~= 0 then call DRAW_CURSOR;

  return (WLEN);

end PLOT_POSITION;

PLOT_INITIALIZE:procedure (STAFF) swapable;

  dcl STAFF fixed,
      I     fixed;

  call viewport (0,SCREEN_RIGHT,EDIT_BOT,EDIT_TOP);
  call window   (0,SCREEN_RIGHT,-Y_POS(STAFF)+EDIT_BOT,
                                EDIT_TOP-Y_POS(STAFF));
/*
  BEAM_DEG(0) = 0; SM_BEAM_DEG(0) = 0;
  BEAM_DEG(1) = 0; SM_BEAM_DEG(1) = 0;
*/
  BLOCKS      = (shr(CLICNOTE,8) * RESOLUTION * MULTIPLIER * CLICS) / (CLICNOTE & LBYTE);
  BLOCKS_MEAS = BLOCKS / MEASRS;
  OVER_BLOCKS = (shr(CLICNOTE,8) * MAX_BLOCKS * CLICS) / (CLICNOTE & LBYTE);
  NOTE_LEFT   = EDIT_LEFT + 24 + MAX_KEY_LENGTH; /* Allow room for clef & keysig */
  EDIT_FIELD  = NOTE_RIGHT - NOTE_LEFT + 1;
  NOTE_FIRST  = NOTE_LEFT + SIEVE(EDIT_FIELD,OVER_BLOCKS,1)/2; /* Center notes in edit field */

end PLOT_INITIALIZE;

PLOT_TIMBRE_NAME:procedure (P) swapable;
  dcl namearr(8) fixed;

  dcl (P,Q,I) pointer;

  namearr(0) = 16;
  write(mam)=OLD_SECT+shr(p,8);
  write(mal)=p;
  call copy.in(addr(namearr(1)),8);

  do while namearr(0) > 0 & byte(namearr,namearr(0)-1) = sp;
    namearr(0) = namearr(0) - 1;
  end;
  if namearr(0) = 0 then return;

  P = EDIT_LEFT + 4;
  Q = P + namearr(0) * 8;
  if P <= TIME_CUR_POS & TIME_CUR_POS <= Q
  then call TIME_CURSOR (0);
  if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF &
     P <= OLD_POS(X#) & OLD_POS(X#) <= Q
  then call DRAW_CURSOR;
  call move_to (EDIT_LEFT+4,-40);

  alpha_mode;

  // Set character size one size down for timbre name to let it fit under the g clef
  call pc(ESC);
  call pc(scolon-2);

  do I = 0 to namearr(0) - 1;
    call ALPHA_CHAR (byte(namearr,I));
  end;

  // Restire character size here since we do not set it before drawing other strings
  call pc(ESC);
  call pc(scolon-3);

  vector_mode;

  if P <= TIME_CUR_POS & TIME_CUR_POS <= Q
  then call TIME_CURSOR (TIME_CUR_POS);
  if SEQED_MODE = EDIT# & OLD_POS(STAFF#) = RL_STAFF &
     P <= OLD_POS(X#) & OLD_POS(X#) <= Q
  then call DRAW_CURSOR;

end PLOT_TIMBRE_NAME;

PLOT_CLEFKEY:procedure swapable;

  dcl (X,Y,K,ACC,I) fixed;

  dcl SHARP_TABLE    fixed data (0,L5,S3,S5,L4,S2,S4,L3),
      FLAT_TABLE     fixed data (0,L3,S4,S2,L4,L2,S3,S1);

  /* Plot clef */

  do case CLEF;
    call GCLEF (EDIT_LEFT + 12);   /* Treble */
    call CCLEF (EDIT_LEFT + 12,0); /* Alto */
    call CCLEF (EDIT_LEFT + 12,8); /* Tenor */
    call FCLEF (EDIT_LEFT + 12);   /* Bass */
    call PCLEF (EDIT_LEFT + 12);   /* Percussion */
    call PCLEF (EDIT_LEFT + 12);   /* Percussion with single staff */
    do;
      call GCLEF (EDIT_LEFT + 12);   /* Treble */
      call move_to (EDIT_LEFT+8,-40); alpha_mode;
      call ALPHA_CHAR (asc.8);
    end;
  end;

  /* Plot key */

  K = KEY_CODES(KEY);
  X = EDIT_LEFT + 24;
  if K > 0 then ACC = SHARP#; else ACC = FLAT#;
  do I = 1 to abs(K);
    if ACC = FLAT# then call FLAT  (X,
                                   ((FLAT_TABLE(I) + CLEF_MODS(CLEF))
                                   / 3 - 30) * VERTICAL_UNIT);
                   else call SHARP (X,
                                   ((SHARP_TABLE(I) + CLEF_MODS(CLEF))
                                   / 3 - 30) * VERTICAL_UNIT);
    X = X + 8;
  end;

end PLOT_CLEFKEY;

PLOT_TIES:procedure (NP,NEP) swapable; /* Plot bars, ties, and whole rests only */

  dcl (NP)      pointer,
      (NEP)     pointer,
      POSN           fixed,
      WLEN           fixed,
      BITS           fixed,
      NBITS          fixed,
      (VAL)          fixed,   /* Note value, stem length */
      (X,Y,DIR)      fixed,   /* Y position, Stem direction */
      (B,I,J,K)      fixed;

  do while NP ~= NEP;

    write(SECT$) = NEW_SECT + shr(NP,8);
    write(WORD$) = NP;

    POSN         = shr(read(DATA$),6);
    WLEN         = read(DATI$) & MASK6;
    BITS         = read(DATI$);

    VAL  = shr(BITS,5) & MASK3; /* Note value */
    DIR  = 1; if (BITS & P_DIR) ~= 0 then DIR = -1;
    X    = NOTE_FIRST + SIEVE(EDIT_FIELD,BLOCKS,POSN);

    /* Redraw bars */
    if (BITS & P_BAR) ~= 0 then do;
      if POSN = BLOCKS - 1 then call BAR (EDIT_RIGHT);
      else do;
        I = (OVER_BLOCKS * (POSN+1)) / BLOCKS;
        call BAR (NOTE_FIRST + (SIEVE(EDIT_FIELD,OVER_BLOCKS,I-1) +
                                SIEVE(EDIT_FIELD,OVER_BLOCKS,I)) / 2);
      end;
    end;

    do K = 5 to WLEN by 3;

      NBITS      = read(DATI$);
      Y          = ((NBITS & MASK6) - 30) * VERTICAL_UNIT;
      write(NOP) = read(DATI$); /* Eat pointer (sector) */
      write(NOP) = read(DATI$); /* Eat pointer (word) */

      if (NBITS & N_REST) ~= 0 then do; /* This note is actually a rest */
        if VAL = WHOLE# then do; /* Center whole rests in measure */
          // Handle case of partial first measure
          if ((POSN == 0) && ((START_CLICK-1) MOD CLICKS_MEASURE != 0))
            B = POSN + BLOCKS_MEAS - ((((START_CLICK-1) MOD CLICKS_MEASURE) * BLOCKS_MEAS) / CLICKS_MEASURE) - 1; /* Position of 1st block in next measure */

          // Handle case of full first measure
          else
            B = POSN + BLOCKS_MEAS - 1; /* Position of 1st block in next measure */

          I = (X + NOTE_FIRST + SIEVE(EDIT_FIELD,BLOCKS,B)) / 2;

          call REST (VAL,0,I,Y); /* Plot the rest */
        end;
      end;
      else do; /* Note */
        if (NBITS & N_OTIE) ~= 0 then do; /* Open a tie */
          B = shr(RESOLUTION * MULTIPLIER,VAL);
          if (NBITS & N_DOT) ~= 0 then B = B + shr(B,1);
          B = POSN + B;
          if B >= BLOCKS then J = EDIT_RIGHT; /* X2 point */
                         else J = NOTE_FIRST + SIEVE(EDIT_FIELD,BLOCKS,B);
          call SLUR (X+4,Y+4*(-DIR),J-4,Y+4*(-DIR),DIR=1);
        end;
      end; /* End of note record case */

    end; /* End of chord loop */

    NP = NP + WLEN;
    if ABORT_REFRESH(1) then return;

  end;

end PLOT_TIES;
