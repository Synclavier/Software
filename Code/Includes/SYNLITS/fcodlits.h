/* Fcodlits  $title  function Code Literals for FM and PolySamp Synthesizers */

/*
   9/15/86 - Cj - changed poly rev for mouse playback
*/

/*  Synthesizer definitions: original syncl ii fm synthesizer
  
 the synthesizer is accessed by devices 160,161, and 163.  A channel number
 (0-63 for quad synth systems) is written to device 160.  A function code
 is then written to device 161.  There are 16 function codes, some of which
 are not used.  An 8-bit data word is then written to device 162 */

#define	cha				0x0070					/* Write channel #								*/
#define	fun				0x0071					/* Write function code							*/
#define	dat				0x0072					/* Write data here								*/

/* Function code definitions: */

#define	inc				0x0000					/* Increment									*/
#define	nnu				0x0001					/* Note number									*/
#define	onu				0x0002					/* Octave number								*/
#define	tim				0x0004					/* Timbre bits									*/
#define	zma				0x0006					/* Zero mar										*/
#define	mem				0x0007					/* Wave memory									*/

/* Even channels only: */

#define	idiv			0x0008					/* Index interp divisor							*/
#define	iadd			0x0009					/* Index adder number							*/
#define	ilim			0x000A					/* Index limit									*/
#define	ishc			0x000B					/* Index shift count							*/

/* Odd channels only: */

#define	ediv			0x0008					/* Env divisor									*/
#define	eadd			0x0009					/* Env adder number								*/
#define	elim			0x000A					/* Env limit									*/
#define	vol				0x000B					/* Volume										*/


/* $Subtitle  synth function code definitions for poly synth */


/* Poly synth io: */

#define	psc				0x006D					/* Write channel #								*/
#define	psf				0x006E					/* Write function code							*/
#define	psd				0x006F					/* Write data here								*/
#define	psx				0x006C					/* Write extended data here						*/

#define	pswl			 0						/* Fc  0 - minus wave table length				*/  // incr to pswl+1, then to psba
#define	psba			 2						/* Fc  2 - wave table base address				*/  // incr to psba+1, then to psll
#define	psll			 4						/* Fc  4 - minus loop length					*/  // incr to psll+1, then to pspi
#define	pspi			 6						/* Fc  6 - phase increment						*/  // incr to pspi+1, then to psinc
#define	psinc			 8						/* Fc  8 - frequency incrment					*/  // incr to psmod
#define	psmod			 9						/* Fc  9 - frequency modulus					*/  // incr to pson
#define	pson			10						/* Fc 10 - on/off								*/  // incr back to pswl

#define	psron			11						/* Fc 11 - read on/off bits (ugh!)				*/
#define	psnumv			12						/* Fc 12 - read approx # of da cards			*/

// Long play -
// To write memory, computer writes chanel then function = pswma.
// Then computer writes the first address to PSD; that increments function code to pswma_1
// Then computer writes the second address to PSD; that increments function code to psmwa_2

#define	pswma			16						/* Fc 16 - write memory address					*/
#define	pswma_1			17
#define	pswma_2			18                      /* Fc 18 - logger refers to this as cwdata      */  // Continue with data. Pick up with PSD data transfers from where we left off

#define	psrma			20						/* Fc 20 - read memory  address					*/
#define	psrma_1			21
#define	psrma_2			22

#define	pspat           24                      /* Fc 24 - phase accumulator test    24->24     */
#define	pscat           25                      /* Fc 25 - comp addr high test       25->0      */

#define	pstmod          26                      /* Fc 26 - test mode for PST board   26->0      */
#define	pstwen          27                      /* Fc 27 - write envelope channel    27->28     */
#define	pstren          28                      /* Fc 28 - read envelope channel     28->28     */

// Envelope delay counter:  00500 counts in  20000 microseconds
// Envelope delay counter:  00050 counts in  02000 microseconds
// Envelope delay counter increments at 25 khz rate until 4095 is reached.

// Enveope interpolator documentation:

//	A 12 bit envelope/delay counter increments at a 25 khz. rate. The contents of this
// counter can be read via the PSECNT function code. When the counter reaches 4096 it
// wraps to the PSEMOD value. That is, the PSECNT register acts as a divide-by-N counter
// where N equals 4096 - the PSEMOD value.

// When the PSECNT register wraps, the 12-bit PSEACU register is incremented by the
// PSEINC value. When it wraps, it's remainder is left intact (e.g. it accumulates).
// Also when the PSEINC register wraps, the envelope curent value (PSEVAL) is moved towards
// the envelope current limit (PSELIM) by an amount equal to PSEDEL.

#define	psecnt			32						/* Fc 32 - envelope/delay counter				*/  // incr to pseacu
#define	psemod			33						/* Fc 33 - envelope rate modulus				*/  // incr to pseinc
#define	pseacu			34						/* Fc 34 - envelope accumulator					*/  // incr to pselim
#define	pseinc			35						/* Fc 35 - envelope increment					*/  // incr to psedel
#define	pselim			36						/* Fc 36 - envelope limit 0-4095				*/  // incr to pseval
#define	pseval			37						/* Fc 37 - envelope current val					*/  // incr to psemod
#define	psedel			38						/* Fc 38 - envelope current delta				*/  // incr to psecnt

// The hardware constantly moves the current value towards the destination value at a rate equal to 1/512 the difference
// between the destination value and the current value.
// Reaches 4095 after 1838 counts or 73.520  msecs
#define	psrcvol			40						/* Fc 40 - right cur  vol						*/  // incr to pslcvol
#define	psrdvol			41						/* Fc 41 - right dest vol						*/  // incr to psldvol
#define	pslcvol			42						/* Fc 42 - left  cur  vol						*/  // incr to psrdvol
#define	psldvol			43						/* Fc 43 - left  dest vol						*/  // incr to psrcvol

#define	psinhon			44						/* Fc 44 - inhibit on							*/

/* A to D controller function codes */

#define	psadmode		52						/*  Fc 52 - set mode for sampling				*/
#define	psadvol			53						/*  Fc 53 - write volume						*/
#define	psadact			54						/*  Fc 54 - activate channel					*/
#define	psadid			55						/*  Fc 55 - identify							*/
#define	psadsc0			56						/*  Fc 56 - read sample count of chan 0			*/
#define	psadsc32		57						/*  Fc 57 - read sample count of chan 32		*/
#define	psadmc			58						/*  Fc 58 - max number of sampling chans		*/
#define	psadevsm		59						/*  Fc 59 - even stereo master					*/
#define	psadenv			60						/*  Fc 60 - envelope							*/

/* D to A controller function codes */

#define	psmute			64						/*  Fc 64 - mute channel						*/
#define	pscmap			65						/*  Fc 65 - channel map							*/
#define	psrpeak			66						/*  Fc 66 - read peak							*/
#define psflast         67

/* D34gpi output bits: */

#define	b_preroll		10						/* D34gpi bit numbers for record triggers		*/
#define	b_rec_in		 9
#define	b_rec_out		11
