/*
 * Copyright (c) 2013-2015 Dmitry V. Levin <ldv@altlinux.org>
 * Copyright (c) 2013-2018 The strace developers.
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
#include <unistd.h>
#ifdef HAVE_PRCTL
# include <sys/prctl.h>
#endif

int main(int argc, char **argv)
{
	if (argc < 2)
		return 99;
#ifdef HAVE_PRCTL
	/* Turn off restrictions on tracing if applicable.  If the command
	 * aren't available on this system, that's OK too.  */
# ifndef PR_SET_PTRACER
#  define PR_SET_PTRACER 0x59616d61
# endif
# ifndef PR_SET_PTRACER_ANY
#  define PR_SET_PTRACER_ANY -1UL
# endif
	(void) prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
#endif
	if (write(1, "\n", 1) != 1) {
		perror("write");
		return 99;
	}
	(void) execvp(argv[1], argv + 1);
	perror(argv[1]);
	return 99;
}
