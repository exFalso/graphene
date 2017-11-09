/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#include "api.h"
#include "pal.h"
#include "pal_debug.h"
#include "bench.h"

int caught, to_catch, n, pos;
double * times;

#define ITER_UNITS 50

static void handler (PAL_PTR event, PAL_NUM arg, PAL_CONTEXT * context)
{
}

static void prot (PAL_PTR event, PAL_NUM arg, PAL_CONTEXT * context)
{
    if (++caught == to_catch) {
        double level, u, mean, var, ci;
        char buffer[256];
        int n;

        u = stop(0,0);
        u /= (double) to_catch;

        times[pos++] = u;

        mean = calc_mean(times, pos);
        var = calc_variance(mean, times, pos);
        ci = ci_width(sqrt(var), pos);

        n = 0;
        strcpy_static(buffer + n, "mean=", 256 - n);
        n += 5;
        for (level = 1; level < mean; level *= 10);
        while (level > 0.0001) {
            if (level == 1) buffer[n++] = '.';
            level /= 10;
            buffer[n++] = '0' + (int) (mean / level);
            mean -= ((int) (mean / level)) * level;
        }
        strcpy_static(buffer + n, " +/-", 256 -n);
        n += 4;
        for (level = 1; level < ci; level *= 10);
        while (level > 0.0001) {
            if (level == 1) buffer[n++] = '.';
            level /= 10;
            buffer[n++] = '0' + (int) (ci / level);
            mean -= ((int) (ci / level)) * level;
        }
        n += 4;
        buffer[n++] = 0;
        pal_printf("%s\n", buffer);

        DkProcessExit(0);
    }

    if (caught == ITER_UNITS) {
        double u;

        u = stop(0,0);
        u /= (double) ITER_UNITS;

        times[pos++] = u;

        caught = 0;
        to_catch -= ITER_UNITS;

        start(0);
    }

}

static void install (void)
{
    DkSetExceptionHandler(handler, PAL_EVENT_MEMFAULT, 0);
}

static void do_install (void)
{
    double u, mean, var, ci;

    /*
     * Installation cost
     */
    BENCH(install(), 0);
    u = gettime();
    u /= get_n();

    mean = getmeantime();
    var = getvariancetime();
    ci = ci_width(sqrt(var), TRIES);

    pal_printf("Signal handler installation: "
               "median=" FLOATFMT(3) " [mean=" FLOATFMT(4) " +/-" FLOATFMT(4)
               "] microseconds\n",
               FLOATNUM(u, 3), FLOATNUM(mean, 4), FLOATNUM(ci, 4));
}

static void do_catch (int report)
{
    PAL_HANDLE me = pal_control.first_thread;
    double u, mean, var, ci;

    /*
     * Cost of catching the signal less the cost of sending it
     */
    DkSetExceptionHandler(handler, PAL_EVENT_RESUME, 0);
    BENCH(DkThreadResume(me), 0);

    u = gettime();
    mean = getmeantime();
    var = getvariancetime();
    ci = ci_width(sqrt(var), TRIES);

    u /= get_n();
    n = SHORT/u;
    to_catch = n;

    if (report) {
        pal_printf("Signal handler overhead: "
                   "median=" FLOATFMT(3) " [mean=" FLOATFMT(4) " +/-" FLOATFMT(4)
                   "] microseconds\n",
                   FLOATNUM(u, 3), FLOATNUM(mean, 4), FLOATNUM(ci, 4));
    }
}

static void do_prot (int argc, const char ** argv)
{
    PAL_HANDLE file;
    char buf[256];
    char * where;

    if (argc != 3) {
        pal_printf("usage: pal_loader %s prot file\n", argv[0]);
        DkProcessExit(1);
    }

    snprintf(buf, 256, "file:%s", argv[2]);

    file = DkStreamOpen(buf, PAL_ACCESS_RDONLY, 0, 0, 0);
    if (!file) {
        pal_printf("Opening file failed\n");
        DkProcessExit(1);
    }

    where = DkStreamMap(file, NULL, PAL_PROT_READ, 0, 4096);
    if (!where) {
        pal_printf("DkStreamMap failed\n");
        DkProcessExit(1);
    }

    /*
     * Catch protection faults.
     * Assume that they will cost the same as a normal catch.
     */
    do_catch(0);
    DkSetExceptionHandler(prot, PAL_EVENT_MEMFAULT, 0);

    times = DkVirtualMemoryAlloc(0, align_up(sizeof(double) * ((int)(n / ITER_UNITS) + 1)),
                                 0, PAL_PROT_READ|PAL_PROT_WRITE);
    if (!times) {
        pal_printf("No memory\n");
        DkProcessExit(1);
    }
    start(0);
    *where = 1;
}

int main (int argc, const char ** argv, const char ** envp)
{
    if (argc < 2) goto usage;

    if (strcmp_static(argv[1], "install")) {
        do_install();
    } else if (strcmp_static(argv[1], "catch")) {
        do_catch(1);
    } else if (strcmp_static(argv[1], "prot")) {
        do_prot(argc, argv);
    } else {
usage:  pal_printf("Usage: pal_loader %s install|catch|prot file\n", argv[0]);
    }
    return 0;
}
