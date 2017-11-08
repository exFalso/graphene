/*
 * lat_brk.c - time how fast a mapping can be made and broken down
 *
 * Usage: brk size
 *
 * XXX - If an implementation did lazy address space mapping, this test
 * will make that system look very good.  I haven't heard of such a system.
 *
 * Copyright (c) 1994 Larry McVoy.  Distributed under the FSF GPL with
 * additional restriction that results may published only if
 * (1) the benchmark is unmodified, and
 * (2) the version in the sccsid below is included in the report.
 * Support for this development by Sun Microsystems is gratefully acknowledged.
 */
char	*id = "$Id$\n";

#include "bench.h"

#define	PSIZE	(16<<10)
#define	N	10
#define	STRIDE	(32)
#define	MINSIZE	(STRIDE*2)

#define	CHK(x)	if ((x) == -1) { perror("x"); exit(1); }

/*
 * This alg due to Linus.  The goal is to have both sparse and full
 * mappings reported.
 */
void
mapit(size_t size, int access)
{
	char	*p, *where, *end;
	char	c = size & 0xff;

	where = sbrk(size);

	if ((int)where == -1) {
		perror("brk");
		exit(1);
	}
	if (access) {
		end = where + size;
		for (p = where; p < end; p += STRIDE) {
			*p = c;
		}
	}

	sbrk(-size);
}

int
main(int ac, char **av)
{
	int	fd = -1;
	size_t	size;
	int	access = 1;
	char	*prog = av[0];

	if (ac != 2 && ac != 3) {
		fprintf(stderr, "usage: %s [-n] size\n", prog);
		exit(1);
	}
	if (strcmp("-n", av[1]) == 0) {
		access = 0;
		ac--, av++;
	}
	size = bytes(av[1]);
	if (size < MINSIZE) {
		return (1);
	}
	BENCH(mapit(size, access), REAL_SHORT);
	micromb(size, get_n());
	return(0);
}
