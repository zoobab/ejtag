/*****************************************************************************/

/*
 *      ppntinit.c  -- Parport direct access with eppflex.sys/vxd (init routines).
 *
 *      Copyright (C) 2000-2001  Thomas Sailer (sailer@ife.ee.ethz.ch)
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

#include "w9xdrv/eppflex.h"

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

extern HANDLE pp_win_handle;
extern unsigned int pp_win_flags;
extern const struct parport_ops parport_win_ops, parport_win_emul_ops;

extern unsigned char parport_win_read_data(void);
extern void parport_win_write_data(unsigned char d);
extern unsigned char parport_win_read_status(void);
extern unsigned char parport_win_read_control(void);
extern void parport_win_write_control(unsigned char d);
extern void parport_win_frob_control(unsigned char mask, unsigned char val);
extern unsigned char parport_win_read_configa(void);
extern void parport_win_write_configa(unsigned char d);
extern unsigned char parport_win_read_configb(void);
extern void parport_win_write_configb(unsigned char d);
extern unsigned char parport_win_read_econtrol(void);
extern void parport_win_write_econtrol(unsigned char d);
extern void parport_win_frob_econtrol(unsigned char mask, unsigned char val);
extern unsigned char parport_win_read_eppaddr(void);
extern void parport_win_write_eppaddr(unsigned char d);
extern unsigned char parport_win_read_eppdata(void);
extern void parport_win_write_eppdata(unsigned char d);
extern int pp_win_epp_clear_timeout(void);

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
	pp_win_epp_clear_timeout();  /* prevent lockup of some SMSC IC's */
	/* this routine is mostly copied from Linux Kernel parport */
	parport_win_write_econtrol(0xc);
	parport_win_write_control(0xc);
	parport_win_write_data(0xaa);
	if (parport_win_read_data() != 0xaa)
		return 0;
	parport_win_write_data(0x55);
	if (parport_win_read_data() != 0x55)
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

	parport_win_write_control(r);
	if ((parport_win_read_econtrol() & 0x3) == (r & 0x3)) {
		parport_win_write_control(r ^ 0x2); /* Toggle bit 1 */
		r = parport_win_read_control();	
		if ((parport_win_read_econtrol() & 0x2) == (r & 0x2))
			goto no_reg; /* Sure that no ECR register exists */
	}

	if ((parport_win_read_econtrol() & 0x3 ) != 0x1)
		goto no_reg;

	parport_win_write_econtrol(0x34);
	if (parport_win_read_econtrol() != 0x35)
		goto no_reg;

	parport_win_write_control(0xc);
	
	/* Go to mode 000 */
	parport_win_frob_econtrol(0xe0, 0);

	return PARPORT_MODE_PCECR;

 no_reg:
	parport_win_write_control(0xc);
	return 0; 
}

static int parport_ecp(void)
{
	int i;
	int config;
	int pword;
	int fifo_depth, writeIntrThreshold, readIntrThreshold;

	/* Find out FIFO depth */
	parport_win_write_econtrol(0x00); /* Reset FIFO */
	parport_win_write_econtrol(0xc0); /* TEST FIFO */
	for (i=0; i < 1024 && !(parport_win_read_econtrol() & 0x02); i++)
		parport_win_write_configa(0xaa);
	/*
	 * Using LGS chipset it uses ECR register, but
	 * it doesn't support ECP or FIFO MODE
	 */
	if (i == 1024) {
		parport_win_write_econtrol(0x00);
		return 0;
	}

	fifo_depth = i;
	lprintf(3, "ECP: FIFO depth is %d bytes\n", fifo_depth);

	/* Find out writeIntrThreshold */
        parport_win_frob_econtrol((1<<2), (1<<2));
        parport_win_frob_econtrol((1<<2), 0);
	for (i = 1; i <= fifo_depth; i++) {
		parport_win_read_configa();
		usleep(50);
		if (parport_win_read_econtrol() & (1<<2))
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
        parport_win_frob_econtrol(0xe0, 0x20); /* Reset FIFO, PS2 */
	parport_win_frob_control(PARPORT_CONTROL_DIRECTION, PARPORT_CONTROL_DIRECTION);
	parport_win_frob_econtrol(0xe0, 0xc0); /* Test FIFO */
        parport_win_frob_econtrol((1<<2), (1<<2));
        parport_win_frob_econtrol((1<<2), 0);
        for (i = 1; i <= fifo_depth; i++) {
		parport_win_write_configa(0xaa);
		if (parport_win_read_econtrol() & (1<<2))
			break;
	}

	if (i <= fifo_depth)
		lprintf(3, "ECP: readIntrThreshold is %d\n", i);
	else
		/* Number of bytes we can read if we get an interrupt. */
		i = 0;

	readIntrThreshold = i;

	parport_win_write_econtrol(0x00); /* Reset FIFO */
	parport_win_write_econtrol(0xf4); /* Configuration mode */
	config = parport_win_read_configa();
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

	config = parport_win_read_configb();
	lprintf(3, "ECP: Interrupts are ISA-%s\n", config & 0x80 ? "Level" : "Pulses");

	if (!(config & 0x40))
		lprintf(3, "ECP: IRQ conflict!\n");

	/* Go back to mode 000 */
	parport_win_frob_econtrol(0xe0, 0);

	return PARPORT_MODE_PCECP;
}

/* EPP mode detection  */

static int parport_epp(void)
{
	/* If EPP timeout bit clear then EPP available */
	if (!pp_win_epp_clear_timeout())
		return 0;  /* No way to clear timeout */

	/*
	 * Theory:
	 *     Write two values to the EPP address register and
	 *     read them back. When the transfer times out, the state of
	 *     the EPP register is undefined in some cases (EPP 1.9?) but
	 *     in others (EPP 1.7, ECPEPP?) it is possible to read back
	 *     its value.
	 */

	pp_win_epp_clear_timeout();
	usleep(30); /* Wait for possible EPP timeout */

	parport_win_write_eppaddr(0x55);
	pp_win_epp_clear_timeout();
	usleep(30); /* Wait for possible EPP timeout */

	if (parport_win_read_eppaddr() == 0x55) {
		parport_win_write_eppaddr(0xaa);
		pp_win_epp_clear_timeout();
		usleep(30); /* Wait for possible EPP timeout */
		if (parport_win_read_eppaddr() == 0xaa) {
			pp_win_epp_clear_timeout();
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

	parport_win_frob_control(0x20, 0x20);
	parport_win_frob_control(0x10, 0x10);
	pp_win_epp_clear_timeout();
	
	parport_win_read_eppdata();
	usleep(30);  /* Wait for possible EPP timeout */
	
	if (parport_win_read_status() & 0x01) {
		pp_win_epp_clear_timeout();
		return PARPORT_MODE_PCEPP;
	}
	return 0;
}

static int parport_ecpepp(void)
{
	int mode;
	unsigned char oecr;

	oecr = parport_win_read_econtrol();
	/* Search for SMC style EPP+ECP mode */
	parport_win_write_econtrol(0x80);
	mode = parport_epp();
	parport_win_write_econtrol(oecr);
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
	unsigned char octr = parport_win_read_control();
  
	pp_win_epp_clear_timeout();

	parport_win_write_control(octr | 0x20);  /* try to tri-state the buffer */
	
	parport_win_write_data(0x55);
	if (parport_win_read_data() != 0x55) ok++;
	parport_win_write_data(0xaa);
	if (parport_win_read_data() != 0xaa) ok++;

	parport_win_write_control(octr);          /* cancel input mode */

	return ok ? PARPORT_MODE_PCPS2 : 0;
}

static int parport_ecpps2(void)
{
	int mode;
	unsigned char oecr;

	oecr = parport_win_read_econtrol();
	parport_win_write_econtrol(0x20);
      	mode = parport_ps2();
	parport_win_write_econtrol(oecr);
	return mode ? PARPORT_MODE_PCECPPS2 : 0;
}

/* ---------------------------------------------------------------------- */

#define SERVICENAME        "eppflex"
#define SERVICEDISPLAYNAME "Baycom EPPFLEX"
//#define SERVICEBINARY      "\"\\??\\g:\\nteppflex\\eppflex.sys\""
#define SERVICEBINARY      "System32\\drivers\\eppflex.sys"

static int start_service(void)
{
        int ret = -1;
        SC_HANDLE hscm, hserv;
        
        hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!hscm) {
                lprintf(5, "Cannot open SC manager, error 0x%08lx\n", GetLastError());
                return -1;
        }
        hserv = CreateService(hscm, SERVICENAME, SERVICEDISPLAYNAME,
                              SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                              SERVICEBINARY, NULL, NULL, NULL, NULL, NULL);
        if (!hserv) {
                lprintf(5, "Cannot create service, error 0x%08lx\n", GetLastError());
                hserv = OpenService(hscm, SERVICENAME, SERVICE_ALL_ACCESS);
                if (!hserv) {
                        lprintf(5, "Cannot open service, error 0x%08lx\n", GetLastError());
                        goto closescm;
                }
        }
        if (!StartService(hserv, 0, NULL)) {
                lprintf(5, "Cannot start service, error 0x%08lx\n", GetLastError());
                goto closeserv;
        }
        lprintf(1, "Service %s started successfully\n", SERVICENAME);
        ret = 0;

  closeserv:
        if (!CloseServiceHandle(hserv))
                lprintf(5, "Cannot close service handle, error 0x%08lx\n", GetLastError());
  closescm:
        if (!CloseServiceHandle(hscm))
                lprintf(5, "Cannot close service manager handle, error 0x%08lx\n", GetLastError());
        return ret;
}

static int stop_service(void)
{
        int ret = -1;
        SC_HANDLE hscm, hserv;
        SERVICE_STATUS sstat;
        
        hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!hscm) {
                lprintf(5, "Cannot open SC manager, error 0x%08lx\n", GetLastError());
                return -1;
        }
        hserv = OpenService(hscm, SERVICENAME, SERVICE_ALL_ACCESS);
        if (!hserv) {
                lprintf(5, "Cannot open service, error 0x%08lx\n", GetLastError());
                goto closescm;
        }
        ret = 0;
        if (!ControlService(hserv, SERVICE_CONTROL_STOP, &sstat)) {
                lprintf(5, "Cannot delete service, error 0x%08lx\n", GetLastError());
                ret = -1;
        }
        if (!DeleteService(hserv)) {
                lprintf(5, "Cannot delete service, error 0x%08lx\n", GetLastError());
                ret = -1;
        }
        if (!ret)
                lprintf(1, "Service %s stopped successfully\n", SERVICENAME);
        if (!CloseServiceHandle(hserv))
                lprintf(5, "Cannot close service handle, error 0x%08lx\n", GetLastError());
  closescm:
        if (!CloseServiceHandle(hscm))
                lprintf(5, "Cannot close service manager handle, error 0x%08lx\n", GetLastError());
        return ret;
}

/* ---------------------------------------------------------------------- */

extern inline int isnt()
{
        OSVERSIONINFO info;

        info.dwOSVersionInfoSize = sizeof(info);
        if (GetVersionEx(&info) && info.dwPlatformId == VER_PLATFORM_WIN32_NT)
                return 1;
        return 0;
}

/* ---------------------------------------------------------------------- */

void parport_stop_win(void)
{
        DWORD bytesret;
        int nt = isnt();

        if (pp_win_handle != INVALID_HANDLE_VALUE) {
                if (!nt)
                        DeviceIoControl(pp_win_handle, IOCTL_EPPFLEX_RELEASEPORT, NULL, 0, NULL, 0, &bytesret, NULL);
                CloseHandle(pp_win_handle);
        }
	pp_win_handle = INVALID_HANDLE_VALUE;
        if (nt)
                stop_service();
}

/* ---------------------------------------------------------------------- */

int parport_init_win_flags(unsigned int portnr, unsigned int flags)
{
        char buf[32];
        DWORD bytesret;
        
	if (pp_win_handle != INVALID_HANDLE_VALUE)
		CloseHandle(pp_win_handle);
        if (isnt()) {
                start_service();
                snprintf(buf, sizeof(buf), "\\\\.\\eppflex\\%u", portnr);
                pp_win_handle = CreateFile(buf, GENERIC_READ | GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                           OPEN_EXISTING, 0, NULL);
                if (pp_win_handle == INVALID_HANDLE_VALUE) {
                        lprintf(0, "Cannot open eppflex.sys driver, error 0x%08lx\n", GetLastError());
                        goto err;
                }
        } else {
                pp_win_handle = CreateFile("\\\\.\\EPPFLEX.VXD", 0, 0, NULL, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
                if (pp_win_handle == INVALID_HANDLE_VALUE) {
                        lprintf(0, "Cannot open eppflex.vxd driver, error 0x%08lx\n", GetLastError());
                        goto err;
                }
                if (!DeviceIoControl(pp_win_handle, IOCTL_EPPFLEX_ACQUIREPORT, &portnr, sizeof(portnr),
                                     NULL, 0, &bytesret, NULL)) {
                        CloseHandle(pp_win_handle);
                        pp_win_handle = INVALID_HANDLE_VALUE;
                        lprintf(0, "Cannot open acquire port LPT%u, error 0x%08lx\n", portnr+1, GetLastError());
                        goto err;
                }
        }
	pp_win_flags = FLAGS_PCSPP;
	if (!parport_spp()) {
		lprintf(0, "No parport present\n");
		goto err;
	}
	if (parport_ecr()) {
		pp_win_flags |= FLAGS_PCECR;
		pp_win_flags |= parport_ecp();
		pp_win_flags |= parport_ecpps2();
		pp_win_flags |= parport_ecpepp();
		if ((flags & PPFLAG_FORCEHWEPP) && 
		    (pp_win_flags & (FLAGS_PCPS2|FLAGS_PCECPPS2)) &&
		    !(pp_win_flags & (FLAGS_PCEPP|FLAGS_PCECPEPP)))
			pp_win_flags |= FLAGS_PCECPEPP;
		else
			flags &= ~PPFLAG_FORCEHWEPP;
	} else {
		pp_win_flags |= parport_ps2();
		pp_win_flags |= parport_epp();
		if ((flags & PPFLAG_FORCEHWEPP) && 
		    (pp_win_flags & (FLAGS_PCPS2|FLAGS_PCECPPS2)) &&
		    !(pp_win_flags & (FLAGS_PCEPP|FLAGS_PCECPEPP)))
			pp_win_flags |= FLAGS_PCEPP;
		else
			flags &= ~PPFLAG_FORCEHWEPP;
	}
	lprintf(0, "Parport capabilities: SPP%s%s%s%s%s%s\n",
		(pp_win_flags & FLAGS_PCPS2) ? ", PS2" : "",
		(pp_win_flags & FLAGS_PCEPP) ? ((flags & PPFLAG_FORCEHWEPP) ? ", EPP (forced)" : ", EPP") : "",
		(pp_win_flags & FLAGS_PCECR) ? ", ECR" : "",
		(pp_win_flags & FLAGS_PCECP) ? ", ECP" : "",
		(pp_win_flags & FLAGS_PCECPEPP) ? ((flags & PPFLAG_FORCEHWEPP) ? ", ECPEPP (forced)" : ", ECPEPP") : "",
		(pp_win_flags & FLAGS_PCECPPS2) ? ", ECPPS2" : "");
	if (!(pp_win_flags & (FLAGS_PCPS2 | FLAGS_PCECPPS2)))  {
		lprintf(0, "Parport does not even support PS/2 mode, cannot use it\n");
		goto err;
	}
	lprintf(0, "Parport using eppflex.sys/vxd hardware access");
	if (pp_win_flags & FLAGS_PCECR)
		parport_win_write_econtrol(0x30); /* PS/2 mode */
	parport_ops = parport_win_ops;
	if ((flags & PPFLAG_SWEMULECP) || !(pp_win_flags & FLAGS_PCECP)) {
		parport_ops.parport_ecp_write_data = parport_win_emul_ops.parport_ecp_write_data;
		parport_ops.parport_ecp_read_data = parport_win_emul_ops.parport_ecp_read_data;
		parport_ops.parport_ecp_write_addr = parport_win_emul_ops.parport_ecp_write_addr;
		lprintf(0, ", emulating ECP");
	}
	if ((flags & PPFLAG_SWEMULEPP) || !(pp_win_flags & (FLAGS_PCEPP | FLAGS_PCECPEPP))) {
		parport_ops.parport_epp_write_data = parport_win_emul_ops.parport_epp_write_data;
		parport_ops.parport_epp_read_data = parport_win_emul_ops.parport_epp_read_data;
		parport_ops.parport_epp_write_addr = parport_win_emul_ops.parport_epp_write_addr;
		parport_ops.parport_epp_read_addr = parport_win_emul_ops.parport_epp_read_addr;
		lprintf(0, ", emulating EPP");
	}
	lprintf(0, "\n");
	return 0;

  err:
        parport_stop_win();
	return -1;
}

int parport_init_win(unsigned int portnr)
{
        return parport_init_win_flags(portnr, 0);
}
