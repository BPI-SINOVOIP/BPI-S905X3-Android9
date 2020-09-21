#include <stdio.h>
#include <sys/stat.h>

void
test_umask(const mode_t mode)
{
	mode_t rc = umask(0xffff0000 | mode);
	printf("umask(%#03ho) = %#03o\n", (unsigned short) mode, rc);
}

int
main(void)
{
	test_umask(0);
	test_umask(06);
	test_umask(026);
	test_umask(0126);
	test_umask(07777);
	test_umask(0107777);
	test_umask(-1);

	puts("+++ exited with 0 +++");
	return 0;
}
