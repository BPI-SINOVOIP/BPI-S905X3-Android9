#include "tests.h"
#include <asm/unistd.h>

#ifdef __NR_setregid

# define SYSCALL_NR	__NR_setregid
# define SYSCALL_NAME	"setregid"

# if defined __NR_setregid32 && __NR_setregid != __NR_setregid32
#  define UGID_TYPE	short
#  define GETUGID	syscall(__NR_getegid)
#  define CHECK_OVERFLOWUGID(arg)	check_overflowgid(arg)
# else
#  define UGID_TYPE	int
#  define GETUGID	getegid()
#  define CHECK_OVERFLOWUGID(arg)
# endif

# include "setreugid.c"

#else

SKIP_MAIN_UNDEFINED("__NR_setregid")

#endif
