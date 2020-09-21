// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUMOS_WIDE_PROFILING_MYBASE_BASE_LOGGING_H_
#define CHROMIUMOS_WIDE_PROFILING_MYBASE_BASE_LOGGING_H_

#include <errno.h>   // for errno
#include <string.h>  // for strerror

#include <iostream>
#include <sstream>
#include <string>

#include "android-base/logging.h"

// Emulate Chrome-like logging.

namespace logging {

extern bool gVlogEnabled;

}  // namespace logging

#define VLOG(level) ::logging::gVlogEnabled && LOG(INFO)

#define DLOG(x) android::base::kEnableDChecks && LOG(x)
#define DVLOG(x) android::base::kEnableDChecks && VLOG(x)

#endif  // CHROMIUMOS_WIDE_PROFILING_MYBASE_BASE_LOGGING_H_
