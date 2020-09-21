/*
 * Copyright (C) 2002 Andi Kleen
 * Copyright (C) 2017 Cyril Hrubis <chrubis@suse.cz>
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
 * along with this program;  if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * DESCRIPTION
 *	Check if gettimeofday is monotonous
 *
 * ALGORITHM
 *	Call gettimeofday() to get a t1 (fist value)
 *	call it again to get t2, see if t2 < t1, set t2 = t1, repeat for 10 sec
 */

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "tst_test.h"

#define gettimeofday(a,b)  syscall(__NR_gettimeofday,a,b)

static volatile sig_atomic_t done;
static char *str_rtime;
static int rtime = 10;

static struct tst_option options[] = {
	{"T:", &str_rtime, "-T len   Test iteration runtime in seconds"},
	{NULL, NULL, NULL},
};

static void breakout(int sig)
{
	done = sig;
}

static void verify_gettimeofday(void)
{
	struct timeval tv1, tv2;
	unsigned long long cnt = 0;

	done = 0;

	alarm(rtime);

	if (gettimeofday(&tv1, NULL)) {
		tst_res(TBROK | TERRNO, "gettimeofday() failed");
		return;
	}

	while (!done) {
		if (gettimeofday(&tv2, NULL)) {
			tst_res(TBROK | TERRNO, "gettimeofday() failed");
			return;
		}

		if (tv2.tv_sec < tv1.tv_sec ||
		    (tv2.tv_sec == tv1.tv_sec && tv2.tv_usec < tv1.tv_usec)) {
			tst_res(TFAIL,
				"Time is going backwards: old %jd.%jd vs new %jd.%jd!",
				(intmax_t) tv1.tv_sec, (intmax_t) tv1.tv_usec,
				(intmax_t) tv2.tv_sec, (intmax_t) tv2.tv_usec);
			return;
		}

		tv1 = tv2;
		cnt++;
	}


	tst_res(TINFO, "gettimeofday() called %llu times", cnt);
	tst_res(TPASS, "gettimeofday() monotonous in %i seconds", rtime);
}

static void setup(void)
{
	if (str_rtime) {
		rtime = atoi(str_rtime);
		if (rtime <= 0)
			tst_brk(TBROK, "Invalid runtime '%s'", str_rtime);
		tst_set_timeout(rtime + 60);
	}

	SAFE_SIGNAL(SIGALRM, breakout);
}

static struct tst_test test = {
	.setup = setup,
	.options = options,
	.test_all = verify_gettimeofday,
};
