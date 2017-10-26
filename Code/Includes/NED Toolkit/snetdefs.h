// Syncnet opcode definitions

/* SyncNet driver operations are split into
 * a major (interface type) and minor (operation) code
 * not all operations are meaningful
 * be careful about conflicts with the OS calls
 */

#ifndef	NED_SNETDEFS
#define NED_SNETDEFS 
 
/* system-defined control codes */

#define	KILLIO		1			/* stop activity 	*/
#define ACCRUN		65			/* periodic service */
#define	GOODBYE		-1			/* clear system 	*/

/* these codes are from SysEqu.a. they don't appear to be examined by the OS */
#define	STSCMD		5			/* status op code 	*/
#define	CTLCMD		6			/* control op code 	*/

/* SyncNetª Control code components */

/* csCode - major code */
#define	SN_CONN		48			/* connection scheduler 	*/
#define	SN_LSN		49			/* event listener 			*/
#define	SN_REQ		50			/* request/reply 			*/
#define	SN_SYNC		51			/* Synclavier status 		*/
#define	SN_RUPT		52			/* external packet handlers */
#define SN_POOL		53			/* common pool 				*/

/* minor code */
#define	SN_OPEN		1			/* open 					*/
#define	SN_CLOSE	2			/* close 					*/
#define	SN_POLL		3			/* poll status 				*/
#define	SN_PUT		4			/* enqueue/enable			*/
#define	SN_GET		5			/* dequeue/latch			*/
#define	SN_SET		6			/* set/reset 				*/
#define	SN_POOP		7			/* re-open   				*/

#define	SN_VERSION	'32a7'		/* code version number 		*/

/* structs */
typedef struct							/* struct for snet trace messages				*/
{
	char * fmt;
	long n1;
	long n2;

}	snet_trcpkt;

#pragma pack(push,2)

typedef struct							/* struct for snet link messages				*/
{
	int16 slot;
	int16 recvs[2];
	int16 sends[2];
	int16 node [2];

} snet_linkpkt;

#pragma pack(pop)

#endif
