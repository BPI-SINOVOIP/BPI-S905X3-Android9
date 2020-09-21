/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#ifndef _RECOVERY_INSTALL_AMLOGIC_H_
#define _RECOVERY_INSTALL_AMLOGIC_H_

#include "common.h"
#include "device.h"
#include "roots.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>

void amlogic_init();

void amlogic_get_args(std::vector<std::string>& args);

void setup_cache_mounts();

int ensure_path_mounted_extra(Volume *v);

#endif

