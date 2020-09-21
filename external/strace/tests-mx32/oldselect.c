/*
 * Copyright (c) 2015-2018 Dmitry V. Levin <ldv@altlinux.org>
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

#if defined __NR_select && defined __NR__newselect \
 && __NR_select != __NR__newselect \
 && !defined __sparc__

# define TEST_SYSCALL_NR __NR_select
# define TEST_SYSCALL_STR "select"
# define xselect xselect
# include "xselect.c"

static uint32_t *args;

static long
xselect(const kernel_ulong_t nfds,
	const kernel_ulong_t rs,
	const kernel_ulong_t ws,
	const kernel_ulong_t es,
	const kernel_ulong_t tv)
{
	if (!args)
		args = tail_alloc(sizeof(*args) * 5);
	args[0] = nfds;
	args[1] = rs;
	args[2] = ws;
	args[3] = es;
	args[4] = tv;
	long rc = syscall(TEST_SYSCALL_NR, args);
	errstr = sprintrc(rc);
	return rc;
}

#else

SKIP_MAIN_UNDEFINED("__NR_select && __NR__newselect"
		    " && __NR_select != __NR__newselect"
		    " && !defined __sparc__")

#endif
