/*
 * Copyright (c) 2013-2016 Cyril Hrubis <chrubis@suse.cz>
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

#include "test.h"
#include "ltp_priv.h"
#include "tst_mkfs.h"
#include "tst_device.h"

#define OPTS_MAX 32

void tst_mkfs_(const char *file, const int lineno, void (cleanup_fn)(void),
	       const char *dev, const char *fs_type,
	       const char *const fs_opts[], const char *extra_opt)
{
	int i, pos = 1, ret;
	char mkfs[64];
	const char *argv[OPTS_MAX] = {mkfs};
	char fs_opts_str[1024] = "";

	if (!dev) {
		tst_brkm(TBROK, cleanup_fn,
			 "%s:%d: No device specified", file, lineno);
		return;
	}

	if (!fs_type) {
		tst_brkm(TBROK, cleanup_fn,
			 "%s:%d: No fs_type specified", file, lineno);
		return;
	}

	snprintf(mkfs, sizeof(mkfs), "mkfs.%s", fs_type);

	if (fs_opts) {
		for (i = 0; fs_opts[i]; i++) {
			argv[pos++] = fs_opts[i];

			if (pos + 2 > OPTS_MAX) {
				tst_brkm(TBROK, cleanup_fn,
				         "%s:%d: Too much mkfs options",
					 file, lineno);
				return;
			}

			if (i)
				strcat(fs_opts_str, " ");
			strcat(fs_opts_str, fs_opts[i]);
		}
	}

	argv[pos++] = dev;

	if (extra_opt) {
		argv[pos++] = extra_opt;

		if (pos + 1 > OPTS_MAX) {
			tst_brkm(TBROK, cleanup_fn,
			         "%s:%d: Too much mkfs options", file, lineno);
			return;
		}
	}

	argv[pos] = NULL;

	if (tst_clear_device(dev))
		tst_brkm(TBROK, cleanup_fn, "tst_clear_device() failed");

	tst_resm(TINFO, "Formatting %s with %s opts='%s' extra opts='%s'",
	         dev, fs_type, fs_opts_str, extra_opt ? extra_opt : "");
	ret = tst_run_cmd(cleanup_fn, argv, "/dev/null", NULL, 1);

	switch (ret) {
	case 0:
	break;
	case 255:
		tst_brkm(TCONF, cleanup_fn,
			 "%s:%d: %s not found in $PATH", file, lineno, mkfs);
	default:
		tst_brkm(TBROK, cleanup_fn,
			 "%s:%d: %s failed with %i", mkfs, ret, file, lineno);
	}
}

const char *tst_dev_fs_type(void)
{
	const char *fs_type;

	fs_type = getenv("LTP_DEV_FS_TYPE");

	if (fs_type)
		return fs_type;

	return DEFAULT_FS_TYPE;
}
