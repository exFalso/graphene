/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

#define PORT    8000

static void client_main(int argc, const char ** argv);
static void doclient(PAL_HANDLE sock);
static void server_main(int argc, const char ** argv);
static int  doserver(PAL_HANDLE sock);

int main (int argc, const char ** argv, const char ** envp)
{
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
    return(0);
}

static void client_main(int argc, const char **argv)
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
        DkObjectClose(sock);
        return;
    }

    BENCH(doclient(sock), MEDIUM);
    snprintf(buf, 256, "TCP latency using %s", argv[1]);
    micro(buf, get_n());
}

static void doclient(PAL_HANDLE sock)
{
    char c;
    DkStreamWrite(sock, 0, 1, &c, NULL);
    DkStreamRead(sock, 0, 1, &c, NULL, 0);
}

static void server_main (int argc, const char ** argv)
{
    PAL_HANDLE newsock, sock;
    char buf[256];
    const char * host = (argc == 3) ? argv[2] : "127.0.0.1";
    snprintf(buf, 256, "tcp.srv:%s:%d", host, PORT);

    sock = DkStreamOpen(buf, 0, 0, 0, 0);
    for (;;) {
        newsock = DkStreamWaitForClient(sock);
        if (doserver(newsock))
            break;
    }

    DkObjectClose(sock);
}

static int doserver(PAL_HANDLE sock)
{
    char c;
    int n = 0;

    while (DkStreamRead(sock, 0, 1, &c, NULL, 0) == 1) {
        DkStreamWrite(sock, 0, 1, &c, NULL);
        n++;
    }

    /*
     * A connection with no data means shut down.
     */
    if (n == 0) {
        DkObjectClose(sock);
        return 1;
    }

    return 0;
}
