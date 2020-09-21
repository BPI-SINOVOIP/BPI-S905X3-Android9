#include "tests.h"
#include <asm/unistd.h>

#ifdef __NR_unlinkat

# include <stdio.h>
# include <unistd.h>

int
main(void)
{
	static const char sample[] = "unlinkat_sample";
	const long fd = (long) 0xdeadbeefffffffffULL;

	long rc = syscall(__NR_unlinkat, fd, sample, 0);
	printf("unlinkat(%d, \"%s\", 0) = %ld %s (%m)\n",
	       (int) fd, sample, rc, errno2name());

	rc = syscall(__NR_unlinkat, -100, sample, -1L);
	printf("unlinkat(%s, \"%s\", %s) = %ld %s (%m)\n",
	       "AT_FDCWD", sample,
	       "AT_SYMLINK_NOFOLLOW|AT_REMOVEDIR|AT_SYMLINK_FOLLOW"
	       "|AT_NO_AUTOMOUNT|AT_EMPTY_PATH|0xffffe0ff",
	       rc, errno2name());

	puts("+++ exited with 0 +++");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("__NR_unlinkat")

#endif
