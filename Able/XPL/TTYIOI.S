/*  TTYIOI  $TITLE  Terminal Character I/O, with interrupts on */

/*  This module handles the character-level terminal input and output,
.   using a type-ahead buffer and with interrupts enabled.  This is
.   used in the Screen Editor where extreme speed is needed.  Since
.   this module will process the DC1 and DC3 control chars, the 'PRINT'
.   and 'LINPUT' statements must not be used when this is included.
.
.   See the 'TTYIO' module for a version with interrupts off.
.
.   This insert must be preceded by the 'ASCIIDEF' literals file in ':-XPL'.
*/

insert ':-xpl:syslits'; 	/* system literals */
insert ':-xpl:intrprtr'; 	/* interpreter interface */

dcl control.seq boolean public; /* true if in control seq to terminal */
dcl dq640       boolean public; /* TRUE if this is a DQ640 */
dcl tty.no.host boolean public; /* true to disable host running */
dcl handshake   boolean; /* TRUE if we should do XOn/XOff handshaking */

DCL
    hardc     boolean, /* terminal types */
    adm3a     boolean,
    vt100     boolean,
    datamedia boolean,
    vt52      boolean,
    vt.series boolean;

/* character buffer and screen size data */

dcl maxcpl      lit '32767'; /* max chars per line */
dcl maxlpp      lit '30'; /* maximum number of lines per page */
dcl cblen       lit '16'; /* input char buffer len */
dcl tty.buf.len lit '16'; /* output buf length */

dcl cpl   fixed; /* chars per line */
dcl lpp   fixed; /* lines per page */
dcl x.cur fixed; /* current x coord. 0..cpl-1 */
dcl y.cur fixed; /* current y coord. 0..lpp-1 */

dcl charbuf(cblen-1) fixed; /* buffer for input characters */
dcl lprint(maxlpp-1) fixed; /* current last non-blank on each line */
dcl (cbin,cbout)     fixed; /* pointers into buffer */
dcl esc.seq          fixed; /* indicates status in reading of escape seq */
dcl buffer.full      boolean; /* true when VT100 buffer is full */
dcl any.sent         boolean; /* true if any characters sent */

dcl (rptr,sptr)         fixed; /* pointers into output rotary buffer */
dcl pbuf(tty.buf.len-1) fixed; /* output rotary buffer */
dcl busy                fixed; /* true when output is active */

/* run time initialization */

handshake=true; /* default to XOn/XOff handshaking */

do case terminal.type; /* determine what kind of terminal this is */
   ; /* 0: unknown */
   hardc=1; /* 1: hardcopy */
   adm3a=1; /* 2: adm3a or adm5 */
   vt100=1; /* 3: vt100 */
   datamedia=1; /* 4: datamedia */
   vt52=1; /* 5: vt52 */
end;
vt.series=(vt100 or vt52); /* precompute if its a VT series term */
dq640=(vt100&(terminal.graphics=g#dq640)); /* VT100 with graphics G#DQ640 is a DQ640 */

enable; /* turn interrupts on here */

/* $page */


/*  W A I T _ F O R _ B R E A K  (internal)  */

wait_for_break:  proc; /* wait for a break to go away */
   dcl i fixed;

   write("3")=0; write("3")=0; /* get in synch with the D03 timer */

   do i=1 to 30*d03_speed; /* wait 300 ms for break to go away */
      write("3")=0; /* wait 5 ms */
   end;

   invoke break; /* invoke any WHEN BREAK */
end wait_for_break;

/*  B O O T _ T E R M  (internal)  */

boot_term:  proc; /* start up terminal output - MUST CALL WITH INTERRUPTS OFF */
   dcl i fixed;

   if rptr<>sptr then do;
      write("50")=pbuf(rptr); /* write out byte to terminal */
      rptr=(rptr+1)&(tty.buf.len-1); /* bump and wrap read pointer */
      busy=1; /* mark busy if character sent */
      any.sent=true;
   end;
   else busy=0;
end boot_term;

/*  F L U S H _ T E R M  */

#flush_term:  proc public; /* wait for terminal to finish printing */
   do while rptr<>sptr;
      interp_do_interrupts(); // Push the character out to the interpreter
      if (tty.no.host == 0)
         interp_run_host_at_level();
   end; /* loop until all chars outputted */

   if (any.sent != false) {
      interp_wake_up_host();
      any.sent = false;
   }

end #flush_term;
dcl FLUSH_TERM lit 'call #flush_term';

/*  # C K R E A D  */

#ckread:  proc public; /* poll terminal for input (interrupts OFF only) */
end #ckread;

/* $SUBTITLE  Interrupt routines */

when ttiint then begin; /* interrupt handler */
   dcl (chr,brk,t) fixed;

   chr=rcvdcharacter; /* get the character */

   if handshake>=0 then do; /* interpret characters */
      chr=chr&"177"; /* mask off parity */

      if chr=a.nul then do; /* if null, wait for break to go away */
         if buffer.full /* is the terminal buffer full (are we waiting for XOn)? */
         then brk=true; /* yes, remember we received a break */
         else call wait_for_break; /* no, wait for break to go away */
      end; /* of waiting for break */
      else if (handshake>0) & (chr=a.xoff) then do; /* terminal buffer full (XOn/XOff protocol) */
         buffer.full=true;
         brk=false; /* we haven't received a BREAK yet */
         return;
      end;
      else if (handshake>0) & (chr=a.xon) then do; /* terminal buffer ready */
         buffer.full=false;
         if brk then call wait_for_break; /* if we received a break while waiting, wait for it to go away */
         if busy=0 then call boot_term; /* restart output unless character already going */
         return;
      end;
      else if vt.series then do; /* special logic to map esc seqs into meta chars */
         if chr=a.esc then do;
            if vt52 then esc.seq=2; /* state 2: take next char */
            else         esc.seq=1; /* state 1: skip next char */
            return;
         end;
         if esc.seq=1 then do; esc.seq=2; return; end; /* skip char */
         if esc.seq=2 then do; /* end of seq, decode it */
            if      chr>=l.p then chr=kp.0    +chr-l.p; /* keypad digits */
            else if chr>=l.l then chr=kp.comma+chr-l.l; /* keypad , - . */
            else if chr>=l.a then chr=cur.up  +chr-l.a; /* arrow keys in vector mode on MG600 */
            else if chr>=a.p then chr=pf.1    +chr-a.p; /* PF keys */
            else if chr= a.m then chr=kp.enter; /* ENTER */
            else if chr= a.h then chr=cur.home; /* HOME */
            else if chr= a.e then chr=a.refresh; /* refresh screen (on MAC) */
            else                  chr=cur.up  +chr-a.a; /* arrow keys */
            esc.seq=0; /* done getting esc seq */
         end;
      end; /* special logic */
   end; /* of not a null */

   t=cbin; /* remember pointer in case full */
   cbin=(cbin+1) & (cblen-1); /* bump pointer and wrap */
   if cbin=cbout then cbin=t; /* buffer is full */
   else               charbuf(cbin)=chr; /* store char */
   return; /* done */

end; /* of interrupt handler */

when ttoint then begin; /* on output done, start next char */
   if buffer.full=0 then call boot_term; /* perform more output if not stopped */
   else busy=0; /* mark un-busy if stopped */
end;

/* $PAGE */

/*  X O N _ X O F F */

#xon_xoff:  proc(enable) public; /* enable/disable XOn/XOff handshaking */
   dcl enable boolean; /* 1: enable XOn/XOff, 0: disable XOn/XOff, -1: disable all interpretations */

   handshake=enable; /* just assign it */
end #xon_xoff;
DCL XON_XOFF lit 'call #xon_xoff';

/*  S C R E E N _ S I Z E  */

#screen_size:  proc(lines, columns) public; /* set the screen size */
   dcl lines   fixed; /* number of lines in screen */
   dcl columns fixed; /* number of columns in screen */

   if lines IGT maxlpp then lpp = maxlpp; /* restrict number of lines */
   else                     lpp = lines;

   if columns IGT maxcpl then cpl = maxcpl; /* restrict number of columns */
   else                       cpl = columns;
end #screen_size;
DCL SCREEN_SIZE lit 'call #screen_size';

/*  C H E C K _ X O F F  (internal)  */

check_XOff:  proc; /* check for XOff/XOn from printer */
   dcl chr fixed;

   if (handshake>0) & ((read("41")&2)<>0) then do; /* has a character come in? */
      chr=read("40")&"177"; /* get the character */

      if chr=a.xoff /* is it an XOff?? */
      then do while (chr<>a.xon); /* yes, wait for XOn */
         if (read("41")&2)<>0 /* is there a character waiting? */
         then chr=read("40")&"177"; /* yes, get it */
         if (tty.no.host == 0)
            interp_run_host_at_level();
      end; /* of waiting for XOn */
   end; /* of a character has come in */
end check_XOff;


/*  T E R M _ I D L E  */

term_idle:  proc returns(boolean) public;
   flush_term;                   /* flush the terminal's output buffer */
   interp_run_host_250();        /* run host if interpreting		*/
   return (cbin=cbout);          /* true when terminal is idle 	*/
end term_idle;

term_idle_wait:  proc returns(boolean) public;
   flush_term;                   /* flush the terminal's output buffer */
   interp_run_host_at_level();   /* run host if interpreting		*/
   return (cbin=cbout);          /* true when terminal is idle 	*/
end term_idle_wait;


/*  C L E A R _ T Y P E A H E A D  */

#clear_typeahead:  proc public;

   cbin = cbout; /* reset pointers */

end #clear_typeahead;
dcl CLEAR_TYPEAHEAD lit 'call #clear_typeahead';

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

/* $SUBTITLE  Terminal Read and Write Routines */

/*  R C H A R  */

rchar:  proc fixed public; /* return char from buffer */

   flush_term; /* flush the terminal's output buffer */

   do while cbin=cbout;
      flush_term;
		interp_run_host_at_level();
	end; /* buffer empty, wait for char */

   cbout=(cbout+1) & (cblen-1); /* bump output pointer and wrap */
   return charbuf(cbout);

end rchar;


/*  W C H A R  With Interrupts */

#wchar:  proc(ch) public; /* subr to print a character checking for input */
   dcl ch fixed;
  
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

   else do; /* output the character to the screen */
      do while ((sptr+1)&(tty.buf.len-1))=rptr;
         interp_wake_up_host();
         if (tty.no.host == 0)
            interp_run_host_at_level();
		end; /* there is no space in circ queue */
      disable; /* during pointer storage */
      pbuf(sptr)=ch; /* store character in output buffer */
      sptr=(sptr+1)&(tty.buf.len-1); /* bump and wrap output pointer */
      if (buffer.full=0)&(busy=0) then call boot_term; /* start output if buffer ready and terminal is idle */
      enable;
      interp_do_interrupts(); // Push the character out to the interpreter
   end;

end #wchar;
DCL WCHAR lit 'call #wchar';
