#include "tests.h"
#include <signal.h>

int
main(void)
{
	stack_t ss = {
		.ss_sp = (void *) 0xbaadf00d,
		.ss_flags = SS_DISABLE,
		.ss_size = 0xdeadbeef
	};
	if (sigaltstack(&ss, (stack_t *) 0))
		perror_msg_and_skip("sigaltstack");
	return 0;
}
