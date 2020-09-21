# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

{
  'target_defaults': {
    'cflags': [
      '-Wextra',
      '-Wno-unused-parameter',
    ],
    'cflags_cc': [
      '-Wnon-virtual-dtor',
    ],
    'include_dirs': [
      'include',
      # We need this include dir because we include all the local code as
      # "bsdiff/...".
      '<(platform2_root)/../aosp/external',
    ],
    'defines': [
      '_FILE_OFFSET_BITS=64',
    ],
  },
  'targets': [
    # bsdiff static library
    {
      'target_name': 'libbsdiff-static',
      'type': 'static_library',
      'cflags!': ['-fPIE'],
      'cflags': ['-fPIC'],
      'all_dependent_settings': {
        'link_settings': {
          'libraries': [
            '-lbz2',
            '-lbrotlienc',
            '-ldivsufsort',
            '-ldivsufsort64',
          ],
        },
      },
      'sources': [
        'brotli_compressor.cc',
        'bsdiff.cc',
        'bz2_compressor.cc',
        'compressor_buffer.cc',
        'diff_encoder.cc',
        'endsley_patch_writer.cc',
        'logging.cc',
        'patch_writer.cc',
        'patch_writer_factory.cc',
        'split_patch_writer.cc',
        'suffix_array_index.cc',
      ],
    },
    # bsdiff shared library
    {
      'target_name': 'libbsdiff',
      'type': 'shared_library',
      'dependencies': [
        'libbsdiff-static',
      ],
    },
    # bsdiff executable
    {
      'target_name': 'bsdiff',
      'type': 'executable',
      'dependencies': [
        'libbsdiff',
      ],
      'sources': [
        'bsdiff_arguments.cc',
        'bsdiff_main.cc',
      ],
    },
    # bspatch static library
    {
      'target_name': 'libbspatch-static',
      'type': 'static_library',
      'cflags!': ['-fPIE'],
      'cflags': ['-fPIC'],
      'all_dependent_settings': {
        'link_settings': {
          'libraries': [
            '-lbz2',
            '-lbrotlidec',
          ],
        },
      },
      'sources': [
        'brotli_decompressor.cc',
        'bspatch.cc',
        'buffer_file.cc',
        'bz2_decompressor.cc',
        'decompressor_interface.cc',
        'extents.cc',
        'extents_file.cc',
        'file.cc',
        'logging.cc',
        'memory_file.cc',
        'patch_reader.cc',
        'sink_file.cc',
        'utils.cc',
      ],
    },
    # bspatch shared library
    {
      'target_name': 'libbspatch',
      'type': 'shared_library',
      'dependencies': [
        'libbspatch-static',
      ],
    },
    # bspatch executable
    {
      'target_name': 'bspatch',
      'type': 'executable',
      'dependencies': [
        'libbspatch',
      ],
      'sources': [
        'bspatch_main.cc',
      ],
    },
  ],
  'conditions': [
    ['USE_test == 1', {
      'targets': [
        {
          'target_name': 'bsdiff_unittest',
          'type': 'executable',
          'dependencies': [
            'libbsdiff-static',
            'libbspatch-static',
            '../common-mk/testrunner.gyp:testrunner',
          ],
          'variables': {
            'deps': [
              'libchrome-<(libbase_ver)',
            ],
          },
          'includes': ['../common-mk/common_test.gypi'],
          'sources': [
            'brotli_compressor_unittest.cc',
            'bsdiff_arguments.cc',
            'bsdiff_arguments_unittest.cc',
            'bsdiff_unittest.cc',
            'bspatch_unittest.cc',
            'diff_encoder_unittest.cc',
            'endsley_patch_writer_unittest.cc',
            'extents_file_unittest.cc',
            'extents_unittest.cc',
            'patch_reader_unittest.cc',
            'patch_writer_unittest.cc',
            'split_patch_writer_unittest.cc',
            'suffix_array_index_unittest.cc',
            'test_utils.cc',
          ],
        },
      ],
    }],
  ],
}
