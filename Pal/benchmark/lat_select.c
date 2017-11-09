/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

#define PORT       8000

static void doit (int N, PAL_HANDLE * handles)
{
    PAL_HANDLE sock;
    sock = DkObjectsWaitAny(N, handles, NO_TIMEOUT);
    if (!sock) {
        pal_printf("DkObjectsWaitAny failed\n");
        DkProcessExit(1);
    }
}

int main (int argc, const char ** argv, const char ** envp)
{
    char buf[256];
    const char * report_file = "DkObjectsWaitAny on %d files";
    const char * report_tcp = "DkObjectsWaitAny on %d tcp sockets";
    const char * report = report_tcp;
    const char * usage = "pal_loader lat_select tcp|file [n]\n";
    PAL_HANDLE proc;
    PAL_HANDLE * handles;
    int N, i;
    char c;
    const char * newargv[3];

    if (argc != 2 && argc != 3) {
        pal_printf(usage);
        DkProcessExit(1);
    }

    N = 200;

    if (argc > 2) N = bytes(argv[2]);

    handles = __alloca(sizeof(PAL_HANDLE) * N);

    if (strcmp_static(argv[1], "tcp")) {
        report = report_tcp;

        newargv[0] = argv[0];
        newargv[1] = "-c";
        newargv[2] = NULL;

        proc = DkProcessCreate(argv[0], 0, newargv);
        if (!proc) {
            pal_printf("DkProcessCreate failed\n");
            DkProcessExit(1);
        }

        if (!DkStreamRead(proc, 0, 1, &c, NULL, 0)) {
            DkProcessExit(1);
        }

        snprintf(buf, 256, "tcp:127.0.0.1:%d", PORT);
        for (i = 0; i < N; i++) {
            handles[i] = DkStreamOpen(buf, PAL_ACCESS_RDWR, 0, 0, 0);
            if (!handles[i]) {
                pal_printf("lat_select: Could not connect to tcp server socket\n");
                DkProcessExit(1);
            }
            c = 0;
            DkStreamWrite(handles[i], 0, 1, &c, NULL);
        }

        BENCH(doit(N, handles), 0);
        snprintf(buf, 256, report, N);
        micro(buf, get_n());

        for (i = 0; i < N; i++)
            DkObjectClose(handles[i]);

        snprintf(buf, 256, "tcp:127.0.0.1:%d", PORT);
        handles[0] = DkStreamOpen(buf, PAL_ACCESS_RDWR, 0, 0, 0);
        if (!handles[0])
            DkProcessExit(1);
        c = 1;
        DkStreamWrite(handles[0], 0, 1, &c, NULL);

    } else if (strcmp_static(argv[1], "-c")) {
        PAL_HANDLE sock;

        snprintf(buf, 256, "tcp.srv:127.0.0.1:%d", PORT);
        sock = DkStreamOpen(buf, 0, 0, 0, 0);
        if (!sock) {
            pal_printf("lat_select: Could not open tcp server socket\n");
            DkProcessExit(1);
        }

        DkStreamWrite(pal_control.parent_process, 0, 1, &c, NULL);

        while (1) {
            PAL_HANDLE newsock;
            newsock = DkStreamWaitForClient(sock);
            if (!DkStreamRead(newsock, 0, 1, &c, NULL, 0) || c == 1)
                break;
            DkStreamWrite(newsock, 0, 1, &c, NULL);
        }

        DkProcessExit(0);
    } else if (strcmp_static(argv[1], "file")) {
        report = report_file;
    } else {
        pal_printf(usage);
        DkProcessExit(1);
    }

    return 0;
}
