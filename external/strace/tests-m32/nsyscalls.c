/*
 * Check decoding of out-of-range syscalls.
 *
 * Copyright (c) 2015-2016 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2016-2018 The strace developers.
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
#include "sysent.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <asm/unistd.h>

#include "sysent_shorthand_defs.h"

static const struct_sysent syscallent[] = {
#include "syscallent.h"
};

#include "sysent_shorthand_undefs.h"

#ifndef DEBUG_PRINT
# define DEBUG_PRINT 0
#endif

#if defined __X32_SYSCALL_BIT && defined __NR_read \
 && (__X32_SYSCALL_BIT & __NR_read) != 0
# define SYSCALL_BIT __X32_SYSCALL_BIT
#else
# define SYSCALL_BIT 0
#endif

#if DEBUG_PRINT
static const char *strace_name;
static FILE *debug_out;
#endif

static void
test_syscall(const unsigned long nr)
{
	static const kernel_ulong_t a[] = {
		(kernel_ulong_t) 0xface0fedbadc0dedULL,
		(kernel_ulong_t) 0xface1fedbadc1dedULL,
		(kernel_ulong_t) 0xface2fedbadc2dedULL,
		(kernel_ulong_t) 0xface3fedbadc3dedULL,
		(kernel_ulong_t) 0xface4fedbadc4dedULL,
		(kernel_ulong_t) 0xface5fedbadc5dedULL
	};

	long rc = syscall(nr | SYSCALL_BIT,
			  a[0], a[1], a[2], a[3], a[4], a[5]);

#if DEBUG_PRINT
	fprintf(debug_out, "%s: pid %d invalid syscall %#lx\n",
		strace_name, getpid(), nr | SYSCALL_BIT);
#endif

#ifdef LINUX_MIPSO32
	printf("syscall(%#lx, %#lx, %#lx, %#lx, %#lx, %#lx, %#lx)"
	       " = %ld ENOSYS (%m)\n", nr | SYSCALL_BIT,
	       a[0], a[1], a[2], a[3], a[4], a[5], rc);
#else
	printf("syscall_%#lx(%#llx, %#llx, %#llx, %#llx, %#llx, %#llx)"
	       " = %ld (errno %d)\n", nr | SYSCALL_BIT,
	       (unsigned long long) a[0],
	       (unsigned long long) a[1],
	       (unsigned long long) a[2],
	       (unsigned long long) a[3],
	       (unsigned long long) a[4],
	       (unsigned long long) a[5],
	       rc, errno);
#endif
}

int
main(int argc, char *argv[])
{
#if DEBUG_PRINT
	if (argc < 3)
		error_msg_and_fail("Not enough arguments. "
				   "Usage: %s STRACE_NAME DEBUG_OUT_FD",
				   argv[0]);

	strace_name = argv[1];

	errno = 0;
	int debug_out_fd = strtol(argv[2], NULL, 0);
	if (errno)
		error_msg_and_fail("Not a number: %s", argv[2]);

	debug_out = fdopen(debug_out_fd, "a");
	if (!debug_out)
		perror_msg_and_fail("fdopen: %d", debug_out_fd);
#endif

	test_syscall(ARRAY_SIZE(syscallent));

#ifdef SYS_socket_subcall
	test_syscall(SYS_socket_subcall + 1);
#endif

#ifdef SYS_ipc_subcall
	test_syscall(SYS_ipc_subcall + 1);
#endif

	puts("+++ exited with 0 +++");
	return 0;
}
