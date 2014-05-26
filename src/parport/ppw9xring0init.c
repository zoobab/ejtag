/*****************************************************************************/

/*
 *      ppw9xring0init.c  --  Parport direct access under Win9x using Ring0 hack (init routines).
 *
 *      Copyright (C) 1998-2000  Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Please note that the GPL allows you to use the driver, NOT the radio.
 *  In order to use the radio, you need a license from the communications
 *  authority of your country.
 *
 */

/*****************************************************************************/

#if defined(HAVE_SYS_IO_H)
#include <sys/io.h>
#elif defined(HAVE_ASM_IO_H)
#include <asm/io.h>
#endif

#include <errno.h>
#include <string.h>

#include "parport.h"

/* ---------------------------------------------------------------------- */

#ifndef HAVE_IOPL
#ifdef HAVE_WINDOWS_H
#include <windows.h>
extern inline int iopl(unsigned int level)
{
        OSVERSIONINFO info;

        info.dwOSVersionInfoSize = sizeof(info);
        if (GetVersionEx(&info) &&
            (info.dwPlatformId == VER_PLATFORM_WIN32s ||
             info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
                return 0;
        return 1;
}
#else
extern inline int iopl(unsigned int level)
{
        return 0;
}
#endif
#endif

/* ---------------------------------------------------------------------- */

/* LPT registers */
/* ECP specific registers */
#define LPTREG_ECONTROL       0x402
#define LPTREG_CONFIGB        0x401
#define LPTREG_CONFIGA        0x400
#define LPTREG_TFIFO          0x400
#define LPTREG_DFIFO          0x400
#define LPTREG_AFIFO          0x000
#define LPTREG_DSR            0x001
#define LPTREG_DCR            0x002
/* EPP specific registers */
#define LPTREG_EPPDATA        0x004
#define LPTREG_EPPADDR        0x003
/* standard registers */
#define LPTREG_CONTROL        0x002
#define LPTREG_STATUS         0x001
#define LPTREG_DATA           0x000

/* ECP config A */
#define LPTCFGA_INTRISALEVEL  0x80
#define LPTCFGA_IMPIDMASK     0x70
#define LPTCFGA_IMPID16BIT    0x00
#define LPTCFGA_IMPID8BIT     0x10
#define LPTCFGA_IMPID32BIT    0x20
#define LPTCFGA_NOPIPELINE    0x04
#define LPTCFGA_PWORDCOUNT    0x03

/* ECP config B */
#define LPTCFGB_COMPRESS      0x80
#define LPTCFGB_INTRVALUE     0x40
#define LPTCFGB_IRQMASK       0x38
#define LPTCFGB_IRQ5          0x38
#define LPTCFGB_IRQ15         0x30
#define LPTCFGB_IRQ14         0x28
#define LPTCFGB_IRQ11         0x20
#define LPTCFGB_IRQ10         0x18
#define LPTCFGB_IRQ9          0x10
#define LPTCFGB_IRQ7          0x08
#define LPTCFGB_IRQJUMPER     0x00
#define LPTCFGB_DMAMASK       0x07
#define LPTCFGB_DMA7          0x07
#define LPTCFGB_DMA6          0x06
#define LPTCFGB_DMA5          0x05
#define LPTCFGB_DMAJUMPER16   0x04
#define LPTCFGB_DMA3          0x03
#define LPTCFGB_DMA2          0x02
#define LPTCFGB_DMA1          0x01
#define LPTCFGB_DMAJUMPER8    0x00

/* ECP ECR */
#define LPTECR_MODEMASK       0xe0
#define LPTECR_MODESPP        0x00
#define LPTECR_MODEPS2        0x20
#define LPTECR_MODESPPFIFO    0x40
#define LPTECR_MODEECP        0x60
#define LPTECR_MODEECPEPP     0x80
#define LPTECR_MODETEST       0xc0
#define LPTECR_MODECFG        0xe0
#define LPTECR_NERRINTRDIS    0x10
#define LPTECR_DMAEN          0x08
#define LPTECR_SERVICEINTR    0x04
#define LPTECR_FIFOFULL       0x02
#define LPTECR_FIFOEMPTY      0x01

/* ---------------------------------------------------------------------- */

extern unsigned int pp_w9xring0_iobase;
extern unsigned int pp_w9xring0_flags;
extern const struct parport_ops parport_w9xring0_ops, parport_w9xring0_emul_ops;

extern unsigned char ring0_inb(unsigned int port);
extern void ring0_outb(unsigned char val, unsigned int port);
extern void ring0_outsb(unsigned int port, const unsigned char *bp, unsigned int count);
extern void ring0_insb(unsigned int port, unsigned char *bp, unsigned int count);
extern int pp_w9xring0_epp_clear_timeout(void);

#define FLAGS_PCSPP              (1<<0)
#define FLAGS_PCPS2              (1<<1)
#define FLAGS_PCEPP              (1<<2)
#define FLAGS_PCECR              (1<<3)  /* ECR Register Exists */
#define FLAGS_PCECP              (1<<4)
#define FLAGS_PCECPEPP           (1<<5)
#define FLAGS_PCECPPS2           (1<<6)

/* ---------------------------------------------------------------------- */
#if 0
#ifndef EPPEMUL

static int detect_epp(void)
{
	/* pulse PROGRAM low, reset FPGA just to be safe */
	ring0_outb(LPTCTRL_ADDRSTB, iobase+LPTREG_CONTROL);
	usleep(10);
	ring0_outb(LPTCTRL_PROGRAM, iobase+LPTREG_CONTROL);
	/* start of ordinary test */
	if (!epp_clear_timeout())
		return -1;
	ring0_outb(ring0_inb(iobase + LPTREG_CONTROL) | 0x20, iobase + LPTREG_CONTROL);
	ring0_outb(ring0_inb(iobase + LPTREG_CONTROL) | 0x10, iobase + LPTREG_CONTROL);
	epp_clear_timeout();
	ring0_inb(iobase + LPTREG_EPPDATA);
	/* udelay(30); */
	if (ring0_inb(iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
		epp_clear_timeout();
		return 0;
	}
	/*return -1;*/
	lprintf(3, "warning: no EPP timeout\n");
	return 0;
}

#endif /* !EPPEMUL */

static int detect_ecr(void)
{
	unsigned char r, oc, oec;

	oec = ring0_inb(iobase + LPTREG_ECONTROL);
	oc = ring0_inb(iobase + LPTREG_CONTROL);
	if ((oc & 3) == (oec & 3)) {
		ring0_outb(oc ^ 2, iobase + LPTREG_CONTROL);
		r = ring0_inb(iobase + LPTREG_CONTROL);
		if ((ring0_inb(iobase + LPTREG_ECONTROL) & 2) == (r & 2)) {
			ring0_outb(oc, iobase + LPTREG_CONTROL);
			return -1;
		}
	}
	if ((oec & 3) != 1)
		return -1;
	ring0_outb(0x34, iobase + LPTREG_ECONTROL);
	r = ring0_inb(iobase + LPTREG_ECONTROL);
	ring0_outb(oc, iobase + LPTREG_CONTROL);
	ring0_outb(oec, iobase + LPTREG_ECONTROL);
	return -(r != 0x35);
}

int detect_port(void)
{
	ring0_outb(LPTCTRL_ADDRSTB, iobase+LPTREG_CONTROL);
	pp_w9xring0_epp_clear_timeout();  /* prevent lockup of some SMSC IC's */
	/* this routine is mostly copied from Linux Kernel parport */
	ring0_outb(0xc, iobase+LPTREG_ECONTROL);
	ring0_outb(0xc, iobase+LPTREG_CONTROL);
	ring0_outb(0xaa, iobase+LPTREG_DATA);
	if (ring0_inb(iobase+LPTREG_DATA) != 0xaa)
		goto fail_spp;
	ring0_outb(0x55, iobase+LPTREG_DATA);
	if (ring0_inb(iobase+LPTREG_DATA) != 0x55)
		goto fail_spp;
#ifdef EPPEMUL
	/* tentatively enable PS/2 mode if ECP port */
	ring0_outb(0x20, iobase + LPTREG_ECONTROL);
	ring0_outb(LPTCTRL_PROGRAM, iobase+LPTREG_CONTROL);
	if ((ring0_inb(iobase+LPTREG_CONTROL) & 0x3f) != LPTCTRL_PROGRAM)
		goto fail_ps2;
	ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_W9XRING0ION, iobase+LPTREG_CONTROL);
	if ((ring0_inb(iobase+LPTREG_CONTROL) & 0x3f) != (LPTCTRL_PROGRAM | LPTCTRL_W9XRING0ION))
		goto fail_ps2;
	return 0;

  fail_ps2:
	lprintf(3, "parport (PS/2 bidir) test failed\n");
	return -1;

#else /* !EPPEMUL */
	if (!detect_ecr()) {
		ring0_outb(0x80, iobase + LPTREG_ECONTROL);
		lprintf(3, "parport SMSC style ECP+EPP detected\n");
		return 0;
	}
	if (!detect_epp())
		return 0;
	/* goto fail_epp; */
  fail_epp:
	lprintf(3, "parport (EPP) test failed\n");
	return -1;
#endif /* !EPPEMUL */

  fail_spp:
	lprintf(3, "parport (SPP) test failed\n");
	return -1;
}

/* ---------------------------------------------------------------------- */
/*
 * ECP routines
 */

static void ecp_port_cap(void)
{
	static const char *pwordstr[8] = { "2 bytes", "1 byte", "4 bytes", "3?", "4?", 
					   "5?", "6?", "7?" };
	static const char *irqstr[8] = { "jumpered", "7", "9", "10", "11", "14", "15", "5" };
	static const char *dmastr[8] = { "jumpered 8bit", "1", "2", "3", "jumpered 16bit", "5", "6", "7" };
	unsigned char cnfga, cnfgb;
	unsigned int cnt, fwcnt, wthr = ~0, frcnt, rthr = ~0;

	ring0_outb(0x30, iobase + LPTREG_ECONTROL); /* PS/2 mode */
	ring0_outb(0xf0, iobase + LPTREG_ECONTROL); /* config mode */
	cnfga = ring0_inb(iobase + LPTREG_CONFIGA);
	cnfgb = ring0_inb(iobase + LPTREG_CONFIGB);
	ring0_outb(cnfgb & 0x7f, iobase + LPTREG_CONFIGB);  /* disable compression */
	ring0_outb(0x30, iobase + LPTREG_ECONTROL); /* PS/2 mode */
	ring0_outb(0x04, iobase + LPTREG_DCR); /* w9xring0ion=output */
	ring0_outb(0xd0, iobase + LPTREG_ECONTROL); /* test mode */
	for (cnt = 0; cnt < 1024 && !(ring0_inb(iobase + LPTREG_ECONTROL) & 0x02); cnt++)
		ring0_outb(0, iobase + LPTREG_TFIFO);
	ring0_outb(0xd0, iobase + LPTREG_ECONTROL); /* test mode, clear serviceintr */
	for (fwcnt = 0; fwcnt < 1024 && !(ring0_inb(iobase + LPTREG_ECONTROL) & 0x01); fwcnt++) {
		ring0_inb(iobase + LPTREG_TFIFO);
		if (ring0_inb(iobase + LPTREG_ECONTROL) & 0x04) {
			wthr = fwcnt;
			ring0_outb(0xd0, iobase + LPTREG_ECONTROL); /* test mode, clear serviceintr */
		}
	}
	ring0_outb(0x24, iobase + LPTREG_DCR); /* w9xring0ion=input */
	ring0_outb(0xd0, iobase + LPTREG_ECONTROL); /* test mode */
	for (cnt = 0; cnt < 1024 && !(ring0_inb(iobase + LPTREG_ECONTROL) & 0x02); cnt++)
		ring0_outb(0, iobase + LPTREG_TFIFO);
	ring0_outb(0xd0, iobase + LPTREG_ECONTROL); /* test mode, clear serviceintr */
	for (frcnt = 0; frcnt < 1024 && !(ring0_inb(iobase + LPTREG_ECONTROL) & 0x01); frcnt++) {
		ring0_inb(iobase + LPTREG_TFIFO);
		if (ring0_inb(iobase + LPTREG_ECONTROL) & 0x04) {
			rthr = frcnt;
			ring0_outb(0xd0, iobase + LPTREG_ECONTROL); /* test mode, clear serviceintr */
		}
	}
	ring0_outb(0x30, iobase + LPTREG_ECONTROL); /* PS/2 mode */
	ring0_outb(0x04, iobase + LPTREG_DCR); /* w9xring0ion=output */
	lprintf(3, "ECP capabilities: %s int, PWord %s, %sextra tx pipe, IRQ %s, DMA %s\n"
		"Tx: FIFO size %d threshold %d  Rx: FIFO size %d threshold %d\n",
		(cnfga & 0x80) ? "level" : "edge", pwordstr[(cnfga >> 4) & 7],
		(cnfga & 0x04) ? "no" : "", irqstr[(cnfgb >> 3) & 7], dmastr[cnfgb & 7],
		fwcnt, wthr, frcnt, rthr);
}

int detect_port_ecp(void)
{
	epp_clear_timeout();  /* prevent lockup of some SMSC IC's */
	/* this routine is mostly copied from Linux Kernel parport */
	ring0_outb(0xc, iobase+LPTREG_ECONTROL);
	ring0_outb(0xc, iobase+LPTREG_CONTROL);
	ring0_outb(0xaa, iobase+LPTREG_DATA);
	if (ring0_inb(iobase+LPTREG_DATA) != 0xaa)
		goto fail_spp;
	ring0_outb(0x55, iobase+LPTREG_DATA);
	if (ring0_inb(iobase+LPTREG_DATA) != 0x55)
		goto fail_spp;
	/* check for ECP ECR mode */
	if (detect_ecr())
		goto fail_ecp;
	ecp_port_cap();
	return 0;

  fail_ecp:
	lprintf(0, "parport (ECP) test failed\n");
	return -1;

  fail_spp:
	lprintf(0, "parport (SPP) test failed\n");
	return -1;
}
#endif

/* ---------------------------------------------------------------------- */

static int parport_spp(void)
{
	pp_w9xring0_epp_clear_timeout();  /* prevent lockup of some SMSC IC's */
	/* this routine is mostly copied from Linux Kernel parport */
	ring0_outb(0xc, pp_w9xring0_iobase+LPTREG_ECONTROL);
	ring0_outb(0xc, pp_w9xring0_iobase+LPTREG_CONTROL);
	ring0_outb(0xaa, pp_w9xring0_iobase+LPTREG_DATA);
	if (ring0_inb(pp_w9xring0_iobase+LPTREG_DATA) != 0xaa)
		return 0;
	ring0_outb(0x55, pp_w9xring0_iobase+LPTREG_DATA);
	if (ring0_inb(pp_w9xring0_iobase+LPTREG_DATA) != 0x55)
		return 0;
	return PARPORT_MODE_PCSPP;
}

/* Check for ECP
 *
 * Old style XT ports alias io ports every 0x400, hence accessing ECR
 * on these cards actually accesses the CTR.
 *
 * Modern cards don't do this but reading from ECR will return 0xff
 * regardless of what is written here if the card does NOT support
 * ECP.
 *
 * We will write 0x2c to ECR and 0xcc to CTR since both of these
 * values are "safe" on the CTR since bits 6-7 of CTR are unused.
 */
static int parport_ecr(void)
{
	unsigned char r = 0xc;

	ring0_outb(r, pp_w9xring0_iobase + LPTREG_CONTROL);
	if ((ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x3) == (r & 0x3)) {
		ring0_outb(r ^ 0x2, pp_w9xring0_iobase + LPTREG_CONTROL); /* Toggle bit 1 */
		r = ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL);	
		if ((ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x2) == (r & 0x2))
			goto no_reg; /* Sure that no ECR register exists */
	}

	if ((ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x3 ) != 0x1)
		goto no_reg;

	ring0_outb(0x34, pp_w9xring0_iobase + LPTREG_ECONTROL);
	if (ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) != 0x35)
		goto no_reg;

	ring0_outb(0xc, pp_w9xring0_iobase + LPTREG_CONTROL);
	
	/* Go to mode 000 */
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & ~0xe0, pp_w9xring0_iobase + LPTREG_ECONTROL);

	return PARPORT_MODE_PCECR;

 no_reg:
	ring0_outb(0xc, pp_w9xring0_iobase + LPTREG_CONTROL);
	return 0; 
}

static int parport_ecp(void)
{
	int i;
	int config;
	int pword;
	int fifo_depth, writeIntrThreshold, readIntrThreshold;

	/* Find out FIFO depth */
	ring0_outb(0x00, pp_w9xring0_iobase + LPTREG_ECONTROL); /* Reset FIFO */
	ring0_outb(0xc0, pp_w9xring0_iobase + LPTREG_ECONTROL); /* TEST FIFO */
	for (i=0; i < 1024 && !(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x02); i++)
		ring0_outb(0xaa, pp_w9xring0_iobase + LPTREG_TFIFO);
	/*
	 * Using LGS chipset it uses ECR register, but
	 * it doesn't support ECP or FIFO MODE
	 */
	if (i == 1024) {
		ring0_outb(0x00, pp_w9xring0_iobase + LPTREG_ECONTROL);
		return 0;
	}

	fifo_depth = i;
	lprintf(3, "ECP: FIFO depth is %d bytes\n", fifo_depth);

	/* Find out writeIntrThreshold */
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) | (1<<2), pp_w9xring0_iobase + LPTREG_ECONTROL);
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & ~(1<<2) , pp_w9xring0_iobase + LPTREG_ECONTROL);
	for (i = 1; i <= fifo_depth; i++) {
		ring0_inb(pp_w9xring0_iobase + LPTREG_TFIFO);
		usleep(50);
		if (ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & (1<<2))
			break;
	}

	if (i <= fifo_depth)
		lprintf(3, "ECP: writeIntrThreshold is %d\n", i);
	else
		/* Number of bytes we know we can write if we get an
                   interrupt. */
		i = 0;

	writeIntrThreshold = i;

	/* Find out readIntrThreshold */
	ring0_outb((ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & ~0xe0) | 0x20, pp_w9xring0_iobase + LPTREG_ECONTROL); /* Reset FIFO, PS2 */
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL) | PARPORT_CONTROL_DIRECTION, pp_w9xring0_iobase + LPTREG_CONTROL);
	ring0_outb((ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & ~0xe0) | 0xc0, pp_w9xring0_iobase + LPTREG_ECONTROL); /* Test FIFO */
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) | (1<<2), pp_w9xring0_iobase + LPTREG_ECONTROL);
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & ~(1<<2) , pp_w9xring0_iobase + LPTREG_ECONTROL);
	for (i = 1; i <= fifo_depth; i++) {
		ring0_outb(0xaa, pp_w9xring0_iobase + LPTREG_TFIFO);
		if (ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & (1<<2))
			break;
	}

	if (i <= fifo_depth)
		lprintf(3, "ECP: readIntrThreshold is %d\n", i);
	else
		/* Number of bytes we can read if we get an interrupt. */
		i = 0;

	readIntrThreshold = i;

	ring0_outb(0x00, pp_w9xring0_iobase + LPTREG_ECONTROL); /* Reset FIFO */
	ring0_outb(0xf4, pp_w9xring0_iobase + LPTREG_ECONTROL); /* Configuration mode */
	config = ring0_inb(pp_w9xring0_iobase + LPTREG_CONFIGA);
	pword = (config >> 4) & 0x7;
	switch (pword) {
	case 0:
		pword = 2;
		lprintf(0, "ECP: Unsupported pword size! (2)\n");
		break;
	case 2:
		pword = 4;
		lprintf(0, "ECP: Unsupported pword size! (4)\n");
		break;
	default:
		lprintf(0, "ECP: Unknown implementation ID (%d)\n", pword);
		/* Assume 1 */
	case 1:
		pword = 1;
	}
	lprintf(3, "ECP: PWord is %d bits\n", 8 * pword);

	config = ring0_inb(pp_w9xring0_iobase + LPTREG_CONFIGB);
	lprintf(3, "ECP: Interrupts are ISA-%s\n", config & 0x80 ? "Level" : "Pulses");

	if (!(config & 0x40))
		lprintf(3, "ECP: IRQ conflict!\n");

	/* Go back to mode 000 */
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & ~0xe0, pp_w9xring0_iobase + LPTREG_ECONTROL);

	return PARPORT_MODE_PCECP;
}

/* EPP mode detection  */

static int parport_epp(void)
{
	/* If EPP timeout bit clear then EPP available */
	if (!pp_w9xring0_epp_clear_timeout())
		return 0;  /* No way to clear timeout */

	/*
	 * Theory:
	 *     Write two values to the EPP address register and
	 *     read them back. When the transfer times out, the state of
	 *     the EPP register is undefined in some cases (EPP 1.9?) but
	 *     in others (EPP 1.7, ECPEPP?) it is possible to read back
	 *     its value.
	 */

	pp_w9xring0_epp_clear_timeout();
	usleep(30); /* Wait for possible EPP timeout */

	ring0_outb(0x55, pp_w9xring0_iobase + LPTREG_EPPADDR);
	pp_w9xring0_epp_clear_timeout();
	usleep(30); /* Wait for possible EPP timeout */

	if (ring0_inb(pp_w9xring0_iobase + LPTREG_EPPADDR) == 0x55) {
		ring0_outb(0xaa, pp_w9xring0_iobase + LPTREG_EPPADDR);
		pp_w9xring0_epp_clear_timeout();
		usleep(30); /* Wait for possible EPP timeout */
		if (ring0_inb(pp_w9xring0_iobase + LPTREG_EPPADDR) == 0xaa) {
			pp_w9xring0_epp_clear_timeout();
			return PARPORT_MODE_PCEPP;
		}
	}

	/*
	 * Theory:
	 *	Bit 0 of STR is the EPP timeout bit, this bit is 0
	 *	when EPP is possible and is set high when an EPP timeout
	 *	occurs (EPP uses the HALT line to stop the CPU while it does
	 *	the byte transfer, an EPP timeout occurs if the attached
	 *	device fails to respond after 10 micro seconds).
	 *
	 *	This bit is cleared by either reading it (National Semi)
	 *	or writing a 1 to the bit (SMC, UMC, Wring0_inbond), others ???
	 *	This bit is always high in non EPP modes.
	 */

	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL) | 0x20, pp_w9xring0_iobase + LPTREG_CONTROL);
	ring0_outb(ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL) | 0x10, pp_w9xring0_iobase + LPTREG_CONTROL);
	pp_w9xring0_epp_clear_timeout();
	
	ring0_inb(pp_w9xring0_iobase + LPTREG_EPPDATA);
	usleep(30);  /* Wait for possible EPP timeout */
	
	if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & 0x01) {
		pp_w9xring0_epp_clear_timeout();
		return PARPORT_MODE_PCEPP;
	}
	return 0;
}

static int parport_ecpepp(void)
{
	int mode;
	unsigned char oecr;

	oecr = ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL);
	/* Search for SMC style EPP+ECP mode */
	ring0_outb(0x80, pp_w9xring0_iobase + LPTREG_ECONTROL);
	mode = parport_epp();
	ring0_outb(oecr, pp_w9xring0_iobase + LPTREG_ECONTROL);
       	return mode ? PARPORT_MODE_PCECPEPP : 0;
}

/* Detect PS/2 support.
 *
 * Bit 5 (0x20) sets the PS/2 data w9xring0ion; setting this high
 * allows us to read data from the data lines.  In theory we would get back
 * 0xff but any peripheral attached to the port may drag some or all of the
 * lines down to zero.  So if we get back anything that isn't the contents
 * of the data register we deem PS/2 support to be present. 
 *
 * Some SPP ports have "half PS/2" ability - you can't turn off the line
 * drivers, but an external peripheral with sufficiently beefy drivers of
 * its own can overpower them and assert its own levels onto the bus, from
 * where they can then be read back as normal.  Ports with this property
 * and the right type of device attached are likely to fail the SPP test,
 * (as they will appear to have stuck bits) and so the fact that they might
 * be misdetected here is rather academic. 
 */

static int parport_ps2(void)
{
	int ok = 0;
	unsigned char octr = ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL);
  
	pp_w9xring0_epp_clear_timeout();

	ring0_outb(octr | 0x20, pp_w9xring0_iobase + LPTREG_CONTROL);  /* try to tri-state the buffer */
	
	ring0_outb(0x55, pp_w9xring0_iobase + LPTREG_DATA);
	if (ring0_inb(pp_w9xring0_iobase + LPTREG_DATA) != 0x55) ok++;
	ring0_outb(0xaa, pp_w9xring0_iobase + LPTREG_DATA);
	if (ring0_inb(pp_w9xring0_iobase + LPTREG_DATA) != 0xaa) ok++;

	ring0_outb(octr, pp_w9xring0_iobase);          /* cancel input mode */

	return ok ? PARPORT_MODE_PCPS2 : 0;
}

static int parport_ecpps2(void)
{
	int mode;
	unsigned char oecr;

	oecr = ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL);
	ring0_outb(0x20, pp_w9xring0_iobase + LPTREG_ECONTROL);
      	mode = parport_ps2();
	ring0_outb(oecr, pp_w9xring0_iobase + LPTREG_ECONTROL);
	return mode ? PARPORT_MODE_PCECPPS2 : 0;
}

/* ---------------------------------------------------------------------- */

int parport_init_w9xring0_flags(unsigned io, unsigned int flags)
{
	pp_w9xring0_flags = FLAGS_PCSPP;
	pp_w9xring0_iobase = io;
       	if (iopl(3)) {
		lprintf(0, "Cannot get direct IO port access (iopl: %s)\n", strerror(errno));
		return -1;
	}
	if (!parport_spp()) {
		lprintf(0, "No parport present at 0x%x\n", pp_w9xring0_iobase);
		return -1;
	}
	if (parport_ecr()) {
		pp_w9xring0_flags |= FLAGS_PCECR;
		pp_w9xring0_flags |= parport_ecp();
		pp_w9xring0_flags |= parport_ecpps2();
		pp_w9xring0_flags |= parport_ecpepp();
		if ((flags & PPFLAG_FORCEHWEPP) && 
		    (pp_w9xring0_flags & (FLAGS_PCPS2|FLAGS_PCECPPS2)) &&
		    !(pp_w9xring0_flags & (FLAGS_PCEPP|FLAGS_PCECPEPP)))
			pp_w9xring0_flags |= FLAGS_PCECPEPP;
		else
			flags &= ~PPFLAG_FORCEHWEPP;
	} else {
		pp_w9xring0_flags |= parport_ps2();
		pp_w9xring0_flags |= parport_epp();
		if ((flags & PPFLAG_FORCEHWEPP) && 
		    (pp_w9xring0_flags & (FLAGS_PCPS2|FLAGS_PCECPPS2)) &&
		    !(pp_w9xring0_flags & (FLAGS_PCEPP|FLAGS_PCECPEPP)))
			pp_w9xring0_flags |= FLAGS_PCEPP;
		else
			flags &= ~PPFLAG_FORCEHWEPP;
	}
	lprintf(0, "Parport 0x%x capabilities: SPP%s%s%s%s%s%s\n",
		pp_w9xring0_iobase,
		(pp_w9xring0_flags & FLAGS_PCPS2) ? ", PS2" : "",
		(pp_w9xring0_flags & FLAGS_PCEPP) ? ((flags & PPFLAG_FORCEHWEPP) ? ", EPP (forced)" : ", EPP") : "",
		(pp_w9xring0_flags & FLAGS_PCECR) ? ", ECR" : "",
		(pp_w9xring0_flags & FLAGS_PCECP) ? ", ECP" : "",
		(pp_w9xring0_flags & FLAGS_PCECPEPP) ? ((flags & PPFLAG_FORCEHWEPP) ? ", ECPEPP (forced)" : ", ECPEPP") : "",
		(pp_w9xring0_flags & FLAGS_PCECPPS2) ? ", ECPPS2" : "");
	if (!(pp_w9xring0_flags & (FLAGS_PCPS2 | FLAGS_PCECPPS2)))  {
		lprintf(0, "Parport 0x%x does not even support PS/2 mode, cannot use it\n",
			pp_w9xring0_iobase);
		return -1;
	}
	lprintf(0, "Parport 0x%x using direct Win9x Ring0 hardware access\n", pp_w9xring0_iobase);
	if (pp_w9xring0_flags & FLAGS_PCECR)
		ring0_outb(0x30, pp_w9xring0_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	parport_ops = parport_w9xring0_ops;
	if ((flags & PPFLAG_SWEMULECP) || !(pp_w9xring0_flags & FLAGS_PCECP)) {
		parport_ops.parport_ecp_write_data = parport_w9xring0_emul_ops.parport_ecp_write_data;
		parport_ops.parport_ecp_read_data = parport_w9xring0_emul_ops.parport_ecp_read_data;
		parport_ops.parport_ecp_write_addr = parport_w9xring0_emul_ops.parport_ecp_write_addr;
		lprintf(0, "Parport 0x%x emulating ECP\n", pp_w9xring0_iobase);
	}
	if ((flags & PPFLAG_SWEMULEPP) || !(pp_w9xring0_flags & (FLAGS_PCEPP | FLAGS_PCECPEPP))) {
		parport_ops.parport_epp_write_data = parport_w9xring0_emul_ops.parport_epp_write_data;
		parport_ops.parport_epp_read_data = parport_w9xring0_emul_ops.parport_epp_read_data;
		parport_ops.parport_epp_write_addr = parport_w9xring0_emul_ops.parport_epp_write_addr;
		parport_ops.parport_epp_read_addr = parport_w9xring0_emul_ops.parport_epp_read_addr;
		lprintf(0, "Parport 0x%x emulating EPP\n", pp_w9xring0_iobase);
	}
	return 0;
}

int parport_init_w9xring0(unsigned io)
{
        return parport_init_w9xring0_flags(io, 0);
}
