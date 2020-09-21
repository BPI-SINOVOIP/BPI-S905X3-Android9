/*
 * Check decoding of getpeername syscall.
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

#define TEST_SYSCALL_NAME getpeername
#include "sockname.c"

int
main(void)
{
	int lfd = socket(AF_UNIX, SOCK_STREAM, 0);
	int cfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (lfd < 0 || cfd < 0)
		perror_msg_and_skip("socket");

	(void) unlink(TEST_SOCKET);

	const struct sockaddr_un un = {
		.sun_family = AF_UNIX,
		.sun_path = TEST_SOCKET
	};

	if (bind(lfd, (const void *) &un, sizeof(un)))
		perror_msg_and_skip("bind");
	if (listen(lfd, 1))
		perror_msg_and_skip("listen");
	if (connect(cfd, (const void *) &un, sizeof(un)))
		perror_msg_and_skip("connect");
	if (accept(lfd, 0, 0) < 0)
		perror_msg_and_skip("accept");

	test_sockname_syscall(cfd);

	(void) unlink(TEST_SOCKET);

	puts("+++ exited with 0 +++");
	return 0;
}
