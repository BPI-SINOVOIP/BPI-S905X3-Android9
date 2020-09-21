/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/cdefs.h>
#include <sys/mman.h>
#ifdef __BIONIC__
#include <cutils/ashmem.h>
#else
#include <sys/shm.h>
#endif
#include <errno.h>
#include <syslog.h>
#include <string.h>

#include "cras_shm.h"

#ifdef __BIONIC__

int cras_shm_open_rw (const char *name, size_t size)
{
	int fd;

	/* Eliminate the / in the shm_name. */
	if (name[0] == '/')
		name++;
	fd = ashmem_create_region(name, size);
	if (fd < 0) {
		fd = -errno;
		syslog(LOG_ERR, "failed to ashmem_create_region %s: %s\n",
		       name, strerror(-fd));
	}
	return fd;
}

int cras_shm_reopen_ro (const char *name, int fd)
{
	/* After mmaping the ashmem read/write, change it's protection
	   bits to disallow further write access. */
	if (ashmem_set_prot_region(fd, PROT_READ) != 0) {
		fd = -errno;
		syslog(LOG_ERR,
		       "failed to ashmem_set_prot_region %s: %s\n",
		       name, strerror(-fd));
	}
	return fd;
}

void cras_shm_close_unlink (const char *name, int fd)
{
	close(fd);
}

#else

int cras_shm_open_rw (const char *name, size_t size)
{
	int fd;
	int rc;

	fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (fd < 0) {
		fd = -errno;
		syslog(LOG_ERR, "failed to shm_open %s: %s\n",
		       name, strerror(-fd));
		return fd;
	}
	rc = ftruncate(fd, size);
	if (rc) {
		rc = -errno;
		syslog(LOG_ERR, "failed to set size of shm %s: %s\n",
		       name, strerror(-rc));
		return rc;
	}
	return fd;
}

int cras_shm_reopen_ro (const char *name, int fd)
{
	/* Open a read-only copy to dup and pass to clients. */
	fd = shm_open(name, O_RDONLY, 0);
	if (fd < 0) {
		fd = -errno;
		syslog(LOG_ERR,
		       "Failed to re-open shared memory '%s' read-only: %s",
		       name, strerror(-fd));
	}
	return fd;
}

void cras_shm_close_unlink (const char *name, int fd)
{
	shm_unlink(name);
	close(fd);
}

#endif
