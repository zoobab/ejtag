
#include "ntddk.h"
#include "string.h"
#include "ddk/parallel.h"

#include "eppflex.h"


#define printk DbgPrint
#define kmalloc(size) ExAllocatePool(NonPagedPool,size)
#define kfree ExFreePool

#if 0
#define VLOG(x)  do { x; } while (0)
#else
#define VLOG(x)  do {  } while (0)
#endif

#if 1
#define ELOG(x)  do { x; } while (0)
#else
#define ELOG(x)  do {  } while (0)
#endif

struct eppflex {
        unsigned int dummy;
};

struct eppflexfile {
        PDEVICE_OBJECT pdo;
        PFILE_OBJECT pdfo;
        PARALLEL_PORT_INFORMATION info;
        PARALLEL_PNP_INFORMATION pnpinfo;
};


static NTSTATUS parcompletion(PDEVICE_OBJECT devobj, PIRP irp, PKEVENT event)
{
        KeSetEvent(event, 0, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS callparport(struct eppflexfile *fs, unsigned long ioctlcode,
			    void *buffer, unsigned int inlen, unsigned int outlen,
			    LARGE_INTEGER *timeout)
{
        NTSTATUS status;
        PIRP irp;
        PIO_STACK_LOCATION irpsp;
        KEVENT event;

        irp = IoAllocateIrp((CCHAR)(fs->pdo->StackSize+1), FALSE);
        if (!irp)
                return STATUS_INSUFFICIENT_RESOURCES;
        irpsp = IoGetNextIrpStackLocation(irp);
        irpsp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpsp->Parameters.DeviceIoControl.OutputBufferLength = outlen;
        irpsp->Parameters.DeviceIoControl.InputBufferLength = inlen;
        irpsp->Parameters.DeviceIoControl.IoControlCode = ioctlcode;
        irp->AssociatedIrp.SystemBuffer = buffer;
  
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        IoSetCompletionRoutine(irp, parcompletion, &event, TRUE, TRUE, TRUE);
        status = IoCallDriver(fs->pdo, irp);
        if (!NT_SUCCESS(status)) {
                IoFreeIrp(irp);
                return status;
        }
  
        status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, timeout);
        if (status == STATUS_TIMEOUT) {
                IoCancelIrp(irp);
                status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }
        status = irp->IoStatus.Status;
        IoFreeIrp(irp);
        return status;
}

static NTSTATUS read_port(PDEVICE_OBJECT devobj, PIRP irp, PUCHAR port)
{
        NTSTATUS status = STATUS_SUCCESS;
        PIO_STACK_LOCATION irpsp;
        PFILE_OBJECT fileobj;
        struct eppflex *s = devobj->DeviceExtension;
        struct eppflexfile *fs;
        struct eppflex_rwdata *rw = (struct eppflex_rwdata *)irp->AssociatedIrp.SystemBuffer;
  
        irpsp = IoGetCurrentIrpStackLocation(irp);
        fileobj = irpsp->FileObject;
        fs = fileobj->FsContext;

        irp->IoStatus.Information = 0;
        if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(struct eppflex_rwdata))
                status = STATUS_INVALID_PARAMETER;
        else if ((port < fs->info.Controller || port >= fs->info.Controller+fs->info.SpanOfController) &&
                 (port < fs->pnpinfo.EcpController || port >= fs->pnpinfo.EcpController+fs->pnpinfo.SpanOfEcpController))
                status = STATUS_ACCESS_VIOLATION;
        else {
                rw->data = READ_PORT_UCHAR(port);
                irp->IoStatus.Information = sizeof(struct eppflex_rwdata);
        }
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
}

static NTSTATUS write_port(PDEVICE_OBJECT devobj, PIRP irp, PUCHAR port)
{
        NTSTATUS status = STATUS_SUCCESS;
        PIO_STACK_LOCATION irpsp;
        PFILE_OBJECT fileobj;
        struct eppflex *s = devobj->DeviceExtension;
        struct eppflexfile *fs;
        struct eppflex_rwdata *rw = (struct eppflex_rwdata *)irp->AssociatedIrp.SystemBuffer;
  
        irpsp = IoGetCurrentIrpStackLocation(irp);
        fileobj = irpsp->FileObject;
        fs = fileobj->FsContext;

        irp->IoStatus.Information = 0;
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct eppflex_rwdata))
                status = STATUS_INVALID_PARAMETER;
        else if ((port < fs->info.Controller || port >= fs->info.Controller+fs->info.SpanOfController) &&
                 (port < fs->pnpinfo.EcpController || port >= fs->pnpinfo.EcpController+fs->pnpinfo.SpanOfEcpController))
                status = STATUS_ACCESS_VIOLATION;
        else {
                WRITE_PORT_UCHAR(port, rw->data);
        }
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
}

static NTSTATUS frob_port(PDEVICE_OBJECT devobj, PIRP irp, PUCHAR port)
{
        NTSTATUS status = STATUS_SUCCESS;
        PIO_STACK_LOCATION irpsp;
        PFILE_OBJECT fileobj;
        struct eppflex *s = devobj->DeviceExtension;
        struct eppflexfile *fs;
        struct eppflex_rwdata *rw = (struct eppflex_rwdata *)irp->AssociatedIrp.SystemBuffer;
        UCHAR data;

        irpsp = IoGetCurrentIrpStackLocation(irp);
        fileobj = irpsp->FileObject;
        fs = fileobj->FsContext;

        irp->IoStatus.Information = 0;
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct eppflex_rwdata) ||
            irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(struct eppflex_rwdata))
                status = STATUS_INVALID_PARAMETER;
        else if ((port < fs->info.Controller || port >= fs->info.Controller+fs->info.SpanOfController) &&
                 (port < fs->pnpinfo.EcpController || port >= fs->pnpinfo.EcpController+fs->pnpinfo.SpanOfEcpController))
                status = STATUS_ACCESS_VIOLATION;
        else {
                data = READ_PORT_UCHAR(port);
                data = (data & ~rw->mask) ^ rw->data;
                WRITE_PORT_UCHAR(port, data);
                rw->data = data;
                irp->IoStatus.Information = sizeof(struct eppflex_rwdata);
        }
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
}

static NTSTATUS ioctl(PDEVICE_OBJECT devobj, PIRP irp)
{
        NTSTATUS status = STATUS_NOT_IMPLEMENTED;
        PIO_STACK_LOCATION irpsp;
        PFILE_OBJECT fileobj;
        struct eppflex *s = devobj->DeviceExtension;
        struct eppflexfile *fs;

        irpsp = IoGetCurrentIrpStackLocation(irp);
        fileobj = irpsp->FileObject;
        fs = fileobj->FsContext;

        VLOG(printk("eppflex: ioctl\n"));

        if (!fs) {
                status = irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return status;
        }
        switch (irpsp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_EPPFLEX_READ_DATA:
                return read_port(devobj, irp, fs->info.Controller);

        case IOCTL_EPPFLEX_WRITE_DATA:
                return write_port(devobj, irp, fs->info.Controller);

        case IOCTL_EPPFLEX_READ_STATUS:
                return read_port(devobj, irp, fs->info.Controller + 1);

        case IOCTL_EPPFLEX_WRITE_STATUS:
                return write_port(devobj, irp, fs->info.Controller + 1);

        case IOCTL_EPPFLEX_READ_CONTROL:
                return read_port(devobj, irp, fs->info.Controller + 2);

        case IOCTL_EPPFLEX_WRITE_CONTROL:
                return write_port(devobj, irp, fs->info.Controller + 2);

        case IOCTL_EPPFLEX_FROB_CONTROL:
                return frob_port(devobj, irp, fs->info.Controller + 2);

        case IOCTL_EPPFLEX_READ_ECONTROL:
                return read_port(devobj, irp, fs->pnpinfo.EcpController + 2);

        case IOCTL_EPPFLEX_WRITE_ECONTROL:
                return write_port(devobj, irp, fs->pnpinfo.EcpController + 2);

        case IOCTL_EPPFLEX_FROB_ECONTROL:
                return frob_port(devobj, irp, fs->pnpinfo.EcpController + 2);

        case IOCTL_EPPFLEX_READ_CONFIGA:
                return read_port(devobj, irp, fs->pnpinfo.EcpController);

        case IOCTL_EPPFLEX_WRITE_CONFIGA:
                return write_port(devobj, irp, fs->pnpinfo.EcpController);

        case IOCTL_EPPFLEX_READ_CONFIGB:
                return read_port(devobj, irp, fs->pnpinfo.EcpController + 1);

        case IOCTL_EPPFLEX_WRITE_CONFIGB:
                return write_port(devobj, irp, fs->pnpinfo.EcpController + 1);

        case IOCTL_EPPFLEX_READ_EPPADDR:
                return read_port(devobj, irp, fs->info.Controller + 3);

        case IOCTL_EPPFLEX_WRITE_EPPADDR:
                return write_port(devobj, irp, fs->info.Controller + 3);

        case IOCTL_EPPFLEX_READ_EPPDATA:
                return read_port(devobj, irp, fs->info.Controller + 4);

        case IOCTL_EPPFLEX_WRITE_EPPDATA:
                return write_port(devobj, irp, fs->info.Controller + 4);
        }
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
}

static NTSTATUS create(PDEVICE_OBJECT devobj, PIRP irp)
{
        NTSTATUS status;
        PIO_STACK_LOCATION irpsp;
        PFILE_OBJECT fileobj;
        WCHAR ppname[22] = L"\\Device\\ParallelPort0";
        UNICODE_STRING uppname;
        struct eppflex *s = devobj->DeviceExtension;
        struct eppflexfile *fs;
        ULONG devnr = 0;
        LARGE_INTEGER timeout;
        unsigned int i;
        
        VLOG(printk("eppflex: create\n"));
        irpsp = IoGetCurrentIrpStackLocation(irp);
        fileobj = irpsp->FileObject;
        status = STATUS_NO_MEMORY;
        fileobj->FsContext = fs = kmalloc(sizeof(struct eppflexfile));
        if (!fs)
                goto out;

        VLOG(printk("eppflex: filename length: %u\n", fileobj->FileName.Length));

        for (i = 0; i < fileobj->FileName.Length; i++) {
                VLOG(printk("eppflex: fn[%2u]: %04x\n", i, fileobj->FileName.Buffer[i]));
                if (fileobj->FileName.Buffer[i] == (WCHAR)'\\')
                        continue;
                if (fileobj->FileName.Buffer[i] < (WCHAR)'0' ||
                    fileobj->FileName.Buffer[i] > (WCHAR)'9')
                        break;
                devnr = 10 * devnr + fileobj->FileName.Buffer[i] - (WCHAR)'0';
        }
        
        VLOG(printk("eppflex: Device number %lu\n", devnr));

        status = STATUS_INVALID_PARAMETER;
        if (devnr > 9)
                goto out;
    

        ppname[20] = ((WCHAR)'0') + (WCHAR)devnr;
        RtlInitUnicodeString(&uppname, ppname);

        status = IoGetDeviceObjectPointer(&uppname, STANDARD_RIGHTS_ALL, &fs->pdfo, &fs->pdo);
        if (!NT_SUCCESS(status)) {
                ELOG(printk("eppflex: cannot open parallel port %u\n", devnr));
                goto out;
        }
    
        timeout.QuadPart = -(LONGLONG)10*1000*500;  /* 500ms, in 100ns units */
        status = callparport(fs, IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
                             &fs->info, 0, sizeof(fs->info), &timeout);
        if (!NT_SUCCESS(status)) {
                ELOG(printk("eppflex: GET_PARALLEL_PORT_INFO error 0x%lx\n", status));
                goto out;
        }
        status = callparport(fs, IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO,
                             &fs->pnpinfo, 0, sizeof(fs->pnpinfo), &timeout);
        if (!NT_SUCCESS(status)) {
                ELOG(printk("eppflex: warning: GET_PARALLEL_PNP_INFO error 0x%lx\n", status));
                fs->pnpinfo.EcpController = fs->info.Controller + 0x400;
                fs->pnpinfo.SpanOfEcpController = fs->info.SpanOfController;
                fs->pnpinfo.HardwareCapabilities = 0;
                fs->pnpinfo.FifoDepth = 0;
                fs->pnpinfo.FifoWidth = 0;
        }

        VLOG(printk("eppflex: ioaddr: 0x%x(%u)  ecpioaddr: 0x%x(%u)  cap: 0x%x  FIFO: %d,%d\n",
                    fs->info.Controller, fs->info.SpanOfController,
                    fs->pnpinfo.EcpController, fs->pnpinfo.SpanOfEcpController,
                    fs->pnpinfo.HardwareCapabilities, fs->pnpinfo.FifoDepth, fs->pnpinfo.FifoWidth));
        
        timeout.QuadPart = -(LONGLONG)10*1000*50;  /* 50ms, in 100ns units */
        status = callparport(fs, IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE, NULL, 0, 0, &timeout);
        if (!NT_SUCCESS(status)) {
                ELOG(printk("eppflex: PARALLEL_PORT_ALLOCATE error 0x%lx\n", status));
                goto out;
        }

        status = STATUS_SUCCESS;

  out:
        if (!NT_SUCCESS(status) && fs) {
                kfree(fs);
                fileobj->FsContext = NULL;
        }
        irp->IoStatus.Status = status;
        irp->IoStatus.Information = 0L;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
}

static NTSTATUS close(PDEVICE_OBJECT devobj, PIRP irp)
{
        NTSTATUS status;
        PIO_STACK_LOCATION irpsp;
        PFILE_OBJECT fileobj;
        struct eppflex *s = devobj->DeviceExtension;
        struct eppflexfile *fs;
        LARGE_INTEGER timeout;

        irpsp = IoGetCurrentIrpStackLocation(irp);
        fileobj = irpsp->FileObject;
        fs = fileobj->FsContext;
        VLOG(printk("eppflex: close\n"));
        if (fs) {

                timeout.QuadPart = -(LONGLONG)10*1000*50;  /* 50ms, in 100ns units */
                status = callparport(fs, IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO, NULL, 0, 0, &timeout);
                if (!NT_SUCCESS(status))
                        ELOG(printk("eppflex: RELEASE_PARALLEL_PORT_INFO error 0x%lx\n", status));
                status = callparport(fs, IOCTL_INTERNAL_PARALLEL_PORT_FREE, NULL, 0, 0, &timeout);
                if (!NT_SUCCESS(status))
                        ELOG(printk("eppflex: PARALLEL_PORT_FREE error 0x%lx\n", status));
                ObDereferenceObject(fs->pdfo);
                kfree(fs);
                fileobj->FsContext = NULL;
        }
        status = irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = 0L;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
}

static void unload(PDRIVER_OBJECT drvobj)
{
        struct eppflex *s = drvobj->DeviceObject->DeviceExtension;
        UNICODE_STRING nameString, linkString;

        printk("eppflex: stop\n");

        IoDeleteDevice(drvobj->DeviceObject);
        RtlInitUnicodeString(&linkString, L"\\DosDevices\\EPPFLEX" );
        RtlInitUnicodeString(&nameString, L"\\Device\\eppflex" );
        IoDeleteSymbolicLink(&linkString);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT drvobj, PUNICODE_STRING RegistryPath)
{
        UNICODE_STRING nameString, linkString;
        PDEVICE_OBJECT devobj;
        NTSTATUS status;
        struct eppflex *s;


        /*
         * Create the device object.
         */
        RtlInitUnicodeString(&nameString, L"\\Device\\eppflex" );
        status = IoCreateDevice(drvobj, sizeof(struct eppflex), &nameString, FILE_DEVICE_UNKNOWN, 0, TRUE, &devobj);
        if (!NT_SUCCESS(status))
                return status;

        /*
         * Create the symbolic link so the VDD can find us.
         */
        RtlInitUnicodeString( &linkString, L"\\DosDevices\\EPPFLEX" );
        status = IoCreateSymbolicLink (&linkString, &nameString);
        if (!NT_SUCCESS(status)) {
                IoDeleteDevice(drvobj->DeviceObject);
                return status;
        }

        /*
         * Initialize the driver object with this device driver's entry points.
         */

        drvobj->DriverUnload = unload;
        drvobj->MajorFunction[IRP_MJ_CREATE] = create;
        drvobj->MajorFunction[IRP_MJ_CLOSE] = close;
        drvobj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctl;

        s = devobj->DeviceExtension;

        /*
         * Initialize the device extension.
         */

        printk("eppflex: started "__DATE__" "__TIME__"\n");

        return STATUS_SUCCESS;
}
