/*
 * Check decoding of invalid pointer by umoven.
 *
 * Copyright (c) 2016 Dmitry V. Levin <ldv@altlinux.org>
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
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <asm/unistd.h>

int
main(void)
{
	if (F8ILL_KULONG_SUPPORTED) {
		struct timespec ts = { 0, 0 };
		const void *const p = tail_memdup(&ts, sizeof(ts));

		long rc = syscall(__NR_nanosleep, p, NULL);
		printf("nanosleep({tv_sec=0, tv_nsec=0}, NULL) = %s\n",
		       sprintrc(rc));

		const kernel_ulong_t ill = f8ill_ptr_to_kulong(p);
		rc = syscall(__NR_nanosleep, ill, NULL);
		printf("nanosleep(%#llx, NULL) = %s\n",
		       (unsigned long long) ill, sprintrc(rc));

		puts("+++ exited with 0 +++");
		return 0;
	} else {
		return 77;
	}
}
