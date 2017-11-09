/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

static void doit(PAL_HANDLE proc)
{
    char c;

    if (DkStreamWrite(proc, 0, 1, &c, NULL) != 1 ||
        DkStreamRead(proc, 0, 1, &c, NULL, 0) != 1) {
        pal_printf("read/write on proc failed\n");
        DkProcessExit(1);
    }
}

int main (int argc, char ** argv, char ** envp)
{
    PAL_HANDLE proc;
    char c = 1;

    if (argc == 1) {
        const char * newargv[3];

        newargv[0] = argv[0];
        newargv[1] = "-c";
        newargv[2] = NULL;

        proc = DkProcessCreate(argv[0], 0, newargv);

        if (!proc) {
            pal_printf("DkProcessCreate failed\n");
            return 1;
        }


        for ( ;; ) {
            if (DkStreamRead(proc, 0, 1, &c, NULL, 0) != 1 ||
                DkStreamWrite(proc, 0, 1, &c, NULL) != 1) {
                break;
            }
        }

        DkObjectClose(proc);

    } else {
        if (!strcmp_static(argv[1], "-c"))
            return 1;

        BENCH(doit(pal_control.parent_process), SHORT);
        micro("Proc read/write latency", get_n());
    }
}
