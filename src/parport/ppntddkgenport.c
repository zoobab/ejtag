/*****************************************************************************/

/*
 *      ppntddkgenport.c  -- Parport direct access with NTDDK genport.sys.
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

#define DEVICE_TYPE DWORD

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          (0x0001) 
#define FILE_WRITE_ACCESS         (0x0002)

#define GPD_TYPE 40000

#define IOCTL_GPD_READ_PORT_UCHAR \
    CTL_CODE( GPD_TYPE, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_READ_PORT_USHORT \
    CTL_CODE( GPD_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_READ_PORT_ULONG \
    CTL_CODE( GPD_TYPE, 0x902, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_WRITE_PORT_UCHAR \
    CTL_CODE(GPD_TYPE,  0x910, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_WRITE_PORT_USHORT \
    CTL_CODE(GPD_TYPE,  0x911, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_GPD_WRITE_PORT_ULONG \
    CTL_CODE(GPD_TYPE,  0x912, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct  _GENPORT_WRITE_INPUT {
	ULONG PortNumber;     /* Port # to write to */
	union {               /* Data to be output to port */
		ULONG  LongData;
		USHORT ShortData;
		UCHAR  CharData;
	};
} GENPORT_WRITE_INPUT;

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

HANDLE pp_ntddkgenport_handle = INVALID_HANDLE_VALUE;
unsigned int pp_ntddkgenport_flags = 0;

#define FLAGS_PCSPP              (1<<0)
#define FLAGS_PCPS2              (1<<1)
#define FLAGS_PCEPP              (1<<2)
#define FLAGS_PCECR              (1<<3)  /* ECR Register Exists */
#define FLAGS_PCECP              (1<<4)
#define FLAGS_PCECPEPP           (1<<5)
#define FLAGS_PCECPPS2           (1<<6)

/* ---------------------------------------------------------------------- */

unsigned char pp_ntddkgenport_inb(unsigned int port)
{
	ULONG p = port;
	UCHAR ch;
	BOOL res;
	DWORD retl;

	if (pp_ntddkgenport_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_ntddkgenport_handle, IOCTL_GPD_READ_PORT_UCHAR, &p, sizeof(p), &ch, sizeof(ch), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: inb ioctl failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(ch)) {
		lprintf(0, "Parport: inb ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
	return ch;
}

void pp_ntddkgenport_outb(unsigned char val, unsigned int port)
{
	GENPORT_WRITE_INPUT buf;
	BOOL res;
	DWORD retl;

	if (pp_ntddkgenport_handle == INVALID_HANDLE_VALUE)
		return;
	buf.PortNumber = port;
	buf.CharData = val;
	res = DeviceIoControl(pp_ntddkgenport_handle, IOCTL_GPD_WRITE_PORT_UCHAR, &buf, sizeof(buf), NULL, 0, &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: outb ioctl failed, error 0x%08lx\n", GetLastError());
		return;
	}
}

void pp_ntddkgenport_insb(unsigned int port, unsigned char *bp, unsigned int len)
{
	for (; len > 0; len--, bp++)
		*bp = pp_ntddkgenport_inb(port);
}

void pp_ntddkgenport_outsb(unsigned int port, const unsigned char *bp, unsigned int len)
{
	for (; len > 0; len--, bp++)
		pp_ntddkgenport_outb(*bp, port);
}

/* ---------------------------------------------------------------------- */

extern inline void setecr(unsigned char ecr)
{
	if (pp_ntddkgenport_flags & FLAGS_PCECR)
		pp_ntddkgenport_outb(ecr, LPTREG_ECONTROL);
}

int pp_ntddkgenport_epp_clear_timeout(void)
{
        unsigned char r;

        if (!(pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT))
                return 1;
        /* To clear timeout some chips require double read */
	pp_ntddkgenport_inb(LPTREG_STATUS);
	r = pp_ntddkgenport_inb(LPTREG_STATUS);
	pp_ntddkgenport_outb(r | 0x01, LPTREG_STATUS); /* Some reset by writing 1 */
	pp_ntddkgenport_outb(r & 0xfe, LPTREG_STATUS); /* Others by writing 0 */
        r = pp_ntddkgenport_inb(LPTREG_STATUS);
        return !(r & 0x01);
}

/* ---------------------------------------------------------------------- */

unsigned char parport_ntddkgenport_read_data(void)
{
	return pp_ntddkgenport_inb(LPTREG_DATA);
}

void parport_ntddkgenport_write_data(unsigned char d)
{
	pp_ntddkgenport_outb(d, LPTREG_DATA);
}

unsigned char parport_ntddkgenport_read_status(void)
{
	return pp_ntddkgenport_inb(LPTREG_STATUS);
}


unsigned char parport_ntddkgenport_read_control(void)
{
	return pp_ntddkgenport_inb(LPTREG_CONTROL);
}

void parport_ntddkgenport_write_control(unsigned char d)
{
	pp_ntddkgenport_outb(d, LPTREG_CONTROL);
}

void parport_ntddkgenport_frob_control(unsigned char mask, unsigned char val)
{
	unsigned char d = pp_ntddkgenport_inb(LPTREG_CONTROL);
	d = (d & (~mask)) ^ val;
	pp_ntddkgenport_outb(d, LPTREG_CONTROL);
}

/* ---------------------------------------------------------------------- */

unsigned parport_ntddkgenport_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		pp_ntddkgenport_outb(*bp, LPTREG_EPPDATA);
		if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_ntddkgenport_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_ntddkgenport_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = pp_ntddkgenport_inb(LPTREG_EPPDATA);
		if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_ntddkgenport_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_ntddkgenport_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		pp_ntddkgenport_outb(*bp, LPTREG_EPPADDR);
		if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_ntddkgenport_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_ntddkgenport_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = pp_ntddkgenport_inb(LPTREG_EPPADDR);
		if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_EPPTIMEOUT) {
			pp_ntddkgenport_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_ntddkgenport_emul_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		pp_ntddkgenport_outb(*bp, LPTREG_DATA);
		for (tmo = 0; ; tmo++) {
			if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_DATASTB, LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_ntddkgenport_emul_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_DATASTB, LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = pp_ntddkgenport_inb(LPTREG_DATA);
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_ntddkgenport_emul_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		pp_ntddkgenport_outb(*bp, LPTREG_DATA);
		for (tmo = 0; ; tmo++) {
			if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_ADDRSTB, LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_WRITE, LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

unsigned parport_ntddkgenport_emul_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, LPTREG_CONTROL);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_ADDRSTB, LPTREG_CONTROL);
		for (tmo = 0; ; tmo++) {
			if (!(pp_ntddkgenport_inb(LPTREG_STATUS) & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = pp_ntddkgenport_inb(LPTREG_DATA);
		pp_ntddkgenport_outb(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION, LPTREG_CONTROL);
		ret++;
	}
	return ret;
}

/* ---------------------------------------------------------------------- */

static int ecp_forward(void)
{
	unsigned tmo = 0x10000;

	if (pp_ntddkgenport_inb(LPTREG_DSR) & 0x20)
		return 0;
	/* Event 47: Set nInit high */
	pp_ntddkgenport_outb(0x26, LPTREG_DCR);
	/* Event 49: PError goes high */
	while (!(pp_ntddkgenport_inb(LPTREG_DSR) & 0x20)) {
		if (!(--tmo))
			return -1;
	}
	/* start driving the bus */
	pp_ntddkgenport_outb(0x04, LPTREG_DCR);
	return 0;
}

static int ecp_reverse(void)
{
	unsigned tmo = 0x10000;

	if (!(pp_ntddkgenport_inb(LPTREG_DSR) & 0x20))
		return 0;
	pp_ntddkgenport_outb(0x24, LPTREG_DCR);
	/* Event 39: Set nInit low to initiate bus reversal */
	pp_ntddkgenport_outb(0x20, LPTREG_DCR);
	while (pp_ntddkgenport_inb(LPTREG_DSR) & 0x20) {
		if (!(--tmo))
			return -1;
	}
	return 0;
}

static unsigned emptyfifo(unsigned cnt)
{
	unsigned fcnt = 0;

	pp_ntddkgenport_outb(0xd0, LPTREG_ECONTROL); /* FIFOtest mode */
	while ((pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x01) && fcnt < 32 && fcnt < cnt) {
		pp_ntddkgenport_inb(LPTREG_TFIFO);
		fcnt++;
	}
	lprintf(10, "emptyfifo: FIFO contained %d bytes\n", fcnt);
	pp_ntddkgenport_outb(0x30, LPTREG_ECONTROL); /* PS/2 mode */
	return cnt - fcnt;
}

unsigned parport_ntddkgenport_ecp_write_data(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (ecp_forward())
		return 0;
	pp_ntddkgenport_outb(0x70, LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = pp_ntddkgenport_inb(LPTREG_ECONTROL)) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
			pp_ntddkgenport_outsb(LPTREG_DFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			pp_ntddkgenport_outb(*bp++, LPTREG_DFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x01) || !(pp_ntddkgenport_inb(LPTREG_DSR) & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
	}
	pp_ntddkgenport_outb(0x30, LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

unsigned parport_ntddkgenport_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;
	
	if (ecp_reverse())
		return 0;
	pp_ntddkgenport_outb(0x70, LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = pp_ntddkgenport_inb(LPTREG_ECONTROL)) & 0x01)
			if (!(--tmo)) {
				pp_ntddkgenport_outb(0xd0, LPTREG_ECONTROL); /* FIFOTEST mode */
				while (!(pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x01) && sz > 0) {
					*bp++ = pp_ntddkgenport_inb(LPTREG_TFIFO);
					sz--;
					ret++;
				}
				pp_ntddkgenport_outb(0x30, LPTREG_ECONTROL); /* PS/2 mode */
				return ret;
			}
		if (stat & 0x02 && sz >= 8) {
			pp_ntddkgenport_insb(LPTREG_DFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			*bp++ = pp_ntddkgenport_inb(LPTREG_DFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	pp_ntddkgenport_outb(0x30, LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

unsigned parport_ntddkgenport_ecp_write_addr(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (ecp_forward())
		return 0;
	pp_ntddkgenport_outb(0x70, LPTREG_ECONTROL); /* ECP mode */
	while (sz > 0) {
		while ((stat = pp_ntddkgenport_inb(LPTREG_ECONTROL)) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
			pp_ntddkgenport_outsb(LPTREG_AFIFO, bp, 8);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			pp_ntddkgenport_outb(*bp++, LPTREG_AFIFO);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x01) || !(pp_ntddkgenport_inb(LPTREG_DSR) & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
 	}
	pp_ntddkgenport_outb(0x30, LPTREG_ECONTROL); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_ntddkgenport_emul_ecp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = pp_ntddkgenport_inb(LPTREG_CONTROL) & ~PARPORT_CONTROL_AUTOFD;
	/* HostAck high (data, not command) */
	pp_ntddkgenport_outb(ctl, LPTREG_CONTROL);
	for (ret = 0; ret < sz; ret++, bp++) {
		pp_ntddkgenport_outb(*bp, LPTREG_DATA);
		pp_ntddkgenport_outb(ctl | PARPORT_CONTROL_STROBE, LPTREG_CONTROL);
		for (tmo = 0; pp_ntddkgenport_inb(LPTREG_STATUS) & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		pp_ntddkgenport_outb(ctl, LPTREG_CONTROL);
		for (tmo = 0; !(pp_ntddkgenport_inb(LPTREG_STATUS) & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_ntddkgenport_emul_ecp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = pp_ntddkgenport_inb(LPTREG_CONTROL) | PARPORT_CONTROL_AUTOFD;
	/* HostAck low (command, not data) */
	pp_ntddkgenport_outb(ctl, LPTREG_CONTROL);
	for (ret = 0; ret < sz; ret++, bp++) {
		pp_ntddkgenport_outb(*bp, LPTREG_DATA);
		pp_ntddkgenport_outb(ctl | PARPORT_CONTROL_STROBE, LPTREG_CONTROL);
		for (tmo = 0; pp_ntddkgenport_inb(LPTREG_STATUS) & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		pp_ntddkgenport_outb(ctl, LPTREG_CONTROL);
		for (tmo = 0; !(pp_ntddkgenport_inb(LPTREG_STATUS) & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_ntddkgenport_emul_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned char ctl, stat;
	int command;
	unsigned tmo;

	if (ecp_reverse())
		return 0;	
	ctl = pp_ntddkgenport_inb(LPTREG_CONTROL);
	/* Set HostAck low to start accepting data. */
	pp_ntddkgenport_outb(ctl | PARPORT_CONTROL_AUTOFD, LPTREG_CONTROL);
	while (ret < sz) {
		/* Event 43: Peripheral sets nAck low. It can take as
                   long as it wants. */
		tmo = 0;
		do {
			stat = pp_ntddkgenport_inb(LPTREG_STATUS);
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
		*bp = pp_ntddkgenport_inb(LPTREG_DATA);
		/* Event 44: Set HostAck high, acknowledging handshake. */
		pp_ntddkgenport_outb(ctl & ~PARPORT_CONTROL_AUTOFD, LPTREG_CONTROL);
		/* Event 45: The peripheral has 35ms to set nAck high. */
		tmo = 0;
		do {
			stat = pp_ntddkgenport_inb(LPTREG_STATUS);
			if ((++tmo) > 0x10000)
				goto out;
		} while (!(stat & PARPORT_STATUS_ACK));
		/* Event 46: Set HostAck low and accept the data. */
		pp_ntddkgenport_outb(ctl | PARPORT_CONTROL_AUTOFD, LPTREG_CONTROL);
		/* Normal data byte. */
		bp++;
		ret++;
	}

 out:
	return ret;
}
 
/* ---------------------------------------------------------------------- */

#if 1
unsigned parport_ntddkgenport_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned u;

	for (u = 0; u < sz; u++, bp++)
		pp_ntddkgenport_outb(*bp, LPTREG_DATA);
	return sz;
}
#else

unsigned parport_ntddkgenport_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;

	if (pp_ntddkgenport_modes & PARPORT_MODE_PCECR) {
		pp_ntddkgenport_outb(0x50, LPTREG_ECONTROL); /* COMPAT FIFO mode */
		while (sz > 0) {
			while ((stat = pp_ntddkgenport_inb(LPTREG_ECONTROL)) & 0x02) {
				if (!(--tmo))
					return emptyfifo(ret);
			}
			if (stat & 0x01 && sz >= 8) {
				pp_ntddkgenport_outsb(LPTREG_DFIFO, bp, 8);
				bp += 8;
				sz -= 8;
				ret += 8;
			} else {
				pp_ntddkgenport_outb(*bp++, LPTREG_DFIFO);
				sz--;
				ret++;
			}
			tmo = 0x10000;
		}
		while (!(pp_ntddkgenport_inb(LPTREG_ECONTROL) & 0x01) || !(pp_ntddkgenport_inb(LPTREG_DSR) & 0x80)) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		pp_ntddkgenport_outb(0x30, LPTREG_ECONTROL); /* PS/2 mode */
		return ret;
	}
	for (ret = 0; ret < sz; ret++, bp++)
		pp_ntddkgenport_outb(*bp, LPTREG_DATA);
	return sz;
}
#endif

/* ---------------------------------------------------------------------- */

const struct parport_ops parport_ntddkgenport_ops = {
	parport_ntddkgenport_read_data,
	parport_ntddkgenport_write_data,
	parport_ntddkgenport_read_status,
	parport_ntddkgenport_read_control,
	parport_ntddkgenport_write_control,
	parport_ntddkgenport_frob_control,
	parport_ntddkgenport_epp_write_data,
	parport_ntddkgenport_epp_read_data,
	parport_ntddkgenport_epp_write_addr,
	parport_ntddkgenport_epp_read_addr,
	parport_ntddkgenport_ecp_write_data,
	parport_ntddkgenport_ecp_read_data,
	parport_ntddkgenport_ecp_write_addr,
	parport_ntddkgenport_fpgaconfig_write
};

const struct parport_ops parport_ntddkgenport_emul_ops = {
	parport_ntddkgenport_read_data,
	parport_ntddkgenport_write_data,
	parport_ntddkgenport_read_status,
	parport_ntddkgenport_read_control,
	parport_ntddkgenport_write_control,
	parport_ntddkgenport_frob_control,
	parport_ntddkgenport_emul_epp_write_data,
	parport_ntddkgenport_emul_epp_read_data,
	parport_ntddkgenport_emul_epp_write_addr,
	parport_ntddkgenport_emul_epp_read_addr,
	parport_ntddkgenport_emul_ecp_write_data,
	parport_ntddkgenport_emul_ecp_read_data,
	parport_ntddkgenport_emul_ecp_write_addr,
	parport_ntddkgenport_fpgaconfig_write
};

/* ---------------------------------------------------------------------- */
