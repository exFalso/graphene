/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

static void cb (void * param)
{
    DkEventSet((PAL_HANDLE) param);
    DkThreadExit();
}

static void do_thread (PAL_HANDLE event)
{
    PAL_HANDLE thread;

    DkEventClear(event);
    thread = DkThreadCreate(cb, event, 0);
    if (!thread) {
        pal_printf("DkThreadCreate failed\n");
        return;
    }

    DkObjectsWaitAny(1, &event, NO_TIMEOUT);
    while (!DkObjectsWaitAny(1, &thread, NO_TIMEOUT))
        ;
    DkObjectClose(thread);
}

int main (int argc, char ** argv, char ** envp)
{
    PAL_HANDLE event = DkNotificationEventCreate(0);
    BENCH(do_thread(event), 0);
    micro("DkThreadCreate+exit", get_n());
    return 0;
}
