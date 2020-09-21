// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_SINK_FILE_H_
#define _BSDIFF_SINK_FILE_H_

#include <stdint.h>

#include <functional>

#include "bsdiff/file_interface.h"

using sink_func = std::function<size_t(const uint8_t*, size_t)>;

namespace bsdiff {

class SinkFile : public FileInterface {
 public:
  // Creates a SinkFile based on the underlying |sink| function passed.
  // The SinkFile will call |sink| function upon write.
  // Read, Seek and GetSize are not supported.
  explicit SinkFile(const sink_func& sink);

  ~SinkFile() = default;

  // FileInterface overrides.
  bool Read(void* buf, size_t count, size_t* bytes_read) override;
  bool Write(const void* buf, size_t count, size_t* bytes_written) override;
  bool Seek(off_t pos) override;
  bool Close() override;
  bool GetSize(uint64_t* size) override;

 private:
  // The sink() function used to write data.
  const sink_func& sink_;
};

}  // namespace bsdiff

#endif  // _BSDIFF_SINK_FILE_H_
