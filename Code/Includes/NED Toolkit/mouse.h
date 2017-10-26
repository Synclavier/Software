/*	NED Toolkit - Macintosh Mouse Overriding
	
	Copyright � 1990 by New England Digital Corporation
*/

#ifndef NED__MOUSE
#define NED__MOUSE

void 	SampleMouseDelta (Point *delta);		/* pick up mouse movement */
void	InstallMouseTask (void);				/* install VBL task */
void	InstallMouseTask (CGFloat scale);		/* install VBL task */
void	RemoveMouseTask (void);					/* remove VBL task */
void 	haul_cursor (Point dest, bool local);	/* force mouse cursor to screen location */

extern	long mouse_task_installed;				/* true if installed */
extern	Point mouse_task_point;					/* current working mouse position in global coordinates */

#endif	/* NED__MOUSE */
