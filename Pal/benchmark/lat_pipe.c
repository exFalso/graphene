/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

static void doit(PAL_HANDLE pipe)
{
    char c;

    if (DkStreamWrite(pipe, 0, 1, &c, NULL) != 1 ||
        DkStreamRead(pipe, 0, 1, &c, NULL, 0) != 1) {
        pal_printf("read/write on pipe failed\n");
        DkProcessExit(1);
    }
}

int main (int argc, char ** argv, char ** envp)
{
    PAL_HANDLE proc, srv, pipe;
    char c = 1;

    if (argc == 1) {
        const char * newargv[3];

        srv = DkStreamOpen("pipe.srv:1", PAL_ACCESS_RDWR, 0, 0, 0);
        if (!srv) {
            pal_printf("Opening pipe server failed\n");
            return 1;
        }

        newargv[0] = argv[0];
        newargv[1] = "-c";
        newargv[2] = NULL;

        proc = DkProcessCreate(argv[0], 0, newargv);

        if (!proc) {
            pal_printf("DkProcessCreate failed\n");
            return 1;
        }

        pipe = DkStreamWaitForClient(srv);
        if (!pipe) {
            pal_printf("Accepting pipe connection failed\n");
            return 1;
        }

        for ( ;; ) {
            if (DkStreamRead(pipe, 0, 1, &c, NULL, 0) != 1 ||
                DkStreamWrite(pipe, 0, 1, &c, NULL) != 1) {
                break;
            }
        }

        DkObjectClose(srv);
        DkObjectClose(proc);

    } else {
        if (!strcmp_static(argv[1], "-c"))
            return 1;

        pipe = DkStreamOpen("pipe:1", PAL_ACCESS_RDWR, 0, 0, 0);
        if (!pipe) {
            pal_printf("Connecting pipe failed\n");
            return 1;
        }

        BENCH(doit(pipe), SHORT);
        DkObjectClose(pipe);
        micro("Pipe latency", get_n());
    }
}
