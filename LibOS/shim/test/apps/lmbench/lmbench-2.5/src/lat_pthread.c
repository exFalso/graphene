/*
 * lat_syscall.c - time simple system calls
 *
 * Copyright (c) 1996 Larry McVoy.  Distributed under the FSF GPL with
 * additional restriction that results may published only if
 * (1) the benchmark is unmodified, and
 * (2) the version in the sccsid below is included in the report.
 */
char	*id = "$Id$\n";

#include "bench.h"

#include <pthread.h>
#include <string.h>

int state = 0;

void
do_cond(void ** obj)
{
	pthread_cond_t *cv1 = obj[0], *cv2 = obj[1];
	pthread_mutex_t *mut1 = obj[2], *mut2 = obj[3];

	pthread_mutex_lock(mut1);
	state = 1;
	pthread_cond_signal(cv1);
	pthread_mutex_unlock(mut1);

	pthread_mutex_lock(mut2);
	while (state != 2)
		pthread_cond_wait(cv2, mut2);
	pthread_mutex_unlock(mut2);
}

void *
thread_c(void *p)
{
	void **obj = (void **)p;
	pthread_cond_t *cv1 = obj[0], *cv2 = obj[1];
	pthread_mutex_t *mut1 = obj[2], *mut2 = obj[3];

	while(1) {
		pthread_mutex_lock(mut1);
		while (state != 1)
			pthread_cond_wait(cv1, mut1);
		pthread_mutex_unlock(mut1);

		pthread_mutex_lock(mut2);
		state = 2;
		pthread_cond_signal(cv2);
		pthread_mutex_unlock(mut2);
	}
	return NULL;
}


void
do_mutex(pthread_mutex_t *mut)
{
	pthread_mutex_lock(mut);
	pthread_mutex_unlock(mut);
}

void *
thread_m(void *p)
{
	pthread_mutex_t *mut = (pthread_mutex_t *) p;
	while(1)
		do_mutex(mut);
	return NULL;
}

int
main(int ac, char **av)
{
	int	fd;
	char	*file;

	if (ac < 2) goto usage;

	if (!strcmp("cond", av[1])) {
		pthread_cond_t cv[2];
		pthread_mutex_t mut[2];
		void * obj[4] = { &cv[0], &cv[1], &mut[0], &mut[1] };
		pthread_t thread;

		pthread_cond_init(&cv[0], NULL);
		pthread_cond_init(&cv[1], NULL);
		pthread_mutex_init(&mut[0], NULL);
		pthread_mutex_init(&mut[1], NULL);

		pthread_create(&thread, NULL, thread_c, obj);

		BENCH(do_cond(obj), 0);
		micro("Condvar latency", get_n());
		exit(0);
	} else if (!strcmp("mutex", av[1])) {
		pthread_mutex_t mut;
		pthread_t thread;
		int n = 2, i;

		if (av[2]) n = atoi(av[2]);

		pthread_mutex_init(&mut, NULL);

		for (i = 1; i < n; i++)
			pthread_create(&thread, NULL, thread_m, &mut);

		BENCH(do_mutex(&mut), 0);
		micro("Mutex latency", get_n());
		exit(0);
	} else {
usage:		printf("Usage: %s cond|mutex [n]\n", av[0]);
	}
	return(0);
}
