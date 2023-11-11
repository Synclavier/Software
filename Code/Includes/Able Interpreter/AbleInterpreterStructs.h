// Interpreter working variables: and data structures

// The following struct holds variables that are shared between C and
// PowerPC assembler.  The first 25 are loaded directly into PowerPC
// registers at the start of assembly language interpretation.

// The assembly language image of this struct is defined
// in AbleInterpreterPPC.s

// Note for OS X Implementation

//	During initialization of the interpreter a local regs struct is filled to reflect a booting
//	machine. The struct at this point will have no pointer variables set.

//  The local regs struct is stuffed into the kernel/user shared memory, and the kernel
//  extracts if from there and substitutes appropriate kernel memory poitners to the shared
//  simulation memory spaces (internal memory, poly memory, etc.).

//  During actual interpretation in the kernel the regs struct in the shared memory is
//	not updated (except during a fatal error for debugging purposes). That is, if the kernel
//  driver is present all interpretation is done in the kernel whether or not the hardware
//  is actually available.

#ifndef ABLE_INTERPRETER_STRUCTS_H
#define ABLE_INTERPRETER_STRUCTS_H

// Provide padding on Intel platforms to provide compatibility between 32-bit and 64-bit compilations.
// This is easy on intel because we don't have any assembly language.
// PowerPC assembly language offsets (and code) would have to change to implement 64 bit powerpc.

#if defined(__i386__) && !__LP64__
	#define	DO_PAD(NAME) int NAME;
#else
	#define	DO_PAD(NAME);
#endif

#pragma pack(push,2)

typedef	struct	able_registers				/* struct for accessing able register	*/
{											/* simulation quickly					*/
	unsigned	short*	our_pc;				/* abs working pointer to current instr.*/
	DO_PAD(pad1)
		
	unsigned	int		source;				/* extracted source field				*/
	unsigned	int		dest;				/* extracted destinatin field			*/
	unsigned	int		dr;					/* data register						*/
	unsigned	int		mask;				/* holds a handy mask					*/
		
	unsigned	short*	imem;				/* pointer to page of instruct mem		*/
	DO_PAD(pad2)
	unsigned	short*	memory;				/* pointer to page 0 of memory 			*/
	DO_PAD(pad3)
	unsigned	short*	pc_ptr;				/* pointer to PC in use					*/
	DO_PAD(pad4)

	unsigned	short	simw0;			    /* filler - nonzero if simulated W0     */
	unsigned	short	ppc;				/* holds prior program counter			*/
	            short*	d60_addr;			/* working pointer to ext mem space		*/
	DO_PAD(pad5)
	            
	unsigned 	int		rpc;				/* holds repeat counter					*/
	
	unsigned	short	simw0_ms;           /* sectors start MSB where write is OK  */
	unsigned	short	reg0;				/* access unsigned registers			*/
	unsigned	short	simw0_ls;           /* sectors start LSB where write is OK  */
	unsigned	short	reg1;
	unsigned	short	spare2;
	unsigned	short	reg2;
	unsigned	short	spare3;
	unsigned	short	reg3;

	unsigned	short	spare4;
	unsigned	short	reg4;
	unsigned	short	spare5;
	unsigned	short	reg5;
	unsigned	short	spare6;
	unsigned	short	reg6;
	unsigned	short	spare7;
	unsigned	short	reg7;

	unsigned	short	spare10;
	unsigned	short	reg10;
	unsigned	short	spare11;
	unsigned	short	reg11;
	unsigned	short	spare12;
	unsigned	short	reg12;
	unsigned	short	spare13;
	unsigned	short	reg13;

	unsigned	short	spare14;
	unsigned	short	reg14;
	unsigned	short	spare15;
	unsigned	short	reg15;
	unsigned	short	spare16;
	unsigned	short	reg16;
	
	unsigned	short	sparepc;
	unsigned	short	pc;

	unsigned	short	sparez;
	unsigned	short	zflags;				/* 0 for Z flag; 0x8000 for minus flag	*/

	unsigned	short	sparec;
	unsigned 	short	carry;				/* 0 = no carry; 1 = carry				*/
	
	unsigned	short	sparepreg;
	unsigned	short	preg;

	_mul_div_union	 	d4567;				/* our own d4567						*/
	
	fixed				*d60;				/* pointer to d60 memory space			*/
	DO_PAD(pad6)
	unsigned 	int		xmem_size;			/* size of external memory (bytes)		*/
	fixed				*d60_max;			/* max pointer for bounds check			*/
	DO_PAD(pad7)
	int					swap_base;			/* base sector of swap file				*/
	unsigned	short	sparepsw;
	unsigned 	short	psw;				/* d1 processor status word				*/
	
	volatile	unsigned 	int* able_0_fifo;		/* PCI address of Able-0 PCI-1 cmd fifo	*/
	DO_PAD(pad8)
	volatile	unsigned 	int* able_0_tlimreg;	/* PCI address of Able-0 PCI-1 tlim reg */
	DO_PAD(pad9)

	unsigned 	int		fast_time;			/* TLim register for fast devices		*/
	unsigned 	int		scsi_time;			/* TLim register for scsi devices		*/
	unsigned 	int		slow_time;			/* TLim register for slow devices		*/
	
				short*	d117_read_data;		/* read (d117) reads from here 			*/
				DO_PAD(pad10)
				short*	d117_writ_data;		/* write(d117) writes to here			*/
				DO_PAD(pad11)
	
				char*	d50_write_data;		// D50 writes data to here
				DO_PAD(pad12)
	
	unsigned	int		d50_to_wrap;		// counts down to buffer wrap
	
	unsigned	int		check_time;			/* set true when timer interrupt occurs	*/
	unsigned	int		check_desired;		/* check ints in main loop				*/

				int		d03_count;			// count of d03 interrupts pending
				int		d16_count;			// count of d16 interrupts pending
				int		tto_int;			// true if tto output int pending
				int		tti_int;			// true if tti input int pending
				int		d115_int;			// true if d115 input int pending

	unsigned 	int		d54_enabled_int;	// bits for d54 interrupt enabled devices
	unsigned	int		d54_pending_int;	// bits for d54 interrupt devices with pending interrupts

	short		fill0;
	ufixed		pri_enc;					// priority encoder value
	
	short		fill1;
	ufixed		d51_hardware_device_bits;	// D51 bits set in actual hardware
	short		fill2;
	ufixed		d51_software_device_bits;	// D51 bits set because we simulate the device

	short		fill3;
	ufixed		d57_hardware_device_bits;	// D57 bits set in actual hardware
	short		fill4;
	ufixed		d57_software_device_bits;	// D57 bits set because we simulate the device

    int			rtp_is_running;				// true if RTP is running (VS other applications); 2 if SFM sampling to/from disk
	
	fixed		*sim_pmem_ptr;				// working pointer for simulated poly mem; points to last word fetched/stored
	DO_PAD(pad13)

	fixed		*was_sim_pmem_prior;		// unused copy of sim_pmem_ptr saved when channel/function changes
	DO_PAD(pad14)
	
	int			tlim_is_not_fast;			// nonzero if tlim register has been left in slow or scsi time
	int			d130_data;					// last data written to d130
	int			d131_data;					// last data written to d131

	unsigned 	int  next_atick_time;       // tbr lsb of next millisecond boundary
	
	unsigned 	int  devreads [128];		// PCI address for device reads
	unsigned 	int  devwrites[128];		// PCI address for device writes
	
	unsigned	char  penc_data[256];		// priority encoder lookup table

}	able_registers;

#pragma pack(pop)

#endif

