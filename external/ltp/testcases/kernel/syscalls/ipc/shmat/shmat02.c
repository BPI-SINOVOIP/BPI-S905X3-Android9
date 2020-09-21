/*
 * Copyright (c) International Business Machines  Corp., 2001
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 */

/*
 * DESCRIPTION
 *
 * 1) shmat() fails and set errno to EINVAL when shmid is invalid.
 * 2) shmat() fails and set errno to EINVAL when shmaddr is not page
 *    aligned and SHM_RND is not given
 * 3) shmat() fails and set errno to EACCES when the shm resource has
 *    no read/write permission.
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pwd.h>

#include "tst_test.h"
#include "tst_safe_sysv_ipc.h"
#include "libnewipc.h"

static int shm_id1 = -1;
static int shm_id2 = -1;
static void *aligned_addr;
static void *unaligned_addr;
static key_t shm_key1;
static struct passwd *pw;

static struct test_case_t {
	int *shmid;
	void **shmaddr;
	int exp_err;
	int exp_user;
} tcases[] = {
	{&shm_id1, &aligned_addr, EINVAL, 0},
	{&shm_id2, &unaligned_addr, EINVAL, 0},
	{&shm_id2, &aligned_addr, EACCES, 1},
};

static void verify_shmat(struct test_case_t *tc)
{
	void *addr;

	addr = shmat(*tc->shmid, *tc->shmaddr, 0);
	if (addr != (void *)-1) {
		tst_res(TFAIL, "shmat() succeeded unexpectedly");
		return;
	}

	if (errno == tc->exp_err) {
		tst_res(TPASS | TERRNO, "shmat() failed as expected");
	} else {
		tst_res(TFAIL | TERRNO, "shmat() failed unexpectedly,"
			 "expected: %s", tst_strerrno(tc->exp_err));
	}
}

static void do_shmat(unsigned int n)
{
	pid_t pid;

	struct test_case_t *tc = &tcases[n];

	if (!tc->exp_user) {
		verify_shmat(tc);
	} else {
		pid = SAFE_FORK();
		if (pid) {
			tst_reap_children();
		} else {
			SAFE_SETUID(pw->pw_uid);
			verify_shmat(tc);
			exit(0);
		}
	}
}

static void setup(void)
{
	aligned_addr = PROBE_FREE_ADDR();
	unaligned_addr = aligned_addr + SHMLBA - 1;

	shm_key1 = GETIPCKEY();

	shm_id2 = SAFE_SHMGET(shm_key1, INT_SIZE, SHM_RW | IPC_CREAT | IPC_EXCL);

	pw = SAFE_GETPWNAM("nobody");
}

static void cleanup(void)
{
	if (shm_id2 != -1)
		SAFE_SHMCTL(shm_id2, IPC_RMID, NULL);
}

static struct tst_test test = {
	.needs_root = 1,
	.forks_child = 1,
	.test = do_shmat,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup
};
