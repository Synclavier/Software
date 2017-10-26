/* LPNLITS  $TITLE  Literals for Logical Parameter Numbers */

/*
2000/10/07 - TY  - added mod wheel proxy lpn
1991/01/09 - pf  - added click value lpn
1990/09/27 - cj  - added info for startup control button
1986/09/10 - ts  - added literals for startloop,chain,insert,delete buttons,beats per measure&loop parms
1986/05/15 - "official" creation of release-M modules
*/

/*
.  A new method for handling the scanning and displaying of buttons and
.lights has been implemented in the Synclavier II.  This new algorithm
.is designed to more clearly separate the button panel harware
.organization from the Synclavier II Software.   An important benefit
.of this change is the ability to easily change the button panel layout
.or the button panel hardware without having to dive into the Synclavier
.Program itself.  Of course, the main Synclavier program will need to
.be modified if new button features are added, or if some buttons want
.to operate differently on the new panel.
.
.  Some new terms are used, as follows:
.
.     1.  "Physical Button Number"   - abrev. PBN - A Phycial Button Number
.                                    is a 16-bit number that is used to
.                                    identify a particular button on the
.             *********              Synclavier II Panel.  Buttons are
.             * PBN's *              numbered beginning in the upper
.             *********              left hand corner, proceeding
.                                    across and then down each panel,
.                                    and then across the entire Synclavier
.                                    II.  (0-127, or 0-159)
.
.     2.  "Logical Parameter Number" - abrev. LPN - A Logical Parameter
.                                    number is a 16-bit number that is
.                                    used by the software to identify
.             *********              a certain software function.   
.             * LPN's *              LPN's closely match the button layout
.             *********              on the original Synclavier II button
.                                    panel.
.
.   A look-up table is used to map physical buttons into logical parameters.
.A second look-up table is used to map logical parameters back onto the
.button panel so that the correct  button can be lit.   Normally the tables
.are inverses of one another (so that pressing a button causes that button
.to light), although this need not be the case.   Currently, the LPN.LOOKUP
.is programmed in as a DATA array, while an initialization routine (PANEL.INIT)
.constructs the inverse table (PBN.LOOKUP).   The inverse table (PBN.LOOKUP) is
.also used to hold the current state of each logical parameter (on, off, blinking, etc.),
.which saves an extra array of memory.
.
.   To change the location of a particular button on the panel, the LPN.LOOKUP
.and the PBN.LOOKUP tables need to be modified.  The button will operate
.exactly as before only from a different location on the panel.
.
.   To change the button hardware, different driver routines are needed.
.Four routines interact with the panel hardware.  See following documentation
.on PANEL.SCAN, ON, BLINK, and OFF.
.
.   To change how a particular button operates, code in the main synclavier
.program will have to be modified.
.
. */


/* $PAGE - Logical Parameter Number definitions */


/* The following literals are available to help construct the LPN.LOOKUP */
/* table.                                                                */

#ifndef __SynclavierIP__SynclavierLPNLits__
#define __SynclavierIP__SynclavierLPNLits__
#define SynclavierLPN_undef_l         (  0)                         /*  undefined; not used; empty */
#define SynclavierLPN_psel_l          (  1)                         /*  4 partial select buttons */
#define SynclavierLPN_par_l           (  5)                         /* 64 basic parameters (for each partial) */
#define SynclavierLPN_mpar_l          ( 69)                         /* 64 misc parameter buttons - chorus, rate, overall stuff */
#define SynclavierLPN_seq_l           (133)                         /* 32 sequencer buttons (start, stop, etc.) */
#define SynclavierLPN_trak_l          (165)                         /* 32 track buttons */
#define SynclavierLPN_rte_l           (197)                         /* 32 rte buttons   */
#define SynclavierLPN_bank_l          (229)                         /*  8 bank select buttons */
#define SynclavierLPN_timb_l          (237)                         /*  8 timbre recall buttons */
#define SynclavierLPN_seqr_l          (245)                         /*  8 sequence recall buttons */
#define SynclavierLPN_misc_l          (253)                         /* 32 miscellaneous buttons   */

#define SynclavierLPN_num_lpns        (285)                         /* number of logical parameters - used to dimension inverse lookup table */

/* literals provided for parameter buttons: */

#define SynclavierLPN_ve_del_l        (SynclavierLPN_par_l+ 0)      /* volume envelope delay       lpn */
#define SynclavierLPN_ve_atk_l        (SynclavierLPN_par_l+ 1)      /* volume envelope attack      lpn */
#define SynclavierLPN_ve_idec_l       (SynclavierLPN_par_l+ 2)      /* volume envelope idecay      lpn */
#define SynclavierLPN_ve_fdec_l       (SynclavierLPN_par_l+ 3)      /* volume envelope fdecay      lpn */
#define SynclavierLPN_ve_peak_l       (SynclavierLPN_par_l+ 4)      /* volume envelope peak        lpn */
#define SynclavierLPN_ve_sust_l       (SynclavierLPN_par_l+ 5)      /* volume envelope sustain     lpn */

#define SynclavierLPN_he_del_l        (SynclavierLPN_par_l+ 6)      /* harmonic envelope delay     lpn */
#define SynclavierLPN_he_atk_l        (SynclavierLPN_par_l+ 7)      /* harmonic envelope attack    lpn */
#define SynclavierLPN_he_idec_l       (SynclavierLPN_par_l+ 8)      /* harmonic envelope idecay    lpn */
#define SynclavierLPN_he_fdec_l       (SynclavierLPN_par_l+ 9)      /* harmonic envelope fdecay    lpn */
#define SynclavierLPN_he_peak_l       (SynclavierLPN_par_l+10)      /* harmonic envelope peak      lpn */
#define SynclavierLPN_he_sust_l       (SynclavierLPN_par_l+11)      /* harmonic envelope sustain   lpn */

#define SynclavierLPN_coef_l          (SynclavierLPN_par_l+12)      /* coefficient                 lpns*/

#define SynclavierLPN_ptun_l          (SynclavierLPN_par_l+36)      /* partial tuning              lpn */
#define SynclavierLPN_vwave_l         (SynclavierLPN_par_l+37)      /* vibrato wave                lpn */
#define SynclavierLPN_vrate_l         (SynclavierLPN_par_l+38)      /* vibrato rate                lpn */
#define SynclavierLPN_vdepth_l        (SynclavierLPN_par_l+39)      /* vibrato depth               lpn */
#define SynclavierLPN_vatk_l          (SynclavierLPN_par_l+40)      /* vibrato attack              lpn */
#define SynclavierLPN_prate_l         (SynclavierLPN_par_l+42)      /* portamento rate             lpn */
#define SynclavierLPN_ratio_l         (SynclavierLPN_par_l+43)      /* fm ratio                    lpn */
#define SynclavierLPN_decay_l         (SynclavierLPN_par_l+44)      /* decay adjust                lpn */
#define SynclavierLPN_pchorus_l       (SynclavierLPN_par_l+45)      /* partial chorus              lpn */
#define SynclavierLPN_hadj_l          (SynclavierLPN_par_l+46)      /* harmonic adjust             lpn */
#define SynclavierLPN_stcen_l         (SynclavierLPN_par_l+47)      /* stereo center               lpn */
#define SynclavierLPN_stdepth_l       (SynclavierLPN_par_l+48)      /* stereo depth                lpn */
#define SynclavierLPN_stmode_l        (SynclavierLPN_par_l+49)      /* stereo mode                 lpn */
#define SynclavierLPN_strate_l        (SynclavierLPN_par_l+50)      /* stereo rate                 lpn */
#define SynclavierLPN_kbdlkey_l       (SynclavierLPN_par_l+51)      /* keyboard envelope left key  lpn */
#define SynclavierLPN_kbdrkey_l       (SynclavierLPN_par_l+52)      /* keyboard envelope right key lpn */
#define SynclavierLPN_kbdlsl_l        (SynclavierLPN_par_l+53)      /* keyboard envelope left slop lpn */
#define SynclavierLPN_kbdrsl_l        (SynclavierLPN_par_l+54)      /* keyboard envelope right slo lpn */
#define SynclavierLPN_pvol_l          (SynclavierLPN_par_l+55)      /* partial volume                  */
#define SynclavierLPN_moddep_l        (SynclavierLPN_par_l+56)      /* modulator depth                 */
#define SynclavierLPN_dynenvl_l       (SynclavierLPN_par_l+57)      /* dyn env low                     */
#define SynclavierLPN_dynenvh_l       (SynclavierLPN_par_l+58)      /* dyn env high                    */


/* $PAGE - literals provided for miscellaneous parameter buttons, sequencer buttons: */

/* POLY.L - RFIL.L:  OVERALL TIMBRE PARAMETER LPNS */

#define SynclavierLPN_poly_l          (SynclavierLPN_mpar_l+ 0)     /* keyboard polyphony          lpn */
#define SynclavierLPN_lpfc_l          (SynclavierLPN_mpar_l+ 1)     /* low pass cutoff             lpn */
#define SynclavierLPN_hpfc_l          (SynclavierLPN_mpar_l+ 2)     /* high pass cutoff            lpn */
#define SynclavierLPN_bpfc_l          (SynclavierLPN_mpar_l+ 3)     /* band pass cutoff            lpn */
#define SynclavierLPN_bwdth_l         (SynclavierLPN_mpar_l+ 4)     /* bandwidth                   lpn */
#define SynclavierLPN_rrate_l         (SynclavierLPN_mpar_l+ 5)     /* repeat rate                 lpn */
#define SynclavierLPN_chorus_l        (SynclavierLPN_mpar_l+ 6)     /* chorus                      lpn */
#define SynclavierLPN_tname_l         (SynclavierLPN_mpar_l+ 7)     /* timbre name                 lpn */
#define SynclavierLPN_ttben_l         (SynclavierLPN_mpar_l+ 8)     /* tone bend depth for timbre  lpn */
                                         /* leave room for possible kcv lpn */
#define SynclavierLPN_pfil_l          (SynclavierLPN_mpar_l+11)     /* pressure filter t.c.        lpn */
#define SynclavierLPN_rfil_l          (SynclavierLPN_mpar_l+12)     /* ribbon   filter t.c.        lpn */

/* SCALE.L - MARK.L:  SEQUENCER PARAMETER LPNS SAVED WITH SEQUENCES */

#define SynclavierLPN_scale_l         (SynclavierLPN_mpar_l+16)     /* sequencer scale adjust (12) lpn */
#define SynclavierLPN_speed_l         (SynclavierLPN_mpar_l+28)     /* speed                       lpn */
#define SynclavierLPN_click_l         (SynclavierLPN_mpar_l+29)     /* click rate                  lpn */
#define SynclavierLPN_crm_l           (SynclavierLPN_mpar_l+30)     /* click rate multiplier       lpn */
#define SynclavierLPN_smpte_l         (SynclavierLPN_mpar_l+31)     /* smpte (special case!!!)     lpn */
#define SynclavierLPN_bpm_l           (SynclavierLPN_mpar_l+32)     /* beats per measure           lpn */
#define SynclavierLPN_mark_l          (SynclavierLPN_mpar_l+33)     /* mark button (start time)    lpn */

/* TBASE.L - VKXPOS.L:  MISC GLOBAL SYSTEM PARAMETER LPNS (NOT SAVED) */

#define SynclavierLPN_tbase_l         (SynclavierLPN_mpar_l+34)     /* tuning base                 lpn */
#define SynclavierLPN_oratio_l        (SynclavierLPN_mpar_l+35)     /* octave ratio                lpn */
#define SynclavierLPN_tbdepth_l       (SynclavierLPN_mpar_l+36)     /* knob tone bend/depth        lpn */
#define SynclavierLPN_smin_l          (SynclavierLPN_mpar_l+37)     /* sensitivity min             lpn */
#define SynclavierLPN_scon_l          (SynclavierLPN_mpar_l+38)     /* sensitivity constant        lpn */
#define SynclavierLPN_sdelay_l        (SynclavierLPN_mpar_l+39)     /* sync delay                  lpn */
#define SynclavierLPN_nump_l          (SynclavierLPN_mpar_l+40)     /* numpoints for loop search   lpn */
#define SynclavierLPN_startup_l       (SynclavierLPN_mpar_l+41)     /* startup control             lpn */
#define SynclavierLPN_mwprox_l        (SynclavierLPN_mpar_l+42)     /* mod wheel proxy			   lpn */
#define SynclavierLPN_vkxpos_l        (SynclavierLPN_mpar_l+43)     /* keyboard transpose		   lpn */

/* PTTUN.L - LOOP.L:  SPECIAL CASE TIMBRE/SEQUENCE PARAMETER LPNS */

#define SynclavierLPN_pttun_l         (SynclavierLPN_mpar_l+44)     /* patch list file tuning      lpn */
#define SynclavierLPN_trrout_l        (SynclavierLPN_mpar_l+45)     /* track routing               lpn */
#define SynclavierLPN_trvol_l         (SynclavierLPN_mpar_l+46)     /* track volume                lpn */
#define SynclavierLPN_pttlen_l        (SynclavierLPN_mpar_l+47)     /* patch list total length     lpn */
#define SynclavierLPN_ptllen_l        (SynclavierLPN_mpar_l+48)     /* patch list loop  length     lpn */
#define SynclavierLPN_midi_l          (SynclavierLPN_mpar_l+49)     /* midi button             	   lpn */
#define SynclavierLPN_loop_l          (SynclavierLPN_mpar_l+50)     /* one of the loop/edit parms  lpn */
#define SynclavierLPN_clickval_l      (SynclavierLPN_mpar_l+51)     /* click value                 lpn */
#define SynclavierLPN_midixpos_l      (SynclavierLPN_mpar_l+52)     /* midi output transpose       lpn */

/* sequencer functions: */

#define SynclavierLPN_start_l         (SynclavierLPN_seq_l+ 0)       /* start                       lpn */
#define SynclavierLPN_stop_l          (SynclavierLPN_seq_l+ 1)       /* stop                        lpn */
#define SynclavierLPN_record_l        (SynclavierLPN_seq_l+ 2)       /* record                      lpn */
#define SynclavierLPN_punch_l         (SynclavierLPN_seq_l+ 3)       /* punch                       lpn */
#define SynclavierLPN_cont_l          (SynclavierLPN_seq_l+ 4)       /* continue                    lpn */
#define SynclavierLPN_rew_l           (SynclavierLPN_seq_l+ 5)       /* rewind                      lpn */
#define SynclavierLPN_ff_l            (SynclavierLPN_seq_l+ 6)       /* ff                          lpn */
#define SynclavierLPN_erase_l         (SynclavierLPN_seq_l+ 7)       /* erase  *** out of order *** lpn */
#define SynclavierLPN_eloop_l         (SynclavierLPN_seq_l+ 8)       /* end loop                    lpn */
#define SynclavierLPN_transp_l        (SynclavierLPN_seq_l+ 9)       /* transp                      lpn */
#define SynclavierLPN_bounce_l        (SynclavierLPN_seq_l+10)       /* bounce                      lpn */
#define SynclavierLPN_smt_l           (SynclavierLPN_seq_l+11)       /* smt                         lpn */
#define SynclavierLPN_skt_l           (SynclavierLPN_seq_l+12)       /* skt                         lpn */
#define SynclavierLPN_ext_l           (SynclavierLPN_seq_l+13)       /* ext sync                    lpn */
#define SynclavierLPN_just_l          (SynclavierLPN_seq_l+14)       /* justify                     lpn */
#define SynclavierLPN_sloop_l         (SynclavierLPN_seq_l+15)       /* start loop                  lpn */
#define SynclavierLPN_chain_l         (SynclavierLPN_seq_l+16)       /* chain                       lpn */
#define SynclavierLPN_insert_l        (SynclavierLPN_seq_l+17)       /* insert                      lpn */
#define SynclavierLPN_delete_l        (SynclavierLPN_seq_l+18)       /* delete                      lpn */
#define SynclavierLPN_trackpan_l      (SynclavierLPN_seq_l+19)       /* track pan                   lpn */

/* $PAGE - literals provided for Real Time Effects */

/* ORIGINAL KEYBOARD ONLY: */

#define SynclavierLPN_mem_l           (SynclavierLPN_rte_l+ 0)       /* memorize                    lpn */
#define SynclavierLPN_pedal_l         (SynclavierLPN_rte_l+ 1)       /* rte pedal                   lpn */
#define SynclavierLPN_vel_l           (SynclavierLPN_rte_l+ 2)       /* velocity                    lpn */
#define SynclavierLPN_pp1_l           (SynclavierLPN_rte_l+ 3)       /* patch partial 1             lpn */
#define SynclavierLPN_pp2_l           (SynclavierLPN_rte_l+ 4)       /* patch partial 2             lpn */
#define SynclavierLPN_pp3_l           (SynclavierLPN_rte_l+ 5)       /* patch partial 3             lpn */
#define SynclavierLPN_pp4_l           (SynclavierLPN_rte_l+ 6)       /* patch partial 4             lpn */
#define SynclavierLPN_recrdr_l        (SynclavierLPN_rte_l+ 7)       /* recorder button             lpn */
#define SynclavierLPN_rvatk_l         (SynclavierLPN_rte_l+ 8)       /* rte volume env attack       lpn */
#define SynclavierLPN_rvdec_l         (SynclavierLPN_rte_l+ 9)       /* rte volume env decays       lpn */
#define SynclavierLPN_rvpks_l         (SynclavierLPN_rte_l+10)       /* rte volume env peaks        lpn */
#define SynclavierLPN_rhatk_l         (SynclavierLPN_rte_l+11)       /* rte harmonic env attack     lpn */
#define SynclavierLPN_rhdec_l         (SynclavierLPN_rte_l+12)       /* rte harmonic env decay      lpn */
#define SynclavierLPN_rhpeak_l        (SynclavierLPN_rte_l+13)       /* rte harmonic env peak       lpn */
#define SynclavierLPN_rhsust_l        (SynclavierLPN_rte_l+14)       /* rte harmonic env sustain    lpn */
#define SynclavierLPN_rprate_l        (SynclavierLPN_rte_l+15)       /* rte portamento rate         lpn */
                          
/* Velocity/Pressure Keyboard only: */

#define SynclavierLPN_vkvel_l         (SynclavierLPN_rte_l+16)       /* vk - velocity button       */
#define SynclavierLPN_vkpre_l         (SynclavierLPN_rte_l+17)       /* vk - pressure button       */
#define SynclavierLPN_vkpe1_l         (SynclavierLPN_rte_l+18)       /* vk - pedal 1  button       */
#define SynclavierLPN_vkpe2_l         (SynclavierLPN_rte_l+19)       /* vk - pedal 2  button       */
#define SynclavierLPN_vkmod_l         (SynclavierLPN_rte_l+20)       /* vk - mod wh   button       */
#define SynclavierLPN_vkrib_l         (SynclavierLPN_rte_l+21)       /* vk - ribbon   button       */
#define SynclavierLPN_vkkcv_l         (SynclavierLPN_rte_l+22)       /* vk - kbd cv   button       */
#define SynclavierLPN_vkbth_l         (SynclavierLPN_rte_l+23)       /* vk - breath c button       */

#define SynclavierLPN_vkper_l         (SynclavierLPN_rte_l+24)       /* vk - perform  button       */
#define SynclavierLPN_vkrec_l         (SynclavierLPN_rte_l+25)       /* vk - recorder button       */
#define SynclavierLPN_vkove_l         (SynclavierLPN_rte_l+26)       /* vk - ovewrite button       */
#define SynclavierLPN_vkclr_l         (SynclavierLPN_rte_l+27)       /* vk - clear    button       */

/* $PAGE - literals provided for misc buttons */

#define SynclavierLPN_ponoff_l        (SynclavierLPN_misc_l+ 0)       /* portamento on/off           lpn */
#define SynclavierLPN_plogl_l         (SynclavierLPN_misc_l+ 1)       /* portamento log/lin          lpn */
#define SynclavierLPN_invert_l        (SynclavierLPN_misc_l+ 2)       /* vibrato invert              lpn */
#define SynclavierLPN_quant_l         (SynclavierLPN_misc_l+ 3)       /* vibrato quantize            lpn */
#define SynclavierLPN_raise_l         (SynclavierLPN_misc_l+ 4)       /* vibrato raise               lpn */

#define SynclavierLPN_lpt_l           (SynclavierLPN_misc_l+ 8)       /* low pass track              lpn */
#define SynclavierLPN_hpt_l           (SynclavierLPN_misc_l+ 9)       /* high pass track             lpn */
#define SynclavierLPN_bpt_l           (SynclavierLPN_misc_l+10)       /* band pass track             lpn */
#define SynclavierLPN_repeat_l        (SynclavierLPN_misc_l+11)       /* repeat function             lpn */
#define SynclavierLPN_arpeg_l         (SynclavierLPN_misc_l+12)       /* arpeggiate function         lpn */

#define SynclavierLPN_hgs1_l          (SynclavierLPN_misc_l+16)       /* harmonic group select #1    lpn */
#define SynclavierLPN_hgs2_l          (SynclavierLPN_misc_l+17)       /* harmonic group select #2    lpn */
#define SynclavierLPN_hgs_l           (SynclavierLPN_misc_l+18)       /* combined harmonic group select button on new panel */
#define SynclavierLPN_scale_a_l       (SynclavierLPN_misc_l+19)       /* scale adjust                lpn */

#define SynclavierLPN_bankb_l         (SynclavierLPN_misc_l+20)       /* bank button                 lpn */
#define SynclavierLPN_entryb_l        (SynclavierLPN_misc_l+21)       /* entry button                lpn */
#define SynclavierLPN_seqb_l          (SynclavierLPN_misc_l+22)       /* sequence button             lpn */

#define SynclavierLPN_bnkovwrt_l      (SynclavierLPN_misc_l+24)       /* bank overwrite              lpn */
#define SynclavierLPN_entrywrt_l      (SynclavierLPN_misc_l+25)       /* entry    write              lpn */
#define SynclavierLPN_dsel_l          (SynclavierLPN_misc_l+26)       /* drive select                lpn */
#define SynclavierLPN_split_l         (SynclavierLPN_misc_l+27)       /* split                       lpn */
#define SynclavierLPN_info_l          (SynclavierLPN_misc_l+28)       /* information button          lpn */
#define SynclavierLPN_slib_l          (SynclavierLPN_misc_l+29)       /* sequence library button     lpn */
#endif
