/*

This file represents all the activities necessary to bring
the Au1000 out of reset. It sets all Au1000 resources to
known, usually disabled and safe, state.

This is an example startup file, tailored for the Pb1000
reference board. Pb1000-specific items are commented as
such, but in general are confined to the CPU endian
selection and memory controller values.

*/

/********************************************************************/

/*
 * MIPS ABI register defintions
 */
#define zero	$0
#define v0		$2
#define v1		$3
#define a0		$4
#define a1		$5
#define a2		$6
#define a3		$7
#define t0		$8
#define t1		$9
#define t2		$10
#define t3		$11
#define t4		$12
#define t5		$13
#define t6		$14
#define t7		$15
#define s0		$16
#define s1		$17
#define s2		$18
#define s3		$19
#define s4		$20
#define s5		$21
#define s6		$22
#define s7		$23
#define t8		$24
#define t9		$25
#define k0		$26
#define k1		$27
#define gp		$28
#define sp		$29
#define fp		$30
// #define s8		$30 conflicts with MIPS.H
#define ra		$31

/********************************************************************/

/*
 * Au1000 CP0 registers
 */
#define CP0_Index		$0
#define CP0_Random		$1
#define CP0_EntryLo0	$2
#define CP0_EntryLo1	$3
#define CP0_Context		$4
#define CP0_PageMask	$5
#define CP0_Wired		$6
#define CP0_BadVAddr	$8
#define CP0_Count		$9
#define CP0_EntryHi		$10
#define CP0_Compare		$11
#define CP0_Status		$12
#define CP0_Cause		$13
#define CP0_EPC			$14
#define CP0_PRId		$15
#define CP0_Config		$16
#define CP0_Config0		$16
#define CP0_Config1		$16,1
#define CP0_LLAddr		$17
#define CP0_WatchLo		$18
#define CP0_IWatchLo	$18,1
#define CP0_WatchHi		$19
#define CP0_IWatchHi	$19,1
#define CP0_Scratch		$22
#define CP0_Debug		$23
#define CP0_DEPC		$24
#define CP0_PerfCnt		$25
#define CP0_PerfCtrl	$25,1
#define CP0_DTag		$28
#define CP0_DData		$28,1
#define CP0_ITag		$29
#define CP0_IData		$29,1
#define CP0_ErrorEPC	$30
#define CP0_DESave		$31

/********************************************************************/

/*
 * Au1000 base addresses (in KSEG1 region)
 */
#define AU1000_MEM_ADDR		0xB4000000

#define AU1000_AC97_ADDR	0xB0000000
#define AU1000_USBH_ADDR	0xB0100000
#define AU1000_USBD_ADDR	0xB0200000
#define AU1000_IRDA_ADDR	0xB0300000
#define AU1000_MACEN_ADDR	0xB0520000
#define AU1000_I2S_ADDR		0xB1000000
#define AU1000_UART0_ADDR	0xB1100000
#define AU1000_UART1_ADDR	0xB1200000
#define AU1000_UART2_ADDR	0xB1300000
#define AU1000_UART3_ADDR	0xB1400000
#define AU1000_SSI0_ADDR	0xB1600000
#define AU1000_SSI1_ADDR	0xB1680000
#define AU1000_SYS_ADDR		0xB1900000


/*
 * Au1000 memory controller register offsets
 */
#define mem_sdmode0		0x0000
#define mem_sdmode1		0x0004
#define mem_sdmode2		0x0008
#define mem_sdaddr0		0x000C
#define mem_sdaddr1		0x0010
#define mem_sdaddr2		0x0014
#define mem_sdrefcfg		0x0018
#define mem_sdprecmd		0x001C
#define mem_sdautoref		0x0020
#define mem_sdwrmd0		0x0024
#define mem_sdwrmd1		0x0028
#define mem_sdwrmd2		0x002C
#define mem_sdsleep		0x0030
#define mem_sdsmcke		0x0034

#define mem_stcfg0		0x1000
#define mem_sttime0		0x1004
#define mem_staddr0		0x1008
#define mem_stcfg1		0x1010
#define mem_sttime1		0x1014
#define mem_staddr1		0x1018
#define mem_stcfg2		0x1020
#define mem_sttime2		0x1024
#define mem_staddr2		0x1028
#define mem_stcfg3		0x1030
#define mem_sttime3		0x1034
#define mem_staddr3		0x1038

/*
 * Au1000 peripheral register offsets
 */
#define ac97_enable		0x0010
#define usbh_enable		0x0007FFFC
#define usbd_enable		0x0058
#define irda_enable		0x0040
#define macen_mac0		0x0000
#define macen_mac1		0x0004
#define i2s_enable		0x0008
#define uart_enable		0x0100
#define ssi_enable		0x0100

#define sys_scratch0		0x0018
#define sys_scratch1		0x001c
#define sys_cntctrl		0x0014
#define sys_freqctrl0		0x0020
#define sys_freqctrl1		0x0024
#define sys_clksrc		0x0028
#define sys_pinfunc		0x002C
#define sys_powerctrl		0x003C
#define sys_endian		0x0038
#define sys_wakesrc		0x005C
#define sys_cpupll		0x0060
#define sys_auxpll		0x0064
#define sys_trioutrd		0x0100
#define sys_trioutclr		0x0100
#define sys_outputrd		0x0108
#define sys_outputset		0x0108
#define sys_outputclr		0x010C
#define sys_pinstaterd		0x0110
#define sys_pininputen		0x0110


/********************************************************************/

/*
 * Pb1000-specific values
 * NOTE: All values are for operation at 396MHz, SD=2
 */

#define SYS_CPUPLL		33	/* 396MHz */
#define SYS_POWERCTRL		0	/* SD=2   */
#define SYS_AUXPLL		8	/* 96MHz  */


/* RCE0: 8MB AMD29D323 Flash */
#define MEM_STCFG0	0x00001403
#define MEM_STTIME0	0xFFFFFFDD
#define MEM_STADDR0	0x11F83FE0

/* RCE1: CPLD glue, no AuPCI
#define MEM_STCFG1	0x00000000
#define MEM_STTIME1	0x0002018A
#define MEM_STADDR1	0x11803E40
*/

/* RCE1: CPLD glue, with AuPCI */
#define MEM_STCFG1	0x00000083
#define MEM_STTIME1	0x33030A10
#define MEM_STADDR1	0x11803E40

/* RCE2: Epson SED1356 LCD */
#define MEM_STCFG2	0x00000004
#define MEM_STTIME2	0x08061908
#define MEM_STADDR2	0x12A03FC0

/* RCE3: PCMCIA */
#define MEM_STCFG3	0x00000002
#define MEM_STTIME3	0x280E3E07
#define MEM_STADDR3	0x10000000

/*
 * SDCS0 - Not used, for SMROM
 * SDCS1 - 32MB Micron 48LCBM16A2
 * SDCS2 - 32MB Micron 48LCBM16A2
 */
#define MEM_SDMODE0		0x00000000
#define MEM_SDMODE1		0x00552229
#define MEM_SDMODE2		0x00552229
#define MEM_SDADDR0		0x00000000
#define MEM_SDADDR1		0x001003F8
#define MEM_SDADDR2		0x001023F8
#define MEM_SDREFCFG_D		0x74000c30	/* disable */
#define MEM_SDREFCFG_E		0x76000c30	/* enable */
#define MEM_SDWRMD0		0x00000023
#define MEM_SDWRMD1		0x00000023
#define MEM_SDWRMD2		0x00000023

#define MEM_1MS			((396000000/1000000) * 1000)

/* we are slow enough when running from the EJTAG debug memory */
#undef MEM_1MS
#define MEM_1MS			1
	
/********************************************************************/
/********************************************************************/
/********************************************************************/
/********************************************************************/

	.text
	.set noreorder

	.global __start
__start:
	/*
	 * Step 1) Establish CPU endian mode.
	 * NOTE: A fair amount of code is necessary on the Pb1000 to
	 * obtain the value of Switch S8.1 which is used to determine
	 * endian at run-time.
	 */
	li		t0, AU1000_MEM_ADDR

	/* RCE1 */
	li		t1, MEM_STCFG1
	sw		t1, mem_stcfg1(t0)

	li		t1, MEM_STTIME1
	sw		t1, mem_sttime1(t0)

	li		t1, MEM_STADDR1
	sw		t1, mem_staddr1(t0)

	/* Set DSTRB bits so switch will read correctly */
	li		t1, 0xBE00000C
	lw		t2, 0(t1)
	or		t2, t2, 0x00000300
	sw		t2, 0(t1)

	/* Check switch setting */
	li		t1, 0xBE000014
	lw		t2, 0(t1)
	and		t2, t2, 0x00000100
	bne		t2, zero, big_endian
	nop

little_endian:	

	li		t0, AU1000_SYS_ADDR
	li		t1, 1
	sw		t1, sys_endian(t0)
	mfc0		t2, CP0_Config
	mtc0		t2, CP0_Config
	nop
	nop

big_endian:

	/*
	 * NOTE: Config0[BE] now reflects endian mode
	 */

	/*
	 * Step 2) Establish Status Register
	 * (set BEV, clear ERL, clear EXL, clear IE)
	 */
	li		t1, 0x00400000
	mtc0		t1, CP0_Status

	/*
	 * Step 3) Establish CP0 Config0
	 * (set OD, set K0=3)
	 */
	li		t1, 0x00080003
	mtc0		t1, CP0_Config0

	/*
	 * Step 4) Disable Watchpoint facilities
	 */
	li		t1, 0x00000000
	mtc0		t1, CP0_WatchLo
	mtc0		t1, CP0_IWatchLo

	/*
	 * Step 5) Disable the performance counters
	 */
	mtc0		zero, CP0_PerfCtrl
	nop

	/*
	 * Step 6) Establish EJTAG Debug register
	 */
	mtc0		zero, CP0_Debug
	nop

	/*
	 * Step 7) Establish Cause
	 * (set IV bit)
	 */
	li		t1, 0x00800000
	mtc0		t1, CP0_Cause

	/*
	 * Step 8) Initialize the caches
	 */
	li		t0, (16*1024)
	li		t1, 32
	li		t2, 0x80000000
	addu		t3, t0, t2
cacheloop:
	cache		0, 0(t2)
	cache		1, 0(t2)
	addu		t2, t1
	bne		t2, t3, cacheloop
	nop

	/*
	 * Step 9) Initialize the TLB
	 */
	li		t0, 0		 	# index value
	li		t1, 0x00000000 	# entryhi value
	li		t2, 32		   	# 32 entries
tlbloop:
	/* Probe TLB for matching EntryHi */
	mtc0		t1, CP0_EntryHi
	tlbp
	nop

	/* Examine Index[P], 1=no matching entry */
	mfc0		t3, CP0_Index
	li		t4, 0x80000000
	and		t3, t4, t3
	addiu		t1, t1, 1		# increment t1 (asid)
	beq		zero, t3, tlbloop
	nop

	/* Initialize the TLB entry */
	mtc0		t0, CP0_Index
	mtc0		zero, CP0_EntryLo0
	mtc0		zero, CP0_EntryLo1
	mtc0		zero, CP0_PageMask
	tlbwi

	/* Do it again */
	addiu		t0, t0, 1	
	bne		t0, t2, tlbloop
	nop

	/* Establish Wired (and Random) */
	mtc0		zero, CP0_Wired
	nop

	/*
	 * Step 10) Establish CPU PLL frequency
	 */
	li		t0, AU1000_SYS_ADDR
	li		t1, SYS_CPUPLL
	sw		t1, sys_cpupll(t0)
	sync
	nop
	nop

	/*
	 * Step 11) Establish system bus divider
	 */
	li		t1, SYS_POWERCTRL
	sw		t1, sys_powerctrl(t0)
	sync

	/*
	 * Step 12) Establish AUX PLL frequency
	 */
	li		t1, SYS_AUXPLL
	sw		t1, sys_auxpll(t0)
	sync

	/*
	 * Step 13) Start the 32kHz oscillator
	 */
	li		t1, 0x00000100
	sw		t1, sys_cntctrl(t0)
	sync

	/*
	 * Step 14) Initialize static memory controller
	 */
	li		t0, AU1000_MEM_ADDR

	/* RCE0 */
	li		t1, MEM_STCFG0
	sw		t1, mem_stcfg0(t0)

	li		t1, MEM_STTIME0
	sw		t1, mem_sttime0(t0)

	li		t1, MEM_STADDR0
	sw		t1, mem_staddr0(t0)

	/* RCE1 */
	li		t1, MEM_STCFG1
	sw		t1, mem_stcfg1(t0)

	li		t1, MEM_STTIME1
	sw		t1, mem_sttime1(t0)

	li		t1, MEM_STADDR1
	sw		t1, mem_staddr1(t0)

	/* RCE2 */
	li		t1, MEM_STCFG2
	sw		t1, mem_stcfg2(t0)

	li		t1, MEM_STTIME2
	sw		t1, mem_sttime2(t0)

	li		t1, MEM_STADDR2
	sw		t1, mem_staddr2(t0)

	/* RCE3 */
	li		t1, MEM_STCFG3
	sw		t1, mem_stcfg3(t0)

	li		t1, MEM_STTIME3
	sw		t1, mem_sttime3(t0)

	li		t1, MEM_STADDR3
	sw		t1, mem_staddr3(t0)

	sync

	/*
	 * Step 15) Set peripherals to a known state
	 */
	li		t0, AU1000_SYS_ADDR
	sw		zero, sys_freqctrl0(t0)
	sw		zero, sys_freqctrl1(t0)
	sw		zero, sys_clksrc(t0)
	sw		zero, sys_pininputen(t0)
	sync

	li		t0, AU1000_AC97_ADDR
	li		t1, 0x2
	sw		t1, ac97_enable(t0)
	sync

	li		t0, AU1000_USBH_ADDR
	li		t1, usbh_enable
	addu		t0, t1, t0
	sw		zero, 0(t0)
	sync

	li		t0, AU1000_USBD_ADDR
	sw		zero, usbd_enable(t0)
	sync

	li		t0, AU1000_IRDA_ADDR
	sw		zero, irda_enable(t0)
	sync

	li		t0, AU1000_MACEN_ADDR
	sw		zero, macen_mac0(t0)
	sw		zero, macen_mac1(t0)
	sync

	li		t0, AU1000_I2S_ADDR
	li		t1, 0x02
	sw		t1, i2s_enable(t0)

	li		t0, AU1000_UART0_ADDR
	sw		zero, uart_enable(t0)
	sync

	li		t0, AU1000_UART1_ADDR
	sw		zero, uart_enable(t0)
	sync

	li		t0, AU1000_UART2_ADDR
	sw		zero, uart_enable(t0)
	sync

	li		t0, AU1000_UART3_ADDR
	sw		zero, uart_enable(t0)
	sync

	li		t0, AU1000_SSI0_ADDR
	li		t1, 0x02
	sw		t1, ssi_enable(t0)
	sync

	li		t0, AU1000_SSI1_ADDR
	li		t1, 0x02
	sw		t1, ssi_enable(t0)
	sync

	/*
	 * Step 16) Determine cause of reset
	 */
	/* wait 10mS to debounce external signals */
	li		t1, MEM_1MS*10
1:	add		t1, -1
	bne		t1, zero, 1b
	nop

	li		t0, AU1000_SYS_ADDR
	lw		t1, sys_wakesrc(t0)

	/* Clear sys_wakesrc */
	//sw		zero, sys_wakesrc(t0)
	sync

	/* Check for Hardware Reset */
	andi		t2, t1, 0x01
	bne		zero, t2, hardwarereset
	nop

	/* Check for Sleep Wakeup */
	andi		t2, t1, 0x02
	bne		zero, t2, sleepwakeup
	nop

	/* Assume run-time reset */
	beq		zero, zero, runtimereset
	nop

/********************************************************************/

hardwarereset:

	/*
	 * Step 1) Initialize SDRAM
	 */
	bal		initSDRAM
	nop

	/*
	 * Step 2) Invoke application
	 */
	beq		zero, zero, alldone
	nop


/********************************************************************/

runtimereset:

	/*
	 * Step 1) Initialize SDRAM
	 */
	bal		initSDRAM
	nop

	/*
	 * Step 2) Invoke application
	 */
	beq		zero, zero, alldone
	nop


/********************************************************************/

sleepwakeup:

#if 1
	/*
	 * Step 1) Initialize SDRAM. The SDRAM must be in self-refresh mode.
	 */
	bal		wakeupSDRAM	
	nop

	/*
	 * Step 2) Invoke application
	 */
	la		t0, AU1000_SYS_ADDR
	lw		sp, sys_scratch0(t0)
	lw		ra, sys_scratch1(t0)

	jr		ra
	nop

#else

	/*
	 * Step 1) Initialize SDRAM
	 */
	bal		initSDRAM
	nop 

	/*
	 * Step 2) Invoke application
	 */
	beq		zero, zero, alldone
	nop
#endif


/********************************************************************/

	/*
	 * This routine initializes the SDRAM controller from Initial
	 * Power-up Reset or Running Reset.
	 */
initSDRAM:

	/* Only perform SDRAM init if running from ROM/Flash */
	addu		t2, ra, zero	/* preserve ra */
	bal		getPC
	nop
getPC:
	lui		t0, 0x1F00      /* ROM/flash address? */
	and		t1, t0, ra
	addu		ra, t2, zero	/* restore ra */
	bne		t0, t1, initSDRAMdone
	nop

	/* wait 1mS before setup */
	li		t1, MEM_1MS
1:	add		t1, -1
	bne		t1, zero, 1b
	nop

	li		t0, AU1000_MEM_ADDR
	li		t1, MEM_SDMODE0
	sw		t1, mem_sdmode0(t0)

	li		t1, MEM_SDMODE1
	sw		t1, mem_sdmode1(t0)

	li		t1, MEM_SDMODE2
	sw		t1, mem_sdmode2(t0)
	
	li		t1, MEM_SDADDR0
	sw		t1, mem_sdaddr0(t0)

	li		t1, MEM_SDADDR1
	sw		t1, mem_sdaddr1(t0)

	li		t1, MEM_SDADDR2
	sw		t1, mem_sdaddr2(t0)

	sync

	li		t1, MEM_SDREFCFG_D
	sw		t1, mem_sdrefcfg(t0)
	sync

	sw		zero, mem_sdprecmd(t0)
	sync

	sw		zero, mem_sdautoref(t0)
	sync

	sw		zero, mem_sdautoref(t0)
	sync

	li		t1, MEM_SDREFCFG_E
	sw		t1, mem_sdrefcfg(t0)
	sync

	li		t1, MEM_SDWRMD1
	sw		t1, mem_sdwrmd1(t0)		
	sync

	li		t1, MEM_SDWRMD2
	sw		t1, mem_sdwrmd2(t0)
	sync

	/* wait 1mS after setup */
	li		t1, MEM_1MS
1:	add		t1, -1
	bne		t1, zero, 1b
	nop

initSDRAMdone:
	jr		ra
	nop

/********************************************************************/

wakeupSDRAM:

	/*
	 * With SDRAM in self refresh mode, update the
	 * ADDR, MODE and refresh registers
	 */
	li		t0, AU1000_MEM_ADDR

	li		t1, MEM_SDMODE0
	sw		t1, mem_sdmode0(t0)

	li		t1, MEM_SDMODE1
	sw		t1, mem_sdmode1(t0)

	li		t1, MEM_SDMODE2
	sw		t1, mem_sdmode2(t0)
	
	li		t1, MEM_SDADDR0
	sw		t1, mem_sdaddr0(t0)

	li		t1, MEM_SDADDR1
	sw		t1, mem_sdaddr1(t0)

	li		t1, MEM_SDADDR2
	sw		t1, mem_sdaddr2(t0)

	sw		zero, mem_sdautoref(t0)

	li		t1, MEM_SDREFCFG_E
	sw		t1, mem_sdrefcfg(t0)

	sync

	jr		ra
	nop


/********************************************************************/

	.extern		_beginucopy
	.extern		_ucopylength
	
alldone:
	/*
	 * External and/or board-specific peripheral initialization
	 */

	bal	1f
	nop
	.word	_beginucopy
	.word	0x80000000
	.word	_ucopylength
1:
	lw	t0, 0(ra)
	lw	t1, 4(ra)
	lw	t2, 8(ra)
	or	t4, t1, zero

2:
        lw      t3, 0(t0)
        addiu   t2, t2, -4
        sw      t3, 0(t1)
        addiu   t0, t0, 4
        bgtz    t2, 2b
        addiu   t1, t1, 4
	/*
	 * Prepare to invoke application main()
	 */
        li      ra, 0xbfc00000
        jr      t4
        nop



/********************************************************************/
