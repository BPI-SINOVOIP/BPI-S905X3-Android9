#include "tests.h"
#include <asm/unistd.h>

#ifdef __NR_setresgid

# define SYSCALL_NR	__NR_setresgid
# define SYSCALL_NAME	"setresgid"

# if defined __NR_setresgid32 && __NR_setresgid != __NR_setresgid32
#  define UGID_TYPE	short
#  define GETUGID	syscall(__NR_getegid)
#  define CHECK_OVERFLOWUGID(arg)	check_overflowgid(arg)
# else
#  define UGID_TYPE	int
#  define GETUGID	getegid()
#  define CHECK_OVERFLOWUGID(arg)
# endif

# include "setresugid.c"

#else

SKIP_MAIN_UNDEFINED("__NR_setresgid")

#endif
