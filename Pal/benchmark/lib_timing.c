#include "bench.h"
#include "pal.h"
#include "pal_debug.h"

#define    nz(x)    ((x) == 0 ? 1 : (x))

/*
 * I know you think these should be 2^10 and 2^20, but people are quoting
 * disk sizes in powers of 10, and bandwidths are all power of ten.
 * Deal with it.
 */
#define    MB    (1000*1000.0)
#define    KB    (1000.0)

static double u_var, u_mean, ops_s_mean, ops_s_var;
static uint64_t start_time, stop_time;
static uint64_t iterations;

static void init_timing (void);

void start (uint64_t * time)
{
    if (time == NULL) {
        time = &start_time;
    }

    *time = DkSystemTimeQuery();
}

uint64_t stop (uint64_t * begin, uint64_t * end)
{
    if (end == NULL)
        end = &stop_time;

    *end = DkSystemTimeQuery();

    if (begin == NULL)
        begin = &start_time;

    return *end - *begin;
}

void save_n(uint64_t n)
{
    iterations = n;
}

uint64_t get_n (void)
{
    return (iterations);
}

void settime (uint64_t usecs)
{
    start_time = 0;
    stop_time = usecs;
}

void setmeantime (double usecs)
{
    u_mean = usecs;
}

void setvariancetime (double usecs)
{
    u_var = usecs;
}

void setmeanratetime (double usecs)
{
    ops_s_mean = usecs;
}

void setvarianceratetime (double usecs)
{
    ops_s_var = usecs;
}

void bandwidth (uint64_t bytes, uint64_t times, int verbose)
{
    double  mb, secs;

    secs = (stop_time - start_time) / 1000000.0;
    secs /= times;
    mb = bytes / MB;
    if (verbose) {
        pal_printf(FLOATFMT(4) " MB in " FLOATFMT(4) " secs, " FLOATFMT(4) " MB/sec\n",
                   FLOATNUM(mb, 4), FLOATNUM(secs, 4), FLOATNUM(mb/secs, 4));
    } else {
        if (mb < 1) {
            pal_printf(FLOATFMT(6) " ", FLOATNUM(mb, 6));
        } else {
            pal_printf(FLOATFMT(2) " ", FLOATNUM(mb, 2));
        }
        if (mb / secs < 1) {
            pal_printf(FLOATFMT(6) "\n", FLOATNUM(mb/secs, 6));
        } else {
            pal_printf(FLOATFMT(2) "\n", FLOATNUM(mb/secs, 2));
        }
    }
}

void
kb(uint64_t bytes)
{
    double s, bs;

    s = (stop_time - start_time) / 1000000.0;
    bs = bytes / nz(s);

    pal_printf("%ld KB/sec\n", (long) (bs / KB));
}

void mb (uint64_t bytes)
{
    double s, bs;

    s = (stop_time - start_time) / 1000000.0;
    bs = bytes / nz(s);

    pal_printf(FLOATFMT(2) " MB/sec\n", FLOATNUM(bs / MB, 2));
}

void latency (uint64_t xfers, uint64_t size)
{
    double s;

    s = (stop_time - start_time) / 1000000.0;

    if (xfers > 1) {
        pal_printf("%d %dKB xfers in %.2f secs, ",
                  (int) xfers, (int) (size / KB), s);
    } else {
        pal_printf(FLOATFMT(1) "KB in ", FLOATNUM(size / KB, 1));
    }
    if ((s * 1000 / xfers) > 100) {
        pal_printf("%ld millisec%s, ",
                   (long) (s * 1000 / xfers), xfers > 1 ? "/xfer" : "s");
    } else {
        pal_printf(FLOATFMT(4) " millisec%s, ",
                   FLOATNUM(s * 1000 / xfers, 4), xfers > 1 ? "/xfer" : "s");
    }
    if (((xfers * size) / (MB * s)) > 1) {
        pal_printf(FLOATFMT(2) " MB/sec\n", FLOATNUM((xfers * size) / (MB * s), 2));
    } else {
        pal_printf(FLOATFMT(2) " KB/sec\n", FLOATNUM((xfers * size) / (KB * s), 2));
    }
}

void
nano(char *s, uint64_t n)
{
    double  micro;

    micro = stop_time - start_time;
    micro *= 1000;

    pal_printf("%s: %ld nanoseconds\n", s, (long) (micro / n));
}

static result_t results;

void micro (const char * s , uint64_t n)
{
    double micro, mean, var, ci;

    micro = stop_time - start_time;
    micro /= n;

    if (micro == 0.0) return;

    mean = getmeantime();
    var = getvariancetime();
    if (var < 0.0)
        var = 0.0;
    ci = ci_width(sqrt(var), results.N);

    pal_printf("%s median=" FLOATFMT(4) " [mean=" FLOATFMT(4) " +/-" FLOATFMT(4)
               "] microseconds\n",
               s, FLOATNUM(micro, 4), FLOATNUM(mean, 4), FLOATNUM(ci, 4));
}

void
micromb(uint64_t sz, uint64_t n)
{
    double mb, micro, mean, var, ci;

    micro = stop_time - start_time;
    micro /= n;
    mb = sz;
    mb /= MB;

    if (micro == 0.0) return;

    mean = getmeantime();
    var = getvariancetime();
    if (var < 0.0)
        var = 0.0;
    ci = ci_width(sqrt(var), results.N);

    pal_printf(FLOATFMT(6) " median=" FLOATFMT(4) " [mean=" FLOATFMT(4) " +/-" FLOATFMT(4)
               "] microseconds\n",
               FLOATNUM(mb, 6), FLOATNUM(micro, 4), FLOATNUM(mean, 4), FLOATNUM(ci, 4));
}

void
milli(char *s, uint64_t n)
{
    uint64_t milli;

    milli = (stop_time - start_time) * 1000;
    milli /= n;

    pal_printf("%s: %d milliseconds\n", s, (int)milli);
}

void
ptime(uint64_t n)
{
    double s;

    s = (stop_time - start_time) / 1000000.0;

    pal_printf("%d in " FLOATFMT(2) " secs, %ld microseconds each\n",
               (int)n, FLOATNUM(s, 2), (long) (s * 1000000 / n));
}

double getmeantime(void)
{
    return u_mean;
}

double getvariancetime(void)
{
    return u_var;
}

double getmeanratetime(void)
{
    return ops_s_mean;
}

double getvarianceratetime(void)
{
    return ops_s_var;
}

uint64_t gettime (void)
{
    return stop_time - start_time;
}

uint64_t bytes (const char * s)
{
    uint64_t n = 0;

    for (; *s >= '0' && *s <= '9' ; s++)
        n = n * 10 + (*s - '0');

    if (*s == 'k' || *s == 'K')
        n *= 1024;
    if (*s == 'm' || *s == 'M')
        n *= 1024 * 1024;

    return n;
}

void use_pointer (void * result)
{
    static volatile uint64_t use_result_dummy;
    use_result_dummy += *(int *)result;
}

void insertinit (result_t *r)
{
    int    i;

    r->N = 0;
    for (i = 0; i < TRIES; i++) {
        r->u[i] = 0;
        r->n[i] = 1;
    }
}

/* biggest to smallest */
void insertsort (uint64_t u, uint64_t n, result_t *r)
{
    int    i, j;

    if (u == 0) return;

    for (i = 0; i < r->N; ++i) {
        if (u/(double)n > r->u[i]/(double)r->n[i]) {
            for (j = r->N; j > i; --j) {
                r->u[j] = r->u[j-1];
                r->n[j] = r->n[j-1];
            }
            break;
        }
    }
    r->u[i] = u;
    r->n[i] = n;
    r->N++;
}

void
print_results(int details)
{
    int i;

    for (i = 0; i < results.N; ++i) {
        pal_printf(FLOATFMT(2), FLOATNUM((double)results.u[i]/results.n[i], 2));
        if (i < results.N - 1)
            pal_printf(" ");
    }
    pal_printf("\n");
    if (details) {
        for (i = 0; i < results.N; ++i) {
            pal_printf("%llu/%llu", results.u[i], results.n[i]);
            if (i < results.N - 1)
                pal_printf(" ");
        }
        pal_printf("\n");
    }
}

void get_results (result_t *r)
{
    *r = results;
}

void save_results (result_t *r)
{
    results = *r;
    save_median();
    save_mean();
    save_variance();
}

void save_minimum (void)
{
    if (results.N == 0) {
        save_n(1);
        settime(0);
    } else {
        save_n(results.n[results.N - 1]);
        settime(results.u[results.N - 1]);
    }
}

void save_median (void)
{
    int i = results.N / 2;
    uint64_t u, n;

    if (results.N == 0) {
        n = 1;
        u = 0;
    } else if (results.N % 2) {
        n = results.n[i];
        u = results.u[i];
    } else {
        n = (results.n[i] + results.n[i-1]) / 2;
        u = (results.u[i] + results.u[i-1]) / 2;
    }

    save_n(n); settime(u);
}

void save_mean (void)
{
    int    i;
    double sum = 0;
    double mean, mean_ops_s;
    if (results.N == 0) {
        mean = 0.;
        mean_ops_s = 0.;
    } else {
        for(i = 0; i < results.N; i++)
            sum += (double)results.u[i] / (double)results.n[i];

        mean = sum / (double)results.N;
        /* harmonic average for rates = n / (sum^n_{i=1} 1/rate_i) */
        mean_ops_s = ((double)results.N  * 1000000.) / sum;
    }
    setmeantime(mean);
    setmeanratetime(mean_ops_s);
}

double calc_variance (double mean, double * times, int size)
{
    unsigned int i;
    double sum;

    if (size <= 1)
        return 0;

    for (i = 0, sum = 0; i < size; i++)
        sum += (times[i] - mean) * (times[i] - mean);

    return sum / (double) (size - 1);
}

double calc_variance_rate (double mean, double * times, int size,
                           int ops_per_measure)
{
    unsigned int i;
    double sum, x_r;

    if (size <= 1)
        return 0;

    for (i = 0, sum = 0; i < size; i++) {
        x_r = (double) ops_per_measure / (double) times[i];
        sum += (x_r - mean) * (x_r - mean);
    }

    return sum / (double) (size - 1);
}

double calc_mean(double *times, int size)
{
    unsigned int i;
    double sum;

    if (size <= 0)
        return 0;

    for (i = 0, sum = 0; i < size; i++)
        sum += times[i];

    return sum / (double) size;
}

double calc_mean_rate(double *times, int size, int ops_per_measure)
{
    int i;
    double sum;
    for(i = 0, sum = 0; i < size; i++)
        sum += times[i] / (double) ops_per_measure;

    /* harmonic average for rates = n / (sum^n_{i=1} 1/rate_i) */
    return ((double)size) / sum;
}

void
save_variance()
{
    double  mean, variance, sum;
    double  mean_r, variance_r, sum_r;
    int     i;

    if (results.N == 0) {
        sum = 0;
        mean = 0;
        variance = 0;
        sum_r = 0;
        mean_r = 0;
        variance_r = 0;
        goto done;
    } else {
        mean = getmeantime();
        mean_r = getmeanratetime();
        if (mean <= 0) {
            save_mean();
            mean = getmeantime();
            mean_r = getmeanratetime();
            if (mean <= 0) {
                variance = 0;
                variance_r = 0;
                goto done;
            }
        }

        for (i = 0, sum = 0; i < results.N; i++) {
            double x, x_r;

            x = (double)results.u[i] / (double)results.n[i];
            sum += (x-mean) * (x-mean);

            /* multiply by 1000000?: dividing by us, but want ops/s */
            x_r = ((double)results.n[i] * 1000000.) / (double)results.u[i];
            sum_r += (x_r-mean_r) * (x_r-mean_r);
        }

        if (results.N == 1) {
            variance = sum;
            variance_r = sum;
        } else {
            variance = sum / (double) (results.N - 1);
            variance_r = sum_r / (double) (results.N - 1);
        }
    }
done:
    setvariancetime(variance);
    setvarianceratetime(variance_r);
}

/*
 * The inner loop tracks bench.h but uses a different results array.
 */
static long * one_op (register long *p)
{
    BENCH_INNER(p = (long *)*p, 0);
    return (p);
}

static long * two_op (register long * p, register long * q)
{
    BENCH_INNER(p = (long *)*q; q = (long*)*p, 0);
    return (p);
}

static long * p = (long *) &p;
static long * q = (long *) &q;

double
l_overhead(void)
{
    int    i;
    uint64_t    N_save, u_save;
    static    double overhead;
    static    int initialized = 0;
    result_t one, two, r_save;

    init_timing();
    if (initialized) return (overhead);

    initialized = 1;
    get_results(&r_save); N_save = get_n(); u_save = gettime(); 
    insertinit(&one);
    insertinit(&two);
    for (i = 0; i < TRIES; ++i) {
        use_pointer((void*)one_op(p));
        if (gettime() > t_overhead())
            insertsort(gettime() - t_overhead(), get_n(), &one);
        use_pointer((void*)two_op(p, q));
        if (gettime() > t_overhead())
            insertsort(gettime() - t_overhead(), get_n(), &two);
    }
    /*
     * u1 = (n1 * (overhead + work))
     * u2 = (n2 * (overhead + 2 * work))
     * ==> overhead = 2. * u1 / n1 - u2 / n2
     */
    save_results(&one); save_minimum();
    overhead = 2. * gettime() / (double)get_n();
    save_results(&two); save_minimum();
    overhead -= gettime() / (double)get_n();

    if (overhead < 0.) overhead = 0.;    /* Gag */

    save_results(&r_save); save_n(N_save); settime(u_save); 
    return (overhead);
}

/*
 * Figure out the timing overhead.  This has to track bench.h
 */
uint64_t t_overhead (void)
{
    uint64_t N_save, u_save;
    static int initialized = 0;
    static uint64_t overhead = 0;
    result_t r_save;

    init_timing();
    if (initialized) return (overhead);

    initialized = 1;
    if (get_enough(0) <= 50000) {
        /* it is not in the noise, so compute it */
        int i;
        result_t r;

        get_results(&r_save); N_save = get_n(); u_save = gettime();
        insertinit(&r);
        for (i = 0; i < TRIES; ++i) {
            BENCH_INNER(DkSystemTimeQuery(), 0);
            insertsort(gettime(), get_n(), &r);
        }
        save_results(&r);
        save_minimum();
        overhead = gettime() / get_n();

        save_results(&r_save); save_n(N_save); settime(u_save);
    }
    return (overhead);
}

/*
 * Figure out how long to run it.
 * If enough == 0, then they want us to figure it out.
 * If enough is !0 then return it unless we think it is too short.
 */
static    int    long_enough;
static    int    compute_enough();

int get_enough (int e)
{
    init_timing();
    return (long_enough > e ? long_enough : e);
}


static void init_timing (void)
{
    static    int done = 0;

    if (done) return;
    done = 1;
    long_enough = compute_enough();
    t_overhead();
    l_overhead();
}

typedef long TYPE;

static TYPE **
enough_duration(register long N, register TYPE ** p)
{
#define    ENOUGH_DURATION_TEN(one)    one one one one one one one one one one
    while (N-- > 0) {
        ENOUGH_DURATION_TEN(p = (TYPE **) *p;);
    }
    return (p);
}

static uint64_t
duration(long N)
{
    uint64_t    usecs;
    TYPE   *x = (TYPE *)&x;
    TYPE  **p = (TYPE **)&x;

    start(0);
    p = enough_duration(N, p);
    usecs = stop(0, 0);
    use_pointer((void *)p);
    return (usecs);
}

/*
 * find the minimum time that work "N" takes in "tries" tests
 */
static uint64_t time_N (long N)
{
    int i;
    uint64_t usecs;
    result_t r;

    insertinit(&r);
    for (i = 1; i < TRIES; ++i) {
        usecs = duration(N);
        insertsort(usecs, N, &r);
    }
    save_results(&r);
    save_minimum();
    return (gettime());
}

/*
 * return the amount of work needed to run "enough" microseconds
 */
static long find_N (int enough)
{
    int tries;
    static long N = 10000;
    static uint64_t usecs = 0;

    if (!usecs) usecs = time_N(N);

    for (tries = 0; tries < 10; ++tries) {
        if (0.98 * enough < usecs && usecs < 1.02 * enough)
            return (N);
        if (usecs < 1000)
            N *= 10;
        else {
            double  n = N;

            n /= usecs;
            n *= enough;
            N = n + 1;
        }
        usecs = time_N(N);
    }
    return (0);
}

/*
 * We want to verify that small modifications proportionally affect the runtime
 */
static double test_points[] = {1.015, 1.02, 1.035};
static int test_time (int enough)
{
    int i;
    long N;
    uint64_t usecs, expected, baseline, diff;

    if ((N = find_N(enough)) <= 0)
        return (0);

    baseline = time_N(N);

    for (i = 0; i < sizeof(test_points) / sizeof(double); ++i) {
        usecs = time_N((int)((double) N * test_points[i]));
        expected = (uint64_t)((double)baseline * test_points[i]);
        diff = expected > usecs ? expected - usecs : usecs - expected;
        if (diff / (double)expected > 0.0025)
            return (0);
    }
    return (1);
}


/*
 * We want to find the smallest timing interval that has accurate timing
 */
static int possibilities[] = { 5000, 10000, 50000, 100000 };
static int compute_enough (void)
{
    int i;
    for (i = 0; i < sizeof(possibilities) / sizeof(int); ++i) {
        if (test_time(possibilities[i]))
            return (possibilities[i]);
    }

    /* 
     * if we can't find a timing interval that is sufficient, 
     * then use SHORT as a default.
     */
    return (SHORT);
}
