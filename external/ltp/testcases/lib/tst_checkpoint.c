/*
 * Copyright (c) 2016 Linux Test Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

static void print_help(void)
{
	printf("Usage: tst_checkpoint wait TIMEOUT ID\n");
	printf("   or: tst_checkpoint wake TIMEOUT ID NR_WAKE\n");
	printf("       TIMEOUT - timeout in ms\n");
	printf("       ID - checkpoint id\n");
	printf("       NR_WAKE - number of processes to wake up\n");
}

static int get_val(const char *name, const char *arg, unsigned int *val)
{
	unsigned long temp;
	char *e;

	errno = 0;
	temp = strtoul(arg, &e, 10);
	if (errno || (e == arg) || (temp > UINT_MAX)) {
		fprintf(stderr, "ERROR: Invalid %s '%s'\n",
			name, arg);
		return -1;
	}

	*val = temp;

	return 0;
}

int main(int argc, char *argv[])
{
	unsigned int id, timeout, nr_wake;
	int ret;
	int type;

	if (argc < 3)
		goto help;

	if (!strcmp(argv[1], "wait")) {
		type = 0;

		if (argc != 4)
			goto help;
	} else if (!strcmp(argv[1], "wake")) {
		type = 1;

		if (argc != 5)
			goto help;

		if (get_val("NR_WAKE", argv[4], &nr_wake))
			goto help;
	} else {
		fprintf(stderr, "ERROR: Invalid COMMAND '%s'\n",
			argv[1]);
		goto help;
	}

	if (get_val("TIMEOUT", argv[2], &timeout)
	    || get_val("ID", argv[3], &id)) {
		goto help;
	}

	tst_reinit();

	if (type)
		ret = tst_checkpoint_wake(id, nr_wake, timeout);
	else
		ret = tst_checkpoint_wait(id, timeout);

	if (ret)
		return 1;
	else
		return 0;

help:
	print_help();
	return 1;
}
