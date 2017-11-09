/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

#ifndef _BENCH_H
#define _BENCH_H

#include <pal.h>
#include <stdint.h>

#define pageshift       (pal_control.pagesize - 1)
#define pagemask        (~pageshift)
#define align_up(val)   ((val + pageshift) & pagemask)
#define align_down(val) ((val) & pagemask)

#ifndef XFERSIZE
#define XFERSIZE        (64*1024)   /* all bandwidth I/O should use this */
#endif

#define REAL_SHORT        50000
#define SHORT           1000000
#define MEDIUM          2000000
#define LONGER          7500000    /* for networking data transfers */
#define ENOUGH       REAL_SHORT

#define TRIES               111

typedef struct {
    int         N;
    uint64_t    u[TRIES];
    uint64_t    n[TRIES];
} result_t;

void insertinit (result_t *r);
void insertsort (uint64_t, uint64_t, result_t *);
void save_median (void);
void save_minimum (void);
void save_mean (void);
void save_variance (void);
void save_results (result_t * r);
void get_results (result_t * r);

void setmeantime (double usecs);
void setvariancetime (double usecs);
double getvariancetime (void);
double getmeantime (void);

void setmeanratetime (double usecs);
void setvarianceratetime (double usecs);
double getvarianceratetime (void);
double getmeanratetime (void);

double calc_variance (double mean, double *times, int size);
double calc_variance_rate (double mean, double *times, int size,
                           int ops_per_measure);
double calc_mean (double *times, int size);
double calc_mean_rate (double *times, int size, int ops_per_measure);

uint64_t bytes (const char * s);
double l_overhead (void);
uint64_t t_overhead (void);
int get_enough (int);
void save_n (uint64_t);
uint64_t get_n (void);
void settime (uint64_t usecs);
uint64_t gettime (void);
void mb (uint64_t bytes);
void micro (const char * s, uint64_t n);
void micromb (uint64_t mb, uint64_t n);
void start (uint64_t * time);
uint64_t stop (uint64_t * begin, uint64_t * end);

double ci_width (double stddev, int n);
double sqrt (double x);

#define BENCHO(loop_body, overhead_body, enough) {              \
        int __i, __N; double __oh; result_t __overhead, __r;    \
        insertinit(&__overhead); insertinit(&__r);              \
        __N = (enough == 0 || get_enough(enough) <= 100000) ? TRIES : 1;    \
        if (enough < LONGER) {loop_body;} /* warm the cache */  \
        for (__i = 0; __i < __N; ++__i) {                       \
            BENCH1(overhead_body, enough);                      \
            if (gettime() > 0)                                  \
                insertsort(gettime(), get_n(), &__overhead);    \
            BENCH1(loop_body, enough);                          \
            if (gettime() > 0)                                  \
                insertsort(gettime(), get_n(), &__r);           \
        }                                                       \
        for (__i = 0; __i < __r.N; ++__i) {                     \
            __oh = __overhead.u[__i] / (double)__overhead.n[__i];   \
            if (__r.u[__i] > (uint64_t)((double)__r.n[__i] * __oh)) \
                __r.u[__i] -= (uint64_t)((double)__r.n[__i] * __oh);\
            else                                                \
                __r.u[__i] = 0;                                 \
        }                                                       \
        save_results(&__r);                                     \
    }

#define BENCH(loop_body, enough) {                              \
        long __i, __N; result_t __r;                            \
        insertinit(&__r);                                       \
        __N = (enough == 0 || get_enough(enough) <= 100000) ? TRIES : 1;\
        if (enough < LONGER) {loop_body;} /* warm the cache */  \
        for (__i = 0; __i < __N; ++__i) {                       \
            BENCH1(loop_body, enough);                          \
            if (gettime() > 0)                                  \
                insertsort(gettime(), get_n(), &__r);           \
        }                                                       \
        save_results(&__r);                                     \
    }

#define BENCH1(loop_body, enough) {                             \
        double __usecs;                                         \
        BENCH_INNER(loop_body, enough);                         \
        __usecs = gettime();                                    \
        __usecs -= t_overhead() + get_n() * l_overhead();       \
        settime(__usecs >= 0. ? (uint64_t)__usecs : 0);         \
    }

#define BENCH_INNER(loop_body, enough) {                        \
        static uint64_t __iterations = 1;                       \
        int __enough = get_enough(enough);                      \
        uint64_t __n; double __result = 0.;                     \
                                                                \
        while(__result < 0.95 * __enough) {                     \
            start(0);                                           \
            for (__n = __iterations; __n > 0; __n--) {          \
                loop_body;                                      \
            }                                                   \
            __result = stop(0,0);                               \
            if (__result < 0.99 * __enough                      \
                || __result > 1.2 * __enough) {                 \
                if (__result > 150.) {                          \
                    double tmp = __iterations / __result;       \
                    tmp *= 1.1 * __enough;                      \
                    __iterations = (uint64_t)(tmp + 1);         \
                } else {                                        \
                    if (__iterations > (uint64_t)1<<27) {       \
                        __result = 0.;                          \
                        break;                                  \
                    }                                           \
                    __iterations <<= 3;                         \
                }                                               \
            }                                                   \
        } /* while */                                           \
        save_n((uint64_t)__iterations);                         \
        settime((uint64_t)__result);                            \
    }

#endif /* _BENCH_H */
