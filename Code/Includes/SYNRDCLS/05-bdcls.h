/* :Synrdcls:05-bdcls  $title  external declarations for the modules in 05-butt *//*Modified:1998/05/15 - TY  - Added FIND.FIRST.NONREST dcl.1991/12/15 - PF  - Added MIDISUST.MODE dcl1991/04/26 - cj  - Removed some more publics1991/03/14 - cj  - Removed some publics1991/02/11 - PF  - added DISPLAY.CLICK.RATE1990/11/06 - PF  - Add START.MIDI.SUSTAIN and STOP.MIDI.SUSTAIN1990/10/11 - CJ  - removed unneeded publics for knob, modwheel, etc.1990/08/27 - PF  - Support all MIDI controllers1990/06/17 - PF  - Changed argument to DISPLAY.BEAT.NUMBER from beat number						 to seqeunce time1989/03/09 - PF  - Increase MAX.INPUTS to 1281990/01/25 - MWH - Fix "300 msec delay starting cues" bug (from EditView)1989/07/19 - PF  - Added PATH argument to MIDI.NOTE.OFF and MIDI.NOTE.ON1989/07/28 - cj  - Removed Display.Enabled.Tracks1989/07/07 - TSS - Added dcls for Pressure Calibration stuff1989/07/07 - TSS - ADDED LAST.NOTE.KEY# & LAST.NOTE.VEL DCLS1989/06/15 - PF  - Sysex dcls for MIDInet - SYSEX.PACKET and SEND.SYSEX.PACKET1989/06/09 - PF  - Added MIDI.BUF dcl1989/06/08 - MWH - Process start/stop record protocol packets faster1989/04/20 - PF  - Added MIDI.NOTE.OFF dcl1989/03/14 - PF  - Added HELD.STOP.BUTTON,MIDI.SONG.PTR.INFO1989/01/13 - MWH - Add Auto Punch feature1988/09/30 - TSS - Added CLAVIER.EMULATION.KNOB.DELTA dcl1988/09/22 - TSS - Added RESET.KEYBOARD.TIMBRE.CONTROL dcl1988/06/15 - CJ  - changed name & arguments of smpte generator procedure1987/12/01 - cj  - added set.sequencer.mark.start1987/10/13 - EG  - ADDED INFO.BUTTON.PRESS1987/05/14 - TS  - ADDED SET.TRACK.SOLO.STATE 1987/04/16 - TS  - ADDED ARG TO TOGGLE.TRACK.SOLO.STATE DCL1986/11/19 - TS  - ADDED 2 DCLS TO DEBOUNCE INSERT/DELETE OPERATIONS1986/10/30 - TS  - ADDED MIDI.ECHO.DISABLED & HELD.INSERT.BUTTON & HELD.DELETE.BUTTON DCLS1986/09/15 - TS  - ADDED HELD.STARTLOOP.BUTTON & HELD.ENDLOOP.BUTTON DCL1986/07/15 - CJ  - CHANGED DECLARATIONS FOR MIDI.CONTROL.CHANGE, ETC1986/07/01 - TSS - ADDED DCL FOR STEP.RTE.PATCHING1986/06/25 - TSS - ADDED DCL FOR FIND.ANY.NOTE1986/05/16 - TIM - send MIDI Program Changes from Synclavier buttons1986/05/15 - "official" creation of release-M modules*//* From 071-newl */extern	void	lpn_lookup(fixed);extern	data	blinktable;						/* Used to look up blinking parameters - none for the moment	*//* From 072-newi */extern	array	new_note_stak;extern	array	new_note_key_;extern	array	new_note_vel;extern	array	new_note_time;extern	array	midi_rte_stak;extern	array	midi_rte_value;extern	array	midi_rte_msb;extern	array	midi_rte_lsb;extern	array	new_button_list;extern	array	ext_trig_stak;extern	fixed	num_held_keys;extern	array	dp_datas;extern	data	dp_table;extern	fixed	sw1_changes;					/* Set to 1 when switchdata states have changed	*/extern	fixed	sw2_changes;					/* Set to 1 when switchdata states have changed	*/extern	array	switchdata;						/* Individual switch state bits					*/extern	array	analog_in;						/* Analog controller values for synclavier		*/extern	fixed	last_note_type;extern	fixed	last_note_key_;extern	fixed	last_note_vel;extern	fixed	ttime_min;						/* Transit time required to achieve max volume	*/extern	fixed	newkey;							/* Set when new key press detected during interrupt	*/extern	fixed	oldkcr;extern	fixed	old_sar;extern	fixed	both_display;					/* True if both upper & lower used for same message	*/extern	void	compute_sync_ttime_map(fixed);	/* Computes a lookup table for transit time to volume	*/extern	void	scan_for_notes();				/* Create list of new notes on keyboard			*/extern	void	scan_new_buttons();				/* Create list of buttons changing state		*/extern	void	scan_analog_inputs();			/* Read analog inputs and digital switches		*/extern	void	clear_light(fixed);extern	void	set_light(fixed);extern	void	blink_light(fixed);extern	void	kbd_init();						/* Turn off all button panel lights				*/extern	void	set_decimal_point(fixed);		/* Updated for new display board - do not swap so track buttons are fast	*/extern	void	clear_decimal_point(fixed);		/* Do not swap - called often from emit.number	*/extern	void	emit_number(fixed, fixed, fixed, fixed);	/* Write value to alpha, starting at pos		*/extern	void	emit_large_number(fixed, fixed, fixed);	/* Emit number up to 9,999,999					*/extern	void	emit_string(fixed, char *);		/* Non-swap!!									*/  /* Write string to alpha, starting at pos - note - does not clear decimal point	*/extern	void	clear_display();extern	void	clear_upper();					/* Clear the upper line of display				*/extern	void	clear_lower();					/* Clear the lower line of display				*//* From 073-neww */extern	fixed	display_ebl;					/* Set to nonzero after sign on to allow all displays	*/extern	fixed	new_beat_number;				/* Set true when changed						*/extern	fixed	supress_beat_display;			/* Set to true if should suspend beat display	*/extern	fixed	beat_display_time;				/* Time to resume								*/extern	fixed	last_err;extern	fixed	held_rte_buttons;extern	fixed	notes_left_msb;					/* For display purposes only  -  msb			*/extern	fixed	notes_left_lsb;					/*                            -  Lsb			*/extern	fixed	remove_kbd_error;extern	void	display_signon(fixed);			/* Display sign-on message						*/extern	void	display_err(fixed);				/* Display err0 - err9, er10 - er19				*/extern	void	info_prompt();					/* Re-display info prompt						*/extern	void	display_frame_number(fixed);	/* Display a frame number in window				*/extern	void	remove_frame_number();			/* Remove frame number from display (upon partial button release)	*/extern	void	display_our_dev();				/* Put name of device to access in window		*/extern	void	remove_our_dev();extern	void	display_click_rate(fixed, fixed, fixed);extern	void	display_beat_number(array);		/* Special procedure for beat number			*/extern	void	display_timbre_name(fixed, fixed);	/* Put timbre name on upper or lower display - pass tinfo pointer, position	*/extern	void	display_timbre_recall_info(fixed, fixed, fixed);	/* Pass tinfo ptr, number of partials, number of frames	*/extern	void	display_numb_left();			/* Display # of notes left in sequencer			*/extern	void	display_track_start(fixed, fixed, fixed, fixed);	/* Display beat # of first note on track - pass format, play time, play time carries, click rate	*/extern	void	remove_track_start_display();	/* Clear track start display					*/extern	void	display_units(fixed);			/* Display units word							*/extern	void	display_par_num(fixed, fixed, fixed, fixed);	/* Display parameter number						*/extern	void	display_large_par_num(fixed, fixed, fixed, fixed);	/* Display (large) parameter number				*/extern	void	display_signed_par_num(fixed, fixed, fixed);	/* Display a signed short byte (stereo center, harmonic adjust)	*/extern	void	display_vibrato(fixed, fixed);	/* Display a vibrato wave type					*/extern	void	display_stereo(fixed, fixed);	/* Display a stereo mode						*/extern	void	kbd_error(fixed);				/* Present error on keyboard note				*/extern	void	display_more_notes(fixed);extern	void	disk_message(fixed);			/* For new panel - display # of disk sectors written	*/extern	void	display_smode(fixed);			/* Display smpte mode in lower					*/extern	void	display_ext_sync_mode(fixed);	/* Displays ext sync mode						*/extern	void	present_tpan_display();			/* Present track pan button display				*/extern	fixed	pressure_calibration_mode;extern	fixed	pressure_calibration_key;extern	fixed	pressure_calibration_value;extern	void	pressure_calibration_display(fixed, fixed);/* From 150-butt */extern	fixed	new_gdisplay;					/* Set true when button changes so guitar panel will be updated	*/extern	fixed	held_psel_buttons;				/* Bits for partial buttons that are held		*/extern	fixed	tb_enabled;						/* Set true if tone bend active					*/extern	fixed	num_of_held_track_buttons;		/* Number of currently held track buttons		*/extern	fixed	held_trinf_button;				/* Track info button held (routing, volume)		*/extern	fixed	held_insert_button;extern	fixed	held_delete_button;extern	fixed	trout_trk_;						/* Button # (0 to num.track.buttons-1) or (-1) for kbd	*/extern	fixed	hgs;							/* 0 = 1-12,  1 = 13-24,  2=25-36 (New panel only)	*/extern	fixed	scale_adjust_is_active;extern	fixed	present_rte_display;extern	fixed	rte_display_state;extern	fixed	track_first_time_msb;			/* Global outputs of find.first.note			*/extern	fixed	track_first_time_lsb;			/* Only valid if find.first.note				*/extern	fixed	track_first_sec;				/* Returns a 1. Sec & wrd point to				*/extern	fixed	track_first_wrd;				/* The first sounding note on the track			*/extern	fixed	track_first_keyn;extern	fixed	any_tracking;					/* Tracking bits for filters					*/extern	data	misc_toggle_bits;extern	array	active_parms;					/* Allow 20 active parameters					*/extern	fixed	in_blink_mode;extern	void	doctor_button_list(fixed);		/* convert phys button numbers to logical		*/extern	void	panel_scan();					/* scan button panel and map to logical parm numbers	*/extern	void	stack_on_panel(fixed);			/* store info on panel stack - used by upper routine to simulate certain switches	*/extern	void	off(fixed);						/* Set lpn to blink mode						*/extern	void	on(fixed);						/* Set lpn to on mode							*/extern	void	blink(fixed);					/* Set lpn to blink mode						*/extern	void	assign(fixed, fixed);			/* Assign off, on, blinking to lpn				*/extern	void	assign_multiple(fixed, fixed, fixed);	/* Assign same state to many buttons			*/extern	void	tbut_mapping(fixed);			/* Returns logical fixed # corresponding to button #	*/extern	void	push_tbut_solos();				/* Saves current trk solo state in little pdl	*/extern	void	pop_tbut_solos();				/* Restores current trk solo state from pdl		*/extern	void	display_partial_buttons();extern	void	display_tone_bend_button();		/* Set tone.bend light in scale.reset.button	*/extern	void	turn_off_params();				/* Call routine to turn off all parameter lites	*/extern	void	flicker_parameter_buttons();	/* Call to quickly light all parameter buttons	*/extern	void	turn_on_params();				/* Light those required							*/extern	void	display_parameter_buttons();	/* Set parameter buttons						*/extern	void	display_bounce_and_recd_just_buttons();	/* Set appropriate state of bounce button and justified recording button	*/extern	void	display_bmode();				/* Display correct bmode button (new panel only)	*/extern	void	display_timbre_recall_buttons();	/* Set correct timbre recall buttons (changes with skt/smt)	*/extern	void	display_bank_buttons();extern	void	display_sequencer_recall_buttons();extern	void	find_first_held_track_button();extern	void	display_sequencer_status();		/* Sets up correct sequencer display lights		*/extern	void	display_active_tracks();		/* An info mode function to show which trks have notes	*/extern	void	display_track_buttons();		/* Set up correct track buttons - do not swap this so track buttons are fast	*/extern	void	display_smt_skt_buttons();		/* Display correct smt, skt buttons				*/extern	void	display_hgs();					/* Display appropriate harmonic group select	*/extern	void	compute_rte_buttons();			/* Compute propper rte display from internal variables	*/extern	fixed	find_any_note(fixed);			/* Find time & ptrs for first actual note record on specified track	*/extern	fixed	find_first_nonrest(fixed);		/* Find time & ptrs for first non-rest note on specified track	*/extern	fixed	find_first_note(fixed);			/* Find time, ptrs & key# for first sounding note on specified track	*/extern	void	compute_start_time_display(fixed);	/* Compute display of starting time of track - given abs. Track# (0-255)	*/extern	void	compute_toggle_display();		/* Call to set up toggle buttons on panel (port,vibrato,filters,repeat/arpeg)	*/extern	void	compute_dsel_button();extern	void	panel_init();					/* initialize panel and set up PBN.LOOKUP table	*//* From 155-scn1 */extern	fixed	toggle;							/* Flag to scan buttons every other time to reduce bouncing & compute load	*//* Variables used for both keyboards     */extern	fixed	raw_knob_base;					/* Neutral position of knob						*/extern	fixed	knob_delta;						/* Current position of knob from base			*/extern	fixed	clavier_emulation_knob_delta;	/* Pick up potential setting					*/extern	fixed	ribbon_release;					/* Set true for ribbon release					*/extern	fixed	ribbon_amount;					/* Amount of deflection before release			*/extern	fixed	new_gvol, new_grte;				/* Set true if change in guitar input knob		*/extern	array	rte_changes;					/* Bits are set for changing rte's which need to be recorded	*/extern	array	rte_channels;					/* Bits are set for channels with changing rte's for recording	*/extern	fixed	new_info;						/* Set true on change of any rte data			*/extern	fixed	new_toggles;					/* On change of foot switch/toggles				*/extern	array	switchbits;						/* Packed bits of foot switch bits				*/extern	fixed	amount;							/* Amount to change knob parameter (see 'update')	*/extern	fixed	new_pm;							/* Set true if new pitch/mod wheel				*/extern	fixed	midi_echo_disabled;				/* 0 To disallow echo of midi notes, 1 to allow	*/extern	void	restore_power();				/* Re-measure input items on power restore		*/extern	void	scandata();						/* Scans all input devices						*//* From 157-scn2 */extern	void	panel_rescan();					/* Procedure to always rescan					*//* From 171-prf1 */extern	fixed	reset_info;						/* Holds button # of button pressed twice (for resetting variables)	*/extern	fixed	split_keyboard;					/* True if split active							*/extern	fixed	split_kbd_loc;					/* Defines location of keyboard split			*/extern	fixed	track_state;					/* Indicates what to do if track button pressed	*/extern	fixed	track_one;						/* Indicates source track for bounce			*/extern	fixed	partial_state;					/* Used for bouncing partials					*/extern	fixed	partial_one;					/* Indicates first partial						*/extern	fixed	tb_pos, vb_pos;					/* Tone bend/vibrato depth from master knob		*/extern	fixed	num_held_buttons;				/* Counts held button for algorithm				*/extern	fixed	preset_kbd_envelope;extern	fixed	new_frame_coeff;extern	fixed	kbd_val;extern	fixed	new_kbd_env;extern	fixed	poly_timbre_midi_control;extern	fixed	keyboard_timbre_control;extern	fixed	held_kbd_env_buttons;extern	fixed	held_scale_adjust_buttons;extern	fixed	held_tname_button;extern	fixed	held_vibr_button;extern	fixed	held_smpte_button;extern	fixed	held_ext_button;extern	fixed	held_pvol_button;extern	fixed	held_ptun_button;extern	fixed	held_clik_button;extern	fixed	held_speed_button;extern	fixed	held_rate_button;extern	fixed	held_mark_button;extern	fixed	held_startloop_button;extern	fixed	held_endloop_button;extern	fixed	held_midi_button;extern	void	abort_bounce_smt_skt();			/* Stop these functions midstream if required	*/extern	void	clear_eras_button();			/* Dim erase light and clear erase display		*/extern	void	skt_splt_sub(fixed);			/* Process skt/splt								*/extern	void	step_rte_patching(fixed, fixed);	/* Changes rte patching on kbd timbre			*/extern	void	clear_parameters();				/* Procedure to erase parameters - do not swap so track buttons are fast	*/extern	void	toggle_blink();					/* Procedure to toggle blinking mode			*/extern	void	display_length_loop();			/* Call if poly patch list partial is selected	*/extern	void	partial_button(fixed, fixed);	/* Process partial select button				*/extern	void	select_partial(fixed);			/* Active a selected partial only - called from tds	*/extern	void	toggle_smpte();					/* Toggle smpte mode							*/extern	void	parameter_button(fixed, fixed);	/* Process parameter button						*/extern	void	select_timbre_frame(fixed, fixed);	/* activates an active timbre frame				*/     extern	void	select_parameter(fixed);		/* Activate a parameter selection from tds		*/extern	void	smpte_generator(fixed);			/* Smpte generator via software					*/extern	void	info_button_press();extern	void	reset_keyboard_timbre_control(fixed);	/* Restores normal keyboard timbre mode			*//* From 172-prf2 */extern	fixed	last_ff_rewind_press;			/* Abs time when ff/rewind button was last pressed	*/extern	fixed	held_stop_button;				/* Set to 1 if stop         button pressed		*/extern	fixed	held_rewind_button;				/* Set to 1 if rewind       button pressed		*/extern	fixed	held_ff_button;					/* Set to 1 if fast forward button pressed		*/extern	fixed	held_scale_adjust_button;extern	fixed	insert_delete_locked;			/* Set to 1 to lock out subsequent insert/deletes	*/extern	fixed	insert_delete_unlock_time;		/* Time at which to turn off lockout			*/extern	void	compute_nearest_beat_time(array);	/* Rounds play.time.msb & play.time.lsb to nearest beat time	*/extern	void	take_pre_roll_time();			/* Snapshot the current sequencer time			*/extern	void	take_record_time(fixed);		/* Snapshot the current sequencer time			*/extern	void	start_sequencer();				/* Start sequencer playing						*/extern	void	stop_sequencer();extern	void	clear_loop_parms();extern	void	set_loop_start_time(fixed, fixed);	/* Set loop.start to passed time & select overall loop start parm	*/extern	void	continue_sequencer();extern	void	continue_seq_with_pre_roll(fixed);extern	void	fast_forward_sequencer();extern	void	rewind_sequencer();extern	void	set_sequencer_mark_start_point(fixed, fixed, fixed);	/* Msw, lsw, code (0=no locate. 1 = Locate if play = 0.  2 = Stop play and locate	*/extern	void	control_sequencer();			/* Processes sequencer.control.reg				*/extern	fixed	safe_ready_toggles;/* From 173-prf3 */extern	fixed	held_kcv_button;				/* Used for setting kcv							*/extern	fixed	kcv_track;						/* For kcv output								*/extern	fixed	display_track_info;				/* Time to do fixed display						*/extern	fixed	display_track__;				/* Abs. Fixed # to display						*/extern	fixed	info_mode_update_time;			/* Used to time track button info mode display refreshes	*/extern	fixed	mipgm_on_skt_control;			/* On/off flag for midi skt feature				*/extern	fixed	flicker_params;					/* Set true to start flicker					*/extern	fixed	perform_flicker;				/* Set true below to perform parameter flickering	*/extern	fixed	bank_write, entry_write;extern	fixed	kbd_special;					/* Set true if special check in keyboard scan is required (done this way for speed)	*/extern	void	process_record_button();extern	void	process_punch_button();extern	void	process_erase_button();extern	void	sequencer_button(fixed, fixed);	/* Routine to process sequencer buttons			*/extern	void	clear_track_solo_states();		/* Clear all solos								*/extern	void	set_track_solo_state(fixed, fixed, fixed);	/* Set solo state of track range				*/extern	void	toggle_track_solo_state(fixed);	/* Toggle solo state of track or group			*/extern	void	track_button(fixed, fixed);		/* Process press/release of track button		*/extern	void	rte_button(fixed, fixed);		/* Press of rte button							*/extern	void	bank_button(fixed, fixed);		/* Process bank button press					*/extern	void	timbre_button(fixed, fixed);extern	void	sequencer_recall_button(fixed, fixed);extern	void	misc_button(fixed, fixed);		/* Miscellaneous butons							*/extern	void	new_button();extern	void	perform();						/* Perform a panel-directed command				*//* From 174-prf4 */extern	fixed	num_enabled_trks;extern	fixed	midisust_mode;/* From 181-upd1 */extern	void	display_updated_parameter(fixed, fixed, fixed, fixed);	/* Update the display - pass parameter index number,  the new value	*/extern	void	add_to_smpte(fixed);extern	void	add_to_tempo(fixed);extern	void	add_to_length(fixed);			/* Pass pointer off of ptptr					*/extern	void	slide_tracks(fixed);			/* Procedure to move track time					*/extern	void	frame_slide();/* From 182-upd2 */extern	fixed	xxxxch;							/* Channel # for xxxx program					*/extern	void	major_update();extern	void	store_syncl_param(fixed, fixed, fixed, fixed);	/* Pass partial # (0,1,2,3),  frame # (0-x), lpn# (!!), value	*/extern	fixed	tds_value;						/* Value     from tds for store					*//* From 142-midi */extern	fixed	midi_overrun;					/* 1 If detect an overrun on input, else 0		*/extern	array	last_midi_status;				/* Value of last midi status sent on each output	*/extern	array	midi_buf;						/* Buffer for midi events to network			*/extern	array	midi_crout;						/* Midi to synclavier routing array for continuous controllers	*/extern	array	midi_srout;						/* Midi to synclavier routing array for switches	*/extern	fixed	midi_sync_out;					/* Mode of operation for midi sync output (0->don't send bytes, 1-32 send to output #1-32)	*/extern	fixed	midi_sync_is_tc;				/* True if midi sync output format is midi tc	*/extern	array	midi_tc_val;					/* Hrs, min, sec, fra, bit time code value		*/extern	fixed	midi_tc_num;					/* Holds ratio of milliseconds per quarter-frame	*/extern	fixed	midi_tc_den;extern	fixed	midi_tc_next_code;				/* Counts quarter frame message type			*/extern	fixed	display_mtc;					/* Set when should show mtc in lower window whilst playing	*/extern	fixed	new_mtc_val;					/* Set when new mtc value available for display	*/extern	fixed	midi_input_chan;				/* Input chan to listen to.						*//* Declarations for midi continuous controllers and switches */extern	array	midi_pressure;					/* Midi channel pressure						*/extern	fixed	midi_key_press;					/* Midi individual pressure						*/extern	fixed	midi_analog_in;					/* Midi continuous controller values			*/extern	fixed	midi_switchdata;				/* Midi switch values							*/extern	array	midi_analog_changes;			/* Bits for continuous controllers which have changed	*/extern	array	midi_analog_channels;			/* Bits for input channels with changing contiuous controllers	*/extern	array	midi_switch_changes;			/* Bits for switches which have changed			*/extern	array	midi_switch_channels;			/* Bits for input channels with changing switches	*/extern	fixed	missed_midi_clocks;				/* Counts unprocessed midi timing clocks		*/extern	fixed	timing_clock_enable;			/* 1 Enables accumulation of timing clocks received	*/extern	array	new_program_stak;				/* List for new program changes of size "list.size"	*/extern	array	midi_mapping;					/* Midi channels to track routing				*/extern	fixed	sysex_buf;						/* Pointer to 16 sector circular buffer in xmem	*/extern	fixed	sysex_write_ptr;				/* Next location to write to					*/extern	fixed	sysex_read_ptr;					/* Next location to read from					*/extern	fixed	sysex_packet;					/* Pointer to packet reserved for sysex output	*/extern	fixed	midi_track_rtes;				/* Pointer to xmem storage for 128 midi controllers for all tracks	*//* $Subtitle  routines to set up and clear controller routing info */extern	fixed	_mout, _msta;					/* Temps used below for all procs except midi.note.on	*/extern	fixed	midi_seq_status;				/* Current state of midi sequencers (0=stopped, 1=playing)	*/extern	fixed	midi_song_ptr_info;extern	void	clear_midi_out_controllers(fixed);	/* Removes midi controller routings				*/extern	void	set_midi_out_controllers(fixed);	/* Sets up routings for dx7						*/extern	void	init_midi_routings();extern	void	init_midi_rte_area();extern	void	compute_midi_velmaps();			/* Computes new velocity to volume table		*/extern	void	init_midi_fifo(fixed, fixed);	/* Init the desired fifo						*/extern	void	init_midi_subsystems();			/* Init routine for midi subsystems				*/extern	void	cleanup_midi_pressure(fixed);	/* Turns off pressure in current mode			*/extern	void	all_midi_notes_off();			/* Turns off all notes on all subsystems and midi channels	*/extern	void	scan_midi();					/* Processes midi bytes							*/extern	void	midi_control_change(fixed, fixed);	/* Sends midi control change					*/extern	void	real_midi_control_change(fixed, fixed);	/* Sends midi control change					*/extern	void	midi_pitch_wheel(fixed);		/* Sends midi pitch wheel update				*/extern	void	midi_channel_pressure(fixed);	/* Sends midi channel pressure update			*/extern	void	midi_individual_pressure(fixed, fixed);	/* Sends midi individ. Pressure update			*/extern	void	midi_note_off(fixed, fixed, fixed, fixed);	/* Sends midi note off event					*/extern	void	midi_note_on(fixed, fixed, fixed, fixed);	/* Sends midi note on event						*/extern	void	send_midi_start();extern	void	send_midi_continue();extern	void	send_midi_stop();extern	void	midi_song_pos_ptr(fixed, fixed);	/* Tells midi devices to go to desired beat		*/extern	data	midi_full_mode_code;			/* maps sm.mode encoding to bit					*/extern	void	midi_full_frame(fixed, fixed);	/* creates midi full frame message				*/extern	void	broadcast_mtc();				/* Broadcast cur position over midi				*/extern	void	prep_for_mtc_output(fixed, fixed);	/* set up real time variables					*/extern	void	midi_program_change(fixed);		/* Tells midi devices to goto new timbre		*/extern	void	check_midi_effects();			/* Check midi effects for update				*/extern	void	check_midi_rtes();				/* Check midi rtes for update					*/extern	void	check_all_midi_tracks();		/* Check all tracks on startupt					*/extern	void	check_all_midi_rtes();			/* Check all midi rte tracks on startup			*/extern	void	start_midi_sustain();			/* Check all tracks for current sustain state at start play	*/extern	void	stop_midi_sustain();			/* Check all tracks for current sustain state at stop play	*/extern	void	send_sysex_packet();			/* Sends a packet of sysex to midinet			*/extern	void	send_sysex(fixed);				/* Sends a byte of system exclusive data		*/extern	void	locate_midi_slaves();			/* Locate midi slave devices					*//* From 400-misc */extern	void	do_rte_display();				/* Do rte display in several loops for speed	*/extern	void	handle_misc_new_pan_funcs();	/* Does a few miscellaneous functions for new rte compilations	*/extern	void	is_button_soloed(fixed);		/* True if track connected to button is soloed	*/