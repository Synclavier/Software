/* $TITLE MNOTSOU  - Module for:   Music NOtation Screen */

/*
   06/29/88 - MWH - Remove unused inserts
   10/23/87 - LPG - Added Mouse insert/delete
   2/18/87 - eg - created this file
*/

module mnotmod;

   insert ':synlits:alltlits';  /* get all terminal literals */
   insert ':synmods:seqdcl';  
   insert ':synmods:globdcl';
   insert ':synmods:xmemdcl';
   insert ':synmods:ttydcl';
   insert ':synmods:plotdcl';
   insert ':synmods:errdcl';
   insert ':synmods:getvdcl';  

   insert ':synmods:scrndcl';
   insert ':synmods:d40dcl';
   insert ':synmods:tformdcl';
   insert ':synmods:getdcl';
   insert ':synmods:tprimdcl';
   insert ':synmods:moussdcl';

   insert ':synrdcls:03-pdcls';
   insert ':synrdcls:04-sdcls';
   insert ':synrdcls:05-bdcls';
   insert ':synrdcls:11-tdcls';
   insert ':synlits:synclits';

   begin;                       /* to reuse declarations     */

	// Local literals
	
	dcl MNOT_SEQEVENTS_OF_INTERST lit '(1\2\4\8\16\64)';
	
   /* procedure to check play boundaries */
   /* while plotting                     */

dcl INITIALIZE.SEQED   procedure (array) recursive,
    SEQED.REFRESH      procedure (array) recursive,
    SEQED.TIME.CURSOR  procedure recursive,
    SEQED.MENU.INPUT   procedure (fixed,fixed,fixed,fixed,array,fixed) recursive, /*lpg*/
    POS.SEQED.CURSOR   procedure recursive;


   CHECK.WINDOW.BOUNDARIES:procedure;  /* CALLED FROM SEQED ONLY NOW */

      dcl i fixed;

      if inc.music.notation=0 then return;

      if (new.ABLE.seq.info&32) <> 0 then do;    /* cursor movement */
         call SEQED.TIME.CURSOR;
      end;

   end CHECK.WINDOW.BOUNDARIES;


   /* get source files            */
   enter ':synsou:mnotmod';

   insert 'SQEDLITS'; /* Special literals for SEQED */
   insert 'GLOBDCLS'; /* Global variables for SEQED */

   insert 'PICSFNCS'; /* Pictures */
   insert 'GLOBFNCS'; /* Utility procedures */
   insert 'NMAPFNCS'; /* Note map building procedures */
   insert 'PLOTFNCS'; /* Plotting procedures */
   insert 'REFRFNCS'; /* REFRESH, compare and menu drawing procedures */
   insert 'EDITFNCS'; /* Edit mode procedures */
   insert 'MENUFNCS'; /* Menu mode procedures */
   insert 'MAINFNCS'; /* Main drivers */

   SEQED.MENU.MOUSE.POS:  proc(x,y,core_area,action) public swapable;        /* Sequence Editor Screen Mouse Positioner*/
      dcl (x,y)     fixed;  /* mouse position in screen coordinates */
      dcl action    fixed;  /* indicates delete, position, add note */
      dcl (i,j)     fixed;
      dcl (k,l)     fixed;
      dcl (row,col) fixed;
      dcl core_area array;

      if mouse.hole(x,y) then do;   /* if cursor over mouse hole */
         EXIT.MY.SCREEN = 1;
         return;
      end;

      /* see if mouse cursor is in main menu */

      do col=0 to 3;                /* loop over columns */
         do row=0 to 1;             /* loop over rows    */
            i=row*4+col;            /* get entry #       */

            if  (y >= menu_top+orows(row)-4)
            and (y <  menu_top+orows(row)+char.height-4)
            and (x >= mcols(col))
            and (x < (ocols(col)+max(olens(i),olens(i xor 1))*char.width))
            then do;
               call SEQED.MENU.INPUT(in.abss,0,100+row,col,core_area,action);    /* Sequence Editor input routine */
            end;
         end;
      end;

      /* check each of the staff areas */

      if  (y >= edit_bot)
      and (y <= edit_top)
      then do;

         i=0; j=abs(y-Y_POS(0));
         do k=1 to staves;
            l=abs(y-Y_POS(k));
            if l < j then do;
               i=k;
               j=l;
            end;
         end;

         if x < edit_left then do;     /* in part menu */
            do col=0 to 1;             /* loop over columns */
               do row=0 to 3;          /* loop over rows    */
                  
                  j=row*2+col;         /* get entry #       */

                  if plens(j)<>0       /* make sure entry exists */
                  then do;
                     if col=0 then k=0;  /* extend column 0 position to include label.   column 1 has no label */
                     else          k=pcols(col);

                     if  (y >= y_pos(i)+prows(row)-4)
                     and (y <  Y_pos(i)+prows(row)+char.height-4)
                     and (x >= k)
                     and (x <  pcols(col)+plens(j)*char.width)
                     then call SEQED.MENU.INPUT(in.abss,i,row,col,core_area,action);    /* Sequence Editor input routine */
                  end;
               end;
            end;
         end;

         else if x >= note_left then do;                      /* in music area */
            call SEQED.MENU.INPUT(in.abss,256+i,y-y_pos(i),x,core_area,action);    /* Sequence Editor input routine */
         end;
      end;

   end SEQED.MENU.MOUSE.POS;

   /* $PAGE - driver for Music Notation Screen */

   MUSIC.NOTATION.SCREEN:proc (arg) public swapable;
      dcl arg       fixed;              /* argument - must pass 0 for now */
      dcl code      fixed;              /* code returned by get.next.event */
      dcl core_area (255) fixed;

      insert ':synmods:mathdcl';

      if (inc.music.notation=0)
      then do; 
         call feature.not.available;
         return (-1);
      end;

      if (vt640\mg600)=0 then do;
         call no.graphics.terminal;   /* graphics terminal required for this screen */
         return (-1);
      end;

      call STR32(0,   0, Screen.Begin       );
      call STR32(0,   0, Cursor.Begin       );
      call STR32(-1, -1, Screen.End         );
      call STR32(-1, -1, Cursor.End         );

      call STR32(0,   0, Screen.Scroll.Begin);
      call STR32(0,   0, Cursor.Scroll.Begin);
      call STR32(-1, -1, Screen.Scroll.End  );
      call STR32(-1, -1, Cursor.Scroll.End  );

      full.seqed.refresh = 1;
      call SEQED.REFRESH(CORE_AREA);    /* draw main screen */

      EXIT.MY.SCREEN   = 0;             /* clear these flags before loop */
      GOTO.THIS.SCREEN = 0;

      do while EXIT.MY.SCREEN = 0;      /* wait for exit condition       */

         call ENABLE_SEVERAL_GETS(get.in.char,      	/* get in.chars      */
                                  get.mouse.button, 	/* get mouse buttons */
                                  get.play.scrolling,	/* check window boundaries */
                                  get.new.seq.info);  /* new notes, constants, etc */
   
         code = GET.NEXT.EVENT;                     	/* get event         */

         call DISABLE_SEVERAL_GETS(get.in.char,      
                                  get.mouse.button, 
                                  get.play.scrolling, 
                                  get.new.seq.info);  

         do case (code-1);

            do;                        /* input character received */
               if next.event.info >= asc.call.convert
               then call RETURN.TO.PREV.STATE;
               else call SEQED.MENU.INPUT(in.chin,next.event.info,0,0,core_area,#ACTION_NONE); 
            end;

            do;         /* mouse event */
               if next.event.info = 1 then do;  /* right button = add note at mouse click */
                  call SEQED.MENU.MOUSE.POS(mouse(release.x),mouse(release.y),core_area, #action_add);
               end;

               if next.event.info = 2 then do;  /* middle button = position cursor */
                  call SEQED.MENU.MOUSE.POS(mouse(release.x),mouse(release.y),core_area,#action_none);
               end;

               if next.event.info = 3 then do;  /* left button = delete note at mouse click */
                  call SEQED.MENU.MOUSE.POS(mouse(release.x),mouse(release.y),core_area, #action_delete);
               end;
            end;

            ;                          /* mouse movement not used */
            ;                          /* new timbre not used     */
            ;                          /* psfree not displayed    */

            do;                        /* new seq info            */
               next.event.info = next.event.info & (MNOT_SEQEVENTS_OF_INTERST \ 32); /* recall, names, notes, constants, mode, cursor, scroll */
               if (next.event.info&32)<>0 then do;
                  call SEQED.TIME.CURSOR;
                  next.event.info = next.event.info & not(32);
               end;
               if next.event.info<>0        /* seqed screen needs refreshing */
               then call SEQED.REFRESH(core_area);     /* refresh the seqed screen (this should follow CHECK.WINDOW case) */
            end;

            ;                          /* no play scrolling       */
            ;                          /* entry writes no effect  */
            ;                          /* new prm info            */
            ;                          /* dtd info - nothing      */
            ;                          /* poly change - nothing   */
            ;                          /* errors - not displayed  */
            ;                          /* nothing on ork/smpte */

         end;                          /* of do case           */

      end;                             /* of wait for exit     */

      EXIT.MY.SCREEN   = 0;            /* clear flag after loop */

      return GOTO.THIS.SCREEN;

   end MUSIC.NOTATION.SCREEN;

   TEMP_BASE = alloc.examount(1);
   MENU_BASE = alloc.examount(1);

   end;

end mnotmod;
