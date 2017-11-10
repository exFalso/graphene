#include <linux/posix_types.h>

/* Some versions of <linux/posix_types.h> define this macros.  */
#undef	__NFDBITS
/* It's easier to assume 8-bit bytes than to get CHAR_BIT.  */
#define __NFDBITS       (8 * (int) sizeof (long))
#define __FD_ELT(d)     ((d) / __NFDBITS)
#define __FD_MASK(d)    ((long) (1UL << ((d) % __NFDBITS)))
#define __FDS_BITS(set)  ((set)->fds_bits)

/* We don't use `memset' because this would require a prototype and
   the array isn't too big.  */
# define __FD_ZERO(set)                                 \
  do {                                                  \
    unsigned int __i;                                   \
    __kernel_fd_set *__arr = (set);                     \
    for (__i = 0; __i < sizeof (__kernel_fd_set) / sizeof (long); ++__i) \
      __FDS_BITS (__arr)[__i] = 0;                      \
  } while (0)

#define __FD_SET(d, set)                                \
  ((void) (__FDS_BITS (set)[__FD_ELT (d)] |= __FD_MASK (d)))
#define __FD_CLR(d, set)                                \
  ((void) (__FDS_BITS (set)[__FD_ELT (d)] &= ~__FD_MASK (d)))
#define __FD_ISSET(d, set)                              \
  ((__FDS_BITS (set)[__FD_ELT (d)] & __FD_MASK (d)) != 0)
