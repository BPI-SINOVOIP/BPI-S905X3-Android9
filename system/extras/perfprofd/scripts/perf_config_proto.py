#!/usr/bin/python
#
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

# Test converter of a Config proto.

# Generate with:
#  aprotoc -I=system/extras/perfprofd --python_out=system/extras/perfprofd/scripts \
#      system/extras/perfprofd/binder_interface/perfprofd_config.proto
import perfprofd_config_pb2

import sys

config_options = [
    ('collection_interval_in_s', 'u'),
    ('use_fixed_seed', 'u'),
    ('main_loop_iterations', 'u'),
    ('destination_directory', 's'),
    ('config_directory', 's'),
    ('perf_path', 's'),
    ('sampling_period', 'u'),
    ('sample_duration_in_s', 'u'),
    ('only_debug_build', 'b'),
    ('hardwire_cpus', 'b'),
    ('hardwire_cpus_max_duration_in_s', 'u'),
    ('max_unprocessed_profiles', 'u'),
    ('stack_profile', 'b'),
    ('collect_cpu_utilization', 'b'),
    ('collect_charging_state', 'b'),
    ('collect_booting', 'b'),
    ('collect_camera_active', 'b'),
    ('process', 'i'),
    ('use_elf_symbolizer', 'b'),
    ('send_to_dropbox', 'b'),
]

def collect_and_write(filename):
    config = perfprofd_config_pb2.ProfilingConfig()

    for (option, option_type) in config_options:
        input = raw_input('%s(%s): ' % (option, option_type))
        if input == '':
            # Skip this argument.
            continue
        elif input == '!':
            # Special-case input, end argument collection.
            break
        # Now do some actual parsing work.
        if option_type == 'u' or option_type == 'i':
            option_val = int(input)
        elif option_type == 'b':
            if input == '1' or input == 't' or input == 'true':
                option_val = True
            elif input == '0' or input == 'f' or input == 'false':
                option_val = False
            else:
                assert False, 'Unknown boolean %s' % input
        else:
            assert False, 'Unknown type %s' % type
        setattr(config, option, option_val)

    f = open(filename, "wb")
    f.write(config.SerializeToString())
    f.close()

def read_and_print(filename):
    config = perfprofd_config_pb2.ProfilingConfig()

    f = open(filename, "rb")
    config.ParseFromString(f.read())
    f.close()

    print config

if sys.argv[1] == 'read':
    read_and_print(sys.argv[2])
elif sys.argv[1] == 'write':
    collect_and_write(sys.argv[2])
else:
    print 'Usage: python perf_config_proto.py (read|write) filename'
