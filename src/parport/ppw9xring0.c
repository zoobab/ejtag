/*****************************************************************************/

/*
 *      ppw9xring0.c  --  Parport direct access under Win9x using Ring0 hack.
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

#include <windows.h>

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

unsigned int pp_w9xring0_iobase = 0x378;
unsigned int pp_w9xring0_flags = 0;

#define FLAGS_PCSPP              (1<<0)
#define FLAGS_PCPS2              (1<<1)
#define FLAGS_PCEPP              (1<<2)
#define FLAGS_PCECR              (1<<3)  /* ECR Register Exists */
#define FLAGS_PCECP              (1<<4)
#define FLAGS_PCECPEPP           (1<<5)
#define FLAGS_PCECPPS2           (1<<6)

/* ---------------------------------------------------------------------- */

asm(".text\n\t"
    ".align\t4\n"
    "_do_ring0_inb:\n\t"
    "inb %dx,%al\n\t"
    "lret\n\n\t"
    ".align\t4\n"
    "_do_ring0_outb:\n\t"
    "outb %al,%dx\n\t"
    "lret\n\n\t"
    ".align\t4\n\t"
    ".data\n\t");

extern int do_ring0_inb(int,int);
extern int do_ring0_outb(int,int);

static int call_ring0(int (*routine)(int,int), int param1, int param2)
{
        unsigned short gdtbase[3];
        unsigned short callgateaddr[3];
        struct gdt {
                unsigned short offslo;
                unsigned short selector;
                unsigned short flags;
                unsigned short offshi;
        } *gdt, *mygdt;
        unsigned int i, maxgdt;
        int ret;
        
        asm("sgdt\t%0" : "=m" (gdtbase));
        gdt = (struct gdt *)((gdtbase[2] << 16) | gdtbase[1]);
        maxgdt = gdtbase[0]/8; /* gdt limit */
        for (i = 1; i < maxgdt; i++) {
                if (!gdt[i].flags && !gdt[i].selector && !gdt[i].offslo && !gdt[i].offshi)
                        break;
        }
        if (i >= maxgdt) {
                lprintf(3, "Cannot find free GDT entry\n");
                return -1;
        }
        mygdt = &gdt[i];
        mygdt->flags = (1 << 15) | /* present */
                (0x0c00) |         /* type: call gate */
                (3 << 13) |        /* DPL */
                0;                 /* dword count */
        mygdt->selector = 0x28;
        mygdt->offslo = ((unsigned long)routine);
        mygdt->offshi = ((unsigned long)routine) >> 16;
        callgateaddr[0] = callgateaddr[1] = 0;
        callgateaddr[2] = (i << 3) | 3;
        asm("lcall\t%1" : "=a" (ret) : "m" (callgateaddr[0]), "d" (param1), "0" (param2) : "memory");
        memset(mygdt, 0, sizeof(struct gdt));
        return ret;
}

unsigned char ring0_inb(unsigned int port)
{
        return call_ring0(do_ring0_inb, port, 0);
}

void ring0_outb(unsigned char val, unsigned int port)
{
        call_ring0(do_ring0_outb, port, val);
}

void ring0_outsb(unsigned int port, const unsigned char *bp, unsigned int count)
{
        while (count > 0) {
                ring0_outb(*bp++, port);
                count--;
        }
}

void ring0_insb(unsigned int port, unsigned char *bp, unsigned int count)
{
        while (count > 0) {
                *bp++ = ring0_inb(port);
                count--;
        }
}

/* ---------------------------------------------------------------------- */

extern inline void setecr(unsigned char ecr)
{
	if (pp_w9xring0_flags & FLAGS_PCECR)
		ring0_outb(ecr, pp_w9xring0_iobase + LPTREG_ECONTROL);
}

int pp_w9xring0_epp_clear_timeout(void)
{
        unsigned char r;

        if (!(ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT))
                return 1;
        /* To clear timeout some chips require double read */
	ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS);
	r = ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS);
	ring0_outb(r | 0x01, pp_w9xring0_iobase + LPTREG_STATUS); /* Some reset by writing 1 */
	ring0_outb(r & 0xfe, pp_w9xring0_iobase + LPTREG_STATUS); /* Others by writing 0 */
        r = ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS);
        return !(r & 0x01);
}

/* ---------------------------------------------------------------------- */

unsigned char parport_w9xring0_read_data(void)
{
	return ring0_inb(pp_w9xring0_iobase + LPTREG_DATA);
}

void parport_w9xring0_write_data(unsigned char d)
{
	ring0_outb(d, pp_w9xring0_iobase + LPTREG_DATA);
}

unsigned char parport_w9xring0_read_status(void)
{
	return ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS);
}


unsigned char parport_w9xring0_read_control(void)
{
	return ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL);
}

void parport_w9xring0_write_control(unsigned char d)
{
	ring0_outb(d, pp_w9xring0_iobase + LPTREG_CONTROL);
}

void parport_w9xring0_frob_control(unsigned char mask, unsigned char val)
{
	unsigned char d = ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL);
	d = (d & (~mask)) ^ val;
	ring0_outb(d, pp_w9xring0_iobase + LPTREG_CONTROL);
}

/* ---------------------------------------------------------------------- */

unsigned parport_w9xring0_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_EPPDATA);
		if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_w9xring0_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_w9xring0_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = ring0_inb(pp_w9xring0_iobase + LPTREG_EPPDATA);
		if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_w9xring0_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_w9xring0_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_EPPADDR);
		if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_w9xring0_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_w9xring0_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = ring0_inb(pp_w9xring0_iobase + LPTREG_EPPADDR);
		if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_w9xring0_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_w9xring0_emul_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_w9xring0_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_DATA);
		for (tmo = 0; ; tmo++) {
			if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_DATASTB, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_w9xring0_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_w9xring0_emul_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_w9xring0_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_DATASTB, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = ring0_inb(pp_w9xring0_iobase + LPTREG_DATA);
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_w9xring0_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_w9xring0_emul_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_w9xring0_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_DATA);
		for (tmo = 0; ; tmo++) {
			if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_ADDRSTB, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, pp_w9xring0_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_w9xring0_emul_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_w9xring0_iobase + LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_ADDRSTB, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = ring0_inb(pp_w9xring0_iobase + LPTREG_DATA);
		ring0_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, pp_w9xring0_iobase + LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

/* ---------------------------------------------------------------------- */

static int ecp_forward(void)
{
	unsigned tmo = 0x10000;

	if (ring0_inb(pp_w9xring0_iobase + LPTREG_DSR) & 0x20)
		return 0;
	/* Event 47: Set nInit high */
	ring0_outb(0x26, pp_w9xring0_iobase + LPTREG_DCR);
	/* Event 49: PError goes high */
	while (!(ring0_inb(pp_w9xring0_iobase + LPTREG_DSR) & 0x20)) {
		if (!(--tmo))
			return -1;
	}
	/* start driving the bus */
	ring0_outb(0x04, pp_w9xring0_iobase + LPTREG_DCR);
	return 0;
}

static int ecp_reverse(void)
{
	unsigned tmo = 0x10000;

	if (!(ring0_inb(pp_w9xring0_iobase + LPTREG_DSR) & 0x20))
		return 0;
	ring0_outb(0x24, pp_w9xring0_iobase + LPTREG_DCR);
	/* Event 39: Set nInit low to initiate bus reversal */
	ring0_outb(0x20, pp_w9xring0_iobase + LPTREG_DCR);
	while (ring0_inb(pp_w9xring0_iobase + LPTREG_DSR) & 0x20) {
		if (!(--tmo))
			return -1;
	}
	return 0;
}

static unsigned emptyfifo(unsigned cnt)
{
	unsigned fcnt = 0;

	ring0_outb(0xd0, pp_w9xring0_iobase + LPTREG_ECONTROL); /* FIFOtest mode */
	while ((ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x01) && fcnt < 32 && fcnt < cnt) {
		ring0_inb(pp_w9xring0_iobase + LPTREG_TFIFO);
		fcnt++;
	}
	printf("emptyfifo: FIFO contained %d bytes\n", fcnt);
	ring0_outb(0x30, pp_w9xring0_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return cnt - fcnt;
}

unsigned parport_w9xring0_ecp_write_data(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (ecp_forward())
		return 0;
	ring0_outb(0x70, pp_w9xring0_iobase + LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL)) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
			ring0_outsb(pp_w9xring0_iobase + LPTREG_DFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			ring0_outb(*bp++, pp_w9xring0_iobase + LPTREG_DFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x01) || !(ring0_inb(pp_w9xring0_iobase + LPTREG_DSR) & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
	}
	ring0_outb(0x30, pp_w9xring0_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

unsigned parport_w9xring0_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;
	
	if (ecp_reverse())
		return 0;
	ring0_outb(0x70, pp_w9xring0_iobase + LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL)) & 0x01)
			if (!(--tmo)) {
				ring0_outb(0xd0, pp_w9xring0_iobase + LPTREG_ECONTROL); /* FIFOTEST mode */
				while (!(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x01) && sz > 0) {
					*bp++ = ring0_inb(pp_w9xring0_iobase + LPTREG_TFIFO);
					sz--;
					ret++;
				}
				ring0_outb(0x30, pp_w9xring0_iobase + LPTREG_ECONTROL); /* PS/2 mode */
				return ret;
			}
		if (stat & 0x02 && sz >= 8) {
			ring0_insb(pp_w9xring0_iobase + LPTREG_DFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			*bp++ = ring0_inb(pp_w9xring0_iobase + LPTREG_DFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	ring0_outb(0x30, pp_w9xring0_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

unsigned parport_w9xring0_ecp_write_addr(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (ecp_forward())
		return 0;
	ring0_outb(0x70, pp_w9xring0_iobase + LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL)) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
			ring0_outsb(pp_w9xring0_iobase + LPTREG_AFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			ring0_outb(*bp++, pp_w9xring0_iobase + LPTREG_AFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x01) || !(ring0_inb(pp_w9xring0_iobase + LPTREG_DSR) & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
 	}
	ring0_outb(0x30, pp_w9xring0_iobase + LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_w9xring0_emul_ecp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL) & ~PARPORT_CONTROL_AUTOFD;
	/* HostAck high (data, not command) */
	ring0_outb(ctl, pp_w9xring0_iobase + LPTREG_CONTROL);
	for (ret = 0; ret < sz; ret++, bp++) {
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_DATA);
		ring0_outb(ctl | PARPORT_CONTROL_STROBE, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		ring0_outb(ctl, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; !(ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_w9xring0_emul_ecp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL) | PARPORT_CONTROL_AUTOFD;
	/* HostAck low (command, not data) */
	ring0_outb(ctl, pp_w9xring0_iobase + LPTREG_CONTROL);
	for (ret = 0; ret < sz; ret++, bp++) {
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_DATA);
		ring0_outb(ctl | PARPORT_CONTROL_STROBE, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		ring0_outb(ctl, pp_w9xring0_iobase + LPTREG_CONTROL);
		for (tmo = 0; !(ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS) & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_w9xring0_emul_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned char ctl, stat;
	int command;
	unsigned tmo;

	if (ecp_reverse())
		return 0;	
	ctl = ring0_inb(pp_w9xring0_iobase + LPTREG_CONTROL);
	/* Set HostAck low to start accepting data. */
	ring0_outb(ctl | PARPORT_CONTROL_AUTOFD, pp_w9xring0_iobase + LPTREG_CONTROL);
	while (ret < sz) {
		/* Event 43: Peripheral sets nAck low. It can take as
                   long as it wants. */
		tmo = 0;
		do {
			stat = ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS);
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
		*bp = ring0_inb(pp_w9xring0_iobase + LPTREG_DATA);
		/* Event 44: Set HostAck high, acknowledging handshake. */
		ring0_outb(ctl & ~PARPORT_CONTROL_AUTOFD, pp_w9xring0_iobase + LPTREG_CONTROL);
		/* Event 45: The peripheral has 35ms to set nAck high. */
		tmo = 0;
		do {
			stat = ring0_inb(pp_w9xring0_iobase + LPTREG_STATUS);
			if ((++tmo) > 0x10000)
				goto out;
		} while (!(stat & PARPORT_STATUS_ACK));
		/* Event 46: Set HostAck low and accept the data. */
		ring0_outb(ctl | PARPORT_CONTROL_AUTOFD, pp_w9xring0_iobase + LPTREG_CONTROL);
		/* Normal data byte. */
		bp++;
		ret++;
	}

 out:
	return ret;
}
 
/* ---------------------------------------------------------------------- */

#if 1
unsigned parport_w9xring0_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned u;

	for (u = 0; u < sz; u++, bp++)
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_DATA);
	return sz;
}
#else

unsigned parport_w9xring0_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (pp_w9xring0_modes & PARPORT_MODE_PCECR) {
		ring0_outb(0x50, pp_w9xring0_iobase + LPTREG_ECONTROL); /* COMPAT FIFO mode */
		while (sz > 0) {
			while ((stat = ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL)) & 0x02) {
				if (!(--tmo))
					return emptyfifo(ret);
			}
			if (stat & 0x01 && sz >= 8) {
				ring0_outsb(pp_w9xring0_iobase + LPTREG_DFIFO, bp, 8);
				bp += 8;
				sz -= 8;
				ret += 8;
			} else {
				ring0_outb(*bp++, pp_w9xring0_iobase + LPTREG_DFIFO);
				sz--;
				ret++;
			}
			tmo = 0x10000;
		}
		while (!(ring0_inb(pp_w9xring0_iobase + LPTREG_ECONTROL) & 0x01) || !(ring0_inb(pp_w9xring0_iobase + LPTREG_DSR) & 0x80)) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		ring0_outb(0x30, pp_w9xring0_iobase + LPTREG_ECONTROL); /* PS/2 mode */
		return ret;
	}
	for (ret = 0; ret < sz; ret++, bp++)
		ring0_outb(*bp, pp_w9xring0_iobase + LPTREG_DATA);
	return sz;
}
#endif

/* ---------------------------------------------------------------------- */

const struct parport_ops parport_w9xring0_ops = {
	parport_w9xring0_read_data,
	parport_w9xring0_write_data,
	parport_w9xring0_read_status,
	parport_w9xring0_read_control,
	parport_w9xring0_write_control,
	parport_w9xring0_frob_control,
	parport_w9xring0_epp_write_data,
	parport_w9xring0_epp_read_data,
	parport_w9xring0_epp_write_addr,
	parport_w9xring0_epp_read_addr,
	parport_w9xring0_ecp_write_data,
	parport_w9xring0_ecp_read_data,
	parport_w9xring0_ecp_write_addr,
	parport_w9xring0_fpgaconfig_write
};

const struct parport_ops parport_w9xring0_emul_ops = {
	parport_w9xring0_read_data,
	parport_w9xring0_write_data,
	parport_w9xring0_read_status,
	parport_w9xring0_read_control,
	parport_w9xring0_write_control,
	parport_w9xring0_frob_control,
	parport_w9xring0_emul_epp_write_data,
	parport_w9xring0_emul_epp_read_data,
	parport_w9xring0_emul_epp_write_addr,
	parport_w9xring0_emul_epp_read_addr,
	parport_w9xring0_emul_ecp_write_data,
	parport_w9xring0_emul_ecp_read_data,
	parport_w9xring0_emul_ecp_write_addr,
	parport_w9xring0_fpgaconfig_write
};

/* ---------------------------------------------------------------------- */
