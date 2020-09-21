/*
 * Copyright (c) International Business Machines  Corp., 2004
 * Copyright (c) Linux Test Project, 2004-2017
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
 */

/*
 * DESCRIPTION
 *	hugeshmctl02 - check for EACCES, EFAULT and EINVAL errors
 *
 * ALGORITHM
 *	create a large shared memory segment without read or write permissions
 *	create a large shared memory segment with read & write permissions
 *	loop if that option was specified
 *	  call shmctl() using five different invalid cases
 *	  check the errno value
 *	    issue a PASS message if we get EACCES, EFAULT or EINVAL
 *	  otherwise, the tests fails
 *	    issue a FAIL message
 *	call cleanup
 *
 * HISTORY
 *	03/2001 - Written by Wayne Boyer
 *	04/2004 - Updated by Robbie Williamson
 */

#include <pwd.h>
#include <limits.h>
#include "hugetlb.h"

static size_t shm_size;
static int shm_id_1 = -1;
static int shm_id_2 = -1;
static int shm_id_3 = -1;
static struct shmid_ds buf;

static long hugepages = 128;

static struct tst_option options[] = {
	{"s:", &nr_opt, "-s   num  Set the number of the been allocated hugepages"},
	{NULL, NULL, NULL}
};

struct tcase {
	int *shmid;
	int cmd;
	struct shmid_ds *sbuf;
	int error;
} tcases[] = {
	/* EFAULT - IPC_SET & buf isn't valid */
	{&shm_id_2, IPC_SET, (struct shmid_ds *)-1, EFAULT},
	/* EFAULT - IPC_STAT & buf isn't valid */
	{&shm_id_2, IPC_STAT, (struct shmid_ds *)-1, EFAULT},
	/* EINVAL - the shmid is not valid */
	{&shm_id_3, IPC_STAT, &buf, EINVAL},
	/* EINVAL - the command is not valid */
	{&shm_id_2, -1, &buf, EINVAL},
};

static void test_hugeshmctl(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(tcases); i++) {
		TEST(shmctl(*(tcases[i].shmid), tcases[i].cmd, tcases[i].sbuf));
		if (TEST_RETURN != -1) {
			tst_res(TFAIL, "shmctl succeeded "
					"unexpectedly");
			continue;
		}
		if (TEST_ERRNO == tcases[i].error) {
			tst_res(TPASS | TTERRNO, "shmctl failed "
					"as expected");
		} else {
			tst_res(TFAIL | TTERRNO, "shmctl failed "
					"unexpectedly - expect errno = "
					"%d, got", tcases[i].error);
		}
	}
}

static void setup(void)
{
	long hpage_size;

	save_nr_hugepages();
	if (nr_opt)
		hugepages = SAFE_STRTOL(nr_opt, 0, LONG_MAX);

	set_sys_tune("nr_hugepages", hugepages, 1);
	hpage_size = SAFE_READ_MEMINFO("Hugepagesize:") * 1024;

	shm_size = hpage_size * (hugepages / 2);
	update_shm_size(&shm_size);
	shmkey = getipckey();

	/* create a shared memory segment without read or write permissions */
	shm_id_1 = shmget(shmkey, shm_size, SHM_HUGETLB | IPC_CREAT | IPC_EXCL);
	if (shm_id_1 == -1)
		tst_brk(TBROK | TERRNO, "shmget #1");

	/* create a shared memory segment with read and write permissions */
	shm_id_2 = shmget(shmkey + 1, shm_size,
			  SHM_HUGETLB | IPC_CREAT | IPC_EXCL | SHM_RW);
	if (shm_id_2 == -1)
		tst_brk(TBROK | TERRNO, "shmget #2");
}

static void cleanup(void)
{
	rm_shm(shm_id_1);
	rm_shm(shm_id_2);
	restore_nr_hugepages();
}

static struct tst_test test = {
	.needs_root = 1,
	.needs_tmpdir = 1,
	.options = options,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = test_hugeshmctl,
};
