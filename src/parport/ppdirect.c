/*****************************************************************************/

/*
 *      ppdirect.c  -- Parport direct access.
 *
 *      Copyright (C) 1998-1999  Thomas Sailer (sailer@ife.ee.ethz.ch)
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

#include "parport.h"

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

unsigned int pp_direct_iobase = 0x378;
unsigned int pp_direct_flags = 0;

#define FLAGS_PCSPP              (1<<0)
#define FLAGS_PCPS2              (1<<1)
#define FLAGS_PCEPP              (1<<2)
#define FLAGS_PCECR              (1<<3)  /* ECR Register Exists */
#define FLAGS_PCECP              (1<<4)
#define FLAGS_PCECPEPP           (1<<5)
#define FLAGS_PCECPPS2           (1<<6)

/* ---------------------------------------------------------------------- */

extern inline void setecr(unsigned char ecr)
{
	if (pp_direct_flags & FLAGS_PCECR)
		outb(ecr, pp_direct_iobase + LPTREG_ECONTROL);
}

int pp_direct_epp_clear_timeout(void)
{
        unsigned char r;

        if (!(inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT))
                return 1;
        /* To clear timeout some chips require double read */
	inb(pp_direct_iobase + LPTREG_STATUS);
	r = inb(pp_direct_iobase + LPTREG_STATUS);
	outb(r | 0x01, pp_direct_iobase + LPTREG_STATUS); /* Some reset by writing 1 */
	outb(r & 0xfe, pp_direct_iobase + LPTREG_STATUS); /* Others by writing 0 */
        r = inb(pp_direct_iobase + LPTREG_STATUS);
        return !(r & 0x01);
}

/* ---------------------------------------------------------------------- */

unsigned char parport_direct_read_data(void)
{
	return inb(pp_direct_iobase + LPTREG_DATA);
}

void parport_direct_write_data(unsigned char d)
{
	outb(d, pp_direct_iobase + LPTREG_DATA);
}

unsigned char parport_direct_read_status(void)
{
	return inb(pp_direct_iobase + LPTREG_STATUS);
}


unsigned char parport_direct_read_control(void)
{
	return inb(pp_direct_iobase + LPTREG_CONTROL);
}

void parport_direct_write_control(unsigned char d)
{
	outb(d, pp_direct_iobase + LPTREG_CONTROL);
}

void parport_direct_frob_control(unsigned char mask, unsigned char val)
{
	unsigned char d = inb(pp_direct_iobase + LPTREG_CONTROL);
	d = (d & (~mask)) ^ val;
	outb(d, pp_direct_iobase + LPTREG_CONTROL);
}

/* ---------------------------------------------------------------------- */

unsigned parport_direct_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		outb(*bp, pp_direct_iobase + LPTREG_EPPDATA);
		if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_direct_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_direct_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = inb(pp_direct_iobase + LPTREG_EPPDATA);
		if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_direct_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_direct_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		outb(*bp, pp_direct_iobase + LPTREG_EPPADDR);
		if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_direct_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_direct_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = inb(pp_direct_iobase + LPTREG_EPPADDR);
		if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_direct_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_direct_emul_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_direct_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		outb(*bp, pp_direct_iobase + LPTREG_DATA);
		for (tmo = 0; ; tmo++) {
			if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_DATASTB, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_direct_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_direct_emul_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_direct_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_DATASTB, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = inb(pp_direct_iobase + LPTREG_DATA);
		outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_direct_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_direct_emul_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_direct_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		outb(*bp, pp_direct_iobase + LPTREG_DATA);
		for (tmo = 0; ; tmo++) {
			if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_ADDRSTB, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_direct_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_direct_emul_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_direct_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_ADDRSTB, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(inb(pp_direct_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = inb(pp_direct_iobase + LPTREG_DATA);
		outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_direct_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

/* ---------------------------------------------------------------------- */

static int ecp_forward(void)
{
	unsigned tmo = 0x10000;

	if (inb(pp_direct_iobase + LPTREG_DSR) & 0x20)
		return 0;
	/* Event 47: Set nInit high */
	outb(0x26, pp_direct_iobase + LPTREG_DCR);
	/* Event 49: PError goes high */
	while (!(inb(pp_direct_iobase + LPTREG_DSR) & 0x20)) {
		if (!(--tmo))
			return -1;
	}
	/* start driving the bus */
	outb(0x04, pp_direct_iobase + LPTREG_DCR);
	return 0;
}

static int ecp_reverse(void)
{
	unsigned tmo = 0x10000;

	if (!(inb(pp_direct_iobase + LPTREG_DSR) & 0x20))
		return 0;
	outb(0x24, pp_direct_iobase + LPTREG_DCR);
	/* Event 39: Set nInit low to initiate bus reversal */
	outb(0x20, pp_direct_iobase + LPTREG_DCR);
	while (inb(pp_direct_iobase + LPTREG_DSR) & 0x20) {
		if (!(--tmo))
			return -1;
	}
	return 0;
}

static unsigned emptyfifo(unsigned cnt)
{
	unsigned fcnt = 0;

	outb(0xd0, pp_direct_iobase + LPTREG_ECONTROL); /* FIFOtest mode */
	while ((inb(pp_direct_iobase + LPTREG_ECONTROL) & 0x01) && fcnt < 32 && fcnt < cnt) {
		inb(pp_direct_iobase + LPTREG_TFIFO);
		fcnt++;
	}
	printf("emptyfifo: FIFO contained %d bytes\n", fcnt);
	outb(0x30, pp_direct_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return cnt - fcnt;
}

unsigned parport_direct_ecp_write_data(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (ecp_forward())
		return 0;
	outb(0x70, pp_direct_iobase + LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = inb(pp_direct_iobase + LPTREG_ECONTROL)) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
			outsb(pp_direct_iobase + LPTREG_DFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			outb(*bp++, pp_direct_iobase + LPTREG_DFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(inb(pp_direct_iobase + LPTREG_ECONTROL) & 0x01) || !(inb(pp_direct_iobase + LPTREG_DSR) & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
	}
	outb(0x30, pp_direct_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

unsigned parport_direct_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;
	
	if (ecp_reverse())
		return 0;
	outb(0x70, pp_direct_iobase + LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = inb(pp_direct_iobase + LPTREG_ECONTROL)) & 0x01)
			if (!(--tmo)) {
				outb(0xd0, pp_direct_iobase + LPTREG_ECONTROL); /* FIFOTEST mode */
				while (!(inb(pp_direct_iobase + LPTREG_ECONTROL) & 0x01) && sz > 0) {
					*bp++ = inb(pp_direct_iobase + LPTREG_TFIFO);
					sz--;
					ret++;
				}
				outb(0x30, pp_direct_iobase + LPTREG_ECONTROL); /* PS/2 mode */
				return ret;
			}
		if (stat & 0x02 && sz >= 8) {
			insb(pp_direct_iobase + LPTREG_DFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			*bp++ = inb(pp_direct_iobase + LPTREG_DFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	outb(0x30, pp_direct_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

unsigned parport_direct_ecp_write_addr(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (ecp_forward())
		return 0;
	outb(0x70, pp_direct_iobase + LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = inb(pp_direct_iobase + LPTREG_ECONTROL)) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
			outsb(pp_direct_iobase + LPTREG_AFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			outb(*bp++, pp_direct_iobase + LPTREG_AFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(inb(pp_direct_iobase + LPTREG_ECONTROL) & 0x01) || !(inb(pp_direct_iobase + LPTREG_DSR) & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
 	}
	outb(0x30, pp_direct_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_direct_emul_ecp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = inb(pp_direct_iobase + LPTREG_CONTROL) & ~PARPORT_CONTROL_AUTOFD;
	/* HostAck high (data, not command) */
	outb(ctl, pp_direct_iobase + LPTREG_CONTROL);
	for (ret = 0; ret < sz; ret++, bp++) {
		outb(*bp, pp_direct_iobase + LPTREG_DATA);
		outb(ctl | PARPORT_CONTROL_STROBE, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; inb(pp_direct_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		outb(ctl, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; !(inb(pp_direct_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_direct_emul_ecp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = inb(pp_direct_iobase + LPTREG_CONTROL) | PARPORT_CONTROL_AUTOFD;
	/* HostAck low (command, not data) */
	outb(ctl, pp_direct_iobase + LPTREG_CONTROL);
	for (ret = 0; ret < sz; ret++, bp++) {
		outb(*bp, pp_direct_iobase + LPTREG_DATA);
		outb(ctl | PARPORT_CONTROL_STROBE, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; inb(pp_direct_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		outb(ctl, pp_direct_iobase + LPTREG_CONTROL);
		for (tmo = 0; !(inb(pp_direct_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_direct_emul_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned char ctl, stat;
	int command;
	unsigned tmo;

	if (ecp_reverse())
		return 0;	
	ctl = inb(pp_direct_iobase + LPTREG_CONTROL);
	/* Set HostAck low to start accepting data. */
	outb(ctl | PARPORT_CONTROL_AUTOFD, pp_direct_iobase + LPTREG_CONTROL);
	while (ret < sz) {
		/* Event 43: Peripheral sets nAck low. It can take as
                   long as it wants. */
		tmo = 0;
		do {
			stat = inb(pp_direct_iobase + LPTREG_STATUS);
			if ((++tmo) > 0x10000)
				goto out;
		} while (stat & PARPORT_STATUS_ACK);
		/* Is this a command? */
		command = stat & PARPORT_STATUS_BUSY;
		if (command) {
			//lprintf(3, "ECP: warning: emulation does not support RLE\n");
			goto out;
		}
		/* Read the data. */
		*bp = inb(pp_direct_iobase + LPTREG_DATA);
		/* Event 44: Set HostAck high, acknowledging handshake. */
		outb(ctl & ~PARPORT_CONTROL_AUTOFD, pp_direct_iobase + LPTREG_CONTROL);
		/* Event 45: The peripheral has 35ms to set nAck high. */
		tmo = 0;
		do {
			stat = inb(pp_direct_iobase + LPTREG_STATUS);
			if ((++tmo) > 0x10000)
				goto out;
		} while (!(stat & PARPORT_STATUS_ACK));
		/* Event 46: Set HostAck low and accept the data. */
		outb(ctl | PARPORT_CONTROL_AUTOFD, pp_direct_iobase + LPTREG_CONTROL);
		/* Normal data byte. */
		bp++;
		ret++;
	}

 out:
	return ret;
}
 
/* ---------------------------------------------------------------------- */

#if 1
unsigned parport_direct_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned u;

	for (u = 0; u < sz; u++, bp++)
		outb(*bp, pp_direct_iobase + LPTREG_DATA);
	return sz;
}
#else

unsigned parport_direct_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (pp_direct_modes & PARPORT_MODE_PCECR) {
		outb(0x50, pp_direct_iobase + LPTREG_ECONTROL); /* COMPAT FIFO mode */
		while (sz > 0) {
			while ((stat = inb(pp_direct_iobase + LPTREG_ECONTROL)) & 0x02) {
				if (!(--tmo))
					return emptyfifo(ret);
			}
			if (stat & 0x01 && sz >= 8) {
				outsb(pp_direct_iobase + LPTREG_DFIFO, bp, 8);
				bp += 8;
				sz -= 8;
				ret += 8;
			} else {
				outb(*bp++, pp_direct_iobase + LPTREG_DFIFO);
				sz--;
				ret++;
			}
			tmo = 0x10000;
		}
		while (!(inb(pp_direct_iobase + LPTREG_ECONTROL) & 0x01) || !(inb(pp_direct_iobase + LPTREG_DSR) & 0x80)) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		outb(0x30, pp_direct_iobase + LPTREG_ECONTROL); /* PS/2 mode */
		return ret;
	}
	for (ret = 0; ret < sz; ret++, bp++)
		outb(*bp, pp_direct_iobase + LPTREG_DATA);
	return sz;
}
#endif

/* ---------------------------------------------------------------------- */

const struct parport_ops parport_direct_ops = {
	parport_direct_read_data,
	parport_direct_write_data,
	parport_direct_read_status,
	parport_direct_read_control,
	parport_direct_write_control,
	parport_direct_frob_control,
	parport_direct_epp_write_data,
	parport_direct_epp_read_data,
	parport_direct_epp_write_addr,
	parport_direct_epp_read_addr,
	parport_direct_ecp_write_data,
	parport_direct_ecp_read_data,
	parport_direct_ecp_write_addr,
	parport_direct_fpgaconfig_write
};

const struct parport_ops parport_direct_emul_ops = {
	parport_direct_read_data,
	parport_direct_write_data,
	parport_direct_read_status,
	parport_direct_read_control,
	parport_direct_write_control,
	parport_direct_frob_control,
	parport_direct_emul_epp_write_data,
	parport_direct_emul_epp_read_data,
	parport_direct_emul_epp_write_addr,
	parport_direct_emul_epp_read_addr,
	parport_direct_emul_ecp_write_data,
	parport_direct_emul_ecp_read_data,
	parport_direct_emul_ecp_write_addr,
	parport_direct_fpgaconfig_write
};

/* ---------------------------------------------------------------------- */

struct parport_ops parport_ops = {
	parport_direct_read_data,
	parport_direct_write_data,
	parport_direct_read_status,
	parport_direct_read_control,
	parport_direct_write_control,
	parport_direct_frob_control,
	parport_direct_emul_epp_write_data,
	parport_direct_emul_epp_read_data,
	parport_direct_emul_epp_write_addr,
	parport_direct_emul_epp_read_addr,
	parport_direct_emul_ecp_write_data,
	parport_direct_emul_ecp_read_data,
	parport_direct_emul_ecp_write_addr,
	parport_direct_fpgaconfig_write
};

/* ---------------------------------------------------------------------- */
