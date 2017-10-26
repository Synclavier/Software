/* Samplits  $title  memory Management Literals for Poly/Mono Sampling Memory */
/* 

   Modified:
   15 Jun 1987    MWH   Add literals for multiple poly bins
   
*/


/* The poly synth wave memory is managed by using variable length
   blocks.   The following variables are used:

      psfirst - points to the sector of poly memory where the blocks
                start.  Is most likey one.   Starting at this
                sector,  and up through (but not including) pslast
                are a group of these variable length blocks.  Each
                block is an integer number of sectors long.
      pslast  - points to next available sector.
      psmax   - points to the end of the poly synth memory that is
                available.   Memory sector psmax-1 may be used.
      psfree  - indicates sectors of poly memory that are unused.
                some of these sectors may contain a file but that
                file is not being used at the moment.   A complete
                garbage collection will always yield psfree sectors.
      pshere  - a rotary pointer used to indicate where to start loading
                files and/or garbage collecting.   Provides least recently
                used algorithm.

   an identical algorithm (using msfirst, mslast, msmax, msfree) is
   used to manage blocks of attack buffers in external memory for
   the mono sampling code. */

/* Translated to C:     January 19, 2015 at 4:20:34 PM AST  */
/* Translator Version:  0.000          */

#include "XPL.h"

#define  poly_magic           12345                                     // Magic number restored when poly contains valid data
#define  poly_rev             54336                                     // Rev # for held memory - count up
#define  first_base           21                                        // Start poly mem blocks here

#define  psmaxbins            4                                         // Maximum number of poly bins per system
#define  psmaxpages           16                                        // Maximum number of 32mb pages of poly memory -> 512 mb total


/* $Subtitle  definitions for poly/mono memory blocks */

#define  bl_len               0                                         //   0 - Block length in sectors
#define  bl_users             1                                         //   1 - Number of patch list frames using this file/buffer
#define  bl_fname             2                                         //   2 - File name, for matching purposes

#define  bl_dev               6                                         //   6 - Original device we came from to detect same sound on different devices
#define  bl_sec               7                                         //   7 - Original sector

/* Info from sound file:   (must match timbre lits) */

#define  bl_keytc             20                                        // Holds keytc from sound file
#define  bl_vrate             21                                        // Holds vrate from sound file
#define  bl_vdepth            22                                        // Holds vib depth
#define  bl_vat               23                                        // Holds vibrato attack time

/* Precomputed pitch info: */

#define  bl_srate             24                                        // Holds file sample rate correction factor
#define  bl_pitch             25                                        // Set octave pitch correction factor
#define  bl_toffs             26                                        // Holds octave correction with transposition

/* Mono sampling only:     */

#define  bl_start             27                                        // Start of valid data from sector+1 of block
#define  bl_abufl             28                                        // Attack buffer len words
#define  bl_wdd               29                                        // W. Disk device
#define  bl_wds               30                                        // W. Disk start sector
#define  bl_wdn               31                                        // W. Disk # of sectors
#define  bl_fwrds             32                                        // Words in final sector

/* Poly sampling only:     */

#define  bl_wtabl             27                                        // holds number of complete sectors of audio data that should be heard
                                                                        // holds number of additional words of audio data that should be heard
#define  bl_sofs              29                                        // hold sector offset from sfile.base of sector that contains the first sample to be heard
#define  bl_wofs              30                                        // holds word offset within the BL.SOFS sector of first sample that should be heard
#define  bl_loopl             31                                        // holds duplicate copy of BL.WTABL. not really used. massaged in PT.LOOPL.
#define  bl_onloop            33                                        // holds a 1; massaged to 3 in PT.ONLOOP if a loop is required
#define  bl_max               34                                        // holds duplicate copy of BL.WTABL. not really used. massaged in PT.MAX
#define  bl_check             36                                        // hold checksum
#define  bl_sftl              37                                        // holds sound file total length from sound file header, if any
#define  bl_sfll              39                                        // holds sound file loop  length from sound file header, if any
#define  bl_stereo            41                                        // true if stereo
#define  bl_khz               42                                        // khz*10 for this file

#define  bl_copyl             43                                        // Number of words to copy into partial

#define  bl_saved             43                                        // Poly sampling only - true if file saved

#define  bl_lname             63                                        // POLY SAMPLING ONLY - LONG FILE NAME

#define  bl_mono              256                                       //  256 - Start mono sampling data here
#define  bl_poly              1024                                      // 1024 - Start poly sampling data here

#define  sf_hdr               1                                         // offset of sound file header (in sectors) from start of allocated memory when file is in poly memory
                                                                        // also equals length of sound file header (in sectors)

#define  sf_sym               2                                         // offset of sound file symbol table (in sectors) from start of allocated memory when file is in poly memory
                                                                        // also equals length of symbol table area (in sectors)

/* Sound file cache header */

#define  bl_w0ptr             16                                        // Pointer to beginning of w0 area in cache
#define  bl_w1ptr             17                                        // Pointer to beginning of w1 area in cache

#define  bl_filecount         18                                        // No. Sound files in cache
#define  bl_catcount          19                                        // No. Categories in cache
#define  bl_cacheptr          20                                        // No. Words in cache
#define  bl_lines             21                                        // Start of line count array
#define  bl_linesmax          16                                        // Size of line count array

/* $Subtitle  literal Dcls for Sound File Header Information */


#define  max_symbols          64                                        // maximum number of symbols in the sound file symbol table
#define  symbol_length        8                                         // symbol description is eight words long
#define  name_entry           3                                         // point to the name entry location

/* sound file header information */

#define  sf_compatibility     0                                         // sound file revision where
#define  sf_file_data_type    1                                         // data type of the sound file where
#define  sf_valid_data        2                                         // number of data points in file (sector_msb, sector_lsb, words)
#define  sf_total_data        6                                         // total allocated data length (sector_msb, sector_lsb, words) should be sf.valid.data rounded up to the next sector
#define  sf_data_end          9                                         // time corresponding to valid.data-1 (seconds, milliseconds, microseconds)
#define  sf_keyboard_decay_number   12                                  // a time value in milliseconds.
#define  sf_semitones         13                                        // pitch bend range in semitones
#define  sf_vibrato_rate      14                                        // Hz*100; Vibrato is the periodic variation in pitch.
#define  sf_vibrato_depth     15                                        // semitones*100; pitch range for Random wave shapes.
#define  sf_vibrato_attack    16                                        // vibrato wave attack time in milliseconds.
#define  sf_vibrato_type      17                                        /* twelve possible vibrato wave shapes                                  */  
#define  sf_hertz             18                                        // 10*pitch frequency (i.e., 4400); The number of samples per second.
#define  sf_octave            19                                        // floating point keyboard octave.cents set by user.
#define  sf_period_index      21                                        // the number of clock ticks per sampling period
#define  sf_nyquist_freq      22                                        /* in Hz.                                                               */ 
#define  sf_mark_start        23                                        // marks where the sound begins.
#define  sf_mark_end          26                                        // marks where the sound ends.
#define  sf_cursor_time       29                                        // the time of the current cursor location.
#define  sf_gain_exponent     32                                        // for filters, the scale factor which scales the output in steps of 6 dB
#define  sf_number_of_symbols 33                                        // number of symbols in sound file
#define  sf_total_length      34                                        // perfect looping file length in 24-bit format
#define  sf_loop_length       37                                        // perfect loop length in 24-bit format
#define  sf_magic_number      40                                        // used to detect possible changes in file in SFM
#define  sf_stereo            41                                        // indicates if stereo or not 1-> stereo
#define  sf_sample_rate       42                                        // sound file sampling rate in kHz * 10
#define  sf_smpte_mode        45                                        // Smpte mode
#define  sf_smpte_bits        46                                        // Smpte start time bits
#define  sf_smpte_secs_fra    47                                        // Smpte start time of file
#define  sf_smpte_hrs_min     48                                        // Smpte start time of sound file
#define  sf_mark_offset       49                                        // time corresponding to SMPTE offset in sound file
#define  sf_index_base        52                                        // start of category index
#define  sf_file_handle       53                                        // start of file handle name
#define  sf_file_handle_wl    64                                        // words of file handle
#define  sf_file_handle_bl    128                                       // bytes of file handle. the handle is limited to 127 nonzero bytes so a legit c string can fit in the 128-byte buffer
#define  sf_id_field_bytes    127                                       // byte count of sound file caption
#define  sf_id_field          128                                       // word string of user data or file caption.
#define  sf_id_field_bl       256                                       // bytes of caption, max, assuming no categories.
