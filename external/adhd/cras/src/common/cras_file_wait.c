/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/inotify.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cras_file_wait.h"

#define CRAS_FILE_WAIT_EVENT_MIN_SIZE sizeof(struct inotify_event)
#define CRAS_FILE_WAIT_EVENT_SIZE (CRAS_FILE_WAIT_EVENT_MIN_SIZE + NAME_MAX + 1)
#define CRAS_FILE_WAIT_FLAG_MOCK_RACE (1 << 31)

struct cras_file_wait {
	cras_file_wait_callback_t callback;
	void *callback_context;
	const char *file_path;
	size_t file_path_len;
	char *watch_path;
	char *watch_dir;
	char *watch_file_name;
	size_t watch_file_name_len;
	int inotify_fd;
	int watch_id;
	char event_buf[CRAS_FILE_WAIT_EVENT_SIZE];
	cras_file_wait_flag_t flags;
};

int cras_file_wait_get_fd(struct cras_file_wait *file_wait)
{
	if (!file_wait)
		return -EINVAL;
	if (file_wait->inotify_fd < 0)
		return -EINVAL;
	return file_wait->inotify_fd;
}

/* Defined for the unittest. */
void cras_file_wait_mock_race_condition(struct cras_file_wait *file_wait);
void cras_file_wait_mock_race_condition(struct cras_file_wait *file_wait)
{
	if (file_wait)
		file_wait->flags |= CRAS_FILE_WAIT_FLAG_MOCK_RACE;
}

void cras_file_wait_destroy(struct cras_file_wait *file_wait)
{
	if (!file_wait)
		return;
	if (file_wait->inotify_fd >= 0)
		close(file_wait->inotify_fd);
	free(file_wait);
}

static int cras_file_wait_rm_watch(struct cras_file_wait *file_wait)
{
	int rc;

	file_wait->watch_path[0] = 0;
	file_wait->watch_dir[0] = 0;
	file_wait->watch_file_name[0] = 0;
	file_wait->watch_file_name_len = 0;
	if (file_wait->inotify_fd >= 0 && file_wait->watch_id >= 0) {
		rc = inotify_rm_watch(file_wait->inotify_fd,
				      file_wait->watch_id);
		file_wait->watch_id = -1;
		if (rc < 0)
			return -errno;
	}
	return 0;
}

int cras_file_wait_process_event(struct cras_file_wait *file_wait,
				 struct inotify_event *event)
{
	cras_file_wait_event_t file_wait_event;

	syslog(LOG_DEBUG, "file_wait->watch_id: %d, event->wd: %d"
	       ", event->mask: %x, event->name: %s",
	       file_wait->watch_id, event->wd, event->mask,
	       event->len ? event->name : "");

	if (event->wd != file_wait->watch_id)
		return 0;

	if (event->mask & IN_IGNORED) {
		/* The watch has been removed. */
		file_wait->watch_id = -1;
		return cras_file_wait_rm_watch(file_wait);
	}

	if (event->len == 0 ||
	    memcmp(event->name, file_wait->watch_file_name,
		   file_wait->watch_file_name_len + 1) != 0) {
		/* Some file we don't care about. */
		return 0;
	}

	if ((event->mask & (IN_CREATE|IN_MOVED_TO)) != 0)
		file_wait_event = CRAS_FILE_WAIT_EVENT_CREATED;
	else if ((event->mask & (IN_DELETE|IN_MOVED_FROM)) != 0)
		file_wait_event = CRAS_FILE_WAIT_EVENT_DELETED;
	else
		return 0;

	/* Found the file! */
	if (strcmp(file_wait->watch_path, file_wait->file_path) == 0) {
		/* Tell the caller about this creation or deletion. */
		file_wait->callback(file_wait->callback_context,
				    file_wait_event, event->name);
	} else {
		/* Remove the watch for this file, move on. */
		return cras_file_wait_rm_watch(file_wait);
	}
	return 0;
}

int cras_file_wait_dispatch(struct cras_file_wait *file_wait)
{
	struct inotify_event *event;
	char *watch_dir_end;
	size_t watch_dir_len;
	char *watch_file_start;
	size_t watch_path_len;
	int rc = 0;
	int flags;
	ssize_t read_rc;
	ssize_t read_offset;

	if (!file_wait)
		return -EINVAL;

	/* If we have a file-descriptor, then read it and see what's up. */
	if (file_wait->inotify_fd >= 0) {
		read_offset = 0;
		read_rc = read(file_wait->inotify_fd, file_wait->event_buf,
			       CRAS_FILE_WAIT_EVENT_SIZE);
		if (read_rc < 0) {
			rc = -errno;
			if ((rc == -EAGAIN || rc == -EWOULDBLOCK)
			    && file_wait->watch_id < 0) {
				/* Really nothing to read yet: we need to
				 * setup a watch. */
				rc = 0;
			}
		} else if (read_rc < CRAS_FILE_WAIT_EVENT_MIN_SIZE) {
			rc = -EIO;
		} else if (file_wait->watch_id < 0) {
			/* Processing messages related to old watches. */
			rc = 0;
		} else while (rc == 0 && read_offset < read_rc) {
			event = (struct inotify_event *)
				(file_wait->event_buf + read_offset);
			read_offset += sizeof(*event) + event->len;
			rc = cras_file_wait_process_event(file_wait, event);
		}
	}

	/* Report errors from above here. */
	if (rc != 0)
		return rc;

	if (file_wait->watch_id >= 0) {
		/* Assume that the watch that we have is the right one. */
		return 0;
	}

	/* Initialize inotify if we haven't already. */
	if (file_wait->inotify_fd < 0) {
		file_wait->inotify_fd = inotify_init1(IN_NONBLOCK|IN_CLOEXEC);
		if (file_wait->inotify_fd < 0)
			return -errno;
	}

	/* Figure out what we need to watch next. */
	rc = -ENOENT;
	strcpy(file_wait->watch_dir, file_wait->file_path);
	watch_dir_len = file_wait->file_path_len;

	while (rc == -ENOENT) {
		strcpy(file_wait->watch_path, file_wait->watch_dir);
		watch_path_len = watch_dir_len;

		/* Find the end of the parent directory. */
		watch_dir_end = file_wait->watch_dir + watch_dir_len - 1;
		while (watch_dir_end > file_wait->watch_dir &&
		       *watch_dir_end != '/')
			watch_dir_end--;
		watch_file_start = watch_dir_end + 1;
		/* Treat consecutive '/' characters as one. */
		while (watch_dir_end > file_wait->watch_dir &&
		       *(watch_dir_end - 1) == '/')
		       watch_dir_end--;
		watch_dir_len = watch_dir_end - file_wait->watch_dir;

		if (watch_dir_len == 0) {
			/* We're looking for a file in the current directory. */
			strcpy(file_wait->watch_file_name,
			       file_wait->watch_path);
			file_wait->watch_file_name_len = watch_path_len;
			strcpy(file_wait->watch_dir, ".");
			watch_dir_len = 1;
		} else {
			/* Copy out the file name that we're looking for, and
			 * mark the end of the directory path. */
			strcpy(file_wait->watch_file_name, watch_file_start);
			file_wait->watch_file_name_len =
				watch_path_len -
				(watch_file_start - file_wait->watch_dir);
			*watch_dir_end = 0;
		}

		if (file_wait->flags & CRAS_FILE_WAIT_FLAG_MOCK_RACE) {
			/* For testing only. */
			mknod(file_wait->watch_path, S_IFREG | 0600, 0);
			file_wait->flags &= ~CRAS_FILE_WAIT_FLAG_MOCK_RACE;
		}

		flags = IN_CREATE|IN_MOVED_TO|IN_DELETE|IN_MOVED_FROM;
		file_wait->watch_id =
			inotify_add_watch(file_wait->inotify_fd,
					  file_wait->watch_dir, flags);
		if (file_wait->watch_id < 0) {
			rc = -errno;
			continue;
		}

		/* Satisfy the race condition between existence of the
		 * file and creation of the watch. */
		rc = access(file_wait->watch_path, F_OK);
		if (rc < 0) {
			rc = -errno;
			if (rc == -ENOENT) {
				/* As expected, the file still doesn't exist. */
				rc = 0;
			}
			continue;
		}

		/* The file we're looking for exists. */
		if (strcmp(file_wait->watch_path, file_wait->file_path) == 0) {
			file_wait->callback(file_wait->callback_context,
					    CRAS_FILE_WAIT_EVENT_CREATED,
					    file_wait->watch_file_name);
			return 0;
		}

		/* Start over again. */
		rc = cras_file_wait_rm_watch(file_wait);
		if (rc != 0)
			return rc;
		rc = -ENOENT;
		strcpy(file_wait->watch_dir, file_wait->file_path);
		watch_dir_len = file_wait->file_path_len;
	}

	/* Get out for permissions problems for example. */
	return rc;
}

int cras_file_wait_create(const char *file_path,
			  cras_file_wait_flag_t flags,
			  cras_file_wait_callback_t callback,
			  void *callback_context,
			  struct cras_file_wait **file_wait_out)
{
	struct cras_file_wait *file_wait;
	size_t file_path_len;
	int rc;

	if (!file_path || !*file_path || !callback || !file_wait_out)
		return -EINVAL;
	*file_wait_out = NULL;

	/* Create a struct cras_file_wait to track waiting for this file. */
	file_path_len = strlen(file_path);
	file_wait = (struct cras_file_wait *)
		    calloc(1, sizeof(*file_wait) + ((file_path_len + 1) * 5));
	if (!file_wait)
		return -ENOMEM;
	file_wait->callback = callback;
	file_wait->callback_context = callback_context;
	file_wait->inotify_fd = -1;
	file_wait->watch_id = -1;
	file_wait->file_path_len = file_path_len;
	file_wait->flags = flags;

	/* We've allocated memory such that the file_path, watch_path,
	 * watch_dir, and watch_file_name data are appended to the end of
	 * our cras_file_wait structure. */
	file_wait->file_path = (const char *)file_wait + sizeof(*file_wait);
	file_wait->watch_path = (char *)file_wait->file_path +
				file_path_len + 1;
	file_wait->watch_dir = file_wait->watch_path + file_path_len + 1;
	file_wait->watch_file_name = file_wait->watch_dir + file_path_len + 1;
	memcpy((void *)file_wait->file_path, file_path, file_path_len + 1);

	/* Setup the first watch. If that fails unexpectedly, then we destroy
	 * the file wait structure immediately. */
	rc = cras_file_wait_dispatch(file_wait);
	if (rc != 0)
		cras_file_wait_destroy(file_wait);
	else
		*file_wait_out = file_wait;
	return rc;
}
