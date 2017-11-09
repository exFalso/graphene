/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

#define PORT    8000

static void server_main (int argc, const char ** argv);
static void client_main (int argc, const char ** argv);

static void doit (const char * server)
{
    PAL_HANDLE sock;
    sock = DkStreamOpen(server, PAL_ACCESS_RDWR, 0, 0, 0);
    if (!sock) {
        pal_printf("tcp connection failed\n");
        DkProcessExit(1);
    }
    DkObjectClose(sock);
}

int main (int argc, const char ** argv, const char ** envp)
{
    if (argc != 2) {
        pal_printf("Usage: pal_loader %s -s [serverhost] OR "
                   "pal_loader %s [-]serverhost\n", argv[0], argv[0]);
        DkProcessExit(1);
    }

    if (strcmp_static(argv[1], "-s")) {
        server_main(argc, argv);
    } else {
        client_main(argc, argv);
    }

    return 0;
}

static void client_main (int argc, const char ** argv)
{
    PAL_HANDLE sock;
    const char * server;
    char buf[256];

    server = argv[1][0] == '-' ? &argv[1][1] : argv[1];
    snprintf(buf, 256, "tcp:%s:%d", server, PORT);

    sock = DkStreamOpen(buf, PAL_ACCESS_RDWR, 0, 0, 0);

    /*
     * Stop server code.
     */
    if (argv[1][0] == '-') {
        DkStreamWrite(sock, 0, 1, "0", NULL);
        DkObjectClose(sock);
        return;
    }

    BENCH(doit(buf), 0);
    snprintf(buf, 256, "TCP/IP connnection cost to %s", server);
    micro(buf, get_n());
}

static void server_main (int argc, const char ** argv)
{
    PAL_HANDLE sock, newsock;
    char buf[256], c;
    const char * host = (argc == 3) ? argv[2] : "127.0.0.1";

    snprintf(buf, 256, "tcp.srv:%s:%d", host, PORT);
    sock = DkStreamOpen(buf, 0, 0, 0, 0);
    for (;;) {
        newsock = DkStreamWaitForClient(sock);
        if (DkStreamRead(newsock, 0, 1, &c, NULL, 0) == 1 && c == '0') {
            DkObjectClose(sock);
            DkProcessExit(0);
        }
        DkObjectClose(newsock);
    }
}
