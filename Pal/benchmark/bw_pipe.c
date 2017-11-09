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
    PAL_HANDLE proc, srv, control, pipe;
    uint64_t msgsize = XFERSIZE, memsize;
    uint64_t blocksize = XFER;

    memsize = align_up(msgsize);
    buf = DkVirtualMemoryAlloc(NULL, memsize, 0,
                               PAL_PROT_READ|PAL_PROT_WRITE);
    if (!buf) {
        pal_printf("No memory\n");
        return(1);
    }

    memset(buf, 0, msgsize);

    if (argc == 1 || !strcmp_static(argv[1], "-c")) {
        const char * newargv[3];

        if (argc == 2) blocksize = bytes(argv[1]);

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

        control = DkStreamWaitForClient(srv);
        if (!control) {
            pal_printf("Accepting pipe connection failed\n");
            return 1;
        }

        pipe = DkStreamWaitForClient(srv);
        if (!pipe) {
            pal_printf("Accepting pipe connection failed\n");
            return 1;
        }

        buf += 128;    /* destroy page alignment */
        BENCH(reader(control, pipe, blocksize), MEDIUM);
        pal_printf("Pipe bandwidth: ");
        mb(get_n() * blocksize);
        micro("Pipe latency", get_n());

        DkObjectClose(control);
        DkObjectClose(pipe);
        DkObjectClose(srv);
        DkObjectClose(proc);

    } else {
        control = DkStreamOpen("pipe:1", PAL_ACCESS_RDWR, 0, 0, 0);
        if (!control) {
            pal_printf("Connecting pipe failed\n");
            return 1;
        }

        pipe = DkStreamOpen("pipe:1", PAL_ACCESS_RDWR, 0, 0, 0);
        if (!pipe) {
            pal_printf("Connecting pipe failed\n");
            return 1;
        }

        writer(control, pipe);
        return 0;
    }

    return(0);
}

static void writer(PAL_HANDLE control, PAL_HANDLE pipe)
{
    size_t todo;
    size_t bufsize = XFERSIZE;
    size_t n;

    for ( ;; ) {
        if (!DkStreamRead(control, 0, sizeof(todo), &todo, NULL, 0))
            break;

        bufsize = XFERSIZE;
        while (todo > 0) {
            if (todo < bufsize) bufsize = todo;
            if (!(n = DkStreamWrite(pipe, 0, bufsize, buf, NULL))) {
                pal_printf("DkStreamWrite on pipe failed\n");
                break;
            }
            todo -= n;
        }
    }
}

static void reader(PAL_HANDLE control, PAL_HANDLE pipe, size_t bytes)
{
    int done = 0;
    size_t todo = bytes;
    size_t bufsize = XFERSIZE;
    size_t n;

    if (!DkStreamWrite(control, 0, sizeof(bytes), &bytes, NULL)) {
        pal_printf("DkStreamWrite on pipe failed\n");
        DkProcessExit(1);
    }

    while ((done < todo) &&
           ((n = DkStreamRead(pipe, 0, bufsize, buf, NULL, 0)) > 0)) {
        done += n;
        if (todo - done < bufsize) bufsize = todo - done;
    }
    if (done < bytes)
        pal_printf("reader: bytes=%ld, done=%d, todo=%ld\n", bytes, done, todo);
}
