/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

static void
do_read(PAL_HANDLE stream, char * buf, size_t size, size_t maxsize,
        const char * filename)
{
    static uint64_t offset = 0;

    if (DkStreamRead(stream, offset, size, buf, NULL, 0) != size) {
        pal_printf("DkStreamRead failed on %s\n", filename);
        return;
    }

    if (maxsize) {
        offset += size;
        if (offset + size > maxsize)
            offset = 0;
    }
}

static void
do_write(PAL_HANDLE stream, char * buf, size_t size, size_t maxsize,
         const char * filename)
{
    static uint64_t offset = 0;

    if (DkStreamWrite(stream, offset, size, buf, NULL) != size) {
        pal_printf("DkStreamWrite failed on %s\n", filename);
        return;
    }

    if (maxsize) {
        offset += size;
        if (offset + size > maxsize)
            offset = 0;
    }
}

static void
do_open(const char * filename)
{
    PAL_HANDLE stream;

    stream = DkStreamOpen(filename, PAL_ACCESS_RDONLY, 0, 0, 0);
    if (!stream) {
        pal_printf("DkStreamOpen failed on %s\n", filename);
        return;
    }

    DkObjectClose(stream);
}

static void
do_attrquery(const char * filename)
{
    PAL_STREAM_ATTR attr;
    PAL_BOL result;

    result = DkStreamAttributesQuery(filename, &attr);
    if (result == PAL_FALSE) {
        pal_printf("DkStreamAttributesQuery failed on %s\n", filename);
        return;
    }
}

static void
do_attrquerybyhdl(PAL_HANDLE stream, const char * filename)
{
    PAL_STREAM_ATTR attr;
    PAL_BOL result;

    result = DkStreamAttributesQuerybyHandle(stream, &attr);
    if (result == PAL_FALSE) {
        pal_printf("DkStreamAttributesQuerybyHandle failed on %s\n", filename);
        return;
    }
}

int main (int argc, char ** argv, char ** envp)
{
    PAL_HANDLE stream;
    const char * path;
    char uri[256];
    char c;

    if (argc < 2) goto usage;
    path = argv[2] ? argv[2] : "/dev/zero";
    snprintf(uri, 256, "file:%s", path);

    if (strcmp_static(argv[1], "read")) {
        PAL_STREAM_ATTR attr;
        stream = DkStreamOpen(uri, PAL_ACCESS_RDWR, 0, 0, 0);
        if (!stream) {
            pal_printf("Failed opening %s\n", uri);
            return 1;
        }
        if (!DkStreamAttributesQuerybyHandle(stream, &attr)) {
            pal_printf("Failed querying attributes\n");
            return 1;
        }
        BENCH(do_read(stream, &c, 1, attr.pending_size, uri), 0);;
        micro("DkStreamRead", get_n());
        DkObjectClose(stream);
    } else if (strcmp_static(argv[1], "write")) {
        PAL_STREAM_ATTR attr;
        stream = DkStreamOpen(uri, PAL_ACCESS_RDWR, 0, 0, 0);
        if (!stream) {
            pal_printf("Failed opening %s\n", uri);
            return 1;
        }
        if (!DkStreamAttributesQuerybyHandle(stream, &attr)) {
            pal_printf("Failed querying attributes\n");
            return 1;
        }
        BENCH(do_write(stream, &c, 1, attr.pending_size, uri), 0);;
        micro("DkStreamWrite", get_n());
        DkObjectClose(stream);
    } else if (strcmp_static(argv[1], "read-size")) {
        size_t size = bytes(argv[2]);
        char * buf = DkVirtualMemoryAlloc(NULL, align_up(size),
                                          0, PAL_PROT_READ|PAL_PROT_WRITE);
        PAL_STREAM_ATTR attr;
        path = argv[3] ? argv[3] : "/dev/zero";
        snprintf(uri, 256, "file:%s", path);
        stream = DkStreamOpen(uri, PAL_ACCESS_RDWR, 0, 0, 0);
        if (!stream) {
            pal_printf("Failed opening %s\n", uri);
            return 1;
        }
        if (!DkStreamAttributesQuerybyHandle(stream, &attr)) {
            pal_printf("Failed querying attributes\n");
            return 1;
        }
        BENCH(do_read(stream, buf, size, attr.pending_size, uri), 0);;
        micro("DkStreamRead", get_n());
        DkObjectClose(stream);
    } else if (strcmp_static(argv[1], "write-size")) {
        size_t size = bytes(argv[2]);
        char * buf = DkVirtualMemoryAlloc(NULL, align_up(size),
                                          0, PAL_PROT_READ|PAL_PROT_WRITE);
        PAL_STREAM_ATTR attr;
        path = argv[3] ? argv[3] : "/dev/zero";
        snprintf(uri, 256, "file:%s", path);
        stream = DkStreamOpen(uri, PAL_ACCESS_RDWR, 0, 0, 0);
        if (!stream) {
            pal_printf("Failed opening %s\n", uri);
            return 1;
        }
        if (!DkStreamAttributesQuerybyHandle(stream, &attr)) {
            pal_printf("Failed querying attributes\n");
            return 1;
        }
        BENCH(do_write(stream, buf, size, attr.pending_size, uri), 0);;
        micro("DkStreamWrite", get_n());
        DkObjectClose(stream);
    } else if (strcmp_static(argv[1], "open")) {
        BENCH(do_open(uri), 0);;
        micro("DkStreamOpen", get_n());
    } else if (strcmp_static(argv[1], "attrquery")) {
        BENCH(do_attrquery(uri), 0);;
        micro("DkStreamAttributesQuery", get_n());
    } else if (strcmp_static(argv[1], "attrquerybyhdl")) {
        stream = DkStreamOpen(uri, PAL_ACCESS_RDONLY, 0, 0, 0);
        if (!stream) {
            pal_printf("Failed opening %s\n", uri);
            return 1;
        }
        BENCH(do_attrquerybyhdl(stream, uri), 0);;
        micro("DkStreamAttributesQeurybyHandle", get_n());
        DkObjectClose(stream);
    } else {
usage:
        pal_printf("Usage: pal_loader %s read|write|open|attrquery|attrquerybyhdl\n", argv[0]);
    }

    return 0;
}
