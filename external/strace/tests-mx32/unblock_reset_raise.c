/*
 * Unblock, reset, and raise a signal.
 *
 * Copyright (c) 2017 Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tests.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int ac, char **av)
{
	if (ac != 2)
		error_msg_and_fail("usage: unblock_raise signo");

	sigset_t mask;
	sigemptyset(&mask);
	const int signo = atoi(av[1]);
	if (sigaddset(&mask, signo))
		perror_msg_and_fail("sigaddset: %s", av[1]);
	if (sigprocmask(SIG_UNBLOCK, &mask, NULL))
		perror_msg_and_fail("sigprocmask");
	if (signal(signo, SIG_DFL) == SIG_ERR)
		perror_msg_and_fail("signal: %s", av[1]);
	if (raise(signo))
		perror_msg_and_fail("raise: %s", av[1]);

	return 0;
}
