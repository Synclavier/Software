/* LOD-CUE4 - routines to scroll cue and process cue directory: *//* Scroll.DTD.Current.Cue is used to scroll forward or backwards *//* in the cue directory.   It is passed the name of the current  *//* cue,  or a name entered by the user.  It looks at the global  *//* Saved.CM.Display.Switch to see if the scroll should occur in  *//* all projects or just the current project.                     *//* a 1 if returned if the end of the list was reached.           *//* Handy routine to clean up cue name for comparisons. */Clean.cue.name:proc (tmp) public swapable;   dcl tmp     array;   dcl i       fixed;   call Strip.Leading.Spaces(tmp,tmp);   do while  (tmp(0)<>0)   and (byte(tmp,tmp(0)-1) = 32);      tmp(0)=tmp(0)-1;      call pbyte(tmp,tmp(0),0);   end;end Clean.Cue.Name;SCROLL.DTD.CURRENT.CUE: proc(name, dir) public swapable;   dcl name		array;    /* pass name of current cue to scroll from */   dcl dir		fixed;    /* pass direction (0 = previous)           */   dcl com		(17-1)	fixed;   dcl ourname	(17-1)	fixed;   dcl new.id#				fixed;   call COPY.STRING(name, ourname);   /* get working copy            */   if DTD.Max.Secs <> 0 then do;      /* make sure dtd available     */      if ourname(0) = 0 then do;      /* start with space to scroll  */         ourname(0) = 1;              /* from null string.           */         ourname(1) = 32;             /* single space character      */      end;      call Set.DTD.Scroll.Range(Default.CM.Display.Switch + GID(Saved.CM.Display.Switch));      call Locate.DTD.Cue(ourname);               /* see if exists     */      retry:;                                 /* re-enter if dup   */      if dir = 0 then call Send.To.Lod(25,0);  /* Previous alpha cue */      else            call Send.To.Lod(24,0);  /* Next alpha cue     */      if DTD.Cue# = 0                          /* if no cue avail      */      then return 1;                           /* end of list !!       */      write(mam) = Scsi.Ptr;                /* check name to see if */      write(mal) = CUE.NAME;                /* dup cue name         */      call COPY.IN(addr(com(0)),17);      call Uppercase.String(com,com);      call Clean.Cue.Name(com);      call Uppercase.String(ourname,misc.buf);      call Clean.Cue.Name(misc.buf);      if EQSTR(com,misc.buf) = 0      then goto retry;      new.id# = DTD.Cue#;      if DTD.PLAY.STATE <> 0      then call STOP.DTD.PLAY.STATE;    /* stop pending dtd output  */      call SET.DTD.CURRENT.REEL(0);     /* select "cue"             */      call Fetch.Entire.DTD.Cue(new.id#,Current.Cue.Ptr);      call Deposit.A.New.Current.Cue(1);      call UnSet.Cue.Modified;   end;   return 0;end SCROLL.DTD.CURRENT.CUE;