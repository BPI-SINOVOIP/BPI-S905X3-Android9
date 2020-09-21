/*
 * Validate syscallent.h file.
 *
 * Copyright (c) 2015-2016 Dmitry V. Levin <ldv@altlinux.org>
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

#include "tests.h"
#include "sysent.h"
#include <stdio.h>
#include <string.h>
#include <asm/unistd.h>

#include "sysent_shorthand_defs.h"

static const struct_sysent syscallent[] = {
#include "syscallent.h"
};

#include "sysent_shorthand_undefs.h"

typedef const char *pstr_t;
static const pstr_t ksyslist[] = {
#include "ksysent.h"
};

int
main(void)
{
	int rc = 0;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ksyslist); ++i) {
		if (!ksyslist[i])
			continue;
		if (i >= ARRAY_SIZE(syscallent) || !syscallent[i].sys_name) {
			fprintf(stderr, "warning: \"%s\" syscall #%u"
				" is missing in syscallent.h\n",
				ksyslist[i], i);
			continue;
		}
#ifdef SYS_socket_nsubcalls
		if (i >= SYS_socket_subcall &&
		    i < SYS_socket_subcall + SYS_socket_nsubcalls) {
			fprintf(stderr, "error: \"%s\" syscall #%u"
				" is a socket subcall in syscallent.h\n",
				ksyslist[i], i);
			rc = 1;
			continue;
		}
#endif
#ifdef SYS_ipc_nsubcalls
		if (i >= SYS_ipc_subcall &&
		    i < SYS_ipc_subcall + SYS_ipc_nsubcalls) {
			fprintf(stderr, "error: \"%s\" syscall #%u"
				" is an ipc subcall in syscallent.h\n",
				ksyslist[i], i);
			rc = 1;
			continue;
		}
#endif
		if (strcmp(ksyslist[i], syscallent[i].sys_name)) {
			fprintf(stderr, "error: \"%s\" syscall #%u"
				" is \"%s\" in syscallent.h\n",
				ksyslist[i], i, syscallent[i].sys_name);
			rc = 1;
			continue;
		}
	}

	for (i = 0; i < ARRAY_SIZE(syscallent); ++i) {
		if (!syscallent[i].sys_name
#ifdef SYS_socket_nsubcalls
		    || (i >= SYS_socket_subcall &&
			i < SYS_socket_subcall + SYS_socket_nsubcalls)
#endif
#ifdef SYS_ipc_nsubcalls
		    || (i >= SYS_ipc_subcall &&
			i < SYS_ipc_subcall + SYS_ipc_nsubcalls)
#endif
#ifdef ARM_FIRST_SHUFFLED_SYSCALL
		    || (i >= ARM_FIRST_SHUFFLED_SYSCALL &&
			i <= ARM_FIRST_SHUFFLED_SYSCALL +
			    ARM_LAST_SPECIAL_SYSCALL + 1)
#endif
		   )
			continue;
		if (i >= ARRAY_SIZE(ksyslist) || !ksyslist[i]) {
			fprintf(stderr, "note: unknown syscall #%u"
				" is \"%s\" in syscallent.h\n",
				i, syscallent[i].sys_name);
		}
	}

	return rc;
}
