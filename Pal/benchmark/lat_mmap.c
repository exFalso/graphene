/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

#define    PSIZE    (16<<10)
#define    N    10
#define    STRIDE    (10*PSIZE)
#define    MINSIZE    (STRIDE*2)

#define    CHK(x)    if (!(x)) { pal_printf("x" "failed\n"); DkProcessExit(1); }

/*
 * This alg due to Linus.  The goal is to hargve both sparse and full
 * mappings reported.
 */
static void
mapit(PAL_HANDLE file, size_t size, int random)
{
    char * p, * where, * end;
    char c = size & 0xff;

    if (file == NULL) {
        where = DkVirtualMemoryAlloc(0, size, 0, PAL_PROT_READ|PAL_PROT_WRITE);
    } else {
        where = DkStreamMap(file, 0, PAL_PROT_READ|PAL_PROT_WRITE, 0, size);
    }

    if (where == NULL) {
        pal_printf("memory mapping failed\n");
        DkProcessExit(1);
    }
    if (random == 1) {
        end = where + size;
        for (p = where; p < end; p += STRIDE) {
            *p = c;
        }
    } else if (!random) {
        end = where + (size / N);
        for (p = where; p < end; p += PSIZE) {
            *p = c;
        }
    }
    DkVirtualMemoryFree(where, size);
}

int main (int argc, const char ** argv, const char ** envp)
{
    PAL_HANDLE file = NULL;
    size_t size;
    int random = 0;
    const char * prog = argv[0];
    char buf[256];

    if (argc != 3 && argc != 4) {
        pal_printf("usage: %s [-r|-n] size [file|--]\n", prog);
        return 1;
    }
    if (strcmp_static(argv[1], "-r")) {
        random = 1;
        argc--, argv++;
    }
    if (strcmp_static(argv[1], "-n")) {
        random = -1;
        argc--, argv++;
    }
    size = bytes(argv[1]);
    if (size < MINSIZE) {
        pal_printf("size must be larger thabn %d\n", MINSIZE);
        return 1;
    }
    if (!strcmp_static(argv[2], "--")) {
        snprintf(buf, 256, "file:%s", argv[2]);
        CHK(file = DkStreamOpen(buf, PAL_ACCESS_RDWR,
                    PAL_SHARE_GLOBAL_W|PAL_SHARE_GLOBAL_R|
                    PAL_SHARE_GROUP_W|PAL_SHARE_GROUP_R|
                    PAL_SHARE_OWNER_W|PAL_SHARE_OWNER_R,
                    PAL_CREAT_TRY, 0));
        CHK(DkStreamSetLength(file, size));
    }
    BENCH(mapit(file, size, random), 0);
    micromb(size, get_n());
    return(0);
}
