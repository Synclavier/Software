/*
   03/28/18 - cj - added sample rate printout and CSV option
*/
dcl Printing     boolean;
   Printing     =  Get_YN('Send output to printer [Y(es) or N(o)]? '); crlf;
   if (Printing == true)
   DisplayingAllInfo = ShowAllInfo;