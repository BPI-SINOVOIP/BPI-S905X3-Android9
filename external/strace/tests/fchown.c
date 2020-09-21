#include "tests.h"
#include <asm/unistd.h>

#ifdef __NR_fchown

# define SYSCALL_NR __NR_fchown
# define SYSCALL_NAME "fchown"
# define ACCESS_BY_DESCRIPTOR

# if defined __NR_fchown32 && __NR_fchown != __NR_fchown32
#  define UGID_TYPE_IS_SHORT
# endif

# include "xchownx.c"

#else

SKIP_MAIN_UNDEFINED("__NR_fchown")

#endif
