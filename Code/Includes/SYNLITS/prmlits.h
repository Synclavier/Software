/* :Synlits:prmlits  -  $title  timbre Storage Definitions */

/*
Modified:
1991/06/19 - PF  - Added definitions for RAM Event patch frames
1989/04/20 - cj  - added literals for "EVENT" timbre frames
1987/01/20 - TS  - ADDED 'BIT.APHRASE' TO MISC.BITS LITERALS
1986/12/30 - CJ  - SAM.DELAY BIT IS NO LONGER USED
*/


/* The synclavier is capable of producing many different timbres at any
   one time.  These include up to two timbres that are active on the clavier
   and up to <n> timbres that may be active on the sequencer.

   a timbre consists of 4 partial timbres.  Old format partial 
   timbres are a list of 46 numbers that represent values that are 
   controllable from the front panel.  Variable length partial timbres
   are stored in an upwards compatible format described below.

   literal declarations are provided below to access the different partial
   timbre parameters.  The global variable 'ptptr' (for "partial timbre
   pointer") must be set to point to the location of memory containing the
   start of the parameter list before one of the literals is used. */

/* Format for 'new' timbre definitions:
.
.  During 1983, an expanded timbre format was programmed.  The key
.  Features of this new format are:
.
.  1. The new timbre format is based on variable length timbres.  Each
.     Partial timbre consists of a basic partial (frame 0) plus a list
.     Of "timbre frames".  The basic partial is identical to the earlier
.     Partial timbre format.  Timbre frames are described below.
.
.  2. The new format is completely upwards compatible with the earlier
.     Timbre format.  A new format timbre without any timbre frames
.     Is identical to an earlier format timbre.
.
.  3. 'Timbre banks' are now variable length.  Different lengths of .newdata
.     Are supported to accommodate different disk configuratins.  See
.     User documentation.
.
.  4. The timbre area in a sequence also became variable length.  A word
.     In the sequence 'misc' are indicates the length of the timbre
.     Definitions.
.
.  5. A new format sequence is stored on the disk in a slightly different
.     Order than before.  The new order is easier to use. (possibly)
.
.  6. ADDITIONAL PARTIAL & TIMBRE INFO - OPTIONAL DATA RECORDS CAN BE
.     INCLUDED IN THE TIMBRE TO STORE SUCH THINS AS TIMBRE NAME,
.     NEW TIMBRE PARAMETERS,  AND SO FORTH.
.
.  7. EXPANDED MEMORY - TIMBRES CAN BE STORED IN AN EXTERNAL MEMORY
.     (D60-63).
.
.SUPPRESSED & COMPACTED PARTIALS:
.
.  A FEATURE WAS ALSO PROGRAMMED THAT FREES UP MEMORY FOR TIMBRES WITH LESS
.  THAN 4 PARTIALS, OR TRACKS THAT ARE EMPTY:
.
.  A.   IF THE FIRST WORD OF A PARTIAL TIMBRE IS A (-2),  THEN THE ENTIRE
.       PARTIAL TIMBRE HAS BEEN SUPPRESSED TO ONLY THAT ONE WORD.  THIS
.       IS DONE TO CONSERVE SPACE,  SINCE MANY PARTIALS ARE NOT USED.
.
.  B.   THE BASIC PARTIAL IS 46 WORDS LONG.  IF THE WORD IMMEDIATELY
.       FOLLOWING THE BASIC WORD IS A (-1), THEN THAT BASIC
.       PARTIAL IS FOLLOWED BY AT LEAST ONE TIMBRE FRAME.  THE
.       TIMBRE FRAME IS CURRENTLY 172 WORDS LONG.  SUCCESSIVE
.       TIMBRE FRAMES ARE INDICATED BY SUCCESSIVE BLOCKS THAT
.       EACH BEGIN WITH A (-1).

THE TIMBRE FRAME FORMATS ARE DEFINED BELOW. */

/* $Subtitle  literal definitions for the basic partial: */

/* General info: */

/* Translated to C:     September 15, 2015 at 2:38:29 PM ADT   */
/* Translator Version:  0.000          */

#include "XPL.h"

#define  num_tim_partials     4                                         // Number of partials in a timbre
#define  num_params           46                                        // Number of basic parameters per partial
#define  num_misc_params      8                                         // Number of misc timbre words per timbre
#define  num_harmonics        24                                        // Number of harmonics per partial

/* Definitions for original (fixed length) bankdata files and early sequences: */

#define  len_entry            (((fixed) (num_params*num_tim_partials))+num_misc_params)
#define  len_bank             ( ((fixed) (8*len_entry)))                // Length of bank
#define  bank_stride          ( 6          )                            // Sectors/bank
#define  len_seq_timbs        (((fixed) (16*len_entry)))                // Sequencer

/* Definitions for the basic partial: */
/* These literals define the word offsets for elements */
/* Of a timbre definition.                             */

#define  p_edelay             (0)                                       // Envelope delay
#define  p_eattack            (1)                                       // Envelope attack
#define  p_eidecay            (2)                                       // Envelope initial decay
#define  p_efdecay            (3)                                       // Envelope final decay
#define  p_epeakl             (4)                                       // Envelope peak level    (left  key # in upper 6 bits, only if no PINFO)
#define  p_esustl             (5)                                       // Envelope sustain level (right key # in upper 6 bits, only if no PINFO)

#define  p_idelay             (6)                                       // Index delay
#define  p_iattack            (7)                                       // Index attack
#define  p_iidecay            (8)                                       // Index initial decay
#define  p_ifdecay            (9)                                       // Index final decay
#define  p_ipeakl             (10)                                      // Index peak level    (left  keyboard envelope slope, only if no PINFO)
#define  p_isustl             (11)                                      // Index sustain level (right keyboard envelope slope, only if no PINFO)

#define  p_coef_loc           12                                        // Location of coefs - phases in upper half (except frame 0, coef 0 - act.strgs)
#define  p_act_strgs          12                                        // Indicates active strings for guitar - upper 6 bits, negative logic
                                                                        // Active strings - upper 6 bits of coeff 0 for frame 0 only
/* Additional timbre paramters: */

/* Note: order is important here */

#define  p_ptuning            (36)                                      // Partial tuning
#define  p_vibwave            (37)                                      // Vibrato waveshape (4 bits + invert,quan,raise bits), stereo mode (9 bits)
#define  p_vibrate            (38)                                      // Vibrato rate
#define  p_vibdepth           (39)                                      // Vibrato depth
#define  p_vibattack          (40)                                      // Vibrato attack time
#define  p_glidebits          (41)                                      // Glide log/lin/on/off (2 bits), stereo center (7 bits), stereo pan depth (7 bits)
#define  p_gliderate          (42)                                      // Glide rate (low 10 bits), stereo rate (high 6 bits)
#define  p_ratio              (43)                                      // FM ratio
#define  p_decadj             (44)                                      // Decay adjust (low 10 bits), harmonic adj (high 6 bits)
#define  p_new_chorus         (45)                                      // Chorus value for this partial

/* Additional parameters (mapped into bits of above):

     46  harmonic adjust             . These parameters are stored  .
     47  stereo   center             . In the main 46-word partial  .
     48  stereo   depth              . Area.  They are stored in    .
     49  stereo   mode               . Upper word halves (etc.),    .
     50  stereo   rate               . And are indexed by the       .
     51  kbd envelope left key       . Numbers to the left for      .
     52  kbd envelope right key      . Table look-up purposes       .
     53  kbd envelope left slope
     54  kbd envelope right slope

     55  partial volume
     56  modulator depth
     57  dyn env low
     58  dyn env high

*/

/* $Page - synth updating */

/* When knob-controlled parameters are changed, it is often desired to
   have the sound of a held note change.  This is accomplished by
   setting bits in the 'reset.bits' word, as follows: */

#define  r_ilims              0x0001                                    // Reset index pk/sust limit
#define  r_elims              0x0002                                    // Reset vol env pk/sust limit
#define  r_wmems              0x0004                                    // Reset wave memory selection
#define  r_stero              0x0008                                    // Reset stereo info
#define  r_enpar              0x0010                                    // Recompute envelope parameters
#define  r_vrate              0x0020                                    // Reset vibrato rate in partial block/reinit vibrato
#define  r_freqs              0x0040                                    // Reset pitches on tuning/ratio etc change
#define  r_coefs              0x0080                                    // Compute wave memory on changing coef
#define  r_rrate              0x0100                                    // Repeat rate
#define  r_misci              0x0200                                    // Recompute misc info
#define  r_multi              0x0400                                    // Multi out routing
#define  r_smpte              0x0800                                    // Reset smpte syncing
#define  r_loopl              0x1000                                    // Poly loop len
#define  r_xpos               0x2000                                    // Time-stamped xpos function

/* $Subtitle  literal definitions for timbre frames */

/* The following definitions apply to timbre frames: */

/* A timbre definition may contain 'optional' blocks.  Each of these optional
   blocks begins with a keyword that would not otherwise appear in that
   location because of the natural parameter limits.  Optional blocks
   include:

        1. Timbre frames - splice information.  Multiple timbre
                           frames area allowed.

        2. Pinfo block   - partial info.  New,  optional information for
                           the partial

        3. Tinfo block   - timbre info.   New optional information
                           that applies to all partials in this timbre */

/* Key words to distinguish optional blocks: */

#define  mor                  (-1)                                      // -1: Indicates a timbre frame
#define  sup                  (-2)                                      // -2: Indicates a suppressed partial - no other words
#define  pinfo                (-3)                                      // -3: Indicates optional partial info block
#define  tinfo                (-4)                                      // -4: Indicates optional timbre  info block

/* Definitions for timbre frame bock: */

#define  type                 ( 1)                                      // Word 1 of t. Frame - indicates type - always 0 for now
#define  clen                 ( 2)                                      // Word 2 of t. Frame - indicates length
#define  tf_type              ( 0)                                      // Timbre frame type of frame - type word is 0
#define  pt_type              ( 1)                                      // Patch timbre type of frame - type word is 1
#define  cu_type              ( 2)                                      // Cue list     type of frame - type word is 2
#define  ev_type              ( 3)                                      // Event info   type of frame - type word is 3

/* Precomputed splice info:   - definitions for timbre frame type
   (computed by compute.env.params upon timbre recall): */

#define  len_timb_frame       172                                       // 172 Words - 8 system, 12 parameters, 24 coeffs, 128 wave table words

#define  p_e_seg_up           ( 3)                                      // Holds precomputed interpolator stuff for up splice
#define  p_e_seg_dn           ( 4)                                      // Holds precomputed interpolator stuff for down splice
#define  p_e_seg_udl          ( 5)                                      // S curve delta for splice
#define  p_e_seg_ddl          ( 6)                                      // Down splice s curve delta
#define  p_e_seg_lim          ( 7)                                      // Peak limit for this segment

/* Dialed in parameters: */

#define  s_env_p              ( 8)                                      // 12 Parameters start here

#define  p_e_seg_del          ( 8)                                      // Delay (ms) for this segment
#define  p_e_seg_atk          ( 9)                                      // Attack (ms) (splice time)
#define  p_e_seg_exp          (10)                                      // Exponential shape factor
#define  p_e_seg_rnd          (11)                                      // Random pitch amount
#define  p_e_seg_pk           (12)                                      // Peak level control
#define  p_e_seg_vol          (13)                                      // Partial volume control

#define  p_e_seg_fdn          (14)                                      // Final decay down splice
#define  p_e_seg_fdnt         (15)                                      // Final decay down splice time constant
#define  p_e_seg_fup          (16)                                      // Final decay up splice time constant
#define  p_e_seg_fcoe         (17)                                      // Final decay computation coefficient

#define  p_e_seg_pdel         (18)                                      // Pitch delta for envelope segment
#define  p_e_seg_loop         (19)                                      // Looping frame number

#define  s_env_h              (20)                                      // 24 Harmonics start here
#define  s_env_t              (44)                                      // 256 Point wave table starts here

/* $Page - definitions for pinfo block: */

#define  pinfo_len            (32)

#define  pi_pvol              1                                         // Partial volume stored here
#define  pi_mdep              2                                         // Modulator depth
#define  pi_denvl             3                                         // Dynamic envelope - low
#define  pi_denvh             4                                         // Dynamic envelope - high
#define  pi_kbdl              5                                         // Kbd envelope     - left      (overrides value in p_epeakl)
#define  pi_kbdr              6                                         // Kbd envelope     - right     (overrides value in p_esustl)
#define  pi_kbdls             7                                         // Kbd envelope     - l. slope  (overrides value in p_ipeakl)
#define  pi_kbdrs             8                                         // Kbd envelope     - r. slope  (overrides value in p_isustl)

/* Definitions for tinfo block */

#define  tinfo_len            (96)

#define  ti_tbd               1                                         // Indiv tone bend dep
#define  ti_kcvl              2                                         // Keyboard cv left
#define  ti_kcvr              3                                         // Keyboard cv right
#define  ti_pref              4                                         // Pressure filter response
#define  ti_ribf              5                                         // Ribbon   filter response
#define  ti_name              8                                         // Timbre name
#define  ti_rte               16                                        // New real time effects info - 40 words (8*5)
#define  ti_vsens             56                                        // Velocity sensitivity word (0-100)
#define  ti_vcon              57                                        // Velocity constant    word (0-  9)

/* $Page - definitions for 'patch' timbres */

/* A 'patch' timbre is a list of file names,  each including a starting key,
   ending key, volume factor, and transposition factor.   A patch timbre
   also includes a basic 46-word partial timbre for specifications of
   vibrato, stereo, portamento, etc. */

/* Definitions for patch timbres */

#define  pt_name              3                                         // 4-Word file name.   Words 0=more, 1=pt.type, 2=length
#define  pt_skey              7                                         // Starting key # - 0-84
#define  pt_ekey              8                                         // Ending   key # - 0-84
#define  pt_vol               9                                         // Volume factor - 1000 = max
#define  pt_tra               10                                        // Transposition factor.  0=None, else lower half = key number 0-84
#define  pt_tun               11                                        // Patch list semitone correction
#define  pt_tlen              12                                        // Total length - 32-bit format (msb,lsb)
#define  pt_llen              14                                        // Loop  length - 32-bit format (msb,lsb)
#define  pt_insofs            16                                        // Poly event in time sector offset
#define  pt_inwofs            17                                        // Poly event in time word offset
#define  pt_event             18                                        // True if this represents a poly event patch entry

/* Info from sound file:  (must match bl.lits) */

#define  pt_keytc             20                                        // Holds keytc from sound file
#define  pt_vrate             21                                        // Holds vrate from sound file
#define  pt_vdepth            22                                        // Holds vib depth
#define  pt_vat               23                                        // Holds vibrato attack time

/* Precomputed pitch info: */

#define  pt_srate             24                                        // Holds file sample rate correction factor
#define  pt_pitch             25                                        // Set octave pitch correction factor
#define  pt_toffs             26                                        // Holds octave correction with transposition

/* Mono sampling only:     */

#define  pt_start             27                                        // Start of valid data from sector+1 of block
#define  pt_abufl             28                                        // Attack buffer len words
#define  pt_wdd               29                                        // W. Disk device
#define  pt_wds               30                                        // W. Disk start sector
#define  pt_wdn               31                                        // W. Disk # of sectors
#define  pt_fwrds             32                                        // Words in final sector

/* Poly sampling only:     */

#define  pt_wtabl             27                                        // Minus wave table len             Pt.wtabl thru PT.ONLOOP are speedily written to the poly synth
#define  pt_sofs              29                                        // Sector offset for mark start     hardware to actually start the note.
#define  pt_wofs              30                                        // Word offset for mark start
#define  pt_loopl             31                                        // Minus loop len
#define  pt_onloop            33                                        // 1 For on, 3 for loop

#define  pt_max               34                                        // *Pos* max  len
#define  pt_check             36                                        // Checksum
#define  pt_sftl              37                                        // Sound file loop tot l
#define  pt_sfll              39                                        // Sound file loop loop l
#define  pt_stereo            41                                        // True if stereo sound file
#define  pt_khz               42                                        // Original smple rate
#define  pt_blen              43                                        // Length of allocated secion of poly memory for this file; that is, max-max limit for poly ram event total length

#define  pt_copyl             44                                        // Number of words to copy into partial

#define  pt_len               48                                        // Length of patch timbre frame
#define  pt_lname             48                                        // Long file name starts here. Only there if length of the frame > pt_len. Able string format.


/* $Subtitle - literals for cue list frame */

/* A cue patch list consists of a list of variable length frames */
/* As well a having a -1, length and type word,  they contain    */
/* The following information:                                    */


#define  cu_key               3                                         // Cue key # (0-255) for look up table
#define  cu_name              4                                         // Cue name,  variable length,  in string format


/* $Subtitle - literals for event frame */

/* An event patch list consists of a list of variable length frames */
/* As well a having a -1, length and type word,  they contain       */
/* The following information:  (matches section of event record)    */

#define  ev_key               3                                         // Event key # (0-1023) for look up table
#define  ev_cue_id            4                                         // Id of unerlying cue is stored here
#define  ev_in_msb            5                                         // Relative in time of event (msb,lsb)
#define  ev_out_msb           7                                         // Relative out time of event (msb,lsb)
#define  ev_mark_msb          9                                         // Relative offset mark
#define  ev_fade_in           11                                        // Fade in time (milliseconds)
#define  ev_fade_out          12                                        // Fade out time (milliseconds)
#define  ev_bits              13                                        // Event control bits
#define  ev_spare             14                                        // 14,15,16,17,18 = Spare
#define  ev_name              19                                        // Name of underlying cue (ned string)
                                                                        // Followed by caption (if any)

/* $Subtitle - definitions for misc area & guitar bits */

/* All timbres include an 8 word block of misc info.   This area is used to
.  Store items that apply to all the partials.   This 8 word block is
.  Found after the last timbre frame (if any) for partial # 4  (i.e. The
.  Last 8 words of the timbre definition). */

/* Local variable 'misc.base' must be set up pointing to the 8 word
.  Block before these litterals are used: */

#define  misc_bits_loc        0                                         // Holds bits
#define  lpcutf_loc           1
#define  gtinfo_loc           1                                         // Guitar info in same location - inverted, upper 8 bits
#define  hpcutf_loc           2
#define  bpcutf_loc           3
#define  bpwidth_loc          4
#define  rep_loc              5
#define  chorus_loc           6
#define  rte_loc              7

/* Bits stored in 'misc.bits': */

#define  bit_lpt              1                                         // Low pass track
#define  bit_repeat           2                                         // Repeat on/off
#define  bit_hpt              4                                         // High pass track
#define  bit_arpeg            8                                         // Arpeggiate
#define  bit_bpt              16                                        // Band pass track
#define  bit_aphrase          32                                        // Autophrasing
