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
 * Test Name: hugemmap04
 *
 * Test Description:
 *  Verify that, a hugetlb mmap() succeeds when used to map the largest
 *  size possible.
 *
 * Expected Result:
 *  mmap() should succeed returning the address of the hugetlb mapped region.
 *  The number of free huge pages should decrease.
 *
 * Test:
 *  Loop if the proper options are given.
 *  Execute system call
 *  Check return code, if system call failed (return=-1)
 *  Log the errno and Issue a FAIL message.
 *
 * HISTORY
 *  04/2004 Written by Robbie Williamson
 */

#include <sys/mount.h>
#include <stdio.h>
#include <limits.h>
#include <sys/param.h>
#include "hugetlb.h"

static struct tst_option options[] = {
	{"H:", &Hopt,   "-H   /..  Location of hugetlbfs, i.e.  -H /var/hugetlbfs"},
	{"s:", &nr_opt, "-s   num  Set the number of the been allocated hugepages"},
	{NULL, NULL, NULL}
};

static char TEMPFILE[MAXPATHLEN];

static long *addr;
static long long mapsize;
static int fildes;
static long freepages;
static long beforetest;
static long aftertest;
static long hugepagesmapped;
static long hugepages = 128;

static void test_hugemmap(void)
{
	int huge_pagesize = 0;

	/* Creat a temporary file used for huge mapping */
	fildes = SAFE_OPEN(TEMPFILE, O_RDWR | O_CREAT, 0666);

	freepages = SAFE_READ_MEMINFO("HugePages_Free:");
	beforetest = freepages;

	huge_pagesize = SAFE_READ_MEMINFO("Hugepagesize:");
	tst_res(TINFO, "Size of huge pages is %d KB", huge_pagesize);

#if __WORDSIZE == 32
	tst_res(TINFO, "Total amount of free huge pages is %d",
			freepages);
	tst_res(TINFO, "Max number allowed for 1 mmap file in"
			" 32-bits is 128");
	if (freepages > 128)
		freepages = 128;
#endif
	mapsize = (long long)freepages * huge_pagesize * 1024;
	addr = mmap(NULL, mapsize, PROT_READ | PROT_WRITE,
			MAP_SHARED, fildes, 0);
	if (addr == MAP_FAILED) {
		tst_res(TFAIL | TERRNO, "mmap() Failed on %s",
				TEMPFILE);
	} else {
		tst_res(TPASS,
				"Succeeded mapping file using %ld pages",
				freepages);

		/* force to allocate page and change HugePages_Free */
		*(int *)addr = 0;
		/* Make sure the number of free huge pages AFTER testing decreased */
		aftertest = SAFE_READ_MEMINFO("HugePages_Free:");
		hugepagesmapped = beforetest - aftertest;
		if (hugepagesmapped < 1)
			tst_res(TWARN, "Number of HUGEPAGES_FREE stayed the"
					" same. Okay if multiple copies running due"
					" to test collision.");
	}

	munmap(addr, mapsize);
	close(fildes);
}

void setup(void)
{
	save_nr_hugepages();

	if (!Hopt)
		Hopt = tst_get_tmpdir();
	SAFE_MOUNT("none", Hopt, "hugetlbfs", 0, NULL);

	if (nr_opt)
		hugepages = SAFE_STRTOL(nr_opt, 0, LONG_MAX);
	set_sys_tune("nr_hugepages", hugepages, 1);

	snprintf(TEMPFILE, sizeof(TEMPFILE), "%s/mmapfile%d", Hopt, getpid());
}

void cleanup(void)
{
	unlink(TEMPFILE);
	restore_nr_hugepages();

	umount(Hopt);
}

static struct tst_test test = {
	.needs_root = 1,
	.needs_tmpdir = 1,
	.options = options,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = test_hugemmap,
};
