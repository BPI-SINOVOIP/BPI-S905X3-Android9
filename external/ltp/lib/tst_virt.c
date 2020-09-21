/*
 * Copyright (C) 2013 Linux Test Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Further, this software is distributed without any warranty that it
 * is free of the rightful claim of any third person regarding
 * infringement or the like.  Any license provided herein, whether
 * implied or otherwise, applies only to this software file.  Patent
 * licenses, if any, provided herein do not apply to combinations of
 * this program with other software, or any other product whatsoever.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <unistd.h>
#include "test.h"
#include "safe_macros.h"

static int is_kvm(void)
{
	FILE *cpuinfo;
	char line[64];
	int found;

	/* this doesn't work with custom -cpu values, since there's
	 * no easy, reasonable or reliable way to work around those */
	cpuinfo = SAFE_FOPEN(NULL, "/proc/cpuinfo", "r");
	found = 0;
	while (fgets(line, sizeof(line), cpuinfo) != NULL) {
		if (strstr(line, "QEMU Virtual CPU")) {
			found = 1;
			break;
		}
	}

	SAFE_FCLOSE(NULL, cpuinfo);
	return found;
}

static int is_xen(void)
{
	char hypervisor_type[3];

	if (access("/proc/xen", F_OK) == 0)
		return 1;

	if (access("/sys/hypervisor/type", F_OK) == 0) {
		SAFE_FILE_SCANF(NULL, "/sys/hypervisor/type", "%3s",
			hypervisor_type);
		return strncmp("xen", hypervisor_type,
			sizeof(hypervisor_type)) == 0;
	}

	return 0;
}

static int try_systemd_detect_virt(void)
{
	FILE *f;
	char virt_type[64];
	int ret;

	/* See tst_run_cmd.c */
	void *old_handler = signal(SIGCHLD, SIG_DFL);

	f = popen("systemd-detect-virt", "r");
	if (!f) {
		signal(SIGCHLD, old_handler);
		return 0;
	}

	if (!fgets(virt_type, sizeof(virt_type), f))
		virt_type[0] = '\0';

	ret = pclose(f);

	signal(SIGCHLD, old_handler);

	/*
	 * systemd-detect-virt not found by shell or no virtualization detected
	 * (systemd-detect-virt returns non-zero)
         */
	if (ret)
		return 0;

	if (strncmp("kvm", virt_type, 3))
		return VIRT_KVM;

	if (strncmp("xen", virt_type, 3))
		return VIRT_XEN;

	return 0;
}

int tst_is_virt(int virt_type)
{
	int ret = try_systemd_detect_virt();

	if (ret)
		return ret == virt_type;

	switch (virt_type) {
	case VIRT_XEN:
		return is_xen();
	case VIRT_KVM:
		return is_kvm();
	}

	tst_brkm(TBROK, NULL, "invalid virt_type flag: %d", virt_type);
	return -1;
}
