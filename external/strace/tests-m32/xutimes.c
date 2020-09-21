/*
 * Check decoding of utimes/osf_utimes syscall.
 *
 * Copyright (c) 2015-2017 Dmitry V. Levin <ldv@altlinux.org>
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

#ifndef TEST_SYSCALL_NR
# error TEST_SYSCALL_NR must be defined
#endif

#ifndef TEST_SYSCALL_STR
# error TEST_SYSCALL_STR must be defined
#endif

#ifndef TEST_STRUCT
# error TEST_STRUCT must be defined
#endif

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

static void
print_tv(const TEST_STRUCT *const tv)
{
	printf("{tv_sec=%lld, tv_usec=%llu}",
	       (long long) tv->tv_sec,
	       zero_extend_signed_to_ull(tv->tv_usec));
	print_time_t_usec(tv->tv_sec,
			  zero_extend_signed_to_ull(tv->tv_usec), 1);
}

static const char *errstr;

static long
k_utimes(const kernel_ulong_t pathname, const kernel_ulong_t times)
{
	long rc = syscall(TEST_SYSCALL_NR, pathname, times);
	errstr = sprintrc(rc);
	return rc;
}

int
main(void)
{
	static const char proto_fname[] = TEST_SYSCALL_STR "_sample";
	static const char qname[] = "\"" TEST_SYSCALL_STR "_sample\"";

	char *const fname = tail_memdup(proto_fname, sizeof(proto_fname));
	const kernel_ulong_t kfname = (uintptr_t) fname;
	TEST_STRUCT *const tv = tail_alloc(sizeof(*tv) * 2);

	/* pathname */
	k_utimes(0, 0);
	printf("%s(NULL, NULL) = %s\n", TEST_SYSCALL_STR, errstr);

	k_utimes(kfname + sizeof(proto_fname) - 1, 0);
	printf("%s(\"\", NULL) = %s\n", TEST_SYSCALL_STR, errstr);

	k_utimes(kfname, 0);
	printf("%s(%s, NULL) = %s\n", TEST_SYSCALL_STR, qname, errstr);

	fname[sizeof(proto_fname) - 1] = '+';
	k_utimes(kfname, 0);
	fname[sizeof(proto_fname) - 1] = '\0';
	printf("%s(%p, NULL) = %s\n", TEST_SYSCALL_STR, fname, errstr);

	if (F8ILL_KULONG_SUPPORTED) {
		k_utimes(f8ill_ptr_to_kulong(fname), 0);
		printf("%s(%#jx, NULL) = %s\n", TEST_SYSCALL_STR,
		       (uintmax_t) f8ill_ptr_to_kulong(fname), errstr);
	}

	/* times */
	k_utimes(kfname, (uintptr_t) (tv + 1));
	printf("%s(%s, %p) = %s\n", TEST_SYSCALL_STR,
	       qname, tv + 1, errstr);

	k_utimes(kfname, (uintptr_t) (tv + 2));
	printf("%s(%s, %p) = %s\n", TEST_SYSCALL_STR,
	       qname, tv + 2, errstr);

	tv[0].tv_sec = 0xdeadbeefU;
	tv[0].tv_usec = 0xfacefeedU;
	tv[1].tv_sec = (time_t) 0xcafef00ddeadbeefLL;
	tv[1].tv_usec = (suseconds_t) 0xbadc0dedfacefeedLL;

	k_utimes(kfname, (uintptr_t) tv);
	printf("%s(%s, [", TEST_SYSCALL_STR, qname);
	print_tv(&tv[0]);
	printf(", ");
	print_tv(&tv[1]);
	printf("]) = %s\n", errstr);

	tv[0].tv_sec = 1492358607;
	tv[0].tv_usec = 1000000;
	tv[1].tv_sec = 1492356078;
	tv[1].tv_usec = 1000001;

	k_utimes(kfname, (uintptr_t) tv);
	printf("%s(%s, [", TEST_SYSCALL_STR, qname);
	print_tv(&tv[0]);
	printf(", ");
	print_tv(&tv[1]);
	printf("]) = %s\n", errstr);

	tv[0].tv_usec = 345678;
	tv[1].tv_usec = 456789;

	k_utimes(kfname, (uintptr_t) tv);
	printf("%s(%s, [", TEST_SYSCALL_STR, qname);
	print_tv(&tv[0]);
	printf(", ");
	print_tv(&tv[1]);
	printf("]) = %s\n", errstr);

	if (F8ILL_KULONG_SUPPORTED) {
		k_utimes(kfname, f8ill_ptr_to_kulong(tv));
		printf("%s(%s, %#jx) = %s\n", TEST_SYSCALL_STR,
		       qname, (uintmax_t) f8ill_ptr_to_kulong(tv), errstr);
	}

	puts("+++ exited with 0 +++");
	return 0;
}
