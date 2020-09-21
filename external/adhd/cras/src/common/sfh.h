/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * An incremental version of the SuperFastHash hash function from
 * http://www.azillionmonkeys.com/qed/hash.html
 * The code did not come with its own header file, so declaring the function
 * here.
 */

#ifndef SFH_H_
#define SFH_H_

uint32_t SuperFastHash(const char *data, int len, uint32_t hash);

#endif /* SFH_H_ */
