/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

static void do_event (PAL_HANDLE * events)
{
    DkEventSet(events[0]);

    if (DkObjectsWaitAny(1, &events[1], NO_TIMEOUT) != events[1]) {
        pal_printf("Cannot wait for event\n");
        DkProcessExit(1);
    }

    DkEventClear(events[1]);
}

static void thread_e (void * param)
{
    PAL_HANDLE * events = param;
    DkEventSet(events[0]);

    while(1) {
        if (DkObjectsWaitAny(1, &events[1], NO_TIMEOUT) != events[1]) {
            pal_printf("Cannot wait for event\n");
            DkProcessExit(1);
        }

        DkEventClear(events[1]);
        DkEventSet(events[2]);
    }
}

static void do_mutex (PAL_HANDLE mutex)
{
    if (DkObjectsWaitAny(1, &mutex, NO_TIMEOUT) != mutex) {
        pal_printf("Cannot acquire mutexes\n");
        DkProcessExit(1);
    }

    DkSemaphoreRelease(mutex, 1);
}

static void thread_m (void * param)
{
    PAL_HANDLE mutex = param;

    while(1)
        do_mutex(mutex);
}

int main (int argc, char ** argv, char ** envp)
{
    PAL_HANDLE thread;

    if (argc < 2) goto usage;

    if (strcmp_static(argv[1], "event")) {
        PAL_HANDLE events[3];

        events[0] = DkNotificationEventCreate(0);
        events[1] = DkNotificationEventCreate(0);
        events[2] = DkNotificationEventCreate(0);

        thread = DkThreadCreate(thread_e, events, 0);
        if (!thread) {
            pal_printf("DkThreadCreate failed\n");
            return 1;
        }

        DkObjectsWaitAny(1, &events[0], NO_TIMEOUT);

        BENCH(do_event(&events[1]), 0);
        micro("Notification event latency", get_n());
        DkProcessExit(0);
    } else if (strcmp_static(argv[1], "mutex")) {
        PAL_HANDLE mutex;
        int n = 2, i;
        char buf[256];

        if (argv[2]) n = bytes(argv[2]);

        mutex = DkSemaphoreCreate(0, 1);

        for (i = 1; i < n; i++) {
            thread = DkThreadCreate(thread_m, mutex, 0);
            if (!thread) {
                pal_printf("DkThreadCreate failed\n");
                return 1;
            }
        }

        BENCH(do_mutex(mutex), 0);
        snprintf(buf, 256, "Mutex latency with %d threads", n);
        micro(buf, get_n());
        DkProcessExit(0);
    } else {
usage:  pal_printf("Usage: pal_loader %s events|mutex [n]\n", argv[0]);
    }
    return 0;
}
