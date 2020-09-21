// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_INCLUDE_PUFFIN_PUFFDIFF_H_
#define SRC_INCLUDE_PUFFIN_PUFFDIFF_H_

#include <string>
#include <vector>

#include "puffin/common.h"
#include "puffin/stream.h"

namespace puffin {

// Performs a diff operation between input deflate streams and creates a patch
// that is used in the client to recreate the |dst| from |src|.
// |src|          IN   Source deflate stream.
// |dst|          IN   Destination deflate stream.
// |src_deflates| IN   Deflate locations in |src|.
// |dst_deflates| IN   Deflate locations in |dst|.
// |tmp_filepath| IN   A path to a temporary file. The caller has the
//                     responsibility of unlinking the file after the call to
//                     |PuffDiff| finishes.
// |puffin_patch| OUT  The patch that later can be used in |PuffPatch|.
bool PuffDiff(UniqueStreamPtr src,
              UniqueStreamPtr dst,
              const std::vector<BitExtent>& src_deflates,
              const std::vector<BitExtent>& dst_deflates,
              const std::string& tmp_filepath,
              Buffer* patch);

// Similar to the function above, except that it accepts raw buffer rather than
// stream.
PUFFIN_EXPORT
bool PuffDiff(const Buffer& src,
              const Buffer& dst,
              const std::vector<BitExtent>& src_deflates,
              const std::vector<BitExtent>& dst_deflates,
              const std::string& tmp_filepath,
              Buffer* patch);

}  // namespace puffin

#endif  // SRC_INCLUDE_PUFFIN_PUFFDIFF_H_
