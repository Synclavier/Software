/* :Synlits:thdlits  $title  external memory organization, sequencer literals */

/*
Modified:
1999/10/31 - TY  - Added CFIT.MAP# and changed #.OF.MAP.PTRS to accommodate
1999/04/11 - TY  - Nuked DEFAULT.ACTIVE.MIDI.RTES.  It is replaced by a static GID/PID variable now.
1991/03/13 - PF  - Added B.PEDAL1 to default.active.midi.rtes
1991/02/13 - CJ  - Added magic #6
1990/11/06 - PF  - Added THD.SUSTAIN
1989/09/22 - CJ  - Added base pointers for tempo and meter maps
1989/07/23 - PF  - Added SEQ.MIDINET in BASE SECTOR
1989/07/18 - PF  - Added THD.MIDI.PATH definition
1988/10/14 - cj  - added thd.start.msb, thd.start.lsb to assist in
                   startup of notes in middle
1988/08/30 - PF  - CHANGED THD.SEQ.RTES; THD.MIDI.IN.RTES TO THD.LIVE.RTES
1988/08/05 - cj  - stored footage offset and smpte display offset in
1988/06/20 - eg  - added thd.ignore - use to temporarily disable playback - checked in compute.active.trks
1988/05/12 - CJ  - REMOVED SEQ.ROUTS.VALID, ETC.
1988/05/04 - PF  - ADDED DEFS FOR THD.SEQ.RTES, THD.MIDI.IN.RTES
1988/03/03 - cj  - Added definition for THD.CUE.OUT
1988/03/03 - eg  - thd.seq.cut.bits
1987/12/17 - MWH - Merge poly updates with N-Final plus AEE sources
1987/11/17 - cj  - added space in base sector for protect bit, captions
1987/09/08 - MWH - ADDED PREFERED POLY BIN TO TRACK HEADER
1987/01/06 - CJ  - CHANGED KBD.MONO1, MONO2, MONO3 SPLICE TIMES
1986/12/31 - CJ  - ADDED KBD.MONO SPLICE TIME DECLARATIONS
*/


/* External memory is allocated in two phases:

   during initialization - synclavier look up tables are allocated
                           from the top of memory down.  These
                           look up tables include sine tables,
                           frequency tables, log tables, a null timbre,
                           etc.  These tables are not moved during
                           run time.

                           (see variables defined in 040-glob)

   run time              - other areas are allocated in real time
                           out of the remaining (lower) memory.
                           these include areas for timbre information,
                           sequencer notes, timbre bank, etc.


   lower external memory:                 (real time allocation)

                  sector #    length      contents
zeroes            0           1           zeroes
bas.ptr           1           bas.len     base sector for sequence
par.ptr           (var)       par.len     timbre parameter area
nah.ptr           (var)       nah.len     note area
bnk.ptr           (var)       bnk.len     timbre bank area
inf.ptr           (var)       inf.len     sequencer info area
tim.ptr           (var)       tim.len     precomputed timbre/partial
trd.ptr           (var)       trd.len     timbre display - only changed in 'terminal'
trd.ptr+trd.len   (var)       (var)       synclavier look up tables
                  (var)       (var)       object code swap files
examount                                  */


/* $Page  sequencer Data Structure, Memory Management Routines */


/* Definitions for sequencer data structure: */

/* Contents of a sequences:

   1.  Base sector - sector 0 of a sequence - contains misc information
                     and pointers to timbres, notes, music printing,
                     and other information.              (bas.ptr,bas.len)

   2.  Timbre parameters   (format defined in 120-tdef)  (par.ptr,par.len)

   3.  Note information - consists of:                   (nah.ptr,nah.len)

          note area header - 1 sector, holds pointers for up to
                             256 tracks.

          track header     - 1 sector for each non-empty track.
                             contains misc info for the track (loop,
                             play pointers, rte data) as well
                             as a pointer to the first block
                             of actual notes.  A track header
                             sector is also allocated for
                             the keyboard and split keyboard.

          note segments    - 1 sector each.  Linked list of
                             blocks.  Each block contains
                             a forward & reverse pointer and
                             up to 126 2-word note record

   4.  Music printing information - undefined format     (inf.ptr,inf.len) */


/* $Page - definitions for base sector:   */


/* Translated to C:     January 19, 2015 at 12:09:45 PM AST */
/* Translator Version:  0.000          */

#include "XPL.h"

#define  seq_scale            0                                         // Locations 0-11 - holds active scale
#define  seq_speed            12                                        // Holds latest sequencer speed
#define  seq_click            13                                        // Holds sequencer click rate
#define  seq_crm              14                                        // Click rate multiplier
#define  seq_smode            15                                        // Smpte mode
#define  seq_smbits           16                                        // Smpte start # of bits
#define  seq_secfra           17                                        // Smpte seconds/frames
#define  seq_hrsmin           18                                        // Smpte hrs/mins
#define  seq_bpm              19                                        // Beats per measure
#define  seq_mark_msb         20                                        // Mark button time (32-bits)
#define  seq_mark_lsb         21                                        // Mark button time (32-bits)

#define  seq_fmode            22                                        // Footage mode (coded)
#define  seq_foffset          23                                        // Footage display offset  - feet
#define  seq_foffset_ffr      24                                        // Footage display offset  - frames
#define  seq_foffset_ffb      25                                        // Footage display offset  - fbits

#define  seq_doffset          26                                        // Seq.doffset.msb
#define  seq_doffset_msb      26                                        // Smpte display offset - two's
#define  seq_doffset_lsb      27                                        // Complement # of smpte bits
#define  seq_doffdis          28                                        // Set to 1 to disable offset

#define  num_seq_params       29                                        // Number of knob parameters in seq list

#define  seq_dtd_solos        29                                        // Snapshot of dtd solos during save


/* 30-31: Free */

#define  magic_number         32                                        // Used to detect valid data
#define  seq_octratio         33                                        // Store active octave ratio here

/* Seq.loop-seq.s.p.t.c must be in order  */

#define  seq_loop             34                                        // True if overall loop
#define  seq_g_p_t            35                                        // Play time lsb of loop end
#define  seq_g_p_t_c          36                                        // Play time msb of loop end

#define  seq_s_p_t            37                                        // Play time lsb of loop start
#define  seq_s_p_t_c          38                                        // Play time msb of loop start

#define  seq_countin          39                                        // Count in # of beats
#define  seq_looplen          40                                        // Loop length in beats

/* 43-47: Free */

#define  seq_live_click       48                                        // Live click track for sequencer
#define  seq_mprev            49                                        // Music printing rev#
#define  seq_numtimbs         50                                        // Number of timbres in timbre info

#define  seq_poly_freq_table  51                                        // Code for which freq lookup table to use with poly sampling notes

/* 52-62: Free */

#define  seq_numsec           63                                        // Total # of sectors in sequence

#define  seq_bas              64                                        // Relative pointer to base sector
                                                                        //          Length
#define  seq_par              66                                        // Relative pointer to timbre info
                                                                        //          Length
#define  seq_nah              68                                        // Relative pointer to note area
                                                                        //          Length
#define  seq_inf              70                                        // Relative pointer to mp info
                                                                        //          Length

/* 72,74,76,78 - Other relative pointers for future blocks        */

#define  seq_prot             80                                        // Sequencer protect bits
#define  seq_capts            81                                        // Sequencer caption information
                                                                        // Reserve 40 words of space
#define  seq_capts_len        40

#define  seq_cbits_valid      121                                       // True if following bits were set up by a cut
#define  seq_cut_bits         122                                       // 16 Words of bits to show which
                                                                        // Tracks were soloed when a cut was made
                                                                        // Reserve 16 words of space

#define  seq_midinet          138                                       // Holds current filename of midinet routing
                                                                        // Reserve 16 words of space
                                                                        // Format: upper byte of 1st word holds byte count of string
                                                                        // Followed by up to 31 chars


/* 154-169: Free  */

#define  seq_map_ptrs         170                                       // Pointers for tempo and meter maps
#define  __of_map_ptrs        3                                         // Three pointers for now
                                                                        // Note: declare more variables after
                                                                        // Meter.map.ptr if #.of.map.ptrs
                                                                        // Changes !!!
#define  tempo_map_           0
#define  meter_map_           1
#define  cfit_map_            2

#define  seq_map_code         1024                                      // Stored as track # to identify blocks
#define  seq_grpaux_code      2048                                      // Stored as track # to identify blocks

/* 173-189: Reserve more map pointers (later)   */

#define  seq_button_list      190                                       // Working track numbers for button panl
#define  seq_button_len       4                                         // 4 Used, but save space for more...

/* 198-255: Free  */

/* Note: bas.len is 2 sectors long so presumably 256-511 are also free...  */


/* Magic numbers: */

#define  magic_1              0xA72E                                    // Indicates 10 ms. Durations in seq.
#define  magic_2              0xA72F                                    // Indicates 5  ms. Durations in seq.
#define  magic_3              0xA730                                    // Indicates sfm sequence
#define  magic_4              0xA731                                    // Indicates timbre frame format
#define  magic_5              0xA732                                    // Indicates 32 track format
#define  magic_6              0xA733                                    // Indicates include tempo map
#define  magic_7              0xA734                                    // Indicates includes grouped tracks


/* $Page - definitions for note area header, track base sector */


/* Definitions for note area header:   */

/* Note area header contains 256 pointers, 1 for each possible track.
   each pointer is a sector number (relative to the note area header)
   that contains the track header sector for that track  */


/* Definitions track header sector: */

/* Real time information:  */

#define  thd_for              ( 0)                                      // Forward pointer to first note list segment
#define  thd_rev              ( 1)                                      // Reverse pointer (always 0 for track header sector)
#define  thd_trk              ( 2)                                      // Holds track # for garbage collect

#define  thd_nevf             ( 3)                                      // Forward pointer for next event que
#define  thd_nevr             ( 4)                                      // Reverse pointer for next event que

#define  thd_mrnptr           ( 5)                                      // Most recent nptr for this track
#define  thd_arp              ( 6)                                      // Arpeggiate  nptr for this track

#define  thd_active           ( 7)                                      // True for active tracks

/* Current play pointers & info: */

#define  thd_nmsb             ( 8)                                      // Next event msb absolute time
#define  thd_nlsb             ( 9)                                      // Next event lsb absolute time
#define  thd_lmsb             (10)                                      // Last event msb absolute time
#define  thd_llsb             (11)                                      // Last event lsb absolute time

#define  thd_wrd              (12)                                      // Current play pointer - wrd #
#define  thd_sec              (13)                                      // Current play pointer - sec #

#define  thd_cxpos            (14)                                      // Current transposition
#define  thd_ctimb            (15)                                      // Current timbre #
#define  thd_clpitch          (16)                                      // Current last pitch
#define  thd_ctbits           (17)                                      // Current real time rpt/arp bits
#define  thd_cg_val           (18)                                      // Current midi portamento switch
#define  thd_cprogram         (19)                                      // Current midi program number
#define  thd_poly             (20)                                      // Prefered poly bin for any sound files on this track
#define  thd_xposbase         (21)                                      // Base key number for transposing

/* 22-23: Reserved for future expansion   */

#define  thd_rtes             (24)                                      // Current real time effects


/* User information stored here: */

#define  thd_midi             (32)                                      // Midi routing for track
#define  thd_tvol             (33)                                      // Track volume
#define  thd_trou             (34)                                      // Track routing
#define  thd_tpan             (35)                                      // Track pan

#define  thd_oratio           (36)                                      // Octave ratio for this track

/* Must be in order  */

#define  thd_nact             (37)                                      // Number of active notes
#define  thd_ndec             (38)                                      // Number of decay  notes

#define  thd_sxpos            (39)                                      // Track starting transpos
#define  thd_stimb            (40)                                      // Starting timbre #
#define  thd_slpitch          (41)                                      // Starting last pitch info

#define  thd_nn_msb           (42)                                      // Numb of notes - msb
#define  thd_nn_lsb           (43)                                      // Numb of notes - lsb

/* $Page  -  track header sector (continued) */


/* Thd.cplist, thd.cpsum, thd.cppsum must be in order */

#define  thd_cplist           (44)                                      // Pointer for linked list of tracks using channel pressure
#define  thd_cpsum            (45)                                      // Current  channel pressure values on each track
#define  thd_cppsum           (46)                                      // Previous channel pressure values on each track

/* Thd.cplist, thd.rf.tc, thd.rf.pars must be in order   */

#define  thd_arlist           (47)                                      // Active ribbon linked list
#define  thd_rf_tc            (48)                                      // Duplicate ribbon time const
#define  thd_rf_pars          (49)                                      // Params to update

#define  thd_scale            (50)                                      // Special scale for this track

/* Thd.upd.pars, thd.upd.qued, thd.upd.link must be in order   */

#define  thd_upd_pars         (62)                                      // Update par bits
#define  thd_upd_qued         (63)                                      // True if linked
#define  thd_upd_link         (64)                                      // Update link

#define  thd_trigger          (65)                                      // Trigger side for ping/pong

#define  thd_info_trigger_key (66)                                      // Nonzero when info mode key trigger has occurred


/* Info in track header sector for start of overall loop */

#define  thd_lp_nmsb          (67)                                      // Loop next event msb absolute time
#define  thd_lp_nlsb          (68)                                      // Loop next event lsb absolute time
#define  thd_lp_lmsb          (69)                                      // Loop last event msb absolute time
#define  thd_lp_llsb          (70)                                      // Loop last event lsb absolute time

#define  thd_lp_wrd           (71)                                      // Loop play pointer - wrd #
#define  thd_lp_sec           (72)                                      // Loop play pointer - sec #

#define  thd_lp_cxpos         (73)                                      // Loop transposition
#define  thd_lp_ctimb         (74)                                      // Loop timbre #
#define  thd_lp_clpitch       (75)                                      // Loop last pitch
#define  thd_lp_ctbits        (76)                                      // Loop real time rpt/arp bits
#define  thd_lp_cg_val        (77)                                      // Loop midi portamento switch
#define  thd_lp_cprogram      (78)                                      // Loop midi program number

/* 79-82: Reserved for future expansion   */

#define  thd_lp_rtes          (83)                                      // Loop real time effects

/* 91-94: Free */

/* Independent loops:   */

#define  thd_ilp              (95)                                      // True if indep loop
#define  thd_countin_msb      (96)                                      // 32-Bit count-in duration in msec
#define  thd_countin_lsb      (97)
#define  thd_looplen_msb      (98)                                      // 32-Bit ilp length in msec
#define  thd_looplen_lsb      (99)


/* $Page  -  track header sector (continued) */


/* Info for start of independent loop  */

#define  thd_ils_wrd          (100)                                     // Ils play pointer - wrd #
#define  thd_ils_sec          (101)                                     // Ils play pointer - sec #

#define  thd_ils_cxpos        (102)                                     // Ils transposition
#define  thd_ils_ctimb        (103)                                     // Ils timbre #
#define  thd_ils_clpitch      (104)                                     // Ils last pitch
#define  thd_ils_ctbits       (105)                                     // Ils real time rpt/arp bits
#define  thd_ils_cg_val       (106)                                     // Ils midi portamento switch
#define  thd_ils_cprogram     (107)                                     // Ils midi program number

/* 108-111: Reserved for future expansion */

#define  thd_ils_rtes         (112)                                     // Ils real time effects


/* Info for end of independent loop */

#define  thd_ile_wrd          (120)                                     // Ile play pointer - wrd #
#define  thd_ile_sec          (121)                                     // Ile play pointer - sec #

#define  thd_ile_cxpos        (122)                                     // Ile transposition
#define  thd_ile_ctimb        (123)                                     // Ile timbre #
#define  thd_ile_clpitch      (124)                                     // Ile last pitch
#define  thd_ile_ctbits       (125)                                     // Ile real time rpt/arp bits
#define  thd_ile_cg_val       (126)                                     // Ile midi portamento switch
#define  thd_ile_cprogram     (127)                                     // Ile midi program number

/* 128-131: Reserved for future expansion */

#define  thd_ile_rtes         (132)                                     // Ile real time effects


#define  thd_usage            (140)                                     // Timbre usage table
                                                                        // 16 2-Word entries
                                                                        // Holds timbre #, count

#define  thd_usage_len        ( 32)

/* The following two variables contain the controller number to which the
   equivalent synclavier expression is supposed to be routed. Each array can
   support routings of up to 8 continuous controllers and 8 switch inputs for
   each timbre (0-17).  Valid controller/switch numbers are in the range 0-95.   */

/* Thd.syn.crout, thd.syn.srout must be in order   */

#define  thd_syn_crout        (172)                                     // Synclavier to midi routing array for continuous controllers
#define  thd_syn_srout        (180)                                     // Synclavier to midi routing array for switches

#define  thd_num_nls          (188)                                     // # Of note list segments for track

/* Indep loop counters (keep next six words in order) */

#define  thd_ilpctr           (189)                                     // Current ilp counter
#define  thd_any_for          (190)                                     // Set to 1 if more notes in forward dir
#define  thd_any_rev          (191)                                     // Set to 1 if more notes in reverse dir

#define  thd_lp_ilpctr        (192)                                     // Loop ilp counter
#define  thd_lp_any_for       (193)                                     // Set to 1 if more notes in forward dir
#define  thd_lp_any_rev       (194)                                     // Set to 1 if more notes in reverse dir

#define  thd_midi_cg_val      (195)                                     // Most recent midi output
#define  thd_midi_cprogram    (196)                                     // Most recent midi program #

#define  thd_midi_rtes        (197)                                     // Most recent midi rte values

#define  thd_ils_scanned      (205)                                     // Set to one when ils record played (used to do a consistency check)
#define  thd_lp_ils_scanned   (206)                                     // Looped version of thd.ils.scanned


/* Thd.active.midi.rtes contains several bits which are used to      */
/* Enable/disable both input and output of midi rte changes          */
/* (I.e., pitch wheel, mod wheel, pedals, pressure, velocity, etc)   */
/* This info should be moved around in the same way that thd.midi    */
/* Is moved. (I.e., in skt, smt, move.timbre.to)                     */
/* The high bit of this word should be set to indicate that the      */
/* Data is valid.  Otherwise it will be reset to the default values  */

/* Bit definitions: (same order as thd.rtes for 1st 6 bits)       */
/*          Bit 0  -->  pedal1            (b.pedal1)              */
/*          Bit 1  -->  pedal2            (b.pedal2)              */
/*          Bit 2  -->  mod wheel         (b.mwheel)              */
/*          Bit 3  -->  breath controller (b.breath)              */
/*          Bit 4  -->  pitch wheel       (b.pwheel)              */
/*          Bit 5  -->  ribbon controller (b.ribbon)              */

/*          Bit 6  -->  pressure          (b.pressure)            */
/*          Bit 7  -->  velocity          (b.velocity)            */
/*          Bit 15 -->  valid bit         (must be set to 1)      */

#define  thd_active_midi_rtes (207)                                     // Bits for enabled midi expression inputs

#define  thd_ils_time_msb     208                                       // Time of ils loop start
#define  thd_ils_time_lsb     209                                       // Used to detect 0 length loops

#define  thd_cue_track        210                                       // Nonzero if this is cue trk
#define  thd_nev_msb          211                                       // Cued next event time
#define  thd_nev_lsb          212                                       // Lsb

#define  thd_cue_out          213                                       // Dynamic output allocation output #

#define  thd_live_rtes        214                                       // Current live values (from kbd or midi only)
#define  thd_ignore           222                                       // If true, compute.active.trk doesn't set thd.active for track, even if soloed
#define  thd_start_msb        223                                       // Used by start.up.notes.in.middle
#define  thd_start_lsb        224                                       // To know which notes to start

#define  thd_midi_path        225                                       // Logical midi path to midinet

/* Bit definitions for thd.ignore                  */

#define  thd_ignore_ignore    1                                         // Ignore track during playback
#define  thd_ignore_xpos      2                                         // Track is a transpose track

/* Bit definitions for thd.sustain:                */
/*          Bit 0  -->  current sustain state      */
/*          Bit 1  -->  overall loop sustain state */
/*          Bit 2  -->  ils sustain state          */
/*          Bit 3  -->  ile sustain state          */

#define  thd_sustain          226                                       // Sustain pedal bits

#define  thd_grouplist        227                                       // Pointer to group list
#define  thd_auxinfo0         228                                       // Aux sector of info
#define  thd_auxinfo1         229                                       // Aux sector of info
#define  thd_auxinfo2         230                                       // Aux sector of info
#define  thd_numgrpaux        4                                         // Number possible thereof...

/* 231-238: Free  */

#define  thd_track_title      239                                       // 239 - 255 - track title

/* $Page - definitions for note list segment */


/* Each note list segment contains a forward pointer, a reverese pointer,
   a track number, and up to 126 note records.  Each note segment
   terminates with a (-1)  */

/* Definitions:   */

/* Nls.for - nls.lp must be in order   */

#define  nls_for              (  0)                                     // Forward pointer to next note list segment
#define  nls_rev              (  1)                                     // Reverse pointer to last note list segment (or track header sector)
#define  nls_trk              (  2)                                     // Holds track # for garbage collect
#define  nls_fp               (  3)                                     // Pointer in sector to first recrd
#define  nls_lp               (  4)                                     // Pointer in sector to last  recrd

#define  nls_firstl           (  8)                                     // First note record would go here
#define  nls_last             (251)                                     // Last note record would go here

#define  nls_eos              ( -1)                                     // Flag at end of note records


/* Definitions of track group records  */

#define  grpaux_for           (  0)                                     // Forward pointer; same as thd.for and nls.for
#define  grpaux_rev           (  1)                                     // Reverse pointer to prior block or 0
#define  grpaux_trk           (  2)                                     // Holds track in lower, seq.grpaux.code in upper

#define  grpaux_but           (  8)                                     // Holds associated track button assignments

#define  grpaux_num           ( 16)                                     // Holds number of tracks on list
#define  grpaux_lst           ( 24)                                     // This is the start of the list
#define  grpaux_max           (216)                                     // Maximum number of entries in list.  Allows all tracks
