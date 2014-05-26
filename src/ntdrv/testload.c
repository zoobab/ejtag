#include <windows.h>
#include <stdio.h>

static void getdir(void)
{
        HKEY key;
        char buf[256];
        DWORD blen, rtype;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\FlexNet", 0, KEY_READ, &key) != ERROR_SUCCESS) {
                fprintf(stderr, "Cannot open registry key\n");
                return;
        }
        blen = sizeof(buf);
        if (RegQueryValueEx(key, "Home", NULL, &rtype, buf, &blen) != ERROR_SUCCESS || rtype != REG_SZ)
                fprintf(stderr, "Invalid/Nonexisting Registry Key/Type\n");
        else
                printf("FlexNet Directory %s\n", buf);
        RegCloseKey(key);
}

#define SERVICENAME        "eppflex"
#define SERVICEDISPLAYNAME "Baycom EPPFLEX"
//#define SERVICEBINARY      "\"\\??\\g:\\nteppflex\\eppflex.sys\""
#define SERVICEBINARY      "system32\\drivers\\eppflex.sys"

static int start_service(void)
{
        int ret = -1;
        SC_HANDLE hscm, hserv;
        
        hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!hscm) {
                fprintf(stderr, "Cannot open SC manager, error 0x%08lx\n", GetLastError());
                return -1;
        }
        hserv = CreateService(hscm, SERVICENAME, SERVICEDISPLAYNAME,
                              SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                              SERVICEBINARY, NULL, NULL, NULL, NULL, NULL);
        if (!hserv) {
                fprintf(stderr, "Cannot create service, error 0x%08lx\n", GetLastError());
                hserv = OpenService(hscm, SERVICENAME, SERVICE_ALL_ACCESS);
                if (!hserv) {
                        fprintf(stderr, "Cannot open service, error 0x%08lx\n", GetLastError());
                        goto closescm;
                }
        }
        if (!StartService(hserv, 0, NULL)) {
                fprintf(stderr, "Cannot start service, error 0x%08lx\n", GetLastError());
                goto closeserv;
        }
        printf("Service %s started successfully\n", SERVICENAME);
        ret = 0;

  closeserv:
        if (!CloseServiceHandle(hserv))
                fprintf(stderr, "Cannot close service handle, error 0x%08lx\n", GetLastError());
  closescm:
        if (!CloseServiceHandle(hscm))
                fprintf(stderr, "Cannot close service manager handle, error 0x%08lx\n", GetLastError());
        return ret;
}

static int stop_service(void)
{
        int ret = -1;
        SC_HANDLE hscm, hserv;
        SERVICE_STATUS sstat;
        
        hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!hscm) {
                fprintf(stderr, "Cannot open SC manager, error 0x%08lx\n", GetLastError());
                return -1;
        }
        hserv = OpenService(hscm, SERVICENAME, SERVICE_ALL_ACCESS);
        if (!hserv) {
                fprintf(stderr, "Cannot open service, error 0x%08lx\n", GetLastError());
                goto closescm;
        }
        ret = 0;
        if (!ControlService(hserv, SERVICE_CONTROL_STOP, &sstat)) {
                fprintf(stderr, "Cannot delete service, error 0x%08lx\n", GetLastError());
                ret = -1;
        }
        if (!DeleteService(hserv)) {
                fprintf(stderr, "Cannot delete service, error 0x%08lx\n", GetLastError());
                ret = -1;
        }
        if (!ret)
                printf("Service %s stopped successfully\n", SERVICENAME);
        if (!CloseServiceHandle(hserv))
                fprintf(stderr, "Cannot close service handle, error 0x%08lx\n", GetLastError());
  closescm:
        if (!CloseServiceHandle(hscm))
                fprintf(stderr, "Cannot close service manager handle, error 0x%08lx\n", GetLastError());
        return ret;
}

static int open_driver(void)
{
        HANDLE hdrv;
        hdrv = CreateFile("\\\\.\\eppflex\\0", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, NULL);
        if (hdrv == INVALID_HANDLE_VALUE) {
                fprintf(stderr, "Cannot open driver, error 0x%08lx\n", GetLastError());
                return -1;
        }
        CloseHandle(hdrv);
        return 0;
}

int main(int argc, char *argv[])
{
        getdir();

        if (argc < 2) {
                fprintf(stderr, "usage: testload [r|s|o]\n");
                exit(1);
        }
        if (!strcmp(argv[1], "r"))
                exit(start_service() ? 1 : 0);
        if (!strcmp(argv[1], "s"))
                exit(stop_service() ? 1 : 0);
        if (!strcmp(argv[1], "o"))
                exit(open_driver() ? 1 : 0);
        fprintf(stderr, "Invalid command: %s\n", argv[1]);
        exit(1);
}
