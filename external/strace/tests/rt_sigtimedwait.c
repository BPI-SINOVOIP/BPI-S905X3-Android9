/*
 * This file is part of rt_sigtimedwait strace test.
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

#ifdef __NR_rt_sigtimedwait

# include <assert.h>
# include <errno.h>
# include <signal.h>
# include <stdio.h>
# include <stdint.h>
# include <string.h>
# include <unistd.h>

static long
k_sigtimedwait(const sigset_t *const set, siginfo_t *const info,
	       const struct timespec *const timeout, const unsigned long size)
{
	return syscall(__NR_rt_sigtimedwait, set, info, timeout, size);
}

static void
iterate(const char *const text, const void *set,
	const struct timespec *const timeout, unsigned int size)
{
	for (;;) {
		assert(k_sigtimedwait(set, NULL, timeout, size) == -1);
		if (EINTR == errno) {
			tprintf("rt_sigtimedwait(%s, NULL"
				", {tv_sec=%lld, tv_nsec=%llu}, %u)"
				" = -1 EAGAIN (%m)\n", text,
				(long long) timeout->tv_sec,
				zero_extend_signed_to_ull(timeout->tv_nsec),
				size);
		} else {
			if (size < sizeof(long))
				tprintf("rt_sigtimedwait(%p, NULL"
					", {tv_sec=%lld, tv_nsec=%llu}"
					", %u) = -1 EINVAL (%m)\n",
					set, (long long) timeout->tv_sec,
					zero_extend_signed_to_ull(timeout->tv_nsec),
					size);
			else
				tprintf("rt_sigtimedwait(%s, NULL"
					", {tv_sec=%lld, tv_nsec=%llu}"
					", %u) = -1 EINVAL (%m)\n",
					text, (long long) timeout->tv_sec,
					zero_extend_signed_to_ull(timeout->tv_nsec),
					size);
		}
		if (!size)
			break;
		size >>= 1;
		set += size;
	}
}

int
main(void)
{
	tprintf("%s", "");

	TAIL_ALLOC_OBJECT_CONST_PTR(siginfo_t, info);
	TAIL_ALLOC_OBJECT_CONST_PTR(struct timespec, timeout);
	timeout->tv_sec = 0;
	timeout->tv_nsec = 42;

	const unsigned int big_size = 1024 / 8;
	void *k_set = tail_alloc(big_size);
	memset(k_set, 0, big_size);

	unsigned int set_size = big_size;
	for (; set_size; set_size >>= 1, k_set += set_size) {
		assert(k_sigtimedwait(k_set, NULL, timeout, set_size) == -1);
		if (EAGAIN == errno)
			break;
		tprintf("rt_sigtimedwait(%p, NULL, {tv_sec=%lld, tv_nsec=%llu}"
			", %u) = -1 EINVAL (%m)\n",
			k_set, (long long) timeout->tv_sec,
			zero_extend_signed_to_ull(timeout->tv_nsec), set_size);
	}
	if (!set_size)
		perror_msg_and_fail("rt_sigtimedwait");
	tprintf("rt_sigtimedwait([], NULL, {tv_sec=%lld, tv_nsec=%llu}, %u)"
		" = -1 EAGAIN (%m)\n",
		(long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec), set_size);

	timeout->tv_sec = 0xdeadbeefU;
	timeout->tv_nsec = 0xfacefeedU;
	assert(k_sigtimedwait(k_set, NULL, timeout, set_size) == -1);
	tprintf("rt_sigtimedwait([], NULL, {tv_sec=%lld, tv_nsec=%llu}"
		", %u) = -1 EINVAL (%m)\n",
		(long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec), set_size);

	timeout->tv_sec = (time_t) 0xcafef00ddeadbeefLL;
	timeout->tv_nsec = (long) 0xbadc0dedfacefeedLL;
	assert(k_sigtimedwait(k_set, NULL, timeout, set_size) == -1);
	tprintf("rt_sigtimedwait([], NULL, {tv_sec=%lld, tv_nsec=%llu}"
		", %u) = -1 EINVAL (%m)\n",
		(long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec), set_size);

	timeout->tv_sec = 0;
	timeout->tv_nsec = 42;

	TAIL_ALLOC_OBJECT_CONST_PTR(sigset_t, libc_set);
	sigemptyset(libc_set);
	sigaddset(libc_set, SIGHUP);
	memcpy(k_set, libc_set, set_size);

	assert(k_sigtimedwait(k_set, info, timeout, set_size) == -1);
	assert(EAGAIN == errno);
	tprintf("rt_sigtimedwait([HUP], %p, {tv_sec=%lld, tv_nsec=%llu}, %u)"
		" = -1 EAGAIN (%m)\n",
		info, (long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec), set_size);

	sigaddset(libc_set, SIGINT);
	memcpy(k_set, libc_set, set_size);

	assert(k_sigtimedwait(k_set, info, timeout, set_size) == -1);
	assert(EAGAIN == errno);
	tprintf("rt_sigtimedwait([HUP INT], %p, {tv_sec=%lld, tv_nsec=%llu}, %u)"
		" = -1 EAGAIN (%m)\n",
		info, (long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec), set_size);

	sigaddset(libc_set, SIGQUIT);
	sigaddset(libc_set, SIGALRM);
	sigaddset(libc_set, SIGTERM);
	memcpy(k_set, libc_set, set_size);

	assert(k_sigtimedwait(k_set, info, timeout, set_size) == -1);
	assert(EAGAIN == errno);
	tprintf("rt_sigtimedwait(%s, %p, {tv_sec=%lld, tv_nsec=%llu}, %u)"
		" = -1 EAGAIN (%m)\n",
		"[HUP INT QUIT ALRM TERM]",
		info, (long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec), set_size);

	memset(k_set - set_size, -1, set_size);
	assert(k_sigtimedwait(k_set - set_size, info, timeout, set_size) == -1);
	assert(EAGAIN == errno);
	tprintf("rt_sigtimedwait(~[], %p, {tv_sec=%lld, tv_nsec=%llu}, %u)"
		" = -1 EAGAIN (%m)\n",
		info, (long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec), set_size);

	if (sigprocmask(SIG_SETMASK, libc_set, NULL))
		perror_msg_and_fail("sigprocmask");

	assert(k_sigtimedwait(k_set - set_size, info, NULL, set_size << 1) == -1);
	tprintf("rt_sigtimedwait(%p, %p, NULL, %u) = -1 EINVAL (%m)\n",
		k_set - set_size, info, set_size << 1);

	iterate("~[]", k_set - set_size, timeout, set_size >> 1);

	timeout->tv_sec = 1;
	raise(SIGALRM);
	assert(k_sigtimedwait(k_set, info, timeout, set_size) == SIGALRM);
	tprintf("rt_sigtimedwait(%s, {si_signo=%s, si_code=SI_TKILL"
		", si_pid=%d, si_uid=%d}, {tv_sec=%lld, tv_nsec=%llu}, %u)"
		" = %d (%s)\n",
		"[HUP INT QUIT ALRM TERM]", "SIGALRM", getpid(), getuid(),
		(long long) timeout->tv_sec,
		zero_extend_signed_to_ull(timeout->tv_nsec),
		set_size, SIGALRM, "SIGALRM");

	raise(SIGALRM);
	assert(k_sigtimedwait(k_set, NULL, NULL, set_size) == SIGALRM);
	tprintf("rt_sigtimedwait(%s, NULL, NULL, %u) = %d (%s)\n",
		"[HUP INT QUIT ALRM TERM]", set_size, SIGALRM, "SIGALRM");

	tprintf("+++ exited with 0 +++\n");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("__NR_rt_sigtimedwait")

#endif
