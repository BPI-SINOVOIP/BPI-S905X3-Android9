// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <limits>

#include "bsdiff/bsdiff.h"
#include "bsdiff/bsdiff_arguments.h"
#include "bsdiff/constants.h"
#include "bsdiff/patch_writer_factory.h"

namespace {

// mmap() the passed |filename| to read-only memory and store in |filesize| the
// size of the file. To release the memory, call munmap with the returned
// pointer and filesize. In case of error returns nullptr.
void* MapFile(const char* filename, size_t* filesize) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("open()");
    return nullptr;
  }

  struct stat st;
  fstat(fd, &st);
  if (static_cast<uint64_t>(st.st_size) > std::numeric_limits<size_t>::max()) {
    fprintf(stderr, "File too big\n");
    close(fd);
    return nullptr;
  }
  *filesize = st.st_size;

  void* ret = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (ret == MAP_FAILED) {
    perror("mmap()");
    close(fd);
    return nullptr;
  }
  close(fd);
  return ret;
}

// Generate bsdiff patch from the |old_filename| file to the |new_filename|
// file with options in |arguments|. Store the resulting patch in a new
// |patch_filename| file. Returns 0 on success.
int GenerateBsdiffFromFiles(const char* old_filename,
                            const char* new_filename,
                            const char* patch_filename,
                            const bsdiff::BsdiffArguments& arguments) {
  size_t oldsize;
  uint8_t* old_buf = static_cast<uint8_t*>(MapFile(old_filename, &oldsize));
  if (!old_buf) {
    return 1;
  }

  size_t newsize;
  uint8_t* new_buf = static_cast<uint8_t*>(MapFile(new_filename, &newsize));
  if (!new_buf) {
    munmap(old_buf, oldsize);
    return 1;
  }

  std::unique_ptr<bsdiff::PatchWriterInterface> patch_writer;
  std::vector<uint8_t> raw_data;

  if (arguments.format() == bsdiff::BsdiffFormat::kLegacy) {
    patch_writer = bsdiff::CreateBsdiffPatchWriter(patch_filename);
  } else if (arguments.format() == bsdiff::BsdiffFormat::kBsdf2) {
    patch_writer = bsdiff::CreateBSDF2PatchWriter(
        patch_filename, arguments.compressor_type(),
        arguments.compression_quality());
  } else if (arguments.format() == bsdiff::BsdiffFormat::kEndsley) {
    patch_writer =
        bsdiff::CreateEndsleyPatchWriter(&raw_data, arguments.compressor_type(),
                                         arguments.compression_quality());
  } else {
    std::cerr << "unexpected bsdiff format." << std::endl;
    return 1;
  }

  int ret = bsdiff::bsdiff(old_buf, oldsize, new_buf, newsize,
                           arguments.min_length(), patch_writer.get(), nullptr);

  munmap(old_buf, oldsize);
  munmap(new_buf, newsize);

  if (!ret && arguments.format() == bsdiff::BsdiffFormat::kEndsley) {
    // Store the raw_data on disk.
    FILE* fp = fopen(patch_filename, "wb");
    if (!fp) {
      perror("Opening the patch file");
      return 1;
    }
    if (raw_data.size() != fwrite(raw_data.data(), 1, raw_data.size(), fp)) {
      perror("Writing to the patch file");
      ret = 1;
    }
    fclose(fp);
  }
  return ret;
}

void PrintUsage(const std::string& proc_name) {
  std::cerr << "usage: " << proc_name
            << " [options] oldfile newfile patchfile\n";
  std::cerr << "  --format <legacy|bsdiff40|bsdf2|endsley>  The format of the"
               " bsdiff patch.\n"
            << "  --minlen LEN                       The minimum match length "
               "required to consider a match in the algorithm.\n"
            << "  --type <bz2|brotli|nocompression>  The algorithm to compress "
               "the patch, bsdf2 format only.\n"
            << "  --quality                          Quality of the patch "
               "compression, brotli only.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  bsdiff::BsdiffArguments arguments;

  if (!arguments.ParseCommandLine(argc, argv)) {
    PrintUsage(argv[0]);
    return 1;
  }

  // The optind will be updated in ParseCommandLine to parse the options; and
  // we expect the rest of the arguments to be oldfile, newfile, patchfile.
  if (!arguments.IsValid() || argc - optind != 3) {
    PrintUsage(argv[0]);
    return 1;
  }

  return GenerateBsdiffFromFiles(argv[optind], argv[optind + 1],
                                 argv[optind + 2], arguments);
}
