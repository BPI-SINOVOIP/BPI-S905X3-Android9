/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RESOLV_PARAMS_H
#define _RESOLV_PARAMS_H

#include <stdint.h>

/* Hard-coded defines */
#define MAXNS			4	/* max # name servers we'll track */
#define MAXDNSRCH		6	/* max # domains in search path */
#define MAXDNSRCHPATH		256	/* max length of domain search paths */
#define MAXNSSAMPLES		64	/* max # samples to store per server */

/* Defaults used for initializing __res_params */
#define SUCCESS_THRESHOLD	75	/* if successes * 100 / total_samples is less than
					 * this value, the server is considered failing
					 */
#define NSSAMPLE_VALIDITY	1800	/* Sample validity in seconds.
					 * Set to -1 to disable skipping failing
					 * servers.
					 */

/* If EDNS0_PADDING is defined, queries will be padded to a multiple of this length
when EDNS0 is active. */
#define EDNS0_PADDING	128

/* per-netid configuration parameters passed from netd to the resolver */
struct __res_params {
    uint16_t sample_validity; // sample lifetime in s
    // threshold of success / total samples below which a server is considered broken
    uint8_t success_threshold; // 0: disable, value / 100 otherwise
    uint8_t min_samples; // min # samples needed for statistics to be considered meaningful
    uint8_t max_samples; // max # samples taken into account for statistics
};

typedef enum { res_goahead, res_nextns, res_modified, res_done, res_error }
	res_sendhookact;

typedef res_sendhookact (*res_send_qhook)(struct sockaddr * const *,
					      const u_char **, int *,
					      u_char *, int, int *);

typedef res_sendhookact (*res_send_rhook)(const struct sockaddr *,
					      const u_char *, int, u_char *,
					      int, int *);

#endif // _RESOLV_PARAMS_H
