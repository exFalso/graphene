/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

#define PORT    8000

static void client_main (int argc, const char ** argv);
static void server_main (int argc, const char ** argv);

static void doit(PAL_HANDLE sock, int seq)
{
    int net = __htonl(seq);
    int ret;

    if (DkStreamWrite(sock, 0, sizeof(net), &net, NULL) != sizeof(net)) {
        pal_printf("lat_udp client: DkStreamWrite failed\n");
        DkProcessExit(5);
    }
    if (DkStreamRead(sock, 0, sizeof(ret), &ret, NULL, 0) != sizeof(ret)) {
        pal_printf("lat_udp client: DkStreamRead failed\n");
        DkProcessExit(5);
    }
}

int main (int argc, const char ** argv, const char ** envp)
{
    if (sizeof(int) != 4) {
        pal_printf("lat_udp: Wrong sequence size\n");
        return 1;
    }

    if (argc != 2 && argc != 3) {
usage:
        pal_printf("Usage: pal_loader %s -s [serverhost] OR "
                   "pal_loader %s [-]serverhost\n", argv[0], argv[0]);
        return 1;
    }

    if (strcmp_static(argv[1], "-s")) {
        server_main(argc, argv);
    } else {
        if (argc != 2)
            goto usage;
        client_main(argc, argv);
    }

    return 0;
}

static void client_main(int argc, const char ** argv)
{
    PAL_HANDLE sock;
    int seq = -1;
    const char * server;
    char buf[256];

    if (argc != 2) {
        pal_printf("Usage: pal_loader %s udp:hostname:port\n", argv[0]);
        DkProcessExit(1);
    }

    server = argv[1][0] == '-' ? &argv[1][1] : argv[1];
    snprintf(buf, 256, "udp:%s:%d", server, PORT);
    sock = DkStreamOpen(buf, PAL_ACCESS_RDWR, 0, 0, 0);
    if (!sock) {
        pal_printf("UDP connection failed\n");
        DkProcessExit(1);
    }

    /*
     * Stop server code.
     */
    if (argv[1][0] == '-') {
        while (seq-- > -5) {
            int net = __htonl(seq);
            DkStreamWrite(sock, 0, sizeof(net), &net, NULL);
        }
        DkProcessExit(0);
    }
    BENCH(doit(sock, ++seq), MEDIUM);
    snprintf(buf, 256, "UDP latency using %s", server);
    micro(buf, get_n());
    DkProcessExit(0);
}

/* ARGSUSED */
static void server_main(int argc, const char ** argv)
{
    PAL_HANDLE sock;
    int net, sent, seq = 0;
    char buf[256];
    const char * host = (argc == 3) ? argv[2] : "127.0.0.1";
    snprintf(buf, 256, "udp.srv:%s:%d", host, PORT);

    sock = DkStreamOpen(buf, 0, 0, 0, 0);
    if (!sock) {
        pal_printf("create UDP server failed\n");
        DkProcessExit(1);
    }

    while (1) {
        char path[256];

        if (DkStreamRead(sock, 0, sizeof(sent), &sent, path, 256)
            != sizeof(sent)) {
            pal_printf("lat_udp server: DkStreamRead: got wrong size\n");
            DkProcessExit(9);
        }

        sent = __ntohl(sent);
        if (sent < 0) {
            DkObjectClose(sock);
            DkProcessExit(0);
        }

        if (sent != ++seq) {
            seq = sent;
        }

        net = __htonl(seq);
        if (DkStreamWrite(sock, 0, sizeof(net), &net, path)
            != sizeof(net)) {
            pal_printf("lat_udp server: DkStreamWrite: got wrong size\n");
            DkProcessExit(9);
        }
    }
}
