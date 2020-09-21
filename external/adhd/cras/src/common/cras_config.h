/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_CONFIG_H_
#define CRAS_CONFIG_H_

#define CRAS_MIN_BUFFER_TIME_IN_US 1000 /* 1 milliseconds */

#define CRAS_SERVER_RT_THREAD_PRIORITY 12
#define CRAS_CLIENT_RT_THREAD_PRIORITY 10
#define CRAS_CLIENT_NICENESS_LEVEL -10
#define CRAS_SOCKET_FILE ".cras_socket"

/* CRAS_CONFIG_FILE_DIR is defined as $sysconfdir/cras by the configure
   script. */

/* Gets the path to save UDS socket files. */
const char *cras_config_get_system_socket_file_dir();

#endif /* CRAS_CONFIG_H_ */
