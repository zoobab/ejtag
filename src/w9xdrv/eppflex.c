/*
 * Thomas Sailer, (C) 2000
 */

#include <basedef.h>
#include <vmm.h>
#include <vmmreg.h>
#include <debug.h>
#include <vwin32.h>
#include <vcomm.h>        /* must be included before VXDWRAPS.H */
#include <vxdwraps.h>
#include <winerror.h>
#include <configmg.h>
#include "eppflex.h"

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

#define printk(x) Out_Debug_String(x)

#define MAXPORTS 4

static struct portconfig {
	unsigned int addr0, addr0end;
	unsigned int addr1, addr1end;
	int irq;
	int dma;
} portconfig[MAXPORTS];

static DWORD porthandle = 0;
static PFN conthandler = NULL;
static unsigned int portnr;

static DWORD StealNotifyHandler( void *RefData, DWORD dwNotification )
{
        /* steal handler: we do not allow anyone to steal our port */
        return FALSE;
}

static UCHAR inb(WORD port)
{
        _asm {
                mov dx,[port]
                in al,dx
                jmp del1
del1:           jmp del2
del2:           xor ah,ah
        }
}

static void outb(UCHAR data, WORD port)
{
        _asm {
                mov dx,[port]
                mov al,byte ptr data
                out dx,al
                jmp del1
del1:           jmp del2
del2:
        }
}

static void releaseport(void)
{
        if (!conthandler)
                return;
        (*conthandler)(RELEASE_RESOURCE, porthandle, StealNotifyHandler);
        porthandle = 0;
}

static int acquireport(unsigned int port)
{
        char lptname[] = "LPTx";
        int ret;
        
        if (porthandle)
                releaseport();
        porthandle = 0;
        if (port >= MAXPORTS)
                return ERROR_BAD_DEVICE;
	if (!portconfig[port].addr0)
		return ERROR_BAD_DEVICE;
	portnr = port;
	lptname[3] = '1' + port;
        conthandler = VCOMM_Get_Contention_Handler(lptname);
        if (!conthandler)
                return ERROR_BAD_DEVICE;
        porthandle = VCOMM_Map_Name_To_Resource(lptname);
        if (!porthandle)
                return ERROR_BAD_DEVICE;
        ret = (*conthandler)(ACQUIRE_RESOURCE, porthandle, StealNotifyHandler, porthandle, TRUE);
        if (!ret)
                return ERROR_BAD_DEVICE;
        return NO_ERROR;
}

/*
 * LPT find configuration stuff
 */

static void printknum(DWORD num)
{
	char buf[16];
	char *cp = &buf[16];

	*--cp = 0;
	do {
		*--cp = '0' + (num % 10);
		num /= 10;
	} while (num);
	printk(cp);
}

static void printkhex(DWORD num, DWORD digits)
{
	char buf[9];
	char *cp = &buf[9];

	*--cp = 0;
	if (digits > 8)
		digits = 8;
	if (!digits)
		return;
	do {
		if ((num & 15) >= 10)
			*--cp = 'A' - 10 + (num & 15);
		else
			*--cp = '0' + (num & 15);
		num >>= 4;
		digits--;
	} while (digits);
	printk(cp);
}

static void printdevid(DEVNODE dn)
{
	char buf[MAX_DEVICE_ID_LEN];

	if (CM_Get_Device_ID(dn, buf, sizeof(buf), 0) != CR_SUCCESS)
		return;
	printk("Device ID: ");
	printk(buf);
	printk("\n");
}

static void nodefoundport(DEVNODE dn, unsigned int portnr)
{
	LOG_CONF lc;
	RES_DES rd;
	IO_DES iod;
	IRQ_DES irqd;
	DMA_DES dmad;
	struct portconfig cfg;

	if (portnr >= MAXPORTS) {
		printk("Port number overflow\n");
		return;
	}
	cfg.addr0 = cfg.addr0end = cfg.addr1 = cfg.addr1end = 0;
	cfg.irq = cfg.dma = -1;
	if (CM_Get_First_Log_Conf(&lc, dn, ALLOC_LOG_CONF) == CR_SUCCESS) {
		rd = (RES_DES)lc;
		while (CM_Get_Next_Res_Des(&rd, rd, ResType_IO, NULL, 0) == CR_SUCCESS) {
			if (CM_Get_Res_Des_Data(rd, &iod, sizeof(iod), 0) == CR_SUCCESS) {
				if (iod.IOD_Alloc_Base >= 0x400) {
					cfg.addr1 = iod.IOD_Alloc_Base;
					cfg.addr1end = iod.IOD_Alloc_End;
				} else {
					cfg.addr0 = iod.IOD_Alloc_Base;
					cfg.addr0end = iod.IOD_Alloc_End;
				}
			}
		}
		rd = (RES_DES)lc;
		while (CM_Get_Next_Res_Des(&rd, rd, ResType_IRQ, NULL, 0) == CR_SUCCESS) {
			if (CM_Get_Res_Des_Data(rd, &irqd, sizeof(irqd), 0) == CR_SUCCESS) {
				cfg.irq = irqd.IRQD_Alloc_Num;
			}
		}
		rd = (RES_DES)lc;
		while (CM_Get_Next_Res_Des(&rd, rd, ResType_DMA, NULL, 0) == CR_SUCCESS) {
			if (CM_Get_Res_Des_Data(rd, &dmad, sizeof(dmad), 0) == CR_SUCCESS) {
				cfg.dma = dmad.DD_Alloc_Chan;
			}
		}
	}
	if (cfg.addr0)
		portconfig[portnr] = cfg;
	printk("LPT");
	printknum(portnr+1);
	printk(": IO Ranges: ");
	printkhex(cfg.addr0, 4);
	printk("-");
	printkhex(cfg.addr0end, 4);
	printk(" ");
	printkhex(cfg.addr1, 4);
	printk("-");
	printkhex(cfg.addr1end, 4);
	if (cfg.irq != -1) {
		printk("  IRQ ");
		printknum(cfg.irq);
	}
	if (cfg.dma != -1) {
		printk("  DMA ");
		printknum(cfg.dma);
	}
	printk("\n");
}

static void nodeconfig(DEVNODE dn)
{
	char key[MAX_VMM_REG_KEY_LEN];
	char portname[64];
	ULONG portsz;
	VMMHKEY hkey;

	if (CM_Get_DevNode_Key(dn, NULL, &key, sizeof(key), CM_REGISTRY_HARDWARE) == CR_SUCCESS) {
		portsz = sizeof(portname);
		if (_RegOpenKey(HKEY_LOCAL_MACHINE, key, &hkey) == ERROR_SUCCESS) {
			if (_RegQueryValueEx(hkey, "PORTNAME", NULL, NULL, portname, &portsz) == ERROR_SUCCESS) {
				printdevid(dn);
				printk("Port name: ");
				printk(portname);
				printk("\n");
				if (portname[0] = 'L' && portname[1] == 'P' && portname[2] == 'T' && portname[4] == 0)
					nodefoundport(dn, portname[3] - '1');
			}
			_RegCloseKey(hkey);
		}
	}
}

static void nodesubtree(DEVNODE dn)
{
	DWORD dnch;

	do {
		nodeconfig(dn);
		if (CM_Get_Child(&dnch, dn, 0) == CR_SUCCESS)
			nodesubtree(dnch);
	} while (CM_Get_Sibling(&dn, dn, 0) == CR_SUCCESS);
}

static void retrieveconfig(void)
{
	DEVNODE dn;
	CONFIGRET ret;

	ret = CM_Locate_DevNode(&dn, NULL, 0);
	if (ret != CR_SUCCESS) {
		printk("Locate_DevNode error: ");
		printknum(ret);
		printk("\n");
		return;
	}
	nodesubtree(dn);
}

BOOL OnSysDynamicDeviceInit(void)
{
	printk("eppflex: dynamic init\n");

	memset(portconfig, 0, sizeof(portconfig));
	retrieveconfig();

        return TRUE;
}

BOOL OnSysDynamicDeviceExit(void)
{
	printk("eppflex: dynamic exit\n");
        if (porthandle)
                releaseport();
        return TRUE;
}

static DWORD read_port(PDIOCPARAMETERS p, WORD port)
{
        struct eppflex_rwdata *rwout = (struct eppflex_rwdata *)p->lpvOutBuffer;

        if (!porthandle)
                return ERROR_INVALID_PARAMETER;
        if (p->cbOutBuffer < sizeof(struct eppflex_rwdata))
                return ERROR_INVALID_PARAMETER;
	if (!port)
		return ERROR_INVALID_PARAMETER;
	if ((port < portconfig[portnr].addr0 || port > portconfig[portnr].addr0end) &&
	    (port < portconfig[portnr].addr1 || port > portconfig[portnr].addr1end))
		return ERROR_INVALID_PARAMETER;
        rwout->data = inb(port);
	if (p->lpcbBytesReturned)
		*((DWORD *)p->lpcbBytesReturned) = sizeof(struct eppflex_rwdata);
        return NO_ERROR;
}

static DWORD write_port(PDIOCPARAMETERS p, WORD port)
{
        struct eppflex_rwdata *rwin = (struct eppflex_rwdata *)p->lpvInBuffer;

        if (!porthandle)
                return ERROR_INVALID_PARAMETER;
        if (p->cbInBuffer < sizeof(struct eppflex_rwdata))
                return ERROR_INVALID_PARAMETER;
	if (!port)
		return ERROR_INVALID_PARAMETER;
	if ((port < portconfig[portnr].addr0 || port > portconfig[portnr].addr0end) &&
	    (port < portconfig[portnr].addr1 || port > portconfig[portnr].addr1end))
		return ERROR_INVALID_PARAMETER;
        outb(rwin->data, port);
	if (p->lpcbBytesReturned)
		*((DWORD *)p->lpcbBytesReturned) = 0;
        return NO_ERROR;
}

static DWORD frob_port(PDIOCPARAMETERS p, WORD port)
{
        struct eppflex_rwdata *rwin = (struct eppflex_rwdata *)p->lpvInBuffer;
        struct eppflex_rwdata *rwout = (struct eppflex_rwdata *)p->lpvOutBuffer;
        unsigned char data;
        
        if (!porthandle)
                return ERROR_INVALID_PARAMETER;
        if (p->cbInBuffer < sizeof(struct eppflex_rwdata) ||
            p->cbOutBuffer < sizeof(struct eppflex_rwdata))
                return ERROR_INVALID_PARAMETER;
	if (!port)
		return ERROR_INVALID_PARAMETER;
	if ((port < portconfig[portnr].addr0 || port > portconfig[portnr].addr0end) &&
	    (port < portconfig[portnr].addr1 || port > portconfig[portnr].addr1end))
		return ERROR_INVALID_PARAMETER;
        data = inb(port);
        data = (data & ~rwin->mask) ^ rwin->data;
        outb(data, port);
        rwout->data = data;
        rwout->mask = rwin->mask;
	if (p->lpcbBytesReturned)
		*((DWORD *)p->lpcbBytesReturned) = sizeof(struct eppflex_rwdata);
        return NO_ERROR;
}

DWORD OnW32DeviceIoControl(PDIOCPARAMETERS p)
{
	if (p->lpcbBytesReturned)
		*((DWORD *)p->lpcbBytesReturned) = 0;
        switch (p->dwIoControlCode) {
        case DIOC_OPEN:
                return NO_ERROR;
                
        case DIOC_CLOSEHANDLE:
                return NO_ERROR;

        case IOCTL_EPPFLEX_READ_DATA:
                return read_port(p, (WORD)(portconfig[portnr].addr0));

        case IOCTL_EPPFLEX_WRITE_DATA:
                return write_port(p, (WORD)(portconfig[portnr].addr0));

        case IOCTL_EPPFLEX_READ_STATUS:
                return read_port(p, (WORD)(portconfig[portnr].addr0 + 1));

        case IOCTL_EPPFLEX_WRITE_STATUS:
                return write_port(p, (WORD)(portconfig[portnr].addr0 + 1));

        case IOCTL_EPPFLEX_READ_CONTROL:
                return read_port(p, (WORD)(portconfig[portnr].addr0 + 2));

        case IOCTL_EPPFLEX_WRITE_CONTROL:
                return write_port(p, (WORD)(portconfig[portnr].addr0 + 2));

        case IOCTL_EPPFLEX_FROB_CONTROL:
                return frob_port(p, (WORD)(portconfig[portnr].addr0 + 2));

        case IOCTL_EPPFLEX_READ_ECONTROL:
                return read_port(p, (WORD)(portconfig[portnr].addr1 + 2));

        case IOCTL_EPPFLEX_WRITE_ECONTROL:
                return write_port(p, (WORD)(portconfig[portnr].addr1 + 2));

        case IOCTL_EPPFLEX_FROB_ECONTROL:
                return frob_port(p, (WORD)(portconfig[portnr].addr1 + 2));

        case IOCTL_EPPFLEX_READ_CONFIGA:
                return read_port(p, (WORD)(portconfig[portnr].addr1));

        case IOCTL_EPPFLEX_WRITE_CONFIGA:
                return write_port(p, (WORD)(portconfig[portnr].addr1));

        case IOCTL_EPPFLEX_READ_CONFIGB:
                return read_port(p, (WORD)(portconfig[portnr].addr1 + 1));

        case IOCTL_EPPFLEX_WRITE_CONFIGB:
                return write_port(p, (WORD)(portconfig[portnr].addr1 + 1));

        case IOCTL_EPPFLEX_READ_EPPADDR:
                return read_port(p, (WORD)(portconfig[portnr].addr0 + 3));

        case IOCTL_EPPFLEX_WRITE_EPPADDR:
                return write_port(p, (WORD)(portconfig[portnr].addr0 + 3));

        case IOCTL_EPPFLEX_READ_EPPDATA:
                return read_port(p, (WORD)(portconfig[portnr].addr0 + 4));

        case IOCTL_EPPFLEX_WRITE_EPPDATA:
                return write_port(p, (WORD)(portconfig[portnr].addr0 + 4));

        case IOCTL_EPPFLEX_ACQUIREPORT:
                if (p->cbInBuffer < sizeof(unsigned int))
                        return ERROR_INVALID_PARAMETER;
                return acquireport(*((unsigned int *)p->lpvInBuffer));
                
        case IOCTL_EPPFLEX_RELEASEPORT:
                releaseport();
                return NO_ERROR;
     
        default:
                return ERROR_NOT_SUPPORTED;
        }
}
