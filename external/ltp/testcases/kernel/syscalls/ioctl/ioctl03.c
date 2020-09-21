/*
 * Copyright (c) International Business Machines  Corp., 2008
 * Copyright (c) Linux Test Project, 2017
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

/*
 * File:        ioctl03.c
 *
 * Description: This program tests whether all the valid IFF flags are
 *              returned properly by implementation of TUNGETFEATURES ioctl
 *              on kernel 2.6.27
 *
 * Test Name:   ioctl03
 *
 * Author:      Rusty Russell <rusty@rustcorp.com.au>
 *
 * history:     created - nov 28 2008 - rusty russell <rusty@rustcorp.com.au>
 *              ported to ltp
 *                      - nov 28 2008 - subrata <subrata@linux.vnet.ibm.com>
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/if_tun.h>
#include "tst_test.h"

#ifndef TUNGETFEATURES
#define TUNGETFEATURES _IOR('T', 207, unsigned int)
#endif

#ifndef IFF_VNET_HDR
#define IFF_VNET_HDR	0x4000
#endif

#ifndef IFF_MULTI_QUEUE
#define IFF_MULTI_QUEUE	0x0100
#endif

#ifndef IFF_NAPI
#define IFF_NAPI	0x0010
#endif

#ifndef IFF_NAPI_FRAGS
#define IFF_NAPI_FRAGS	0x0020
#endif

static struct {
	unsigned int flag;
	const char *name;
} known_flags[] = {
	{IFF_TUN, "TUN"},
	{IFF_TAP, "TAP"},
	{IFF_NO_PI, "NO_PI"},
	{IFF_ONE_QUEUE, "ONE_QUEUE"},
	{IFF_VNET_HDR, "VNET_HDR"},
	{IFF_MULTI_QUEUE, "MULTI_QUEUE"},
	{IFF_NAPI, "IFF_NAPI"},
	{IFF_NAPI_FRAGS, "IFF_NAPI_FRAGS"}
};

static void verify_features(void)
{
	unsigned int features, i;

	int netfd = open("/dev/net/tun", O_RDWR);
	if (netfd == -1) {
		if (errno == ENODEV)
			tst_brk(TCONF, "TUN support is missing?");

		tst_brk(TBROK | TERRNO, "opening /dev/net/tun failed");
	}

	SAFE_IOCTL(netfd, TUNGETFEATURES, &features);

	tst_res(TINFO, "Available features are: %#x", features);
	for (i = 0; i < ARRAY_SIZE(known_flags); i++) {
		if (features & known_flags[i].flag) {
			features &= ~known_flags[i].flag;
			tst_res(TPASS, "%s %#x", known_flags[i].name,
				 known_flags[i].flag);
		}
	}
	if (features)
		tst_res(TFAIL, "(UNKNOWN %#x)", features);

	SAFE_CLOSE(netfd);
}

static struct tst_test test = {
	.test_all = verify_features,
	.needs_root = 1,
};
