/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef MEMORY_LEAK_TRACK_UTIL_H
#define MEMORY_LEAK_TRACK_UTIL_H

namespace android {
/*
 * Dump the memory address of the calling process to the given fd.
 */
extern void dumpMemoryAddresses(int fd);

};

#endif  // MEMORY_LEAK_TRACK_UTIL_H
