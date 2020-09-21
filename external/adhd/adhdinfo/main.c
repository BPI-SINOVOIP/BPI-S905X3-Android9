/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "adhd_alsa.h"

static unsigned arg_verbose = 0;

static void help(void)
{
    /* TODO(thutt): Add help */
}

static void process_arguments(int argc, char **argv)
{
    static struct option options[] = {
        {
            .name    = "help",
            .has_arg = no_argument,
            .flag    = NULL,
            .val     = 256
        },
        {
            .name    = "verbose",
            .has_arg = no_argument,
            .flag    = NULL,
            .val     = 257
        },
    };

    while (1) {
        int option_index = 0;
        const int choice = getopt_long(argc, argv, "", options, &option_index);

        if (choice == -1) {
            break;
        }

        switch (choice) {
        case 256:
            help();
            break;

        case 257:
            arg_verbose = 1;
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", choice);
        }
    }
}


int main(int argc, char **argv)
{
    adhd_alsa_info_t alsa_info;
    process_arguments(argc, argv);

    adhd_alsa_get_all_card_info(&alsa_info);
    adhd_alsa_release_card_info(&alsa_info);
    return 0;
}
