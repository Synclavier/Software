/*
   10/26/87 - lpg - changed Help to reflect new features
   5/30/86 - adt & eg - updated for rel M
   5/20/86 - eg - made procedures and some variables pub.
   5/15/86 - "official" creation of release-M modules
*/

PRINT_INSTRUCTIONS:procedure swapable;

  if INSTR_MODE = SEQED_MODE then return;
  INSTR_MODE = SEQED_MODE;

  transparent_mode;
  if INSTR_MODE = MENU# then do;
    call CHECK.NEXT.EVENT; call cpos(1,2); call ps('      Use mouse or arrow keys to move cursor around menus and music.       ');
    call CHECK.NEXT.EVENT; call cpos(2,2); call ps('      Remove or restore these instructions with <CTRL-X>.                  ');
    call CHECK.NEXT.EVENT; call cpos(3,2); call ps('      Move between menus and music with <TAB>.                             ');
    call CHECK.NEXT.EVENT; call cpos(4,2); call ps('      Operate the memory recorder with:                                    ');
    call CHECK.NEXT.EVENT; call cpos(5,2); call ps('      <CTRL-P> start,     <CTRL-R> rewind,   <CTRL-C> continue,            ');
    call CHECK.NEXT.EVENT; call cpos(6,2); call ps('      <CTRL-E> stop,      <CTRL-F> F.F.      Print screen with <H>,        ');
    call CHECK.NEXT.EVENT; call cpos(7,2); call ps('      <CTRL-D> record,    <CTRL-V> punch.                                  ');
  end;
  else do;
    call CHECK.NEXT.EVENT; call cpos(1,2); call ps(' Move cursor with mouse or arrow keys.   Toggle instructions with <CTRL-X>.');
    call CHECK.NEXT.EVENT; call cpos(2,2); call ps('    /  Set duration: type 1,2,4,8,6(16th),3(32nd),<DOT> or set menu.');
    call CHECK.NEXT.EVENT; call cpos(3,2); call ps(' Add   Set accidental (if needed): Q - sharp, S - natural, Z - flat.');
    call CHECK.NEXT.EVENT; call cpos(4,2); call ps(' Note  Enter note pitch (A - G), rest (R), set menu, or right mouse button.');
    call CHECK.NEXT.EVENT; call cpos(5,2); call ps('    \  Add note to chord - <SHIFT> pitch (A - G).  Add menu note - <SPACE>');
    call CHECK.NEXT.EVENT; call cpos(6,2); call ps('Delete note  - <DEL>   Lengthen note - ]   Pitch up   - P   Note later  - }');
    call CHECK.NEXT.EVENT; call cpos(7,2); call ps('Justify note -   J     Shorten note  - [   Pitch down - ;   Note sooner - {');
  end;
  vector_mode;

end PRINT_INSTRUCTIONS;

ALPHA_CHAR:procedure (C) swapable; /* Plot an alpha char only if it is in the viewport */

  dcl C fixed;

  if (#lastx >= #view(0) & #lastx+ALPHA_WIDTH <= #view(1)) &
     (#lasty >= #view(2) & #lasty+ALPHA_HIGHT <= #view(3))
  then call pc(C);

end ALPHA_CHAR;

PRINT_PITCH:procedure (N) swapable;         
   dcl j fixed;
   dcl (N,I)  fixed,
          SH  fixed,
       PTCHS  fixed data ('CCDDEFFGGAAB');

   dcl ### fixed data (' # #  # # # ');

  write(5) = N + 12;
  write(7) = 12;
  I=read(4);
  j=read(5);
  sh=byte(###,i);
  call pc(byte(PTCHS,I));
  if sh=asc.sharp then call pc(asc.sharp);
  call pc(asc.0+j);

end PRINT_PITCH;

PRINT_DURATION:procedure (N) swapable;

  dcl N fixed;

  call pnum(shr(N,8),0);
  call pc(slash);
  call pnum(N & LBYTE,0);

end PRINT_DURATION;

STAFF_LINES:procedure (SINGLE) swapable;

  dcl (SINGLE,I) fixed;

  if SINGLE then do;
    call move_to (EDIT_LEFT,0); call plot (EDIT_RIGHT,0);
  end;
  else do I = -2 to 2;
    call move_to (EDIT_LEFT, I*2*VERTICAL_UNIT);
    call plot    (EDIT_RIGHT,I*2*VERTICAL_UNIT);
  end;

end STAFF_LINES;

BAR:procedure (X) swapable;

  dcl X fixed;

  call move_to (X,-VERTICAL_UNIT*4);
  call plot    (X, VERTICAL_UNIT*4);

end BAR;

SPOT:procedure (X,Y) swapable;

  dcl (X,Y) fixed;

  call move_to(X-2,Y); alpha_mode; call ALPHA_CHAR(dot);

end SPOT;

MENU_CURSOR:procedure (X,Y,CHARS) swapable;

  dcl (X,Y)  fixed,
      CHARS  fixed,
      (X2,I) fixed;

  I = 0; if MG600 then I = 1;
  X2 = X + ALPHA_WIDTH*CHARS - I;
  call move_to (X-2-I,Y-3); call plot (X2,Y-3);
  call plot    (X2,Y+ALPHA_HIGHT-3); call plot (X-2-I,Y+ALPHA_HIGHT-3);
  call plot    (X-2-I,Y-3);

end MENU_CURSOR;

EDIT_CURSOR:procedure (X,Y,R) swapable;

  dcl (X,Y,R,D) fixed;

  D = #data.level;
  call data_level (2);
  call move_to (X-7,R+16);
  call plot    (X+7,R+16);
  if R > RANGE_TOP then call move_to (X,R+16);
                   else call move_to (X,RANGE_TOP+16);
  if R < RANGE_BOT then call plot    (X,R-16);
                   else call plot    (X,RANGE_BOT-16);
  call move_to (X-7,R-16);
  call plot    (X+7,R-16);
  if Y ~= nullint then do;
    call move_to (X-7,Y+7); call plot (X+7,Y-7);
    call move_to (X-7,Y-7); call plot (X+7,Y+7);
  end;
  call data_level (D);

end EDIT_CURSOR;

NOTE_CURSOR:procedure (X,Y) swapable;

  dcl (X,Y) fixed;

  call move_to (X-20,Y-20); call plot (X+20,Y+20);
  call move_to (X-20,Y+20); call plot (X+20,Y-20);

end NOTE_CURSOR;

TIME_CURSOR:procedure (NEW_X) swapable;

  dcl NEW_X fixed,
      W(3)  fixed,
      V(3)  fixed,
      (D,M) fixed;

  dcl OLD_X lit 'OLD_TIME_CUR_POS';

  /* This routine preserves plotting package values in a rather poorly
     isolated way... i.e. any change to the plotting package literals
     will cause this to fuck up entirely. */

  if NEW_X = OLD_X then return;

  V(0) = #view(0);
  V(1) = #view(1);
  V(2) = #view(2);
  V(3) = #view(3);
  W(0) = #wind(0);
  W(1) = #wind(1);
  W(2) = #wind(2);
  W(3) = #wind(3);
  M    = #mode;
  D    = #data.level;

  call viewport (0,SCREEN_RIGHT,0,SCREEN_TOP);
  call window   (0,SCREEN_RIGHT,0,SCREEN_TOP);
  vector_mode;
  call data_level (2);
  call line_type (1);

  if OLD_X > 0 then do;
    call move_to (OLD_X,EDIT_BOT+1);
    call plot    (OLD_X,EDIT_TOP-1);
  end;
  if NEW_X > 0 then do;
    call move_to (NEW_X,EDIT_BOT+1);
    call plot    (NEW_X,EDIT_TOP-1);
  end;
  OLD_X = NEW_X;

  call data_level (D);
  call line_type (0);
  call viewport (V(0),V(1),V(2),V(3));
  call window   (W(0),W(1),W(2),W(3));
  do case M;
    transparent_mode;
    alpha_mode;
    vector_mode;
    point_mode;
  end;

end TIME_CURSOR;

LEDGER:procedure (X,Y0,LEN) swapable; /* Draw ledger lines */

  dcl (X,Y0,LEN,Y) fixed;

  do Y = VERTICAL_UNIT*6 to Y0 by VERTICAL_UNIT*2;
    call move_to (X-LEN,Y); call plot (X+LEN,Y);
  end;
  Y = -VERTICAL_UNIT*6;
  do while Y >= Y0;
    call move_to (X-LEN,Y); call plot (X+LEN,Y);
    Y = Y - VERTICAL_UNIT*2;
  end;

end LEDGER;

NOTE:procedure (TYP,X,Y) swapable;

  dcl (TYP,X,Y) fixed;

  if mg600 then do;
    call move_to (X,Y);
    call pc(esc); call ps('\O'); call pc(68);
    if TYP > HALF# then do;
      call pc(esc); call ps('\O'); call pc(67);
      call pc(esc); call ps('\O'); call pc(66);
      call pc(esc); call ps('\O'); call pc(65);
    end;
  end;
  else do;
    call move_to (X-3,Y-4); alpha_mode; call ALPHA_CHAR(asc.o);
    if TYP > HALF# then do;
      call ALPHA_CHAR(bs); call ALPHA_CHAR(asc.sharp);
    end;
  end;

end NOTE;

STEM:procedure (X,Y,LEN,FLAGS) swapable; /* Draw a stem with flags */

  dcl (X,Y,LEN,FLAGS) fixed; /* X,Y position, length of stem, # of flags on stem*/

  call move_to (X,Y); call plot (X,Y+LEN); /* plot stem */

  if LEN > 0 then do while (FLAGS > 0);
    call move_to (X,Y+LEN); call plot (X+6,Y+LEN-8); /* WAS +6 -4 */
    LEN   = LEN   - VERTICAL_UNIT*2;
    FLAGS = FLAGS - 1;
  end;
  else if LEN < 0 then do while (FLAGS > 0);
    call move_to (X,Y+LEN); call plot (X+6,Y+LEN+8);
    LEN   = LEN   + VERTICAL_UNIT*2;
    FLAGS = FLAGS - 1;
  end;

end STEM;

SHARP:procedure (X,Y) swapable; /* Plot a sharp at the specified location */
 
  dcl (X,Y) fixed;

  BAR:proc (X,Y);
    dcl (X,Y) fixed;
    call move_to (X-4,Y-2); call plot (X+4,Y+2);
  end BAR;

  call BAR (X,Y-3);
  call BAR (X,Y+3);
  call move_to (X-2,Y-9); call plot (X-2,Y+8);
  call move_to (X+2,Y-8); call plot (X+2,Y+9);

end SHARP;

NATURAL:procedure (X,Y) swapable;  /* Plot a natural at the specified location */

  dcl (X,Y) fixed;

  call move_to (X-2,Y+4); call plot (X+2,Y+4);
  call move_to (X-2,Y+3); call plot (X+2,Y+3);
  call move_to (X-2,Y-4); call plot (X+2,Y-4);
  call move_to (X-2,Y-3); call plot (X+2,Y-3);
  call move_to (X-2,Y-4); call plot (X-2,Y+9);
  call move_to (X+2,Y-9); call plot (X+2,Y+4);

end NATURAL;

FLAT:procedure (X,Y) swapable; /* Draw a flat at the specified location */

  dcl (X,Y) fixed;

  call move_to (X-2,Y+2);
  call plot    (X+3,Y+2); call plot (X+3,Y);
  call plot    (X  ,Y-3); call plot (X-2,Y-3);
  call plot    (X-2,Y+10);

end FLAT;

WHOLE_REST:procedure (X,Y) swapable;

  dcl (X,Y,I) fixed;

  do I = 1 to 3;
    Y = Y - 1;
    call move_to (X-4,Y); call plot (X+4,Y);
  end;

end WHOLE_REST;

EIGHTH_REST:procedure (X,Y,T) swapable; /* plot a rest of a specified value at the given location */

  dcl (X,Y,T,I,J) fixed;

  I = 1;
  do case T;
    ;                                      /* 8th */
    do; X = X - 2; Y = Y - 8;  I = 2; end; /* 16th */
    do; X = X - 2; Y = Y - 8;  I = 3; end; /* 32nd */
    do; X = X - 4; Y = Y - 16; I = 4; end; /* 64th */
    do; X = X - 6; Y = Y - 24; I = 5; end; /* 128th */
  end;

  do J = 1 to I;

    call move_to (X-4,Y+1); call plot (X-2,Y+1);
    call move_to (X-4,Y);   call plot (X-2,Y);
    call move_to (X-4,Y-1); call plot (X-2,Y-1);
    call plot    (X+3,Y+1); call plot (X  ,Y-12);

    X = X + 2; Y = Y + 8;

  end;

end EIGHTH_REST;

REST:procedure (TYP,CNT,X,Y) swapable; /* Plot a rest of a specified type, count for block rests and given location */

  dcl TYP  fixed, /* Type of rest */
      CNT   fixed, /* Count for block rests */
      (X,Y,I) fixed;

  if      TYP = WHOLE# then call WHOLE_REST (X,Y+VERTICAL_UNIT);
  else if TYP = HALF#  then call WHOLE_REST (X,Y);
  else if TYP = QUARTER# then do;
    call move_to (X-3,Y-2);  alpha_mode; call ALPHA_CHAR(asc.great);
    call move_to (X-3,Y-6);  alpha_mode; call ALPHA_CHAR(asc.less);
    call move_to (X-3,Y-14); alpha_mode; call ALPHA_CHAR(asc.lparen);
  end;
  else call EIGHTH_REST (X,Y,TYP-EIGHTH#); /* Plot an eighth rest */

end REST;

REPEAT:procedure (X,LEN,FLAG) swapable;

  dcl (I,X,LEN,FLAG) fixed;

  I = 1; if FLAG then I = -1;
/*
  call HEAVY_BAR (X+I,LEN);
  call BAR (X+5*I,LEN);
  call PLOT_SYMBOL ("241",X+8*I,S2,UP#);
  call PLOT_SYMBOL ("241",X+8*I,S3,UP#);
*/
end REPEAT;

XBEAM:procedure (X1,Y1,X2,Y2) swapable;

  dcl (X1,Y1,X2,Y2) fixed;

  call move_to (X1,Y1); call plot (X2,Y2);

end XBEAM;

BEAM:procedure (X1,Y1,X2,Y2) swapable; /* Plot a beam */

  dcl (X1,Y1,X2,Y2) fixed;
/*error_case do; call XBEAM (X1,Y1,X2,Y2); return; end;*/

  call move_to (X1,Y1-1); call plot (X2,Y2-1);
     call plot (X2,Y2);   call plot (X1,Y1);
     call plot (X1,Y1+1); call plot (X2,Y2+1);

end BEAM;

SLUR:procedure (X1,Y1,X2,Y2,DIR) swapable;

  dcl (X1,Y1,X2,Y2,DIR,I) fixed;

  call move_to (X1,Y1);
  I = X2 - X1;
  if I < 10 then do;
    I = I / 2;
    if DIR then call plot (X1+I,Y1-I);
           else call plot (X1+I,Y1+I);
  end;
  else do;
    if DIR then I = -5; else I = 5;
    call move_to (X1,Y1);  call plot (X1+5,Y1+I);
    call plot (X2-5,Y2+I);
  end;
  call plot (X2,Y2);

end SLUR;

GCLEF:procedure (X) swapable;

  dcl (X,I) fixed,
      PIC data ( -4,-15,
                 -4,-17,
                  3,-17,
                 -2, 26,
                  4, 32,
                  6, 22,
                 -9,  6,
                 -9, -8,
                  3, -8,
                  7,  0,
                  2,  8,
                 -4,  0);

  call move_to (X+PIC(0),PIC(1)-8);
  do I = 2 to 11 * 2 by 2;
    call plot (X+PIC(I),PIC(I+1)-8);
  end;

end GCLEF;

CCLEF:procedure (X,Y) swapable;

  dcl (X,Y) fixed;

  call move_to (X-8,Y+16);
  call plot    (X-8,Y-16);
  call move_to (X+8,Y+16);
  call plot    (X-8,Y);
  call plot    (X+8,Y-16);

end CCLEF;

FCLEF:procedure (X) swapable;

  dcl (X,I) fixed,
      PIC data ( -8,-12,
                  2, -8,
                  8,  0,
                  8,  8,
                  4, 16,
                 -4, 16,
                 -8, 10,
                 -8,  6,
                 -4,  6,
                 -4, 10,
                 -8, 10);

  X = X - 1;
  call move_to (X+PIC(0),PIC(1));
  do I = 2 to 10 * 2 by 2;
    call plot (X+PIC(I),PIC(I+1));
  end;
  call move_to (X+8,4);  alpha_mode; wchar(dot);
  call move_to (X+8,12); alpha_mode; wchar(dot);

end FCLEF;

PCLEF:procedure (X) swapable;

  dcl X fixed;

  call move_to (X - 2,-8);
  call plot    (X - 2, 8);
  call move_to (X + 2,-8);
  call plot    (X + 2, 8);

end PCLEF;
