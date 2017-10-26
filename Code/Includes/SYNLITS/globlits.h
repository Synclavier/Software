/* Globlits  $title  file of General Literals Used by Most Programs */

/*
Modified:
1998/05/15 - TY  - Corrected a misleading comment for LAST.LOD.TRACK.
1998/05/09 - TY  - Added "is.negative" and "is.odd".
1990/03/09 - PF  - Change NUM.MIDI.TRACKS to MAX.INPUTS = 128
1989/03/14 - PF  - Added DEFAULT.MIDI.VELOCITY
1988/06/22 - EEG - Moved various track literals (tracks above 200) here
1988/05/04 - TSS - MOVED MIDI.MIDDLE.C HERE FROM 142-MIDI
1987/06/15 - MWH - Added 32 bit address literals
1987/01/20 - TS  - ADDED POLYPHONY.DISP LITERAL
1986/11/17 - EG  - CHANGED SWAPABLE TO RECURSIVE SWAP
1986/11/04 - BSW - increased the size of the character input buffer
*/

// XPL literals:
#ifndef true
	#define	true		1						/* a boolean TRUE								*/
#endif

#ifndef false
	#define	false		0						/* a boolean FALSE								*/
#endif

#define	boolean				fixed

// Serial port device addresses
#define	d50					0x0028										// Terminal port
#define	d51					0x0029
#define	d40					0x0020										// Printer  port
#define	d41					0x0021
#define	d42					0x0022										// Voice    port
#define	d43					0x0023
#define	d44					0x0024
#define	d45					0x0025										// Mouse    port
#define	d54					0x002C										// Bits for d4x interrupt status

// SMPTE board is accessed using 4 device addresses
#define	smpte_subsys		     8                                      // Subsystem # for smpte card
#define	smpte_data_avail	0x0100                                      // Bit set D70 if data is available
#define smpte_no_sync       0x8000                                      // Bit set D71 if no sync
#define smpte_drop_frame    0x0400                                      // Bit set D71 if drop frame
#define	d70					0x0038										// Write 8 to reset

#define	d71					0x0039										// Status   bits; Write opcodes here
#define	d72					0x003A										// Seconds  frames
#define	d73					0x003B										// Hours    minutes; Read of d73 triggers new info

// MIDI board - note: they match smpte
#define	syssel				0x0038										// Midi subsystem select address
#define	mcha				0x0039										// Channel select address if write
#define	msta				0x0039										// Input fifo status      if read
#define	mdat				0x003A										// Input and output data port

// Scientific timer device addresses
#define	d16					0x000E
#define	d17					0x000F

// Mono sampling synth addresses
#define	d66					0x0036										// Control register
#define	d67					0x0037										// Data    register

// Guitar device addresses
#define	d134				0x005C
#define	d135				0x005D

// Multi-channel distributor device addresses
#define	d32					0x001A										// Device 32 digital io multiplexor
#define	d33					0x001B										//           Data port

// Keyboard interface device addresses
#define	dreg				0x0058										// Keyboard interface data register
#define	creg				0x0059										// Keyboard interface control register
#define	pulse_exwr			write(CREG)="40"\1; WRITE(CREG)=1; WRITE(CREG)="100"\1	// Mode 1 write then read

// Gate & trigger devices
#define	d164			0x0074
#define	d165			0x0075


// Fundamental synclavier literals:

#define	num_partials		4											// Number of partials per timbre

/* **** Note ****  -  during run time,  the keyboard and split keyboard
   area treated as tracks in many ways.   Look up tables for tracks
   are therefore offset by 2 words when compared to the tracks of the
   sequencer. */

#define	num_keys			(85)										// Allowable software key range - 0-84
#define	max_key_			(num_keys-1)								// Max allowed key number
#define	sync_middle_c		(36)										// Key number of middle c on synclavier
#define	midi_middle_c		(60)										// Key number of middle c in midi world

/* Run-time data structure: */

#define	zero_time			50											// Play.time.lsb of script=0.000
#define	rte_max				225											// Largest rte pedal position
#define	default_midi_velocity	rte_max									// Midi default velocity

#define	max_tracks			256											// Max # of tracks processed
#define	num_kbd_tracks		2											// Num of keyboard tracks
#define	max_inputs			128											// Num of input channels (kbd+midi)

#define	first_lod_track		202											// Abs. Track number of first lod track
#define	last_lod_track		218											// Abs. Track number of first track after last lod track

/* Various important or temp tracks used by procedures */
#define	undo_trk			230											// Abs trk used by g-screen for undo function
#define	undo_tmp_trk		231											// Abs trk used by g-screen for undo function

#define	bounce_trk			240											// Abs trk used by bounce for tmp trk
#define	chain_trk			241											// Abs trk used by chain to chain on to
#define	sou_trk				242											// Abs trk used by chain for copy of source trk
#define	sou_copy			243											// Abs trk used by cut for copy of source trk
#define	temp_trk			244											// Abs trk used by paste for tmp trk

#define	always_empty_track	248											// Abs. Track number of an always empty track
#define	locate_info_track	249											// Abs. Track number where aee locate positions stored
#define	cue_audition_track	254											// Abs. Track number of cue audition trk
#define	reserved_track		255											// Track 255 is reserved for future use


#define	max_timbres			256											// Max # of timbres processed

#define	bit0				1
#define	bit1				2
#define	bit2				4
#define	bit3				8
#define	bit4				16
#define	bit5				32
#define	bit6				64
#define	bit7				128
#define	bit8				256
#define	bit9				512
#define	bit10				1024
#define	bit11				2048
#define	bit12				((fixed) 4096)
#define	bit13				((fixed) 8192)
#define	bit14				((fixed) 16384)
#define	bit15				((ufixed) 32768)

/* 32 bit address literals */
#define	lw_msb				0											// Upper 16 bits of 32-bit "long word"
#define	lw_lsb				1											// Lower 16 bits of 32-bit "long word"
#define	lw_ilt				-1											// 32 bit compare is less than
#define	lw_ieq				0											// 32 bit compare is equal to
#define	lw_igt				1											// 32 bit compare is greater than
#define lw_num				0											// numerator typically stored here
#define lw_den				1											// denominator typically stored here
