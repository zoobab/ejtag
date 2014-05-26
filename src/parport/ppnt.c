/*****************************************************************************/

/*
 *      ppnt.c  -- Parport direct access with NT eppflex.sys.
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

#include "eppflex.h"

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

HANDLE pp_nt_handle = INVALID_HANDLE_VALUE;
unsigned int pp_nt_flags = 0;

#define FLAGS_PCSPP              (1<<0)
#define FLAGS_PCPS2              (1<<1)
#define FLAGS_PCEPP              (1<<2)
#define FLAGS_PCECR              (1<<3)  /* ECR Register Exists */
#define FLAGS_PCECP              (1<<4)
#define FLAGS_PCECPEPP           (1<<5)
#define FLAGS_PCECPPS2           (1<<6)

/* ---------------------------------------------------------------------- */

unsigned char parport_nt_read_data(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_DATA, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_data failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_data(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_DATA, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_data failed, error 0x%08lx\n", GetLastError());
}

unsigned char parport_nt_read_status(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_STATUS, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_status failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_status(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_STATUS, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_status failed, error 0x%08lx\n", GetLastError());
}

unsigned char parport_nt_read_control(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
       
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_CONTROL, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_control failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_control(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_CONTROL, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_control failed, error 0x%08lx\n", GetLastError());
}

void parport_nt_frob_control(unsigned char mask, unsigned char val)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = val;
        rw.mask = mask;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_FROB_CONTROL, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_frob_control failed, error 0x%08lx\n", GetLastError());
		return;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return;
	}
}

unsigned char parport_nt_read_configa(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_CONFIGA, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_configa failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_configa(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_CONFIGA, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_configa failed, error 0x%08lx\n", GetLastError());
}

unsigned char parport_nt_read_configb(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_CONFIGB, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_configb failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_configb(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_CONFIGB, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_configb failed, error 0x%08lx\n", GetLastError());
}

unsigned char parport_nt_read_econtrol(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_ECONTROL, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_econtrol failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_econtrol(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_ECONTROL, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_econtrol failed, error 0x%08lx\n", GetLastError());
}

void parport_nt_frob_econtrol(unsigned char mask, unsigned char val)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = val;
        rw.mask = mask;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_FROB_ECONTROL, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_frob_econtrol failed, error 0x%08lx\n", GetLastError());
		return;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return;
	}
}

unsigned char parport_nt_read_eppaddr(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_EPPADDR, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_eppaddr failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_eppaddr(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_EPPADDR, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_eppaddr failed, error 0x%08lx\n", GetLastError());
}

unsigned char parport_nt_read_eppdata(void)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return 0xff;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_READ_EPPDATA, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res) {
		lprintf(0, "Parport: parport_nt_read_eppdata failed, error 0x%08lx\n", GetLastError());
		return 0xff;
	}
	if (retl != sizeof(rw)) {
		lprintf(0, "Parport: ioctl returned invalid length, %lu\n", retl);
		return 0xff;
	}
        return rw.data;
}

void parport_nt_write_eppdata(unsigned char d)
{
        struct eppflex_rwdata rw;
        BOOL res;
        DWORD retl;
        
	if (pp_nt_handle == INVALID_HANDLE_VALUE)
		return;
        rw.data = d;
	res = DeviceIoControl(pp_nt_handle, IOCTL_EPPFLEX_WRITE_EPPDATA, &rw, sizeof(rw), &rw, sizeof(rw), &retl, NULL);
	if (!res)
		lprintf(0, "Parport: parport_nt_write_eppdata failed, error 0x%08lx\n", GetLastError());
}

/* ---------------------------------------------------------------------- */

extern inline void setecr(unsigned char ecr)
{
	if (pp_nt_flags & FLAGS_PCECR)
		parport_nt_write_econtrol(ecr);
}

int pp_nt_epp_clear_timeout(void)
{
        unsigned char r;

        if (!(parport_nt_read_status() & LPTSTAT_EPPTIMEOUT))
                return 1;
        /* To clear timeout some chips require double read */
	parport_nt_read_status();
	r = parport_nt_read_status();
	parport_nt_write_status(r | 0x01); /* Some reset by writing 1 */
	parport_nt_write_status(r & 0xfe); /* Others by writing 0 */
        r = parport_nt_read_status();
        return !(r & 0x01);
}

/* ---------------------------------------------------------------------- */

unsigned parport_nt_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		parport_nt_write_eppdata(*bp);
		if (parport_nt_read_status() & LPTSTAT_EPPTIMEOUT) {
			pp_nt_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_nt_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = parport_nt_read_eppdata();
		if (parport_nt_read_status() & LPTSTAT_EPPTIMEOUT) {
			pp_nt_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_nt_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		parport_nt_write_eppaddr(*bp);
		if (parport_nt_read_status() & LPTSTAT_EPPTIMEOUT) {
			pp_nt_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_nt_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;

	setecr(0x90); /* EPP mode */
	for (; sz > 0; sz--, bp++) {
		*bp = parport_nt_read_eppaddr();
		if (parport_nt_read_status() & LPTSTAT_EPPTIMEOUT) {
			pp_nt_epp_clear_timeout();
			goto rt;
		}
		ret++;
	}
 rt:
	setecr(0x30); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_nt_emul_epp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_WRITE);
	for (; sz > 0; sz--, bp++) {
		parport_nt_write_data(*bp);
		for (tmo = 0; ; tmo++) {
			if (parport_nt_read_status() & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_DATASTB);
		for (tmo = 0; ; tmo++) {
			if (!(parport_nt_read_status() & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_WRITE);
		ret++;
	}
	return ret;
}

unsigned parport_nt_emul_epp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (parport_nt_read_status() & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_DATASTB);
		for (tmo = 0; ; tmo++) {
			if (!(parport_nt_read_status() & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = parport_nt_read_data();
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION);
		ret++;
	}
	return ret;
}

unsigned parport_nt_emul_epp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_WRITE);
	for (; sz > 0; sz--, bp++) {
		parport_nt_write_data(*bp);
		for (tmo = 0; ; tmo++) {
			if (parport_nt_read_status() & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_WRITE | LPTCTRL_ADDRSTB);
		for (tmo = 0; ; tmo++) {
			if (!(parport_nt_read_status() & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_WRITE);
		ret++;
	}
	return ret;
}

unsigned parport_nt_emul_epp_read_addr(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo;

	parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION);
	for (; sz > 0; sz--, bp++) {
		for (tmo = 0; ; tmo++) {
			if (parport_nt_read_status() & LPTSTAT_WAIT)
				break;
			if (tmo > 1000)
				return ret;
		}
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION | LPTCTRL_ADDRSTB);
		for (tmo = 0; ; tmo++) {
			if (!(parport_nt_read_status() & LPTSTAT_WAIT))
				break;
			if (tmo > 1000)
				return ret;
		}
		*bp = parport_nt_read_data();
		parport_nt_write_control(LPTCTRL_PROGRAM | LPTCTRL_DIRECTION);
		ret++;
	}
	return ret;
}

/* ---------------------------------------------------------------------- */

static int ecp_forward(void)
{
	unsigned tmo = 0x10000;

	if (parport_nt_read_status() & 0x20)
		return 0;
	/* Event 47: Set nInit high */
	parport_nt_write_control(0x26);
	/* Event 49: PError goes high */
	while (!(parport_nt_read_status() & 0x20)) {
		if (!(--tmo))
			return -1;
	}
	/* start driving the bus */
	parport_nt_write_control(0x04);
	return 0;
}

static int ecp_reverse(void)
{
	unsigned tmo = 0x10000;

	if (!(parport_nt_read_status() & 0x20))
		return 0;
	parport_nt_write_control(0x24);
	/* Event 39: Set nInit low to initiate bus reversal */
	parport_nt_write_control(0x20);
	while (parport_nt_read_status() & 0x20) {
		if (!(--tmo))
			return -1;
	}
	return 0;
}

static unsigned emptyfifo(unsigned cnt)
{
	unsigned fcnt = 0;

	parport_nt_write_econtrol(0xd0); /* FIFOtest mode */
	while ((parport_nt_read_econtrol() & 0x01) && fcnt < 32 && fcnt < cnt) {
		parport_nt_read_configa();
		fcnt++;
	}
	lprintf(10, "emptyfifo: FIFO contained %d bytes\n", fcnt);
	parport_nt_write_econtrol(0x30); /* PS/2 mode */
	return cnt - fcnt;
}

unsigned parport_nt_ecp_write_data(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;
        unsigned int i;
        
	if (ecp_forward())
		return 0;
	parport_nt_write_econtrol(0x70); /* ECP mode */
	while (sz > 0) {
		while ((stat = parport_nt_read_econtrol()) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
                        for (i = 0; i < 8; i++)
                                parport_nt_write_configa(bp[i]);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			parport_nt_write_configa(*bp++);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(parport_nt_read_econtrol() & 0x01) || !(parport_nt_read_status() & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
	}
	parport_nt_write_econtrol(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_nt_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;
	unsigned int i;
        
	if (ecp_reverse())
		return 0;
	parport_nt_write_econtrol(0x70); /* ECP mode */
	while (sz > 0) {
		while ((stat = parport_nt_read_econtrol()) & 0x01)
			if (!(--tmo)) {
				parport_nt_write_econtrol(0xd0); /* FIFOTEST mode */
				while (!(parport_nt_read_econtrol() & 0x01) && sz > 0) {
					*bp++ = parport_nt_read_configa();
					sz--;
					ret++;
				}
				parport_nt_write_econtrol(0x30); /* PS/2 mode */
				return ret;
			}
		if (stat & 0x02 && sz >= 8) {
                        for (i = 0; i < 8; i++)
                                bp[i] = parport_nt_read_configa();
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			*bp++ = parport_nt_read_configa();
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	parport_nt_write_econtrol(0x30); /* PS/2 mode */
	return ret;
}

unsigned parport_nt_ecp_write_addr(const void *buf, unsigned sz)
{
	const unsigned char *bp = (const unsigned char *)buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;
        unsigned int i;
        
	if (ecp_forward())
		return 0;
	parport_nt_write_econtrol(0x70); /* ECP mode */
	while (sz > 0) {
		while ((stat = parport_nt_read_econtrol()) & 0x02) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		if (stat & 0x01 && sz >= 8) {
                        for (i = 0; i < 8; i++)
				parport_nt_write_data(bp[i]);
			bp += 8;
			sz -= 8;
			ret += 8;
		} else {
			parport_nt_write_data(*bp++);
			sz--;
			ret++;
		}
		tmo = 0x10000;
	}
	while (!(parport_nt_read_econtrol() & 0x01) || !(parport_nt_read_status() & 0x80)) {
		if (!(--tmo))
			return emptyfifo(ret);
 	}
	parport_nt_write_econtrol(0x30); /* PS/2 mode */
	return ret;
}

/* ---------------------------------------------------------------------- */

unsigned parport_nt_emul_ecp_write_data(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = parport_nt_read_control() & ~PARPORT_CONTROL_AUTOFD;
	/* HostAck high (data, not command) */
	parport_nt_write_control(ctl);
	for (ret = 0; ret < sz; ret++, bp++) {
		parport_nt_write_data(*bp);
		parport_nt_write_control(ctl | PARPORT_CONTROL_STROBE);
		for (tmo = 0; parport_nt_read_status() & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		parport_nt_write_control(ctl);
		for (tmo = 0; !(parport_nt_read_status() & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_nt_emul_ecp_write_addr(const void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret;
	unsigned char ctl;
	unsigned tmo;

	if (ecp_forward())
		return 0;
	ctl = parport_nt_read_control() | PARPORT_CONTROL_AUTOFD;
	/* HostAck low (command, not data) */
	parport_nt_write_control(ctl);
	for (ret = 0; ret < sz; ret++, bp++) {
		parport_nt_write_data(*bp);
		parport_nt_write_control(ctl | PARPORT_CONTROL_STROBE);
		for (tmo = 0; parport_nt_read_status() & PARPORT_STATUS_BUSY; tmo++)
			if (tmo > 0x1000)
				return ret;
		parport_nt_write_control(ctl);
		for (tmo = 0; !(parport_nt_read_status() & PARPORT_STATUS_BUSY); tmo++)
			if (tmo > 0x1000)
				return ret;
	}
	return ret;
}

unsigned parport_nt_emul_ecp_read_data(void *buf, unsigned sz)
{
	unsigned char *bp = (unsigned char *)buf;
	unsigned ret = 0;
	unsigned char ctl, stat;
	int command;
	unsigned tmo;

	if (ecp_reverse())
		return 0;	
	ctl = parport_nt_read_control();
	/* Set HostAck low to start accepting data. */
	parport_nt_write_control(ctl | PARPORT_CONTROL_AUTOFD);
	while (ret < sz) {
		/* Event 43: Peripheral sets nAck low. It can take as
                   long as it wants. */
		tmo = 0;
		do {
			stat = parport_nt_read_status();
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
		*bp = parport_nt_read_data();
		/* Event 44: Set HostAck high, acknowledging handshake. */
		parport_nt_write_control(ctl & ~PARPORT_CONTROL_AUTOFD);
		/* Event 45: The peripheral has 35ms to set nAck high. */
		tmo = 0;
		do {
			stat = parport_nt_read_status();
			if ((++tmo) > 0x10000)
				goto out;
		} while (!(stat & PARPORT_STATUS_ACK));
		/* Event 46: Set HostAck low and accept the data. */
		parport_nt_write_control(ctl | PARPORT_CONTROL_AUTOFD);
		/* Normal data byte. */
		bp++;
		ret++;
	}

 out:
	return ret;
}
 
/* ---------------------------------------------------------------------- */

#if 1
unsigned parport_nt_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned u;

	for (u = 0; u < sz; u++, bp++)
		parport_nt_write_data(*bp);
	return sz;
}
#else

unsigned parport_nt_fpgaconfig_write(const void *buf, unsigned sz)
{
	const unsigned char *bp = buf;
	unsigned ret = 0;
	unsigned tmo = 0x10000;
	unsigned char stat;
        unsigned int i;

	if (pp_nt_modes & PARPORT_MODE_PCECR) {
		parport_nt_write_econtrol(0x50); /* COMPAT FIFO mode */
		while (sz > 0) {
			while ((stat = parport_nt_read_econtrol()) & 0x02) {
				if (!(--tmo))
					return emptyfifo(ret);
			}
			if (stat & 0x01 && sz >= 8) {
                                for (i = 0; i < 8; i++)
                                        parport_nt_write_configa(bp[i]);
				bp += 8;
				sz -= 8;
				ret += 8;
			} else {
				parport_nt_write_configa(*bp++);
				sz--;
				ret++;
			}
			tmo = 0x10000;
		}
		while (!(parport_nt_read_econtrol() & 0x01) || !(parport_nt_read_status() & 0x80)) {
			if (!(--tmo))
				return emptyfifo(ret);
		}
		parport_nt_write_econtrol(0x30); /* PS/2 mode */
		return ret;
	}
	for (ret = 0; ret < sz; ret++, bp++)
		parport_nt_write_data(*bp);
	return sz;
}
#endif

/* ---------------------------------------------------------------------- */

const struct parport_ops parport_nt_ops = {
	parport_nt_read_data,
	parport_nt_write_data,
	parport_nt_read_status,
	parport_nt_read_control,
	parport_nt_write_control,
	parport_nt_frob_control,
	parport_nt_epp_write_data,
	parport_nt_epp_read_data,
	parport_nt_epp_write_addr,
	parport_nt_epp_read_addr,
	parport_nt_ecp_write_data,
	parport_nt_ecp_read_data,
	parport_nt_ecp_write_addr,
	parport_nt_fpgaconfig_write
};

const struct parport_ops parport_nt_emul_ops = {
	parport_nt_read_data,
	parport_nt_write_data,
	parport_nt_read_status,
	parport_nt_read_control,
	parport_nt_write_control,
	parport_nt_frob_control,
	parport_nt_emul_epp_write_data,
	parport_nt_emul_epp_read_data,
	parport_nt_emul_epp_write_addr,
	parport_nt_emul_epp_read_addr,
	parport_nt_emul_ecp_write_data,
	parport_nt_emul_ecp_read_data,
	parport_nt_emul_ecp_write_addr,
	parport_nt_fpgaconfig_write
};

/* ---------------------------------------------------------------------- */
