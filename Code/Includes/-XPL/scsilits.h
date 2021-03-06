/*
             S c s i   l i t e r a l s
*/

#define	scsiboard		((fixed) 0x4000)		/* bit set in D51 if D24 is there				*/

/* D24 scsi ports */
#define	scsisel			0x0014					/* Scsi selection port							*/
#define	scsidata		0x0014					/* Scsi data port								*/
#define	scsibus			0x0015					/* Scsi signal bus								*/
#define	scsibyte		0x0016					/* Read/Write a byte from SCSI					*/
#define	scsiword		0x0017					/* Read/Write a word from SCSI					*/

/* Standard Devices */
#define	s_d24			0						/* Standard D24 board							*/
#define	s_initiator		6						/* Standard Initiator ID						*/
#define	s_target		5						/* Standard Target ID							*/
#define	s_lun			0						/* Standard Target LUN							*/

/* D24 bit masks */

#define	s_selectenable	((fixed) 0x1000)		/* set this on SCSISEL to enable select function	*/
#define	s_selmask		0x0F00					/* to mask off select bits from SCSISEL			*/
#define	s_revmask		((fixed) 0xE000)		/* to mask off revision number from SCSISEL		*/
#define	s_revshift		13						/* left shift to get revision number from SCSISEL	*/

#define	s_arbitrate7	0x0200					/* set this on SCSIBUS to arbitrate for ID 7	*/
#define	s_arbitrate6	0x0A00					/* set this on SCSIBUS to arbitrate for ID 6	*/
#define	s_datamask		0x00FF					/* to mask off data bits from SCSIDATA			*/

#define	s_busmask		0x01FF					/* to mask off signal lines from SCSIBUS		*/
#define	s_intmask		0x0E00					/* to mask off interrupt bits from SCSIBUS		*/
#define	s_intshift		9						/* left shift to get interrupt bits from SCSIBUS	*/

/* Bit masks for signals on Signal Bus */
#define	s_atn			0x0001
#define	s_bsy			0x0002
#define	s_ack			0x0004
#define	s_rst			0x0008
#define	s_msg			0x0010
#define	s_sel			0x0020
#define	s_c_d			0x0040
#define	s_req			0x0080
#define	s_i_o			0x0100

/* Bit masks for Bus Phases */
#define	s_busfree		(s_bsy | s_sel)
#define	s_dataout		(s_bsy)
#define	s_datain		(s_bsy | s_i_o)
#define	s_command		(s_bsy | s_c_d)
#define	s_status		(s_bsy | s_c_d | s_i_o)
#define	s_messout		(s_bsy | s_msg | s_c_d | s_atn)
#define	s_messin		(s_bsy | s_msg | s_c_d | s_i_o)
#define	s_sigmask		(s_atn | s_bsy | s_rst | s_msg | s_sel | s_c_d | s_i_o)

/* Scsi command Codes for all devices */
#define	s_testunitready	0x0000
#define	s_requestsense	0x0003
#define	s_inquiry		0x0012
#define	s_copy			0x0018
#define	s_receivediagnostic	0x001C
#define	s_senddiagnostic	0x001D

/* Scsi command Codes for Direct Access devices */
#define	s_rezerounit		0x0001
#define	s_formatunit		0x0004
#define	s_reassignblocks	0x0007
#define	s_read				0x0008
#define	s_write				0x000A
#define	s_seek				0x000B
#define s_searchemptyblocks 0x000C
#define	s_modeselect		0x0015
#define	s_reserve			0x0016
#define	s_release			0x0017
#define	s_modesense			0x001A
#define	s_startstop			0x001B
#define	s_preventallow		0x001E
#define	s_readcapacity		0x0025
#define	s_extendedread		0x0028
#define	s_extendedwrite		0x002A
#define	s_verifywrite		0x002E
#define	s_verify			0x002F
#define	s_readlong			0x00E8
#define	s_writelong			0x00EA

/* Scsi command Codes for Sequential Access devices */
#define	s_rewind		0x0001
#define	s_readblocklimits	0x0005
#define	s_trackselect	0x000B
#define	s_readreverse	0x000F
#define	s_writefilemarks	0x0010
#define	s_space			0x0011
#define	s_verifytape	0x0013
#define	s_recoverbuffer	0x0014
#define	s_erase			0x0019
#define	s_loadunload	0x001B

/* Scsi statuses */
#define	s_statmask		0x0F00					/* Status mask - masks off bits 8-11			*/

/* General plan for scsi status codes:									*/
/* "Modern" routines shall:												*/
/*		1)	Retrun a 0 if the command completed successfully - S$Good	*/
/*		2)	Return a negative status number if BusConnect fails; e.g.	*/
/*			see S$ArbFailed through S$BadInitiator						*/
/*		3)  Return a sense key in the lower half if the scsi status		*/
/*			code was check condition									*/
/*		4)	Return the scsi status in the upper half in other case.		*/

/* That is:																*/
/*		1)	return value == S$Good or S$GoodConnect ==> OK				*/
/*		2)  return value < 0 ==> connect failure						*/
/*		3)	return value >= 256 ==> S$Busy or S$Reserved in upper half	*/
/*		4)  return value <  256 ==> a Sense Code						*/

/* Scsi status Bytes */
#define	s_good				0x0000				/* Good status									*/
#define	s_checkcondition	0x0002				/* Check condition (should do a REQUEST SENSE)	*/
#define	s_conditionmet		0x0004				/* Condition met/good							*/
#define	s_busy				0x0008				/* Device busy									*/
#define	s_reserved			0x0018				/* Reservation Conflict							*/

/* Scsi extended Sense Keys */
#define	s_nosense			0x0000				/* Everything okay								*/
#define	s_recoverederror	0x0001				/* Recovered error								*/
#define	s_notready			0x0002				/* Device is not ready							*/
#define	s_mediumerror		0x0003				/* Medium error									*/
#define	s_hardwareerror		0x0004				/* Error with hardware							*/
#define	s_illegalrequest	0x0005				/* Illegal parameter in command					*/
#define	s_unitattention		0x0006				/* Unit attention								*/
#define	s_dataprotect		0x0007				/* Device cannot be written to					*/
#define	s_blankcheck		0x0008				/* Device encountered a blank block while reading	*/
#define	s_vendorunique		0x0009				/* Vendor unique sense code						*/
#define	s_copyaborted		0x000A				/* Copy command aborted							*/
#define	s_volumeoverflow	0x000D				/* Device has reached end of media				*/

/* Statuses returned by SCSICONNECT */
#define	s_goodconnect		0					/* Returned by SCSICONNECT if a good connection is made	*/
#define	s_arbfailed			-1					/* Returned by SCSICONNECT if arbitration failed	*/
#define	s_selfailed			-2					/* Returned by SCSICONNECT if selected device did not respond	*/
#define	s_identfailed		-3					/* Returned by SCSICONNECT if the Identify message is not sent	*/
#define	s_d24notthere		-4					/* Returned by SCSICONNECT if D24 board could not be selected	*/
#define	s_badbusstate		-5					/* Returned by SCSICONNECT if SCSI bus is a a weird state	*/
#define	s_badinitiator		-6					/* Returned by SCSICONNECT if Initiator passed is <> 6 or 7	*/
#define	s_devicebusy		shl(s_busy,     8)	/* Returned by SCSICONNECT if Busy status is returned	*/
#define	s_devicereserved	shl(s_reserved, 8)	/* Returned by SCSICONNECT if Reservation Conflict status is returned	*/

/* Scsi messages */
#define	s_cmdcomplete		0x0000				/* Command Complete message						*/
#define	s_savepointer		0x0002				/* Save Data Pointer message					*/
#define	s_restorepointer	0x0003				/* Restore Data Pointer message					*/
#define	s_disconnect		0x0004				/* Disconnect message							*/
#define	s_abortcmd			0x0006				/* Message sent to abort a command				*/
#define	s_linkcmd			0x000A				/* Linked command complete						*/
#define	s_resetdev			0x000C				/* Message sent to reset a particular device	*/

/* Dtd errors: -10 to -29 */
/* More SCSI errors: -30 to -39 */

/* Os errors: -40 to -59 */
#define	s_baddevice		-40						/* S$sensekey set to this if a bad or unconfigured device passed to SCSIIO	*/

/* Winchester disk errors: -60 to -79 */

/* Tape errors: -80 to -99 */
#define	s_notloaded		-80						/* tape drive not loaded						*/

/* Optical disk errors: -100 to -119 */
/* Floppy disk errors: -120 to -139 */
/* Reserved: -140 to -199 */
