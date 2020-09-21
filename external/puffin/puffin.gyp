# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'variables': {
      'deps': [
        'libbrillo-<(libbase_ver)',
        'libchrome-<(libbase_ver)',
      ],
    },
    'cflags': [
      '-Wextra',
    ],
    'cflags_cc': [
      '-Wnon-virtual-dtor',
    ],
    'include_dirs': [
      'src/include',
    ],
    'defines': [
      'USE_BRILLO=1',
      '_FILE_OFFSET_BITS=64',
      'ZLIB_CONST',
    ],
  },
  'targets': [
    # libpuffin-proto library
    {
      'target_name': 'libpuffin-proto',
      'type': 'static_library',
      'variables': {
        'proto_in_dir': 'src',
        'proto_out_dir': 'include/puffin/src',
      },
      'cflags!': ['-fPIE'],
      'cflags': ['-fPIC'],
      'all_dependent_settings': {
        'variables': {
          'deps': [
            'protobuf-lite',
          ],
        },
      },
      'sources': [
        '<(proto_in_dir)/puffin.proto',
      ],
      'includes': ['../../platform2/common-mk/protoc.gypi'],
    },
    # puffpatch static library. The reason to do one static and one shared is to
    # be able to run the unittest.
    {
      'target_name': 'libpuffpatch-static',
      'type': 'static_library',
      'cflags!': ['-fPIE'],
      'cflags': ['-fPIC'],
      'sources': [
        'src/bit_reader.cc',
        'src/bit_writer.cc',
        'src/huffer.cc',
        'src/huffman_table.cc',
        'src/puff_reader.cc',
        'src/puff_writer.cc',
        'src/puffer.cc',
        'src/puffin_stream.cc',
        'src/puffpatch.cc',
      ],
      'dependencies': [
        'libpuffin-proto',
      ],
      'all_dependent_settings': {
        'link_settings': {
          'libraries': [
            '-lbspatch',
          ],
        },
      },
    },
    # puffdiff static library.
    {
      'target_name': 'libpuffdiff-static',
      'type': 'static_library',
      'cflags!': ['-fPIE'],
      'cflags': ['-fPIC'],
      'sources': [
        'src/file_stream.cc',
        'src/memory_stream.cc',
        'src/puffdiff.cc',
        'src/utils.cc',
      ],
      'dependencies': [
        'libpuffpatch-static',
      ],
      'all_dependent_settings': {
        'variables': {
          'deps': [
            'zlib',
          ],
        },
        'link_settings': {
          'libraries': [
            '-lbsdiff',
          ],
        },
      },
    },
    # puffpatch shared library.
    {
      'target_name': 'libpuffpatch',
      'type': 'shared_library',
      'dependencies': [
        'libpuffpatch-static',
      ],
    },
    # puffdiff shared library.
    {
      'target_name': 'libpuffdiff',
      'type': 'shared_library',
      'dependencies': [
        'libpuffdiff-static',
      ],
    },
    # Puffin executable. We don't use the shared libraries because then we have
    # to export symbols that shouldn't be exported otherwise.
    {
      'target_name': 'puffin',
      'type': 'executable',
      'dependencies': [
        'libpuffdiff-static',
      ],
      'sources': [
        'src/extent_stream.cc',
        'src/main.cc',
      ],
    },
  ],
  # unit tests.
  'conditions': [
    ['USE_test == 1', {
      'targets': [
        # Samples generator.
        {
          'target_name': 'libsample_generator',
          'type': 'static_library',
          'sources': [
            'src/sample_generator.cc',
          ],
          'all_dependent_settings': {
            'variables': {
              'deps': [
                'zlib',
              ],
            },
          },
        },
        # Unit tests.
        {
          'target_name': 'puffin_unittest',
          'type': 'executable',
          'dependencies': [
            'libpuffdiff-static',
            'libsample_generator',
            '../../platform2/common-mk/testrunner.gyp:testrunner',
          ],
          'includes': ['../../platform2/common-mk/common_test.gypi'],
          'sources': [
            'src/bit_io_unittest.cc',
            'src/extent_stream.cc',
            'src/patching_unittest.cc',
            'src/puff_io_unittest.cc',
            'src/puffin_unittest.cc',
            'src/stream_unittest.cc',
            'src/unittest_common.cc',
            'src/utils_unittest.cc',
          ],
        },
      ],
    }],
    # fuzzer target
    ['USE_fuzzer == 1', {
      'targets': [
        {
          'target_name': 'puffin_fuzzer',
          'type': 'executable',
          'dependencies': [
            'libpuffin-proto',
            'libpuffdiff-static',
            'libpuffpatch-static',
          ],
          'includes': ['../../platform2/common-mk/common_fuzzer.gypi'],
          'sources': [
            'src/fuzzer.cc',
          ],
        },
      ],
    }],
  ],
}
