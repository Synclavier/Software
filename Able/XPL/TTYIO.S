/*  TTYIO  $TITLE  Terminal Character I/O, with interrupts off */

/*  This module handles the character-level terminal input and output,
.   using a type-ahead buffer and a polling system rather than interrupts.
.   It should be used when PRINT statements are mixed with TERMUTIL calls.
.
.   This insert must be preceded by the 'ASCIIDEF' literals file in ':-XPL'. */

insert ':-xpl:syslits';  /* system literals */
insert ':-xpl:intrprtr'; /* interpreter interface */

dcl control.seq  boolean public; /* true if in control seq to terminal */
dcl tty.no.host  boolean public; /* true to disable host running */
dcl handshake    boolean; /* TRUE if we should do XOn/XOff handshaking */

DCL
    hardc        boolean, /* terminal types */
    adm3a        boolean,
    vt100        boolean,
    datamedia    boolean,
    vt52         boolean,
    vt.series    boolean;

dcl dq640        boolean public; /* TRUE if this is a DQ640 */

/* character buffer and screen size data */

dcl maxcpl         lit '32767'; /* max chars per line */
dcl maxlpp         lit '30'; /* maximum number of lines per page */
dcl cblen          lit '16'; /* char buffer len */

dcl cpl            fixed; /* chars per line */
dcl lpp            fixed; /* lines per page */
dcl x.cur          fixed; /* current x coord. 0..cpl-1 */
dcl y.cur          fixed; /* current y coord. 0..lpp-1 */

dcl charbuf(cblen-1) fixed; /* buffer for input characters */
dcl (cbin,cbout)     fixed; /* pointers into buffer */
dcl lprint(maxlpp-1) fixed; /* current last non-blank on each line */
dcl esc.seq          fixed; /* indicates status in reading of escape seq */

/* run time initialization */

handshake=true; /* default to XOn/XOff handshaking */

do case terminal.type; /* determine terminal type */
   ; /* 0: unknown */
   hardc=1; /* 1: hardcopy */
   adm3a=1; /* 2: adm3a or adm5 */
   vt100=1; /* 3: vt100 */
   datamedia=1; /* 4: datamedia */
   vt52=1; /* 5: vt52 */
end;
vt.series=(vt100 or vt52); /* precompute if its a VT series term */
dq640=(vt100&(terminal.graphics=g#dq640)); /* VT100 with graphics G#DQ640 is a DQ640 */

/* $PAGE */

/*  W A I T _ F O R _ B R E A K  (internal)  */

wait_for_break:  proc; /* wait for a break to go away */
   dcl i fixed;

   write("3")=0; write("3")=0; /* get in synch with the D03 timer */

   do i=1 to 30*d03_speed; /* wait 300 ms for break to go away */
      write("3")=0; /* wait 5 ms */
   end;

   invoke break; /* invoke any WHEN BREAK */
end wait_for_break;

/*  S T O P C H R  (internal) */

stopchr:  proc(chr) fixed; /* return true if it was a control-s */
   dcl (chr, brk) fixed;

   if (handshake>0) and (chr=a.xoff) then do; /* control-s??? */
      if directed_output=0 then do; /* wait for XOn only if the output isn't redirected */
         brk=false; /* assume no break */
         do while chr<>a.xon; /* wait for control-q */
            if chr=a.nul then brk=true; /* flag any BREAK we get */
            chr=read("50")&"177"; /* get next character */
         end;
         if brk then call wait_for_break; /* wait for break here if we received one */
      end; /* of wait for XOn */
      return true; /* was a control-s */
   end; /* of control-s handling */
   if (handshake>0) & (directed_output<>0) & (chr=a.xon) then return true; /* if output is redirected, ignore ctrl-Q's */
   return false; /* ok */
end stopchr;

/*  X O N _ X O F F */

#xon_xoff:  proc(enable) public; /* enable/disable XOn/XOff handshaking */
   dcl enable boolean; /* 1: enable XOn/XOff, 0: disable XOn/XOff, -1: disable all interpretations */

   handshake=enable; /* just assign it */
end #xon_xoff;
DCL XON_XOFF lit 'call #xon_xoff';

/*  S C R E E N _ S I Z E  */

#screen_size:  proc(lines, columns) public; /* set the screen size */
   dcl (lines, columns) fixed; /* number of (lines, columns) in screen */

   if lines IGT maxlpp then lpp = maxlpp; /* restrict number of lines */
   else                     lpp = lines;

   if columns IGT maxcpl then cpl = maxcpl; /* restrict number of columns */
   else                       cpl = columns;
end #screen_size;
DCL SCREEN_SIZE lit 'call #screen_size';

/* $PAGE */

/*  D E C O D E  (internal) */

decode:  proc(chr) fixed; /* decode VT series metachars */
   dcl chr fixed; /* the character to decode */

   if      chr>=l.p then chr=kp.0    +chr-l.p; /* keypad digits */
   else if chr>=l.l then chr=kp.comma+chr-l.l; /* keypad , - . */
   else if chr>=l.a then chr=cur.up  +chr-l.a; /* arrow keys in vector mode on MG600 */
   else if chr>=a.p then chr=pf.1    +chr-a.p; /* PF keys */
   else if chr= a.m then chr=kp.enter; /* ENTER */
   else if chr= a.h then chr=cur.home; /* HOME */
   else if chr= a.e then chr=a.refresh; /* refresh screen (on MAC) */
   else                  chr=cur.up  +chr-a.a; /* arrow keys */

   return (chr);
end decode;

/*  C K R E A D  */

#ckread:  proc public; /* check if any input in terminal and read to buffer */
   dcl t   fixed;
   dcl chr fixed;

   RETRY: /* retry if a control-s received */
   if (read("51")&2)<>0 then do; /* if character ready */
      chr=read("50"); /* get the character */

      if handshake>=0 then do; /* interpret characters */
         chr=chr&"177";
         if stopchr(chr) then goto retry; /* try again */
         if chr=a.nul then call wait_for_break; /* if we get a null, wait for break */
         else if vt.series then do; /* map ESC sequences in meta chars on VT series terminals */
            if chr=a.esc then do;
               if vt52 then esc.seq=2; /* state 2: take next char */
               else         esc.seq=1; /* state 1: skip next char */
               return;
            end;
            if esc.seq=1 then do; esc.seq=2; return; end; /* skip char */
            if esc.seq=2 then do; /* end of seq, decode it */
               chr=decode(chr); /* decode it */
               esc.seq=0; /* done getting esc seq */
            end;
         end; /* of VT series */
      end; /* of interpret characters */

      t = cbin; /* remember pointer in case full */
      cbin=(cbin+1) & (cblen-1); /* bump and wrap pointer */
      if cbin=cbout then cbin=t; /* buffer is full */
      else charbuf(cbin)=chr; /* store char */
   end;

end #ckread;
dcl CKREAD lit 'call #ckread';

/* $page */


/*  T E R M _ I D L E  */

term_idle:  proc returns(boolean) public;
  interp_run_host_250();        /* run host if interpreting		*/
  ckread;                       /* check for input 				*/
  return (cbin=cbout);          /* true when terminal is idle 	*/
end term_idle;

term_idle_wait:  proc returns(boolean) public;
  interp_run_host_at_level();   /* run host if interpreting		*/
  ckread;                       /* check for input 				*/
  return (cbin=cbout);          /* true when terminal is idle 	*/
end term_idle_wait;


/*  C L E A R _ T Y P E A H E A D  */

#clear_typeahead:  proc public;

   cbin = cbout; /* reset pointers */

end #clear_typeahead;
dcl CLEAR_TYPEAHEAD lit 'call #clear_typeahead';

/*  # F L U S H _ T E R M  */

#flush_term:  proc public; /* flush terminal output buffer (interrupts ON only) */
	interp_wake_up_host();
end #flush_term;

/*  C H E C K _ X O F F  (internal) */

check_XOff:  proc; /* check for XOff/XOn from printer */
   dcl chr fixed;

   if (handshake>0) & ((read("41")&2)<>0) then do; /* has a character come in? */
      chr=read("40")&"177"; /* get the character */

      if chr=a.xoff /* is it an XOff?? */
      then do while (chr<>a.xon); /* yes, wait for XOn */
         if (read("41")&2)<>0 /* is there a character waiting? */
         then chr=read("40")&"177"; /* yes, get it */
         else ckread; /* no, check for terminal input */
         if (tty.no.host == 0)
		    interp_run_host_at_level();
      end; /* of waiting for XOn */
   end; /* of a character has come in */
end check_XOff;

/*  P E E K  */

peek:  proc fixed public; /* return next character if one, otherwise -1 */
   dcl (ch,icbout) fixed; /* character to return, internal pointer */

   ch=(-1); /* assume no next character */
   if not term_idle then do; /* character is there */
      icbout=(cbout+1) & (cblen-1); /* pointer to next word */
      ch=charbuf(icbout); /* return next character */
   end; /* character is there */
   return ch; /* return character */
end peek;

/* $page */

/*  R C H A R  */

rchar:  proc fixed public; /* return char from buffer */
   dcl ch fixed;

   RETRY: /* in case of control-s */
   if cbin=cbout then do; /* buffer empty */
      ch=read("50");
      if handshake>=0 then do; /* interpret chars? */
         ch=ch&"177"; /* mask off parity */
         if stopchr(ch) then goto retry; /* get character if it was stop char */
         if ch=a.nul then call wait_for_break; /* if we get a null, wait for break */
         else if vt.series then do; /* map ESC sequences in meta chars on VT series terminals */
            if ch=a.esc then do;
               if vt52 then esc.seq=2; /* state 2: take next char */
               else         esc.seq=1; /* state 1: skip next char */
               goto RETRY;
            end;
            if esc.seq=1 then do; esc.seq=2; goto RETRY; end; /* skip char */
            if esc.seq=2 then do; /* end of seq, decode it */
               ch=decode(ch); /* decode it */
               esc.seq=0; /* done getting esc seq */
            end;
         end; /* of VT series */
      end; /* of interpret characters */
   end; /* of no characters in buffer */
   else do; /* character in buffer */
      cbout=(cbout+1) & (cblen-1); /* bump output pointer and wrap */
      ch=charbuf(cbout);
   end;
   return ch; /* return character */

end rchar;

/*  W C H A R  */

#wchar:  proc(ch) public; /* subr to print a character checking for input */
   dcl ch fixed;
  
   ckread; /* see if any input waiting */

   if ((not control.seq) and (cpl <> 0)) then do; /* do not count if a control sequence or zero width screen */
      if ch>=a.sp then do; /* printable */
         if x.cur>=cpl then return; /* past end of line */
         x.cur=x.cur+1;
         if (ch>a.sp) and (x.cur>lprint(y.cur)) then lprint(y.cur)=x.cur;
      end;
      else do; /* not printable */
         if      ch=a.bs then x.cur=x.cur-1; /* backspace */
         else if ch=a.cr then x.cur=0; /* carriage return */
         else if (ch=a.lf) and (y.cur<lpp-1) then y.cur=y.cur+1; /* line feed */
         if x.cur<0 then x.cur=0;
      end; /* not printable */
   end; /* not a control sequence */

   if directed_output=1 then do; /* redirect output to printer? */
      call check_XOff; /* check for XOn/XOff */

      do while ((read("41")&"4")=0); /* wait until the transmit buffer's empty (in case an XOff comes in) */
         call check_XOff; /* check for XOn/XOff */
         if (tty.no.host == 0)
            interp_run_host_at_level();
      end;
      write("40")=ch; /* output the character to the printer */
   end; /* of redirect output to printer */
   else write("50")=ch; /* output the character to the screen */

   ckread; /* check again for input */

end #wchar;
DCL WCHAR lit 'call #wchar';
