/* Timlits  - $title definitions for timbre head

   modified:
   08/05/91 - PF  - added TIM.POLY.EVENT
   07/06/87 - MWH - ADD LITERALS FOR KEYBOARD LOOKUP TABLE
   
*/


/* Storage areas for each timbre */

/* A block of storage is allocated for each active timbre.   This
   block holds precomputed information for the basic timbre and
   up to four partials.

   this sector contains numerous pre-computed information for
   each partial (stored normally with a stride of 4)  as well
   as precomputed information for each timbre.

   use the following definitions: */


/* For each partial: */


/* Tim.volume.lev is put */
/* First to provide      */
/* Speedy lookup during  */
/* Note start            */

/* Translated to C:     January 19, 2015 at 12:08:27 PM AST */
/* Translator Version:  0.000          */

#include "XPL.h"

#define  tim_volume_lev       (  0)                                     // Holds computed volume level


/* Set up by compact.timbres:  */

#define  tim_partial_pointers (  4)                                     // Wrd pointer to start of partial (off of par.ptr)
#define  tim_pinfo_pointers   (  8)                                     // Wrd pointer to pinfo area (off of par.ptr)


/* Set up by alloc.timbre:     */

#define  tim_synth_typ        ( 12)                                     // Synth type
#define  tim_fm               0                                         // Synth type is fm
#define  tim_poly             2                                         // Synth type is poly
#define  tim_mono             3                                         // Synth type is mono
#define  tim_cue              4                                         // Track is cue track
#define  tim_stereo           8                                         // Synth includes stereo

#define  tim_kbdtab_ptr       ( 16)                                     // Pointer to kbd look up table - relative to tim.ptr, off by one
#define  tim_wmem_            ( 20)                                     // For fm wave memory
#define  tim_can_use_both     ( 24)                                     // Allocation


/* Set up by compute.env.params */

#define  tim_options          ( 28)                                     // Partial options
#define  tim_partial_vol      ( 32)                                     // Partial volume for each partial - from partial

#define  tim_eatlim           ( 36)                                     // Eatlim - eidtmc
#define  tim_eatlim_str       (  5)                                     // Stride thereof

#define  eatlim_loc           0                                         // Env atk  limit  0-255
#define  eatint_loc           1                                         // Env atk  interp info
#define  eidlim_loc           2                                         // Env idec limit  0-255
#define  eidint_loc           3                                         // Env idec interp info
#define  eidtmc_loc           4                                         // Env idec time   const

#define  tim_i_ishc           ( 56)                                     // Index shift count.

#define  tim_iatlim           ( 60)                                     // Iatlim - iidtmc
#define  tim_iatlim_str       (  5)                                     // Stride thereof

#define  iatlim_loc           0                                         // Inx atk  limit  0-255
#define  iatint_loc           1                                         // Inx atk  interp info
#define  iidlim_loc           2                                         // Inx idec limit  0-255
#define  iidint_loc           3                                         // Inx idec interp info
#define  iidtmc_loc           4                                         // Inx idec time   const

#define  tim_efdint           ( 80)                                     // Efdint - sfdtmc
#define  tim_efdint_str       (  6)                                     // Stride thereof

#define  efdint_loc           0                                         // Env fdec interp info
#define  ifdint_loc           1                                         // Inx fdec interp info
#define  efdtmc_loc           2                                         // Env fdec time   const
#define  ifdtmc_loc           3                                         // Inx fdec time   const
#define  sfdenv_loc           4                                         // Env fdec (splicing)
#define  sfdtmc_loc           5                                         //          (Splicing)

/* Write(mal)=tim.dec.adj+(ptl*tim.dec.adj.str); */
#define  tim_dec_adj          (104)                                     // Decay adjust look up for each partial
#define  tim_dec_adj_str      (  5)                                     // Stride thereof

#define  tim_harm_adj         (124)                                     // Hardmonic adjust look up table
#define  tim_harm_adj_str     (  8)                                     // Stride thereof

#define  tim_loop_interp      (156)                                     // Info for looping timbre frame
#define  tim_loop_deltas      (160)                                     // For exponential info for timbre frame loop


/* Set up by compute.logs:  */

#define  tim_log_ra           (164)                                     // Log of ratio
#define  tim_log_pt           (168)                                     // Log of partial tuning
#define  tim_log_nch          (172)                                     // Holds log of par chorus
#define  tim_log_nchch        (176)                                     // Holds log of chorus/par chorus


/* Set up by compute.rbits: */

#define  tim_init_stpos       (180)                                     // Holds initial stereo position

#define  tim_pan_phase        (184)                                     // To synchronize stereo

/* 188-191: Free */


/* For each timbre: */


/* Set up by compact.timbres:    */

#define  tim_misc_pointer     (192)                                     // Wrd pointer to misc area (off of par.ptr)
#define  tim_tinfo_pointer    (193)                                     // Wrd pointer to tinfo area (off of par.ptr)


/* Set up by compute.env.params: */

/* Note: tim.csem must follow tim.fd.splicing - see final decay code       */

#define  tim_toptions         (194)                                     // Holds timbre option bits
#define  tim_fd_splicing      (195)                                     // True if fd splicing performed
#define  tim_csem             (196)                                     // To detect recalls


/* Set up by compute.misc.info:  */

#define  tim_pf_tc            (197)                                     // Pressure filter time constant
#define  tim_rf_tc            (198)                                     // Ribbon   filter time constant


/* Set up by compute.logs:       */

#define  tim_log_ch           (199)                                     // Log of chorus (!!!)


/* Set up by compute.rbits:      */

/* The following entries each contain one bit for each continuous rte */
/* Parameter (volume, tuning, etc.).   The bit will be set if the     */
/* Particular expression input is routed to the continuous parameter. */
/* This provides a quick way of telling which continuous parameters   */
/* Must be updated when a given expression input changes              */

/* Must be in order: */

#define  tim_act_pars         (200)                                     // Bits for pedal 1 & 2, mwheel, breath, pitch, ribbon
#define  tim_vp               (208)                                     // Bits for velocity patching
#define  tim_pp               (209)                                     // Bits for pressure patching
#define  tim_kp               (210)                                     // Bits for kbd cv   patching

/* Used.rtes contains an 8 bit quantity in each half.  These bits      */
/* Tell whether a given expression input affects any continuous        */
/* Parameter (upper half),  or sampled parameter (lower half).         */
/* This information is used to decide which real time effects changes  */
/* To record.                                                          */

#define  tim_used_rtes        (211)                                     // Holds bits for active expression inputs

/* The .look entries are used to quickly look to see if a given parameter */
/* Is affected by 1 or more expression input on 1 or more partials.       */
/* They are checked during note starting (etc.) to actually apply the     */
/* Real time effect.   They contain a 4 bit field for each of 4 partials. */
/* Each field is a pointer to a list if rte patches starting at           */
/* Tim.rte.patches.   Each patch contains 3 5 bit fields that tell what   */
/* Expression inputs are used.                                            */


/* Sampled parameters - must be in order: */

#define  tim_ve_atk_look      (212)
#define  tim_ve_idec_look     (213)
#define  tim_ve_del_look      (214)
#define  tim_he_atk_look      (215)
#define  tim_he_idec_look     (216)
#define  tim_ve_fdec_look     (217)
#define  tim_he_del_look      (218)
#define  tim_p_rate_look      (219)
#define  tim_he_fdec_look     (220)
#define  tim_vib_atk_look     (221)
#define  tim_dyn_env_look     (222)

/* Continuous parameters - must be in order: */

#define  tim_v_rate_look      (223)
#define  tim_v_depth_look     (224)
#define  tim_ve_pksus_look    (225)
#define  tim_v_mdepth_look    (226)
#define  tim_s_rate_look      (227)
#define  tim_he_pk_look       (228)
#define  tim_he_sus_look      (229)
#define  tim_s_dep_look       (230)
#define  tim_s_pan_look       (231)
#define  tim_tuning_look      (232)
#define  tim_fmratio_look     (233)
#define  tim_reprate_look     (234)
#define  tim_chorus_look      (235)
#define  tim_tvol_look        (236)

#define  tim_rte_patches      (237)
#define  tim_rte_length       ( 10)

/* Other: */

#define  tim_random_key_      (247)                                     // Holds 64-bit random # keys for timbre
#define  tim_max_num_notes    (251)                                     // Max # of notes for timbre
#define  tim_any_dpan         (252)                                     // True if any dpaning performed
#define  tim_needs_cue_alloc  (253)                                     // True if cues need allocating
#define  tim_poly_event       (254)                                     // True if any ram events in patch list

/* Timbre head unused words : 255 - 255 */


/* Real time effects bits in timbre head: */

/* The old rte code used a single 16-bit word to indicate rte patchings. */
/* The following bit definitions were used:                              */

/* (Used only in expand.rte below)                                       */

#define  p_rate               0x8000                                    /* Patch to port.rate                                                   */     /* Bit definitions */
#define  he_sus               0x4000                                    /* Harm env sustain lev                                                 */     /* For original */
#define  he_pk                0x2000                                    /* Harm env peak    lev                                                 */     /* Keyboard      */
#define  he_dec               0x1000                                    // Harm env decays
#define  he_atk               0x0800                                    // Harm env attack
#define  ve_pksus             0x0400                                    // Peak & sustain lev
#define  ve_dec               0x0200                                    // Decays
#define  ve_atk               0x0100                                    // Attack
#define  v_depth              0x0080                                    // Vibrato depth - default

#define  ped_in               0x0002                                    // Pedal
#define  vel_in               0x0004                                    // Velocity
#define  rec_bit              0x0080                                    // Patch pedal to recorder playback


/* The new software uses a 40 word look up table with each timbre */
/* To encode the real time effects patchings.   This look up      */
/* Table holds 8 5-word records  (1 record for each of 8          */
/* Expression inputs).                                            */

/*    Word 0   -   bits for each partial affected                 */
/*    Word 1   -   bits for sampled parameters                    */
/*    Word 2   -   bits for continuous parameters                 */
/*    Word 3   -   bits for inverted sampled parameters           */
/*    Word 4   -   bits for inverted continuous parameters        */

/* Sampled parameters:         *//* Put in this order to */
/* Simplify conversion  */
#define  n_ve_atk             0x0001                                    // New vol  env attack
#define  n_ve_idec            0x0002                                    // New vol  env initial decay
#define  n_he_atk             0x0008                                    // New harm env attack
#define  n_he_idec            0x0010                                    // New harm env initial decay
#define  n_p_rate             0x0080                                    /* New patch to port.rate                                               */ 

#define  n_ve_del             0x0004                                    // New vol  env delay
#define  n_ve_fdec            0x0020                                    // New vol  env final decay
#define  n_he_del             0x0040                                    // New harm env delay
#define  n_he_fdec            0x0100                                    // New harm env final decay
#define  n_vib_atk            0x0200                                    // New vibratto attack
#define  n_dyn_env            0x0400                                    // New dynamic envelope patch


/* Bits for continuous parameters: */

/* Note:  these bits are stored into  tim.act.pars(0-7), tim.vp, tim.pp,
   and tim.kp to tell which note parameters must be recomputed when
   a particular expression input changes                                */

/* They are also of course stored in the timbre to indicate a patching  */

#define  n_ve_pksus           0x0004                                    // New vol  env peak & sustain lev
#define  n_he_pk              0x0020                                    /* New harm env peak    lev                                             */ 
#define  n_he_sus             0x0040                                    /* New harm env sustain lev                                             */ 

#define  n_v_rate             0x0001                                    // New vibrato rate
#define  n_v_depth            0x0002                                    // New vibrato depth
#define  n_v_mdepth           0x0008                                    // New vibrato mod depth
#define  n_s_rate             0x0010                                    // New stereo  rate
#define  n_s_dep              0x0080                                    // New stereo  depth
#define  n_s_pan              0x0100                                    // New stereo  pan
#define  n_tuning             0x0200                                    // New tuning patch
#define  n_fmratio            0x0400                                    // New fm ratio patch
#define  n_reprate            0x0800                                    // New reprate  patch
#define  n_chorus             0x1000                                    // New chorus   patch
#define  n_tvol               0x2000                                    // New tvol     patch
#define  n_nfreq              0x4000                                    // Bit set in nupdt only - causes new frequency info to be computed

/* Index numbers for real time effects data: */

#define  r_pedal1             0                                         // Pedal 1
#define  r_pedal2             1                                         // Pedal 2
#define  r_mwheel             2                                         // Mod wheel
#define  r_breath             3                                         // Breath controller
#define  r_pwheel             4                                         // Pitch wheel
#define  r_ribbon             5                                         // Ribbon controller
                                                                        // Note: raw ribbon stored in 5
                                                                        // Ribbon to use is stored in 6
                                                                        // Other filter items in 7

/* Bit literals corresponding to above index numbers */

#define  b_pedal1             1                                         // Pedal 1
#define  b_pedal2             2                                         // Pedal 2
#define  b_mwheel             4                                         // Mod wheel
#define  b_breath             8                                         // Breath controller
#define  b_pwheel             16                                        // Pitch wheel
#define  b_ribbon             32                                        // Ribbon controller
#define  b_pressure           64                                        // Pressure
#define  b_velocity           128                                       // Velocity

/* from 138-pre1 */
/* Partial option definitions: (tim.options) */

#define  any_fm               0x0001                                    // Set if fm is active in sound
#define  splice_info          0x0002                                    // Set if splicing is used
#define  kbd_env              0x0004                                    // Set if keyboard envelope used
#define  dyn_env              0x0008                                    // Set if dynamic envelope used
#define  zero_attack          0x0010                                    // Set if attack time = 0
#define  any_delay            0x0020                                    // Set if envelope or index delay<>0
#define  any_stam             0x0040                                    // Set if any dynamic stereo or am
#define  env_dadj             0x0080                                    // Set if decay adjust is required
#define  inx_hadj             0x0100                                    // Set if harm  adjust is required
#define  env_rtes             0x0200                                    // Set if rtes affect envelope times
#define  inx_rtes             0x0400                                    // Set if rtes affect index    times
#define  min_ptune            0x0800                                    // Set if minus ptuning used
#define  any_glide            0x1000                                    // Set if portamento is active
#define  tun_rtes             0x2000                                    // Set if rtes affect tunings
#define  fd_rtes              0x4000                                    // Set if rtes affect final decay
#define  sam_delay            0x8000                                    // **** Unused - this bit is free ****

/* Timbre options defintions: (tim.toptions) */

#define  not_stealable        0x0001                                    // Can not steal channels during sust
#define  bit_repeat_bit       0x0002                                    // Bit.repeat stored here
#define  any_sampled          0x0004                                    // Set if any partials use mono samp
#define  bit_arpeg_bit        0x0008                                    // Bit.repeat stored here
#define  slap_bass            0x0010                                    // Set if timbre uses fast splicing
#define  bit_phrase_bit       0x0020                                    // Bit.aphrase stored here
#define  long_fdec            0x0040                                    // Indicates timbre has long f decay


/*  Keyboard lookup tables consist of a sector of external memory.  There
 *  are three words per record in the format shown below and one record
 *  per key.  With 85 keys, that uses a total of 255 words in the sector
 *  and covers notes c0 to c7.  The literals used to define the bit
 *  positions are found in polymod; they are part of the definition used
 *  for 32 bit poly "base" pointers.
 *
 *                      keyboard look up table
 *                 +-----------------------------+
 *                 | ptr to patch timbre frame in| klt.patch.tim.ptr
 *                 |    timbre parameter area    |
 *                 |-----------------------------|
 *  data present-->| bin | unused   |stereo|page | klt.base.msb
 * bits in field-->|..2..|....9.....|..1...|..4..|
 *                 |-----------------------------|
 *                 |     ptr to file block in    | klt.base.lsb
 *                 |   external or poly memory   |
 *                 |-----------------------------|
 *                 |              .              |
 *                 |              .              |
 *                 |              .              |
 *                 +-----------------------------+
 *
 */

/* Literals defining keyboard lookup table structure */

#define  klt_size             3                                         // Size of keyboard lookup record

#define  klt_patch_tim_ptr    0                                         // Patch timbre frame pointer
#define  klt_base_msb         1                                         // Sound file ptr msb's & stereo bit
#define  klt_base_lsb         2                                         // Sound file ptr lsb's (all for mono)
