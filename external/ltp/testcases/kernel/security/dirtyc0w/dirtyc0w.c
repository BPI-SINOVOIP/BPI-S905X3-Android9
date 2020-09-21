/*
 * Copyright (c) 2016 Cyril Hrubis <chrubis@suse.cz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is a regression test for write race that allows unprivileged programs
 * to change readonly files on the system.
 *
 * It has been fixed long time ago:
 *
 *   commit 4ceb5db9757aaeadcf8fbbf97d76bd42aa4df0d6
 *   Author: Linus Torvalds <torvalds@g5.osdl.org>
 *   Date:   Mon Aug 1 11:14:49 2005 -0700
 *
 *   Fix get_user_pages() race for write access
 *
 * Then it reappeared and was fixed again in:
 *
 *   commit 19be0eaffa3ac7d8eb6784ad9bdbc7d67ed8e619
 *   Author: Linus Torvalds <torvalds@linux-foundation.org>
 *   Date:   Thu Oct 13 20:07:36 2016 GMT
 *
 *   mm: remove gup_flags FOLL_WRITE games from __get_user_pages()
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>

#include "tst_test.h"

#define FNAME "test"
#define STR   "this is not a test\n"

static uid_t nobody_uid;
static gid_t nobody_gid;

static void setup(void)
{
	struct passwd *pw;

	pw = SAFE_GETPWNAM("nobody");

	nobody_uid = pw->pw_uid;
	nobody_gid = pw->pw_gid;
}

void dirtyc0w_test(void)
{
	int i, fd, pid, fail = 0;
	char c;

	/* Create file */
	fd = SAFE_OPEN(FNAME, O_WRONLY|O_CREAT|O_EXCL, 0444);
	SAFE_WRITE(1, fd, STR, sizeof(STR)-1);
	SAFE_CLOSE(fd);

	pid = SAFE_FORK();

	if (!pid) {
		SAFE_SETGID(nobody_gid);
		SAFE_SETUID(nobody_uid);
		SAFE_EXECLP("dirtyc0w_child", "dirtyc0w_child", NULL);
	}

	TST_CHECKPOINT_WAIT(0);
	for (i = 0; i < 100; i++)  {
		usleep(10000);

		SAFE_FILE_SCANF(FNAME, "%c", &c);

		if (c != 't') {
			fail = 1;
			break;
		}
	}

	SAFE_KILL(pid, SIGUSR1);
	tst_reap_children();
	SAFE_UNLINK(FNAME);

	if (fail)
		tst_res(TFAIL, "Bug reproduced!");
	else
		tst_res(TPASS, "Bug not reproduced");
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.needs_checkpoints = 1,
	.forks_child = 1,
	.needs_root = 1,
	.setup = setup,
	.test_all = dirtyc0w_test,
};
