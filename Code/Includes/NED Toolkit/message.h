/*	NED Toolkit - Definitions of SYNCnet protocol messages.
	
	Copyright © 1990-92 by New England Digital Corp.  All rights reserved.
	
	This file defines all of the SYNCnet messages and interface
	routines to create and send messages to the audio engine and
	parse message from the audio engine.
	
	Modification History:
		09/07/88	EEG	Created this file
*/
/* DAWN Protocol Packet Definitions */

#ifndef NED__MESSAGE
#define NED__MESSAGE

#include "c.h"
#include "packetstructs.h"
#include "nedset.h"
#include "messagetype.h"				/* Specific Packet Type Codes */
#include "mixermessage.h"				/* Mixer Type Codes */


/* Data structures and types */

#define seq_event_template 			/* sequencer event structure template */		\
	int16 track;					/* sequencer track */							\
	int32 time;						/* sequencer time */							\
	int16 count;					/* event count */								\
	int16 type;						/* event type (see above) */					\
	int16 raw [4];					/* internal note record */						\
	int16 key;						/* key number */								\
	int16 cue_id;					/* cue ID */									\
	int32 duration;					/* event duration */							\
	int16 priority;					/* layering priority */							\
	int16 output;					/* output routing */							\
	int16 volume;					/* event volume (0-1000) */						\
	int16 pan;						/* event pan (-50-50) */						\
	int32 in;						/* relative in-time (samples) */				\
	int32 out;						/* relative out-time (samples) */				\
	int32 mark;						/* mark offset (samples) */						\
	int16 fade_in;					/* fade in length */							\
	int16 fade_out;					/* fade out length */							\
	int16 control_bits;				/* control bits (bit 0: mute) */				\
	int16 start_vol;				/* start volume (fade in to this - 0-1000) */	\
	int16 end_vol;					/* end volume (fade out from this - 0-1000) */	\
	int16 reserved [3]				/* reserved */

#pragma pack(push,2)

typedef struct {					/* sequencer event */
	seq_event_template;
} SEQ_EVENT;

#pragma pack(pop)

/* Protocol message manipulation routines */

#ifdef __cplusplus
extern "C" {
#endif

/* to be supplied by the packet-level interface */
extern	void (*new_msg)       (int category, int msg_id);				/* start new message with standard buffering */
extern	void (*new_msg_block) (int category, int msg_id, int length);	/* start new message with large buffering	 */
extern	int	 (*post_msg)      (PACKET *sp);								/* send/enqueue message						 */

/* used by the above */
void start_msg (PACKET *mbuf, int category, int msg_id, int max_length);	/* with supplied buffering */


int	send_msg (uint16 dest_node);			/* returns post_msg status */
int16 get_msg_tag (void);					/* get tag used by last m_req routine */

/* general routines */

CUR_TIME	current_time (void);			/* pick up current sequencer and disk times */
bool		synclavier_present (void);		/* TRUE if Synclavier is present */
int8		get_node_id (void);				/* pick up our node ID */

int16	get_next_tag (void);				/* generate next message tag */
int16	get_snap_tag (void);				/* generate next snapshot tag */
void		ack_snap_tag (int16 tag);		/* acknowledge snapshot */

/* message assembly */
void send_byte (int data);					/* add a byte to the message block */
void send_bytes (int length, ...);			/* variable argument version */
void send_int16 (int data);					/* add an int16 to the message block */
void send_int16s (int length, ...);			/* variable argument version */
void send_int32 (int32 data);				/* add an int32 to the message block */
void send_int32s (int length, ...);			/* variable argument version */
void send_struct (void *data, int length);	/* add a structure to the message block */
void send_string (char *s);					/* add a C string to the message block */
void send_bits (int32 bits[], int length);	/* length is in bits */

void  get_msg (PACKET *packet);
int8  get_byte (void);						/* returns next int8 */
int16 get_int16 (void);						/* returns next int16 */
int32 get_int32 (void);						/* returns next int32 */
void *get_struct (int length);				/* returns pointer to structure */
char *get_string (void);					/* returns pointer to C string */
void  get_bits (int32 bits[], int length);	/* length is in bits */


/******************************** Network Level Messages      **********************************/
int m_here_is (uint16 node, uint16 node_id);


/******************************** Playback Messages           **********************************/
int m_play (uint16 node);
int m_stop (uint16 node);
int m_rewind (uint16 node);
int m_fast_forward (uint16 node);
int m_locate (uint16 node, Locate_Mode mode, Locate_Type what, int32 where, ...);
int m_req_motion_status (uint16 node);

int m_play_cue (uint16 node);
int m_play_edit (uint16 node);
int m_play_region (uint16 node, int32 start, int32 stop, Playback_Source source);
int m_play_from (uint16 node, int32 time, Playback_Source source);
int m_play_to (uint16 node, int32 time, Playback_Source source);
int m_pause_continue_cue (uint16 node);
int m_stop_dtd (uint16 node);

int m_start_seq_scrubbing (uint16 node, int32 start, int32 stop, SET *track_bits);
int m_scrub_to_new_seq_time (uint16 node, int32 sample);
int m_stop_seq_scrubbing (uint16 node);
int m_scrub_current_cue (uint16 node, int16 motion, int8 resolution);
int m_scrub_current_cue_to_time (uint16 node, int32 time);

int m_play_event (uint16 node, int16 seq_track, int32 time, int8 count);

int m_req_audition_mode (uint16 node);
int m_set_audition_mode (uint16 node, bool mode);

int m_audition_ds (uint16 node, int32 dir_id, int32 ds_id, Audio_Source audio_source);


/******************************** Creation/Recording Messages **********************************/
int m_start_record (uint16 node);
int m_start_punch (uint16 node);
int m_stop_record (uint16 node);

int m_req_auto_punch_info (uint16 node);
int m_set_auto_punch_mode (uint16 node, bool mode);
int m_set_rehearse_mode (uint16 node, bool mode);
int m_set_auto_punch_time (uint16 node, int32 time, int8 what);

int m_req_recording_time (uint16 node, SET *track_bits);
int m_req_dtd_record_state (uint16 node);
int m_set_dtd_record_ready (uint16 node, bool mode);
int m_arm_manual_allocate (uint16 node, int32 length);
int m_arm_seq_trig_allocate (uint16 node, int32 seq_time, int32 length, int8 mode);
int m_arm_punch_in (uint16 node, int32 seq_time, int32 length, int8 mode);
int m_set_ext_rec_triggers (uint16 node, int32 preroll, int32 rec_in, int32 rec_out);	/* set external record triggers */
int m_erase_current_cue (uint16 node);
int m_block_cue (uint16 node);


/******************************** Sequence Editing Messages   **********************************/
int m_save_sequence (uint16 node);
int m_restore_sequence (uint16 node);
int m_erase_sequence (uint16 node);

int m_req_seq_data (uint16 node, Sequence_Area area, int32 first, int32 count);
int m_init_sequence_data(uint16 node, Sequence_Area area);
int m_add_to_sequence_data(uint16 node, Sequence_Area area, int32 record, int32 length, void *data);
int m_activate_data_area(uint16 node, Sequence_Area area, int16 data);

int m_req_seq_snapshot (uint16 node, Event_Snapshot_Bits info_bits, int32 start, int32 stop, SET *track_bits);
int m_req_seq_event (uint16 node, int16 track, int32 time, int16 count);
int m_req_default_event (uint16 node, Event_Type type);

int m_place_seq_event (uint16 node, SEQ_EVENT *event);
int m_change_event_item (uint16 node, int16 track, int32 time, int16 count, int32 data, Event_Field item);
int m_req_event_string (uint16 node, int16 track, int32 time, int16 count, Event_String_Type which);
int m_set_event_string (uint16 node, int16 track, int32 time, int16 count, Event_String_Type which, char *str);
int m_delete_event (uint16 node, int16 track, int32 time, int16 count);
int m_recall_event_for_edit (uint16 node, int16 track, int32 time, int16 count); /* make this event the current cue */
int m_req_changed_tracks(uint16 node);	/* get tracks changed since last request */

int m_delete_time_on_track(uint16 node, int16 track, int32 time, int32 length);
int m_insert_time_on_track(uint16 node, int16 track, int32 time, int32 length);

int m_place_cue (uint16 node);

int m_req_able_seq_path (uint16 node, uint16 which);
int m_set_able_seq_path (uint16 node, uint16 which, char *string);
int m_recall_path_sequence (uint16 node, uint16 which_path);
int m_store_path_sequence  (uint16 node, uint16 which_path);
int m_scroll_path_sequence (uint16 node, uint16 scroll_direction);

/******************************** Sound Editing Messages      **********************************/
int m_req_cue_data (uint16 node, int8 reel);
int m_set_cue_time (uint16 node, int32 time, Cue_Time_Select what);
int m_set_cue_tracks (uint16 node, SET *track_bits, int8 state);
int m_set_place_cue_track (uint16 node, int16 track);
int m_set_place_cue_mode (uint16 node, int8 mode);
int m_req_cue_string (uint16 node, int8 reel, int16 what);
int m_set_cue_string (uint16 node, int8 reel, int16 what, char *string);
int m_req_cue_info_by_id (uint16 node, int32 id);
int m_req_cue_string_by_id (uint16 node, int32 id, int16 what);

int m_copy_cue_mem (uint16 node, int8 src, int8 dst, int8 method);
int m_cut_from_cue (uint16 node, int8 dst);
int m_paste_into_cue (uint16 node, int8 src, int8 loop);
int m_chain_cue (uint16 node, int8 src);
int m_fill_cue (uint16 node, int8 src, int8 loop);
int m_slide_cue (uint16 node, int32 slide_amount, int8 method, ...);
int m_select_reel_to_edit (uint16 node, int8 reel);


/******************************** Archival Messages           **********************************/
int m_store_cue (uint16 node);


/******************************** Retrieval Messages          **********************************/
int m_req_dir_key_by_name (uint16 node, char *name, int8 match, int8 sort, int8 search, DS_Type_Filter filter, int8 recall);
int m_req_dir_key_by_index (uint16 node, int32 index, int8 sort, int8 search, DS_Type_Filter filter, int8 recall);
int m_req_dir_key_by_id (uint16 node, int32 id, int8 sort, int8 search, DS_Type_Filter filter, int8 recall);
int m_req_directory_info (uint16 node, int32 id, char *name, int16 match, DS_Type_Filter filter);
int m_req_directory_entries (uint16 node, int32 id, int32 start_entry, int32 entries, int16 max_len, char *name, int16 match, DS_Type_Filter filter);
int m_open_ds (uint16 node, int32 dir_id, int32 ds_id, DS_Access accesses);
int m_close_ds (uint16 node, int16 id);
int m_req_ds_read (uint16 node, int16 id, int32 offset, int32 length);
int m_req_ds_write (uint16 node, int16 id, int32 offset, int32 length, int8 data[]);
int m_copy_ds (uint16 node, int32 src_dir_id, int32 src_ds_id, int32 dest_dir_id, char *dest_name);
int m_activate_ds (uint16 node, int32 dir_id, int32 ds_id);


/******************************** Time Information Messages   **********************************/
int m_req_time_parameters (uint16 node);
int m_set_time_parameter (uint16 node, int32 data, int8 what);
int m_req_trim_time (uint16 node, int8 format);					/* unimplemented */
int m_set_trim_time (uint16 node, int32 data, int8 format); 			/* unimplemented */
int m_req_sync_mode (uint16 node);
int m_set_sync_mode (uint16 node, int8 mode, ...);
int m_set_midi_sync_out (uint16 node, int8 channel);
int m_set_midinet_sync_out (uint16 node, int8 path);
int m_take_current_time (uint16 node, int8 what);
int m_req_current_mark (uint16 node);
int m_set_current_mark (uint16 node, int32 time);
int m_req_cur_smpte_time (uint16 node, int16 options);
int m_req_valid_saved_marks (uint16 node);
int m_req_saved_mark (uint16 node, int16 mark);
int m_set_saved_mark (uint16 node, int16 mark, int32 time, char *name, char *caption);


/******************************** Track Setup Messages        **********************************/
int m_req_track_states (uint16 node, Track_Attribute attr);
int m_set_track_state (uint16 node, int16 track, int8 track_type, Track_Attribute attr, int8 state);
int m_set_mult_track_states (uint16 node, SET *seq_track_bits, SET *dtd_track_bits, Track_Attribute attr, int8 state);
int m_clear_all_tracks (uint16 node, Track_Attribute attr);
int m_req_track_state (uint16 node, int16 track, int8 track_type);
int m_req_group (uint16 node, int8 group);
int m_set_group (uint16 node, SET *seq_track_bits, SET *dtd_track_bits, int8 group);
int m_req_valid_groups (uint16 node);
int m_set_group_state (uint16 node, int8 group, Track_Attribute attr, int8 state);
int m_req_routing_info (uint16 node, SET *track_bits, int16 rout_type);
int m_req_midi_in_out (uint16 node, int16 seq_track, int8 direction);
int m_set_midi_in_out (uint16 node, int16 seq_track, int8 direction, int8 midi_num, int8 midi_chan);
int m_req_multichan_out (uint16 node, int16 seq_track);
int m_set_multichan_out (uint16 node, int16 seq_track, int8 left_out, int8 right_out);
int m_req_dtd_inputs (uint16 node, int16 dtd_track);
int m_set_dtd_inputs (uint16 node, int16 dtd_track, int8 input, int8 input_num);
int m_req_dtd_outputs (uint16 node, int16 dtd_track);
int m_set_dtd_outputs (uint16 node, int16 dtd_track, int8 left_out, int8 right_out);
int m_req_valid_tracks (uint16 node);


/******************************** Utility Messages            **********************************/
int m_req_configuration (uint16 node);
int m_req_echo (uint16 node);

int m_req_error_text (uint16 node, uint16 where);

/******************************** Event Messages              **********************************/
int m_set_event_enables 	   (uint16 node, SET *enables);

/******************************** MIDI Messages               **********************************/
/* undefined as of 8/15/89 */


/**********************  RS-422 Clavier and Clavier Emulation Messages  ************************/
int m_emulate_press_and_hold (uint16 node, int8 bank, int8 button);
int m_emulate_press_and_rel (uint16 node, int8 bank, int8 button);
int m_emulate_release (uint16 node, int8 bank, int8 button);
int m_emulate_release_all (uint16 node);
int m_emulate_clavier_knob_jog (uint16 node, int16 jog_amount);
int m_emulate_clavier_knob (uint16 node, int16 position);
int m_req_clavier_status_info (uint16 node);

/******************************** Mixer Messages              **********************************/
int m_set_meter_control (uint16 node, uint32 inmap, uint32 outmap, int32 interval);	/* set metering channels & rate */

int m_req_mixer_configuration (uint16 node);										/* get mixer configuration */
int m_req_mixer_dials (uint16 node, int32 count, Mixer_Dial_Set dials[]);			/* get mixer dials */
int m_set_mixer_dials (uint16 node, int32 count, Mixer_Dial_Set dials[]);			/* set mixer dials */
int m_req_mixer_switches (uint16 node, int32 count, Mixer_Switch_Set swtchs[]);	/* get mixer switches */
int m_set_mixer_switches (uint16 node, int32 count, Mixer_Switch_Set swtchs[]);	/* set mixer switches */

/******************************** Message Extensions           **********************************/
int m_set_raw (uint16 node, int category, int msg_id, void *data, int data_size);	/* set parameter value from raw data */
int m_req_raw (uint16 node, int category, int msg_id, void *data, int data_size);	/* request parameter value from raw data */

#ifdef __cplusplus
}
#endif

#endif				/* NED__MESSAGE */
