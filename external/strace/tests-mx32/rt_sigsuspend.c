/*
 * This file is part of rt_sigsuspend strace test.
 *
 * Copyright (c) 2016 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2016-2017 The strace developers.
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
#include <asm/unistd.h>

#ifdef __NR_rt_sigsuspend

# include <assert.h>
# include <errno.h>
# include <signal.h>
# include <stdio.h>
# include <stdint.h>
# include <string.h>
# include <unistd.h>

static long
k_sigsuspend(const sigset_t *const set, const unsigned long size)
{
	return syscall(__NR_rt_sigsuspend, set, size);
}

static void
iterate(const char *const text, const int sig,
	const void *const set, unsigned int size)
{
	const void *mask;

	for (mask = set;; size >>= 1, mask += size) {
		raise(sig);
		assert(k_sigsuspend(mask, size) == -1);
		if (EINTR == errno) {
			tprintf("rt_sigsuspend(%s, %u) = ? ERESTARTNOHAND"
				" (To be restarted if no handler)\n",
				text, size);
		} else {
			if (size < sizeof(long))
				tprintf("rt_sigsuspend(%p, %u)"
					" = -1 EINVAL (%m)\n",
					mask, size);
			else
				tprintf("rt_sigsuspend(%s, %u)"
					" = -1 EINVAL (%m)\n",
					set == mask ? text : "~[]", size);
		}
		if (!size)
			break;
	}
}

static void
handler(int signo)
{
}

int
main(void)
{
	tprintf("%s", "");

	const unsigned int big_size = 1024 / 8;
	void *k_set = tail_alloc(big_size);
	memset(k_set, 0, big_size);

	TAIL_ALLOC_OBJECT_CONST_PTR(sigset_t, libc_set);
	sigemptyset(libc_set);
	sigaddset(libc_set, SIGUSR1);
	if (sigprocmask(SIG_SETMASK, libc_set, NULL))
		perror_msg_and_fail("sigprocmask");

	const struct sigaction sa = {
		.sa_handler = handler
	};
	if (sigaction(SIGUSR1, &sa, NULL))
		perror_msg_and_fail("sigaction");

	raise(SIGUSR1);
	unsigned int set_size = big_size;
	for (; set_size; set_size >>= 1, k_set += set_size) {
		assert(k_sigsuspend(k_set, set_size) == -1);
		if (EINTR == errno)
			break;
		tprintf("rt_sigsuspend(%p, %u) = -1 EINVAL (%m)\n",
			k_set, set_size);
	}
	if (!set_size)
		perror_msg_and_fail("rt_sigsuspend");
	tprintf("rt_sigsuspend([], %u) = ? ERESTARTNOHAND"
		" (To be restarted if no handler)\n", set_size);

	sigemptyset(libc_set);
	sigaddset(libc_set, SIGUSR2);
	memcpy(k_set, libc_set, set_size);
	raise(SIGUSR1);
	assert(k_sigsuspend(k_set, set_size) == -1);
	assert(EINTR == errno);
	tprintf("rt_sigsuspend([USR2], %u) = ? ERESTARTNOHAND"
		" (To be restarted if no handler)\n", set_size);

	sigaddset(libc_set, SIGHUP);
	memcpy(k_set, libc_set, set_size);
	raise(SIGUSR1);
	assert(k_sigsuspend(k_set, set_size) == -1);
	assert(EINTR == errno);
	tprintf("rt_sigsuspend([HUP USR2], %u) = ? ERESTARTNOHAND"
		" (To be restarted if no handler)\n", set_size);

	sigaddset(libc_set, SIGINT);
	memcpy(k_set, libc_set, set_size);
	raise(SIGUSR1);
	assert(k_sigsuspend(k_set, set_size) == -1);
	assert(EINTR == errno);
	tprintf("rt_sigsuspend([HUP INT USR2], %u) = ? ERESTARTNOHAND"
		" (To be restarted if no handler)\n", set_size);

	memset(libc_set, -1, sizeof(*libc_set));
	sigdelset(libc_set, SIGUSR1);
	memcpy(k_set, libc_set, set_size);
	raise(SIGUSR1);
	assert(k_sigsuspend(k_set, set_size) == -1);
	assert(EINTR == errno);
	tprintf("rt_sigsuspend(~[USR1], %u) = ? ERESTARTNOHAND"
		" (To be restarted if no handler)\n", set_size);

	assert(k_sigsuspend(k_set - set_size, set_size << 1) == -1);
	tprintf("rt_sigsuspend(%p, %u) = -1 EINVAL (%m)\n",
		k_set - set_size, set_size << 1);

	iterate("~[USR1]", SIGUSR1, k_set, set_size >> 1);

	tprintf("+++ exited with 0 +++\n");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("__NR_rt_sigsuspend")

#endif
