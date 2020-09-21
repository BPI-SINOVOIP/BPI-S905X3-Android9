/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE /* For ppoll() */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <poll.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "cras_util.h"

int cras_set_rt_scheduling(int rt_lim)
{
	struct rlimit rl;

	rl.rlim_cur = rl.rlim_max = rt_lim;

	if (setrlimit(RLIMIT_RTPRIO, &rl) < 0) {
		syslog(LOG_WARNING, "setrlimit %u failed: %d\n",
		       (unsigned) rt_lim, errno);
		return -EACCES;
	}
	return 0;
}

int cras_set_thread_priority(int priority)
{
	struct sched_param sched_param;
	int err;

	memset(&sched_param, 0, sizeof(sched_param));
	sched_param.sched_priority = priority;

	err = pthread_setschedparam(pthread_self(), SCHED_RR, &sched_param);
	if (err)
		syslog(LOG_WARNING,
		       "Failed to set thread sched params to priority %d"
		       ", rc: %d\n", priority, err);

	return err;
}

int cras_set_nice_level(int nice)
{
	int rc;

	/* Linux isn't posix compliant with setpriority(2), it will set a thread
	 * priority if it is passed a tid, not affecting the rest of the threads
	 * in the process.  Setting this priority will only succeed if the user
	 * has been granted permission to adjust nice values on the system.
	 */
	rc = setpriority(PRIO_PROCESS, syscall(__NR_gettid), nice);
	if (rc)
		syslog(LOG_WARNING, "Failed to set nice to %d, rc: %d",
		       nice, rc);

	return rc;
}

int cras_make_fd_nonblocking(int fd)
{
	int fl;

	fl = fcntl(fd, F_GETFL);
	if (fl < 0)
		return fl;
	if (fl & O_NONBLOCK)
		return 0;
	return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

int cras_make_fd_blocking(int fd)
{
	int fl;

	fl = fcntl(fd, F_GETFL);
	if (fl < 0)
		return fl;
	if ((~fl) & O_NONBLOCK)
		return 0;
	return fcntl(fd, F_SETFL, fl & ~O_NONBLOCK);
}

int cras_send_with_fds(int sockfd, const void *buf, size_t len, int *fd,
		       unsigned int num_fds)
{
	struct msghdr msg = {0};
	struct iovec iov;
	struct cmsghdr *cmsg;
	char *control;
	const unsigned int control_size = CMSG_SPACE(sizeof(*fd) * num_fds);
	int rc;

	control = calloc(control_size, 1);

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	iov.iov_base = (void *)buf;
	iov.iov_len = len;

	msg.msg_control = control;
	msg.msg_controllen = control_size;

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(*fd) * num_fds);
	memcpy(CMSG_DATA(cmsg), fd, sizeof(*fd) * num_fds);

	rc = sendmsg(sockfd, &msg, 0);
	free(control);
	return rc;
}

int cras_recv_with_fds(int sockfd, void *buf, size_t len, int *fd,
		       unsigned int *num_fds)
{
	struct msghdr msg = {0};
	struct iovec iov;
	struct cmsghdr *cmsg;
	char *control;
	const unsigned int control_size = CMSG_SPACE(sizeof(*fd) * *num_fds);
	int rc;
	int i;

	control = calloc(control_size, 1);

	for (i = 0; i < *num_fds; i++)
		fd[i] = -1;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	iov.iov_base = buf;
	iov.iov_len = len;
	msg.msg_control = control;
	msg.msg_controllen = control_size;

	rc = recvmsg(sockfd, &msg, 0);
	if (rc < 0) {
		rc = -errno;
		goto exit;
	}

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET
		    && cmsg->cmsg_type == SCM_RIGHTS) {
			size_t fd_size = cmsg->cmsg_len - sizeof(*cmsg);
			*num_fds = MIN(*num_fds, fd_size / sizeof(*fd));
			memcpy(fd, CMSG_DATA(cmsg), *num_fds * sizeof(*fd));
			break;
		}
	}

exit:
	free(control);
	return rc;
}

int cras_poll(struct pollfd *fds, nfds_t nfds, struct timespec *timeout,
	      const sigset_t *sigmask)
{
	struct timespec now;
	struct timespec future;
	struct pollfd *fd = fds;
	nfds_t i;
	int rc = 0;

	if (timeout) {
		/* Treat a negative timeout as valid (but timed-out) since
		 * this function could update timeout to have negative tv_sec
		 * or tv_nsec. */
		if (timeout->tv_sec < 0 || timeout->tv_nsec < 0)
			return -ETIMEDOUT;
		rc = clock_gettime(CLOCK_MONOTONIC_RAW, &future);
		if (rc < 0)
			return -errno;
		add_timespecs(&future, timeout);
	}

	for (i = 0; i < nfds; i++) {
		fd->revents = 0;
		fd++;
	}

	rc = ppoll(fds, nfds, timeout, sigmask);
	if (rc == 0 && timeout) {
		rc = -ETIMEDOUT;
	}
	else if (rc < 0) {
		rc = -errno;
	}

	if (timeout) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		subtract_timespecs(&future, &now, timeout);
	}

	return rc;
}

int wait_for_dev_input_access()
{
	/* Wait for /dev/input/event* files to become accessible by
	 * having group 'input'.  Setting these files to have 'rw'
	 * access to group 'input' is done through a udev rule
	 * installed by adhd into /lib/udev/rules.d.
	 *
	 * Wait for up to 2 seconds for the /dev/input/event* files to be
	 * readable by gavd.
	 *
	 * TODO(thutt): This could also be done with a udev enumerate
	 *              and then a udev monitor.
	 */
	const unsigned max_iterations = 4;
	unsigned i = 0;

	while (i < max_iterations) {
		int		   readable;
		struct timeval	   timeout;
		const char * const pathname = "/dev/input/event0";

		timeout.tv_sec	= 0;
		timeout.tv_usec = 500000;   /* 1/2 second. */
		readable = access(pathname, R_OK);

		/* If the file could be opened, then the udev rule has been
		 * applied and gavd can read the event files.  If there are no
		 * event files, then we don't need to wait.
		 *
		 * If access does not become available, then headphone &
		 * microphone jack autoswitching will not function properly.
		 */
		if (readable == 0 || (readable == -1 && errno == ENOENT)) {
			/* Access allowed, or file does not exist. */
			break;
		}
		if (readable != -1 || errno != EACCES) {
			syslog(LOG_ERR, "Bad access for input devs.");
			return errno;
		}
		select(1, NULL, NULL, NULL, &timeout);
		++i;
	}

	return 0;
}
