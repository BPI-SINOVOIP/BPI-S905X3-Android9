/*
 * Copyright (c) 2016 Cyril Hrubis <chrubis@suse.cz>
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void print_help(void)
{
	printf("Usage: tst_usleep interval[s|ms|us]\n\n");
	printf("       If no unit is specified the interval is in seconds\n");
}

static struct unit {
	const char *unit;
	long mul;
} units[] = {
	{"",   1000000},
	{"s",  1000000},
	{"ms", 1000},
	{"us", 1},
};

static unsigned int units_len = sizeof(units) / sizeof(*units);

int main(int argc, char *argv[])
{
	int opt;
	long interval, secs = 0, usecs = 0;
	unsigned int i;
	char *end;

	while ((opt = getopt(argc, argv, ":h")) != -1) {
		switch (opt) {
		case 'h':
			print_help();
			return 0;
		default:
			print_help();
			return 1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "ERROR: Expected interval argument\n\n");
		print_help();
		return 1;
	}

	interval = strtol(argv[optind], &end, 10);

	if (argv[optind] == end) {
		fprintf(stderr, "ERROR: Invalid interval '%s'\n\n",
		        argv[optind]);
		print_help();
		return 1;
	}

	for (i = 0; i < units_len; i++) {
		if (!strcmp(units[i].unit, end))
			break;
	}

	if (i >= units_len) {
		fprintf(stderr, "ERROR: Invalid interval unit '%s'\n\n", end);
		print_help();
		return 1;
	}

	if (units[i].mul == 1000000)
		secs = interval;

	if (units[i].mul == 1000) {
		secs = interval / 1000;
		usecs = (interval % 1000) * 1000;
	}

	if (units[i].mul == 1) {
		secs = interval / 1000000;
		usecs = interval % 1000000;
	}

	if (secs)
		sleep(secs);

	if (usecs)
		usleep(usecs);

	return 0;
}
