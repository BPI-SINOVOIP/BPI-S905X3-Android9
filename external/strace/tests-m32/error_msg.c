/*
 * Copyright (c) 2016 Dmitry V. Levin <ldv@altlinux.org>
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

#define perror_msg_and_fail perror_msg_and_fail
#define error_msg_and_fail error_msg_and_fail

#include "tests.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
perror_msg_and_fail(const char *fmt, ...)
{
	int err_no = errno;
	va_list p;

	va_start(p, fmt);
	vfprintf(stderr, fmt, p);
	if (err_no)
		fprintf(stderr, ": %s\n", strerror(err_no));
	else
		putc('\n', stderr);
	exit(1);
}

void
error_msg_and_fail(const char *fmt, ...)
{
	va_list p;

	va_start(p, fmt);
	vfprintf(stderr, fmt, p);
	putc('\n', stderr);
	exit(1);
}

void
error_msg_and_skip(const char *fmt, ...)
{
	va_list p;

	va_start(p, fmt);
	vfprintf(stderr, fmt, p);
	putc('\n', stderr);
	exit(77);
}

void
perror_msg_and_skip(const char *fmt, ...)
{
	int err_no = errno;
	va_list p;

	va_start(p, fmt);
	vfprintf(stderr, fmt, p);
	if (err_no)
		fprintf(stderr, ": %s\n", strerror(err_no));
	else
		putc('\n', stderr);
	exit(77);
}
