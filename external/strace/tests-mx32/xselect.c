/*
 * Check decoding of select/_newselect syscalls.
 *
 * Copyright (c) 2015-2018 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2015-2017 The strace developers.
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

/*
 * Based on test by Dr. David Alan Gilbert <dave@treblig.org>
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

static const char *errstr;

static long
xselect(const kernel_ulong_t nfds,
	const kernel_ulong_t rs,
	const kernel_ulong_t ws,
	const kernel_ulong_t es,
	const kernel_ulong_t tv)
#ifndef xselect
{
	long rc = syscall(TEST_SYSCALL_NR,
			  F8ILL_KULONG_MASK | nfds, rs, ws, es, tv);
	errstr = sprintrc(rc);
	return rc;
}
#else
	;
#endif

#define XSELECT(expected_, ...)						\
	do {								\
		long rc = xselect(__VA_ARGS__);				\
		if (rc != (expected_))					\
			perror_msg_and_fail(TEST_SYSCALL_STR		\
					    ": expected %d"		\
					    ", returned %ld",		\
					    (expected_), rc);		\
	} while (0)							\
/* End of XSELECT definition. */

int
main(void)
{
#ifdef PATH_TRACING_FD
	skip_if_unavailable("/proc/self/fd/");
#endif

	for (int i = 3; i < FD_SETSIZE; ++i) {
#ifdef PATH_TRACING_FD
		if (i == PATH_TRACING_FD)
			continue;
#endif
		(void) close(i);
	}

	int fds[2];
	if (pipe(fds))
		perror_msg_and_fail("pipe");

	static const int smallset_size = sizeof(kernel_ulong_t) * 8;
	const int nfds = fds[1] + 1;
	if (nfds > smallset_size)
		error_msg_and_fail("nfds[%d] > smallset_size[%d]\n",
				   nfds, smallset_size);

	struct timeval tv_in = { 0, 123 };
	struct timeval *const tv = tail_memdup(&tv_in, sizeof(tv_in));
	const uintptr_t a_tv = (uintptr_t) tv;

	TAIL_ALLOC_OBJECT_VAR_PTR(kernel_ulong_t, l_rs);
	fd_set *const rs = (void *) l_rs;
	const uintptr_t a_rs = (uintptr_t) rs;

	TAIL_ALLOC_OBJECT_VAR_PTR(kernel_ulong_t, l_ws);
	fd_set *const ws = (void *) l_ws;
	const uintptr_t a_ws = (uintptr_t) ws;

	TAIL_ALLOC_OBJECT_VAR_PTR(kernel_ulong_t, l_es);
	fd_set *const es = (void *) l_es;
	const uintptr_t a_es = (uintptr_t) es;

	long rc;

	/*
	 * An equivalent of nanosleep.
	 */
	if (xselect(0, 0, 0, 0, a_tv)) {
		if (errno == ENOSYS)
			perror_msg_and_skip(TEST_SYSCALL_STR);
		else
			perror_msg_and_fail(TEST_SYSCALL_STR);
	}
#ifndef PATH_TRACING_FD
	printf("%s(0, NULL, NULL, NULL, {tv_sec=%lld, tv_usec=%llu})"
	       " = 0 (Timeout)\n",
	       TEST_SYSCALL_STR, (long long) tv_in.tv_sec,
	       zero_extend_signed_to_ull(tv_in.tv_usec));
#endif

	/* EFAULT on tv argument */
	XSELECT(-1, 0, 0, 0, 0, a_tv + 1);
#ifndef PATH_TRACING_FD
	printf("%s(0, NULL, NULL, NULL, %#lx) = %s\n",
	       TEST_SYSCALL_STR, (unsigned long) a_tv + 1, errstr);
#endif

	/*
	 * Start with a nice simple select with the same set.
	 */
	for (int i = nfds; i <= smallset_size; ++i) {
		*l_rs = (1UL << fds[0]) | (1UL << fds[1]);
		XSELECT(1, i, a_rs, a_rs, a_rs, 0);
#ifndef PATH_TRACING_FD
		printf("%s(%d, [%d %d], [%d %d], [%d %d], NULL) = 1 ()\n",
		       TEST_SYSCALL_STR, i, fds[0], fds[1],
		       fds[0], fds[1], fds[0], fds[1]);
#else
		*l_rs = (1UL << fds[0]) | (1UL << fds[1]) |
			(1UL << PATH_TRACING_FD);
		XSELECT(i > PATH_TRACING_FD ? 3 : 1, i, a_rs, a_rs, a_rs, 0);
		if (i > PATH_TRACING_FD) {
			printf("%s(%d, [%d %d %d], [%d %d %d], [%d %d %d]"
			       ", NULL) = 3 ()\n",
			       TEST_SYSCALL_STR, i,
			       fds[0], fds[1], PATH_TRACING_FD,
			       fds[0], fds[1], PATH_TRACING_FD,
			       fds[0], fds[1], PATH_TRACING_FD);
		}
#endif
	}

	/*
	 * Odd timeout.
	 */
	*l_rs = (1UL << fds[0]) | (1UL << fds[1]);
	tv_in.tv_sec = 0xdeadbeefU;
	tv_in.tv_usec = 0xfacefeedU;
	memcpy(tv, &tv_in, sizeof(tv_in));
	rc = xselect(nfds, a_rs, a_rs, a_rs, a_tv);
	if (rc < 0) {
#ifndef PATH_TRACING_FD
		printf("%s(%d, [%d %d], [%d %d], [%d %d]"
		       ", {tv_sec=%lld, tv_usec=%llu}) = %s\n",
		       TEST_SYSCALL_STR, nfds, fds[0], fds[1],
		       fds[0], fds[1], fds[0], fds[1],
		       (long long) tv_in.tv_sec,
		       zero_extend_signed_to_ull(tv_in.tv_usec),
		       errstr);
#endif /* !PATH_TRACING_FD */
	} else {
#ifndef PATH_TRACING_FD
		printf("%s(%d, [%d %d], [%d %d], [%d %d]"
		       ", {tv_sec=%lld, tv_usec=%llu}) = %ld"
		       " (left {tv_sec=%lld, tv_usec=%llu})\n",
		       TEST_SYSCALL_STR, nfds, fds[0], fds[1],
		       fds[0], fds[1], fds[0], fds[1],
		       (long long) tv_in.tv_sec,
		       zero_extend_signed_to_ull(tv_in.tv_usec),
		       rc, (long long) tv->tv_sec,
		       zero_extend_signed_to_ull(tv->tv_usec));
#endif /* !PATH_TRACING_FD */
	}

	/*
	 * Very odd timeout.
	 */
	*l_rs = (1UL << fds[0]) | (1UL << fds[1]);
	tv_in.tv_sec = (time_t) 0xcafef00ddeadbeefLL;
	tv_in.tv_usec = (suseconds_t) 0xbadc0dedfacefeedLL;
	memcpy(tv, &tv_in, sizeof(tv_in));
	rc = xselect(nfds, a_rs, a_rs, a_rs, a_tv);
	if (rc < 0) {
#ifndef PATH_TRACING_FD
		printf("%s(%d, [%d %d], [%d %d], [%d %d]"
		       ", {tv_sec=%lld, tv_usec=%llu}) = %s\n",
		       TEST_SYSCALL_STR, nfds, fds[0], fds[1],
		       fds[0], fds[1], fds[0], fds[1],
		       (long long) tv_in.tv_sec,
		       zero_extend_signed_to_ull(tv_in.tv_usec),
		       errstr);
#endif /* PATH_TRACING_FD */
	} else {
#ifndef PATH_TRACING_FD
		printf("%s(%d, [%d %d], [%d %d], [%d %d]"
		       ", {tv_sec=%lld, tv_usec=%llu}) = %ld"
		       " (left {tv_sec=%lld, tv_usec=%llu})\n",
		       TEST_SYSCALL_STR, nfds, fds[0], fds[1],
		       fds[0], fds[1], fds[0], fds[1],
		       (long long) tv_in.tv_sec,
		       zero_extend_signed_to_ull(tv_in.tv_usec),
		       rc, (long long) tv->tv_sec,
		       zero_extend_signed_to_ull(tv->tv_usec));
#endif /* PATH_TRACING_FD */
	}

	/*
	 * Another simple one, with a timeout.
	 */
	for (int i = nfds; i <= smallset_size; ++i) {
		*l_rs = (1UL << fds[0]) | (1UL << fds[1]);
		*l_ws = (1UL << 1) | (1UL << 2) |
			(1UL << fds[0]) | (1UL << fds[1]);
		*l_es = 0;
		tv_in.tv_sec = 0xc0de1;
		tv_in.tv_usec = 0xc0de2;
		memcpy(tv, &tv_in, sizeof(tv_in));
		XSELECT(3, i, a_rs, a_ws, a_es, a_tv);
#ifndef PATH_TRACING_FD
		printf("%s(%d, [%d %d], [%d %d %d %d], []"
		       ", {tv_sec=%lld, tv_usec=%llu}) = 3 (out [1 2 %d]"
		       ", left {tv_sec=%lld, tv_usec=%llu})\n",
		       TEST_SYSCALL_STR, i, fds[0], fds[1],
		       1, 2, fds[0], fds[1],
		       (long long) tv_in.tv_sec,
		       zero_extend_signed_to_ull(tv_in.tv_usec),
		       fds[1],
		       (long long) tv->tv_sec,
		       zero_extend_signed_to_ull(tv->tv_usec));
#else
		*l_rs = (1UL << fds[0]) | (1UL << fds[1]) |
			(1UL << PATH_TRACING_FD);
		*l_ws = (1UL << 1) | (1UL << 2) |
			(1UL << fds[0]) | (1UL << fds[1]);
		tv_in.tv_sec = 0xc0de1;
		tv_in.tv_usec = 0xc0de2;
		memcpy(tv, &tv_in, sizeof(tv_in));
		XSELECT(3 + (i > PATH_TRACING_FD), i, a_rs, a_ws, a_es, a_tv);
		if (i > PATH_TRACING_FD) {
			printf("%s(%d, [%d %d %d], [%d %d %d %d], []"
			       ", {tv_sec=%lld, tv_usec=%llu})"
			       " = 4 (in [%d], out [1 2 %d]"
			       ", left {tv_sec=%lld, tv_usec=%llu})\n",
			       TEST_SYSCALL_STR, i,
			       fds[0], fds[1], PATH_TRACING_FD,
			       1, 2, fds[0], fds[1],
			       (long long) tv_in.tv_sec,
			       zero_extend_signed_to_ull(tv_in.tv_usec),
			       PATH_TRACING_FD, fds[1],
			       (long long) tv->tv_sec,
			       zero_extend_signed_to_ull(tv->tv_usec));
		}

		*l_rs = (1UL << fds[0]) | (1UL << fds[1]);
		*l_ws = (1UL << 1) | (1UL << 2) |
			(1UL << fds[0]) | (1UL << fds[1]) |
			(1UL << PATH_TRACING_FD);
		tv_in.tv_sec = 0xc0de1;
		tv_in.tv_usec = 0xc0de2;
		memcpy(tv, &tv_in, sizeof(tv_in));
		XSELECT(3 + (i > PATH_TRACING_FD), i, a_rs, a_ws, a_es, a_tv);
		if (i > PATH_TRACING_FD) {
			printf("%s(%d, [%d %d], [%d %d %d %d %d], []"
			       ", {tv_sec=%lld, tv_usec=%llu})"
			       " = 4 (out [1 2 %d %d]"
			       ", left {tv_sec=%lld, tv_usec=%llu})\n",
			       TEST_SYSCALL_STR, i,
			       fds[0], fds[1],
			       1, 2, fds[0], fds[1], PATH_TRACING_FD,
			       (long long) tv_in.tv_sec,
			       zero_extend_signed_to_ull(tv_in.tv_usec),
			       fds[1], PATH_TRACING_FD,
			       (long long) tv->tv_sec,
			       zero_extend_signed_to_ull(tv->tv_usec));
		}

		*l_rs = (1UL << fds[0]) | (1UL << fds[1]);
		*l_ws = (1UL << 1) | (1UL << 2) |
			(1UL << fds[0]) | (1UL << fds[1]);
		*l_es = (1UL << PATH_TRACING_FD);
		tv_in.tv_sec = 0xc0de1;
		tv_in.tv_usec = 0xc0de2;
		memcpy(tv, &tv_in, sizeof(tv_in));
		XSELECT(3, i, a_rs, a_ws, a_es, a_tv);
		if (i > PATH_TRACING_FD) {
		printf("%s(%d, [%d %d], [%d %d %d %d], [%d]"
		       ", {tv_sec=%lld, tv_usec=%llu}) = 3 (out [1 2 %d]"
		       ", left {tv_sec=%lld, tv_usec=%llu})\n",
		       TEST_SYSCALL_STR, i,
		       fds[0], fds[1],
		       1, 2, fds[0], fds[1], PATH_TRACING_FD,
		       (long long) tv_in.tv_sec,
		       zero_extend_signed_to_ull(tv_in.tv_usec),
		       fds[1],
		       (long long) tv->tv_sec,
		       zero_extend_signed_to_ull(tv->tv_usec));
		}

#endif /* PATH_TRACING_FD */
	}

	/*
	 * Now the crash case that trinity found, negative nfds
	 * but with a pointer to a large chunk of valid memory.
	 */
	static fd_set set[0x1000000 / sizeof(fd_set)];
	FD_SET(fds[1], set);
	XSELECT(-1, -1U, 0, (uintptr_t) set, 0, 0);
#ifndef PATH_TRACING_FD
	printf("%s(-1, NULL, %p, NULL, NULL) = %s\n",
	       TEST_SYSCALL_STR, set, errstr);
#endif

	/*
	 * Big sets, nfds exceeds FD_SETSIZE limit.
	 */
	const size_t big_size = sizeof(fd_set) + sizeof(long);
	fd_set *const big_rs = tail_alloc(big_size);
	const uintptr_t a_big_rs = (uintptr_t) big_rs;

	fd_set *const big_ws = tail_alloc(big_size);
	const uintptr_t a_big_ws = (uintptr_t) big_ws;

	for (unsigned int i = FD_SETSIZE; i <= big_size * 8; ++i) {
		memset(big_rs, 0, big_size);
		memset(big_ws, 0, big_size);
		FD_SET(fds[0], big_rs);
		tv->tv_sec = 0;
		tv->tv_usec = 10 + (i - FD_SETSIZE);
		XSELECT(0, i, a_big_rs, a_big_ws, 0, a_tv);
#ifndef PATH_TRACING_FD
		printf("%s(%d, [%d], [], NULL, {tv_sec=0, tv_usec=%d})"
		       " = 0 (Timeout)\n",
		       TEST_SYSCALL_STR, i, fds[0], 10 + (i - FD_SETSIZE));
#else
		FD_SET(fds[0], big_rs);
		FD_SET(PATH_TRACING_FD, big_rs);
		tv->tv_sec = 0;
		tv->tv_usec = 10 + (i - FD_SETSIZE);
		XSELECT(1, i, a_big_rs, a_big_ws, 0, a_tv);
		printf("%s(%d, [%d %d], [], NULL, {tv_sec=0, tv_usec=%d})"
		       " = 1 (in [%d], left {tv_sec=0, tv_usec=%llu})\n",
		       TEST_SYSCALL_STR, i, fds[0], PATH_TRACING_FD,
		       10 + (i - FD_SETSIZE), PATH_TRACING_FD,
		       zero_extend_signed_to_ull(tv->tv_usec));
#endif /* PATH_TRACING_FD */
	}

	/*
	 * Huge sets, nfds equals to INT_MAX.
	 */
	FD_SET(fds[0], set);
	FD_SET(fds[1], set);
	tv->tv_sec = 0;
	tv->tv_usec = 123;
	XSELECT(0, INT_MAX, (uintptr_t) set, (uintptr_t) &set[1],
		(uintptr_t) &set[2], a_tv);
#ifndef PATH_TRACING_FD
	printf("%s(%d, [%d %d], [], [], {tv_sec=0, tv_usec=123})"
	       " = 0 (Timeout)\n",
	       TEST_SYSCALL_STR, INT_MAX, fds[0], fds[1]);
#else
	FD_SET(fds[0], set);
	FD_SET(fds[1], set);
	FD_SET(PATH_TRACING_FD, set);
	tv->tv_sec = 0;
	tv->tv_usec = 123;
	XSELECT(1, INT_MAX, (uintptr_t) set, (uintptr_t) &set[1],
		(uintptr_t) &set[2], a_tv);
	printf("%s(%d, [%d %d %d], [], [], {tv_sec=0, tv_usec=123})"
	       " = 1 (in [%d], left {tv_sec=0, tv_usec=%llu})\n",
	       TEST_SYSCALL_STR, INT_MAX, fds[0], fds[1], PATH_TRACING_FD,
	       PATH_TRACING_FD, zero_extend_signed_to_ull(tv->tv_usec));
#endif /* PATH_TRACING_FD */

	/*
	 * Small sets, nfds exceeds FD_SETSIZE limit.
	 * The kernel seems to be fine with it but strace cannot follow.
	 */
	*l_rs = (1UL << fds[0]) | (1UL << fds[1])
#ifdef PATH_TRACING_FD
		| (1UL << PATH_TRACING_FD)
#endif
		;
	*l_ws = (1UL << fds[0]);
	*l_es = (1UL << fds[0]) | (1UL << fds[1])
#ifdef PATH_TRACING_FD
		| (1UL << PATH_TRACING_FD)
#endif
		;
	tv->tv_sec = 0;
	tv->tv_usec = 123;
	rc = xselect(FD_SETSIZE + 1, a_rs, a_ws, a_es, a_tv);
	if (rc < 0) {
#ifndef PATH_TRACING_FD
		printf("%s(%d, %p, %p, %p, {tv_sec=0, tv_usec=123}) = %s\n",
		       TEST_SYSCALL_STR, FD_SETSIZE + 1, rs, ws, es, errstr);
#endif
	} else {
#ifndef PATH_TRACING_FD
		printf("%s(%d, %p, %p, %p, {tv_sec=0, tv_usec=123})"
		       " = 0 (Timeout)\n",
		       TEST_SYSCALL_STR, FD_SETSIZE + 1, rs, ws, es);
#endif
	}

	/*
	 * Small sets, one of allocated descriptors exceeds smallset_size.
	 */
	if (dup2(fds[1], smallset_size) != smallset_size)
		perror_msg_and_fail("dup2");
#ifdef PATH_TRACING_FD
	FD_SET(PATH_TRACING_FD, rs);
	FD_SET(PATH_TRACING_FD, ws);
	FD_SET(PATH_TRACING_FD, es);
#endif
	XSELECT(-1, smallset_size + 1, a_rs, a_ws, a_es, 0);
#ifndef PATH_TRACING_FD
	printf("%s(%d, %p, %p, %p, NULL) = %s\n",
	       TEST_SYSCALL_STR, smallset_size + 1, rs, ws, es, errstr);
#endif

	/*
	 * Small and big sets,
	 * one of allocated descriptors exceeds smallset_size.
	 */
	memset(big_rs, 0, big_size);
	FD_SET(fds[0], big_rs);
	FD_SET(smallset_size, big_rs);
	memset(big_ws, 0, big_size);
	FD_SET(fds[1], big_ws);
	FD_SET(smallset_size, big_ws);
	XSELECT(-1, smallset_size + 1, a_big_rs, a_big_ws, a_es, 0);
#ifndef PATH_TRACING_FD
	printf("%s(%d, [%d %d], [%d %d], %p, NULL) = %s\n",
	       TEST_SYSCALL_STR, smallset_size + 1,
	       fds[0], smallset_size,
	       fds[1], smallset_size,
	       es, errstr);
#endif /* !PATH_TRACING_FD */
	XSELECT(-1, smallset_size + 1, a_es, a_big_ws, a_big_rs, 0);
#ifndef PATH_TRACING_FD
	printf("%s(%d, %p, [%d %d], [%d %d], NULL) = %s\n",
	       TEST_SYSCALL_STR, smallset_size + 1,
	       es,
	       fds[1], smallset_size,
	       fds[0], smallset_size,
	       errstr);
#endif /* !PATH_TRACING_FD */

	puts("+++ exited with 0 +++");
	return 0;
}
