/*****************************************************************************/

/*
 *      ppntddkgenportinit.c  -- Parport direct access with NTDDK genport.sys (init routines).
 *
 *      Copyright (C) 2000  Thomas Sailer (sailer@ife.ee.ethz.ch)
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

#include <windows.h>

#include "parport.h"

/* ---------------------------------------------------------------------- */

#define DEVNAME "\\\\.\\GpdDev"

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

extern HANDLE pp_ntddkgenport_handle;
extern unsigned int pp_ntddkgenport_flags;
extern const struct parport_ops parport_ntddkgenport_ops, parport_ntddkgenport_emul_ops;

extern unsigned char pp_ntddkgenport_inb(unsigned int port);
extern void pp_ntddkgenport_outb(unsigned char val, unsigned int port);
extern int pp_ntddkgenport_epp_clear_timeout(void);

#define FLAGS_PCSPP              (1<<0)
#define FLAGS_PCPS2              (1<<1)
#define FLAGS_PCEPP              (1<<2)
#define FLAGS_PCECR              (1<<3)  /* ECR Register Exists */
#define FLAGS_PCECP              (1<<4)
#define FLAGS_PCECPEPP           (1<<5)
#define FLAGS_PCECPPS2           (1<<6)

/* ---------------------------------------------------------------------- */

static int parport_spp(void)
{
	pp_ntddkgenport_epp_clear_timeout();  /* prevent lockup of some SMSC IC's */
	/* this routine is mostly copied from Linux Kernel parport */
	pp_ntddkgenport_outb(0xc, LPTREG_ECONTROL);
	pp_ntddkgenport_outb(0xc, LPTREG_CONTROL);
	pp_ntddkgenport_outb(0xaa, LPTREG_DATA);
	if (pp_ntddkgenport_inb(LPTREG_DATA) != 0xaa)
		return 0;
	pp_ntddkgenport_outb(0x55, LPTREG_DATA);
	if (pp_ntddkgenport_inb(LPTREG_DATA) != 0x55)
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

	pp_ntddkgenport_outb(r, LPTREG_CONTROL);
	if ((pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x3) == (r & 0x3)) {
		pp_ntddkgenport_outb(r ^ 0x2, LPTREG_CONTROL); /* Toggle bit 1 */
		r = pp_ntddkgenport_inb(LPTREG_CONTROL);	
		if ((pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x2) == (r & 0x2))
			goto no_reg; /* Sure that no ECR register exists */
	}

	if ((pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x3 ) != 0x1)
		goto no_reg;

	pp_ntddkgenport_outb(0x34, LPTREG_ECONTROL);
	if (pp_ntddkgenport_inb(LPTREG_ECONTROL) != 0x35)
		goto no_reg;

	pp_ntddkgenport_outb(0xc, LPTREG_CONTROL);
	
	/* Go to mode 000 */
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_ECONTROL) & ~0xe0, LPTREG_ECONTROL);

	return PARPORT_MODE_PCECR;

 no_reg:
	pp_ntddkgenport_outb(0xc, LPTREG_CONTROL);
	return 0; 
}

static int parport_ecp(void)
{
	int i;
	int config;
	int pword;
	int fifo_depth, writeIntrThreshold, readIntrThreshold;

	/* Find out FIFO depth */
	pp_ntddkgenport_outb(0x00, LPTREG_ECONTROL); /* Reset FIFO */
	pp_ntddkgenport_outb(0xc0, LPTREG_ECONTROL); /* TEST FIFO */
	for (i=0; i < 1024 && !(pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x02); i++)
		pp_ntddkgenport_outb(0xaa, LPTREG_TFIFO);
	/*
	 * Using LGS chipset it uses ECR register, but
	 * it doesn't support ECP or FIFO MODE
	 */
	if (i == 1024) {
		pp_ntddkgenport_outb(0x00, LPTREG_ECONTROL);
		return 0;
	}

	fifo_depth = i;
	lprintf(3, "ECP: FIFO depth is %d bytes\n", fifo_depth);

	/* Find out writeIntrThreshold */
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_ECONTROL) | (1<<2), LPTREG_ECONTROL);
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_ECONTROL) & ~(1<<2) , LPTREG_ECONTROL);
	for (i = 1; i <= fifo_depth; i++) {
		pp_ntddkgenport_inb(LPTREG_TFIFO);
		usleep(50);
		if (pp_ntddkgenport_inb(LPTREG_ECONTROL) & (1<<2))
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
	pp_ntddkgenport_outb((pp_ntddkgenport_inb(LPTREG_ECONTROL) & ~0xe0) | 0x20, LPTREG_ECONTROL); /* Reset FIFO, PS2 */
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_CONTROL) | PARPORT_CONTROL_DIRECTION, LPTREG_CONTROL);
	pp_ntddkgenport_outb((pp_ntddkgenport_inb(LPTREG_ECONTROL) & ~0xe0) | 0xc0, LPTREG_ECONTROL); /* Test FIFO */
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_ECONTROL) | (1<<2), LPTREG_ECONTROL);
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_ECONTROL) & ~(1<<2) , LPTREG_ECONTROL);
	for (i = 1; i <= fifo_depth; i++) {
		pp_ntddkgenport_outb(0xaa, LPTREG_TFIFO);
		if (pp_ntddkgenport_inb(LPTREG_ECONTROL) & (1<<2))
			break;
	}

	if (i <= fifo_depth)
		lprintf(3, "ECP: readIntrThreshold is %d\n", i);
	else
		/* Number of bytes we can read if we get an interrupt. */
		i = 0;

	readIntrThreshold = i;

	pp_ntddkgenport_outb(0x00, LPTREG_ECONTROL); /* Reset FIFO */
	pp_ntddkgenport_outb(0xf4, LPTREG_ECONTROL); /* Configuration mode */
	config = pp_ntddkgenport_inb(LPTREG_CONFIGA);
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

	config = pp_ntddkgenport_inb(LPTREG_CONFIGB);
	lprintf(3, "ECP: Interrupts are ISA-%s\n", config & 0x80 ? "Level" : "Pulses");

	if (!(config & 0x40))
		lprintf(3, "ECP: IRQ conflict!\n");

	/* Go back to mode 000 */
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_ECONTROL) & ~0xe0, LPTREG_ECONTROL);

	return PARPORT_MODE_PCECP;
}

/* EPP mode detection  */

static int parport_epp(void)
{
	/* If EPP timeout bit clear then EPP available */
	if (!pp_ntddkgenport_epp_clear_timeout())
		return 0;  /* No way to clear timeout */

	/*
	 * Theory:
	 *     Write two values to the EPP address register and
	 *     read them back. When the transfer times out, the state of
	 *     the EPP register is undefined in some cases (EPP 1.9?) but
	 *     in others (EPP 1.7, ECPEPP?) it is possible to read back
	 *     its value.
	 */

	pp_ntddkgenport_epp_clear_timeout();
	usleep(30); /* Wait for possible EPP timeout */

	pp_ntddkgenport_outb(0x55, LPTREG_EPPADDR);
	pp_ntddkgenport_epp_clear_timeout();
	usleep(30); /* Wait for possible EPP timeout */

	if (pp_ntddkgenport_inb(LPTREG_EPPADDR) == 0x55) {
		pp_ntddkgenport_outb(0xaa, LPTREG_EPPADDR);
		pp_ntddkgenport_epp_clear_timeout();
		usleep(30); /* Wait for possible EPP timeout */
		if (pp_ntddkgenport_inb(LPTREG_EPPADDR) == 0xaa) {
			pp_ntddkgenport_epp_clear_timeout();
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
	 *	or writing a 1 to the bit (SMC, UMC, WinBond), others ???
	 *	This bit is always high in non EPP modes.
	 */

	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_CONTROL) | 0x20, LPTREG_CONTROL);
	pp_ntddkgenport_outb(pp_ntddkgenport_inb(LPTREG_CONTROL) | 0x10, LPTREG_CONTROL);
	pp_ntddkgenport_epp_clear_timeout();
	
	pp_ntddkgenport_inb(LPTREG_EPPDATA);
	usleep(30);  /* Wait for possible EPP timeout */
	
	if (pp_ntddkgenport_inb(LPTREG_STATUS) & 0x01) {
		pp_ntddkgenport_epp_clear_timeout();
		return PARPORT_MODE_PCEPP;
	}
	return 0;
}

static int parport_ecpepp(void)
{
	int mode;
	unsigned char oecr;

	oecr = pp_ntddkgenport_inb(LPTREG_ECONTROL);
	/* Search for SMC style EPP+ECP mode */
	pp_ntddkgenport_outb(0x80, LPTREG_ECONTROL);
	mode = parport_epp();
	pp_ntddkgenport_outb(oecr, LPTREG_ECONTROL);
       	return mode ? PARPORT_MODE_PCECPEPP : 0;
}

/* Detect PS/2 support.
 *
 * Bit 5 (0x20) sets the PS/2 data direction; setting this high
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
	unsigned char octr = pp_ntddkgenport_inb(LPTREG_CONTROL);
  
	pp_ntddkgenport_epp_clear_timeout();

	pp_ntddkgenport_outb(octr | 0x20, LPTREG_CONTROL);  /* try to tri-state the buffer */
	
	pp_ntddkgenport_outb(0x55, LPTREG_DATA);
	if (pp_ntddkgenport_inb(LPTREG_DATA) != 0x55) ok++;
	pp_ntddkgenport_outb(0xaa, LPTREG_DATA);
	if (pp_ntddkgenport_inb(LPTREG_DATA) != 0xaa) ok++;

	pp_ntddkgenport_outb(octr, 0);          /* cancel input mode */

	return ok ? PARPORT_MODE_PCPS2 : 0;
}

static int parport_ecpps2(void)
{
	int mode;
	unsigned char oecr;

	oecr = pp_ntddkgenport_inb(LPTREG_ECONTROL);
	pp_ntddkgenport_outb(0x20, LPTREG_ECONTROL);
      	mode = parport_ps2();
	pp_ntddkgenport_outb(oecr, LPTREG_ECONTROL);
	return mode ? PARPORT_MODE_PCECPPS2 : 0;
}

/* ---------------------------------------------------------------------- */

int parport_init_ntddkgenport(void)
{
	if (pp_ntddkgenport_handle != INVALID_HANDLE_VALUE)
		CloseHandle(pp_ntddkgenport_handle);
	pp_ntddkgenport_handle = CreateFile(DEVNAME, GENERIC_READ | GENERIC_WRITE,
					    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
					    OPEN_EXISTING, 0, NULL);
	if (pp_ntddkgenport_handle == INVALID_HANDLE_VALUE) {
		lprintf(0, "Cannot open genport.sys driver, error 0x%08lx\n", GetLastError());
		return -1;
	}
	pp_ntddkgenport_flags = FLAGS_PCSPP;
	if (!parport_spp()) {
		lprintf(0, "No parport present\n");
		goto err;
	}
	if (parport_ecr()) {
		pp_ntddkgenport_flags |= FLAGS_PCECR;
		pp_ntddkgenport_flags |= parport_ecp();
		pp_ntddkgenport_flags |= parport_ecpps2();
		pp_ntddkgenport_flags |= parport_ecpepp();
	} else {
		pp_ntddkgenport_flags |= parport_ps2();
		pp_ntddkgenport_flags |= parport_epp();
	}
	lprintf(0, "Parport capabilities: SPP");
	if (pp_ntddkgenport_flags & FLAGS_PCPS2)
		lprintf(0, ", PS2");
	if (pp_ntddkgenport_flags & FLAGS_PCEPP)
		lprintf(0, ", EPP");
	if (pp_ntddkgenport_flags & FLAGS_PCECR)
		lprintf(0, ", ECR");
	if (pp_ntddkgenport_flags & FLAGS_PCECP)
		lprintf(0, ", ECP");
	if (pp_ntddkgenport_flags & FLAGS_PCECPEPP)
		lprintf(0, ", ECPEPP");
	if (pp_ntddkgenport_flags & FLAGS_PCECPPS2)
		lprintf(0, ", ECPPS2");
	if (!(pp_ntddkgenport_flags & (FLAGS_PCPS2 | FLAGS_PCECPPS2)))  {
		lprintf(0, "\nParport does not even support PS/2 mode, cannot use it\n");
		goto err;
	}
	lprintf(0, "\nParport using NTDDK genport.sys hardware access");
	if (pp_ntddkgenport_flags & FLAGS_PCECR)
		pp_ntddkgenport_outb(0x30, LPTREG_ECONTROL); /* PS/2 mode */
	parport_ops = parport_ntddkgenport_ops;
	if (!(pp_ntddkgenport_flags & FLAGS_PCECP)) {
		parport_ops.parport_ecp_write_data = parport_ntddkgenport_emul_ops.parport_ecp_write_data;
		parport_ops.parport_ecp_read_data = parport_ntddkgenport_emul_ops.parport_ecp_read_data;
		parport_ops.parport_ecp_write_addr = parport_ntddkgenport_emul_ops.parport_ecp_write_addr;
		lprintf(0, ", emulating ECP");
	}
	if (!(pp_ntddkgenport_flags & (FLAGS_PCEPP | FLAGS_PCECPEPP))) {
		parport_ops.parport_epp_write_data = parport_ntddkgenport_emul_ops.parport_epp_write_data;
		parport_ops.parport_epp_read_data = parport_ntddkgenport_emul_ops.parport_epp_read_data;
		parport_ops.parport_epp_write_addr = parport_ntddkgenport_emul_ops.parport_epp_write_addr;
		parport_ops.parport_epp_read_addr = parport_ntddkgenport_emul_ops.parport_epp_read_addr;
		lprintf(0, ", emulating EPP");
	}
	lprintf(0, "\n");
	return 0;

  err:
	CloseHandle(pp_ntddkgenport_handle);
	pp_ntddkgenport_handle = INVALID_HANDLE_VALUE;
	return -1;
}
