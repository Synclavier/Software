/* Cplits   $title  Communication Processor Definitions   Modified:   04/27/89 - MWH - Support for multiple D115 boards & channels; timeouts   12/01/88 - JSS - New diagnostic opcode, int. en. opcode name changes   10/11/88 - MWH - New literals for hardware additions   10/03/88 - KJC - Author   *//* Hardware specific limits */#define	max_d115d_bds		4					/* Maximum number of boards in a system				*//* Communication processor device addresses */#define	cpsel				0x004D				/* 115 - Communication processor select				*/#define	cpcmd				0x004E				/* 116 - Communication processor command (write)	*/#define	cpstat				0x004E				/* 116 - Communication processor status (read)		*/#define	cpdata				0x004F				/* 117 - Communication processor data				*//* Communication processor commands (op-codes to write to cpcmd) */#define	cp_nop				0x0000				/* No operation										*/#define	cp_reset_68k		0x0001				/* Set "reset 68K"									*/#define	cp_reset_fifo		0x0002				/* Set "reset FIFO"									*/#define	cp_unreset_fifo		0x0003				/* Unset "reset FIFO"								*/#define	cp_unreset_68k		0x0004				/* Unset "reset 68K"								*/#define	cp_disable			0x0005				/* Disable all interrupts							*/#define	cp_decrement		0x0006				/* Decrement ABLE Rx frame counter					*/#define	cp_increment		0x0007				/* Increment ABLE Tx frame counter					*/#define	cp_disable_rx		0x0008				/* Disable Rx interrupts							*/#define	cp_enable_rx_a		0x0009				/* Enable Rx interrupt on condition A (message present)					*/#define	cp_loop_back		0x000B				/* Xfer one word from Tx to Rx FIFO (diagnostic)						*/#define	cp_disable_tx		0x000C				/* Disable Tx interrupts												*/#define	cp_enable_tx_c		0x000D				/* Enable Tx interrupt on condition C (transition to FIFO < 1/2 full)	*/#define	cp_enable_tx_d		0x000E				/* Enable Tx interrupt on condition D (FIFO < 1/2 full)					*/#define	cp_enable_tx_e		0x000F				/* Enable Tx interrupt on condition E (frame-counter <> 0)				*//* communication processor statuses (data bits read from cpstat) */#define	cp_data_present		0x0001				/* Data present in FIFO							*/#define	cp_packet_present	0x0002				/* Packet present in FIFO						*/#define	cp_rx_interrupt		0x0004				/* Receive interrupt							*/#define	cp_tx_half_full		0x0008				/* Transmit FIFO half full						*/#define	cp_tx_full			0x0010				/* Transmit FIFO full							*/#define	cp_tx_interrupt		0x0020				/* Transmit interrupt							*/#define	cp_bd0_interrupt	0x0100				/* Board 0 is interrupting						*/#define	cp_bd1_interrupt	0x0200				/* Board 1 is interrupting						*/#define	cp_bd2_interrupt	0x0400				/* Board 2 is interrupting						*/#define	cp_bd3_interrupt	0x0800				/* Board 3 is interrupting						*//* select register literals (write to cpsel) */#define	cp_sel_group_20		0x0010				/* First group of boards						*/#define	cp_sel_group_40		0x0020				/* Second group of boards						*/