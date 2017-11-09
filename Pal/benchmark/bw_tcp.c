/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

#define PORT    8000

static void server_main (int argc, const char ** argv);
static void client_main (int argc, const char ** argv);
static int  source (PAL_HANDLE stream);

static void transfer(uint64_t msgsize, PAL_HANDLE server, char * buf)
{
    int c;

    while ((c = DkStreamRead(server, 0, msgsize, buf, NULL, 0)) > 0) {
        msgsize -= c;
    }

    if (c < 0) {
        pal_printf("bw_tcp: transfer: read failed\n");
        DkProcessExit(4);
    }
}

/* ARGSUSED */
static void client_main(int argc, const char ** argv)
{
    PAL_HANDLE server;
    uint64_t msgsize = XFERSIZE, memsize;
    char uri[256];
    char t[512];
    char * buf;
    const char * usage = "usage: %s -remotehost OR %s remotehost [msgsize]\n";

    if (argc != 2 && argc != 3) {
        pal_printf(usage, argv[0], argv[0]);
        DkProcessExit(0);
    }
    if (argc == 3) {
        msgsize = bytes(argv[2]);
    }

    /*
     * Disabler message to other side.
     */
    if (argv[1][0] == '-') {
        snprintf(uri, 256, "tcp:%s:%d", &argv[1][1], PORT);
        server = DkStreamOpen(uri, PAL_ACCESS_RDWR, 0, 0, 0);
        if (!server || !DkStreamWrite(server, 0, 1, "0", NULL)) {
            pal_printf("tcp write failed\n");
            DkProcessExit(1);
        }
        DkProcessExit(0);
    }

    memsize = align_up(msgsize);
    buf = DkVirtualMemoryAlloc(NULL, memsize, 0,
                               PAL_PROT_READ|PAL_PROT_WRITE);
    if (!buf) {
        pal_printf("No memory\n");
        DkProcessExit(1);
    }

    memset(buf, 0, msgsize);

    snprintf(uri, 256, "tcp:%s:%d", argv[1], PORT);
    server = DkStreamOpen(uri, PAL_ACCESS_RDWR, 0, 0, 0);
    if (!server) {
        pal_printf("bw_tcp: could not open socket to server\n");
        DkProcessExit(2);
    }

    snprintf(t, 512, "%llu", msgsize);
    if (DkStreamWrite(server, 0, strlen(t) + 1, t, NULL) != strlen(t) + 1) {
        pal_printf("control write failed\n");
        DkProcessExit(3);
    }

    /*
     * Send data over socket for at least 7 seconds.
     * This minimizes the effect of connect & opening TCP windows.
     */
    BENCH1(transfer(msgsize, server, buf), LONGER);

    BENCH(transfer(msgsize, server, buf), 0);

    pal_printf("Socket bandwidth using %s: ", argv[1]);
    mb(msgsize * get_n());

    DkObjectClose(server);
    DkProcessExit(0);
}

/* ARGSUSED */
static void server_main (int argc, const char ** argv)
{
    PAL_HANDLE sock, newsock;
    char buf[256];
    const char * host = (argc == 3) ? argv[2] : "127.0.0.1";
    snprintf(buf, 256, "tcp.srv:%s:%d", host, PORT);

    sock = DkStreamOpen(buf, 0, 0, 0, 0);
    for (;;) {
        newsock = DkStreamWaitForClient(sock);
        if (source(newsock))
            break;
    }

    DkObjectClose(sock);
}

/*
 * Read the number of bytes to be transfered.
 * Write that many bytes on the data socket.
 */
static int source (PAL_HANDLE stream)
{
    char t[512];
    char * buf;
    uint64_t msgsize, memsize;

    memset(t, 0, 512);

    if (!DkStreamRead(stream, 0, 511, t, NULL, 0)) {
        pal_printf("read control nbytes failed\n");
        DkProcessExit(7);
    }

    msgsize = bytes(t);
    /*
     * A hack to allow turning off the absorb daemon.
     */
    if (msgsize == 0) {
        DkObjectClose(stream);
        return 1;
    }

    memsize = align_up(msgsize);
    buf = DkVirtualMemoryAlloc(NULL, memsize, 0,
                               PAL_PROT_READ|PAL_PROT_WRITE);
    if (!buf) {
        pal_printf("no memory\n");
        DkProcessExit(1);
    }

    memset(buf, 0, msgsize);

    while (DkStreamWrite(stream, 0, msgsize, buf, NULL) > 0);

    DkVirtualMemoryFree(buf, memsize);
    DkObjectClose(stream);
    return 0;
}

int main (int argc, const char ** argv, const char ** envp)
{
    const char * usage = "Usage: %s -s OR %s -serverhost OR %s serverhost [msgsize]\n";

    if (argc < 2 || 3 < argc) {
        pal_printf(usage, argv[0], argv[0], argv[0]);
        return 1;
    }
    if (argc == 2 && strcmp_static(argv[1], "-s")) {
        server_main(argc, argv);
    } else {
        client_main(argc, argv);
    }

    return 0;
}
