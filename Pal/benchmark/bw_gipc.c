/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

static void reader(PAL_HANDLE control, PAL_HANDLE pipe, size_t bytes);
static void writer(PAL_HANDLE control, PAL_HANDLE pipe);

size_t XFER = 10*1024*1024;
char * buf;

int main (int argc, char ** argv, char ** envp)
{
    PAL_HANDLE proc, gipc;
    uint64_t msgsize = XFERSIZE, memsize;
    uint64_t blocksize = XFER;
    char tmp[256];

    memsize = align_up(msgsize);
    buf = DkVirtualMemoryAlloc(NULL, memsize, 0,
                               PAL_PROT_READ|PAL_PROT_WRITE);
    if (!buf) {
        pal_printf("No memory\n");
        return(1);
    }

    memset(buf, 0, msgsize);

    if (argc < 3) {
        const char * newargv[4];
        PAL_NUM key;

        if (argc == 2) blocksize = bytes(argv[1]);

        gipc = DkCreatePhysicalMemoryChannel(&key);
        if (!gipc) {
            pal_printf("Opening gipc failed\n");
            return 1;
        }

        snprintf(tmp, 256, "%d", key);
        newargv[0] = argv[0];
        newargv[1] = "-c";
        newargv[2] = tmp;
        newargv[3] = NULL;

        proc = DkProcessCreate(argv[0], 0, newargv);

        if (!proc) {
            pal_printf("DkProcessCreate failed\n");
            return 1;
        }

        BENCH(reader(proc, gipc, blocksize), MEDIUM);
        pal_printf("GIPC bandwidth: ");
        mb(get_n() * blocksize);

        DkObjectClose(proc);

    } else {
        if (!strcmp_static(argv[1], "-c"))
            return 1;

        snprintf(tmp, 256, "gipc:%s", argv[2]);
        gipc = DkStreamOpen(tmp, 0, 0, 0, 0);
        if (!gipc) {
            pal_printf("Opening gipc failed\n");
            return 1;
        }

        writer(pal_control.parent_process, gipc);
        return 0;
    }

    return(0);
}

static void writer(PAL_HANDLE control, PAL_HANDLE gipc)
{
    PAL_NUM bufsize = XFERSIZE;
    size_t todo;
    size_t n;

    for ( ;; ) {
        if (!DkStreamRead(control, 0, sizeof(todo), &todo, NULL, 0))
            break;

        bufsize = XFERSIZE;
        while (todo > 0) {
            if (todo < bufsize) bufsize = todo;
            if (!(n = DkPhysicalMemoryCommit(gipc, 1, &buf, &bufsize, 0))) {
                pal_printf("Commit on gipc failed\n");
                break;
            }
            todo -= n * 4096;
        }
    }
}

static void reader(PAL_HANDLE control, PAL_HANDLE gipc, size_t bytes)
{
    PAL_NUM bufsize = XFERSIZE;
    PAL_FLG prot = PAL_PROT_READ;
    size_t todo = bytes;
    size_t n;
    int done = 0;

    if (!DkStreamWrite(control, 0, sizeof(bytes), &bytes, NULL)) {
        pal_printf("DkStreamWrite on pipe failed\n");
        DkProcessExit(1);
    }

    while ((done < todo) &&
           ((n = DkPhysicalMemoryMap(gipc, 1, &buf, &bufsize, &prot)) > 0)) {
        n *= 4096;
        done += n;
        if (todo - done < bufsize) bufsize = todo - done;
    }
    if (done < bytes)
        pal_printf("reader: bytes=%ld, done=%d, todo=%ld\n", bytes, done, todo);
}
