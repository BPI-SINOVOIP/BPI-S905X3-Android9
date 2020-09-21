#include "tests.h"
#include <asm/unistd.h>

#ifdef __NR_setgid

# define SYSCALL_NR	__NR_setgid
# define SYSCALL_NAME	"setgid"

# if defined __NR_setgid32 && __NR_setgid != __NR_setgid32
#  define UGID_TYPE	short
#  define GETUGID	syscall(__NR_getegid)
#  define CHECK_OVERFLOWUGID(arg)	check_overflowgid(arg)
# else
#  define UGID_TYPE	int
#  define GETUGID	getegid()
#  define CHECK_OVERFLOWUGID(arg)
# endif

# include "setugid.c"

#else

SKIP_MAIN_UNDEFINED("__NR_setgid")

#endif
