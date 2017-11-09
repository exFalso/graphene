/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

static void do_proc (int argc, char ** argv)
{
    PAL_HANDLE proc;
    const char * newargv[3];
    char c;

    newargv[0] = argv[0];
    newargv[1] = "-c";
    newargv[2] = NULL;

    proc = DkProcessCreate(argv[0], 0, newargv);

    if (!proc) {
        pal_printf("DkProcessCreate failed\n");
        return;
    }

    DkStreamRead(proc, 0, 1, &c, NULL, 0);
    DkObjectClose(proc);
}

int main (int argc, char ** argv, char ** envp)
{
    if (argc == 2) {
        if (!strcmp_static(argv[1], "-c"))
            return 1;

        //do_child(argc, argv);
        return 0;
    }

    BENCH(do_proc(argc, argv), 0);
    micro("DkProcessCreate+exit", get_n());
    return 0;
}
