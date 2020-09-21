/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

const char *cras_config_get_system_socket_file_dir()
{
	/* This directory is created by the upstart script, eventually it would
	 * be nice to make this more dynamic, but it isn't needed right now for
	 * Chrome OS. */
	return CRAS_SOCKET_FILE_DIR;
}
