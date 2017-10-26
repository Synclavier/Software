/*	NED Toolkit - Environment Library Interface
	
	Copyright © 1990-1991 by New England Digital Corp.  All rights reserved.
	
	01/16/90	DSY		Added a prototype (about_module()) and a literal ABOUT_MODULE
	09/14/89	ADT		Updated to NED Toolkit 1.0b1
	06/08/89	ADT		Major update
	05/15/89	ADT		Created
*/

#ifndef NED__ENVIRON
#define NED__ENVIRON

#ifndef rez	/*	C includes	*/

#ifndef NED__C
#include "c.h"
#endif

#endif	/* ifndef Rez */

#include "envconfig.h"

#define ENVR_ID				(ENVIRON << 8)

/* Configuration resources */

#define INHIBIT_MODULE_LIST		-3
#define STARTUP_MODULE_LIST		-2
#define ORDER_MODULE_LIST		-1

/* Menus */

#define MODULE_MENU_ID		235					/* ID of Modules submenu */
#define MODULE_MENU_MARK		"\0D235"		/* ID of Modules submenu in Rez string form */

/* Menu check characters */

#define FORWARD_CHAR		checkMark
#define ACTIVE_CHAR			'¥'
#define INACTIVE_CHAR		noMark

/* Alerts */

#define ABOUT_ENVIRON			ENVR_ID			/* ID of About box */
#define BAD_MAC_SYSTEM			ENVR_ID+1		/* ID of bad environment alert */
#define ABOUT_MODULE			ENVR_ID+2		/* ID of About Module */
#define LAUNCH_FILE_NOT_FOUND	ENVR_ID+3		/* ID of launch_prog failure alert (file not found) */
#define LAUNCH_NO_MEM		 	ENVR_ID+4		/* ID of launch_prog failure alert (not enough memory) */
#define SECURE_ALRT				ENVR_ID+5		/* ID of security-related rsrcs */
#define SYNCNET_ALRT			ENVR_ID+6		/* ID of SYNCnet alert */

#ifdef rez	/*	Rez definitions	*/

/* Module key resource type - NED CONFIDENTIAL */

type 'NDmk' {
	array {
		byte;
	};
	byte = 0;
};

#else		/*	C definitions	*/

/*	----- Literals -----	*/

#define ALL_MODULES			NULL		/* Passed to terminate for full quit */

#define UNTIL_CONVENIENT	-1			/* Special sleep times; passed to activate & set_sleep */
#define UNTIL_EVENT			-2
#define DEDICATE			-3

/* Toolbox events */

#define NED_EVENT			app1Evt		/* Protocol/Environment events */
#define NED_EVENT_MASK		app1Mask

/* NED event types (event.message value in parens) */

#define NULL_EVENT			0			/* no event */
#define RECEIVED_PACKET		1			/* packet received event (ptr to packet) */
#define SYNCLAVIER_PRESENT	2			/* Synclavier present (TRUE if it's back, FALSE if it's left) */
#define RX_PACKETS_LOST		3			/* Rx packets have been lost (number of packets lost) */
#define TX_PACKETS_LOST		4			/* Tx packets have been lost (number of packets lost) */
#define PROTOCOL_EVENT		5			/* protocol event message received (ptr to event SET) */
#define DRVR_OUT_OF_MEMORY	6			/* protocol driver cannot allocate memory for received packets (no data) */
#define SNET_DRVR_NOT_PRESENT	7			/* protocol driver not running */
#define RELOAD_MAC422		8			/* reload Mac422 board */

/*	----- Typedefs -----	*/

bool		protocol_lock			(bool lock);				/* lock on to protocol for snapshots */

#if __LP64__
#else
    typedef struct module_rec {				/* Module record (process block) */
        struct module_rec	*next;
        void				(*event_proc)(struct EventRecord *);
        bool				(*kill_proc)(void);
        const char		*name;
        int16			event_mask;
        int32			sleep;
        uint32			time;
        WindowPtr			front_window;
        Handle			menu_bar;
        int				menu_position;
        int				module_number;
        int16			res_file;
        bool				inhibited;
    } module_rec;
    typedef module_rec *module_ptr, *environ_id;

    typedef struct system_list_rec {			/* System module list record (currently unused) */
        struct system_list_rec	*next;
        module_ptr			module;
    } system_list_rec;
    typedef system_list_rec *system_list_ptr;

    /* Window record header for use in all window data records */

    #define winrec_header struct window_rec	*next;	\
            WindowPtr		window;					\
            module_ptr		module;					\
            bool			system_owned

    typedef struct window_rec {				/* Minimum window data record; used for system owned window data recs */
        winrec_header;
    } window_rec;

    typedef window_rec *winrec_ptr;

    /*	----- Global declarations -----	*/

    extern RgnHandle 		cursor_region;		/* Region cursor is valid over */
    extern RgnHandle 		arrow_region;		/* Region arrow is valid over (very large region) */

    extern winrec_ptr		winlist_head;
    extern module_ptr		active_list_head;
    extern system_list_ptr	system_list_head;
    extern module_ptr		inactive_list_head;
    extern module_ptr		menu_bar_owner;

    extern bool			quit_environment;
    extern bool			environment_startup;	/* Set when first starting up to prevent full startups */

    /*	----- Prototypes -----	*/

    #ifdef __cplusplus
    extern "C" {
    #endif

    /* environment
     *	these provide interfaces for separately linked NED modules to
     *	a common runtime environment
     *	this interface was used by (old)EditView, AutoConform, MIDInet,
     *	and to a lesser extent, Termulator
     */

    void 		initialize_mac			(int32 master_blocks);
    environ_id 	initiate				(void (*event_proc)(EventRecord *), bool (*kill_proc)(void), const Str255 name);
    void 		activate				(environ_id id, int16 event_mask, int32 sleep);
    void 		set_sleep				(environ_id id, int32 sleep);
    bool 		terminate				(environ_id id);
    void 		connect_window			(environ_id id, WindowPtr window, void *data_ptr);
    void 		*remove_window			(WindowPtr window);
    void 		window_forward			(WindowPtr window);
    void		connect_menu_bar		(environ_id id, Handle menu_bar);
    void 		set_window_menu			(int32 window_menu_id, int32 fixed_items);
    bool 		select_window_menu		(int32 item);
    void 		set_module_menu			(void);
    void 		select_module_menu		(int32 item);
    void 		start_pending_module	(void);
    void 		select_next_module		(void);
    Handle      module_menu				(void);
    bool 		start_module			(int32 index);
    void 		about_module			(void);

    #ifdef __cplusplus
    }
    #endif
#endif

/*	----- Macros -----	*/

#define get_wdata(window)			( (window)            ? ((void *) GetWRefCon(window))              : (NULL) )
#define get_module(window)			( (get_wdata(window)) ? (((winrec_ptr) get_wdata(window))->module) : (NULL) )
#define get_front_window(module)	( (module)            ? ((module)->front_window)                   : (NULL) )

/* This is part of the mechanism which disallows simultaneous module operation */
#define initiate(event_proc, kill_proc, name) (initiate)(event_proc, kill_proc, name); if (environment_startup) return

#endif

#endif /* NED__ENVIRON */