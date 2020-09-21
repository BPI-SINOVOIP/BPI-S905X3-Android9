/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <getopt.h>
#include <signal.h>
#include <syslog.h>

#include "cras_config.h"
#include "cras_iodev_list.h"
#include "cras_server.h"
#include "cras_system_state.h"
#include "cras_dsp.h"

static struct option long_options[] = {
	{"dsp_config", required_argument, 0, 'd'},
	{"syslog_mask", required_argument, 0, 'l'},
	{"device_config_dir", required_argument, 0, 'c'},
	{"disable_profile", required_argument, 0, 'D'},
	{"internal_ucm_suffix", required_argument, 0, 'u'},
	{0, 0, 0, 0}
};

/* Ignores sigpipe, we'll notice when a read/write fails. */
static void set_signals()
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
}

/* Entry point for the server. */
int main(int argc, char **argv)
{
	int c, option_index;
	int log_mask = LOG_ERR;
	const char default_dsp_config[] = CRAS_CONFIG_FILE_DIR "/dsp.ini";
	const char *dsp_config = default_dsp_config;
	const char *device_config_dir = CRAS_CONFIG_FILE_DIR;
	const char *internal_ucm_suffix = NULL;
	unsigned int profile_disable_mask = 0;

	set_signals();

	while (1) {
		c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		/* To keep this code simple we ask the (technical)
		   user to pass one of integer values defined in
		   syslog.h - this is a development feature after
		   all. While there is no formal standard for the
		   integer values there is an informal standard:
		   http://tools.ietf.org/html/rfc5424#page-11 */
		case 'l':
			log_mask = atoi(optarg);
			break;

		case 'c':
			device_config_dir = optarg;
			break;

		case 'd':
			dsp_config = optarg;
			break;
		/* --disable_profile option takes list of profile names separated by ',' */
		case 'D':
			while ((optarg != NULL) && (*optarg != 0)) {
				if (strncmp(optarg, "hfp", 3) == 0) {
					profile_disable_mask |= CRAS_SERVER_PROFILE_MASK_HFP;
				}
				if (strncmp(optarg, "hsp", 3) == 0) {
					profile_disable_mask |= CRAS_SERVER_PROFILE_MASK_HSP;
				}
				if (strncmp(optarg, "a2dp", 4) == 0) {
					profile_disable_mask |= CRAS_SERVER_PROFILE_MASK_A2DP;
				}
				optarg = strchr(optarg, ',');
				if (optarg != NULL) {
					optarg++;
				}
			}
			break;
		case 'u':
			if (*optarg != 0)
				internal_ucm_suffix = optarg;
			break;
		default:
			break;
		}
	}

	switch (log_mask) {
		case LOG_EMERG: case LOG_ALERT: case LOG_CRIT: case LOG_ERR:
		case LOG_WARNING: case LOG_NOTICE: case LOG_INFO:
		case LOG_DEBUG:
			break;
		default:
			fprintf(stderr,
				"Unsupported syslog priority value: %d; using LOG_ERR=%d\n",
				log_mask, LOG_ERR);
			log_mask = LOG_ERR;
			break;
	}
	setlogmask(LOG_UPTO(log_mask));

	/* Initialize system. */
	cras_server_init();
	cras_system_state_init(device_config_dir);
	if (internal_ucm_suffix)
		cras_system_state_set_internal_ucm_suffix(internal_ucm_suffix);
	cras_dsp_init(dsp_config);
	cras_iodev_list_init();

	/* Start the server. */
	return cras_server_run(profile_disable_mask);
}
