# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# =============================================================================
"""Wraps capture_tpu_profile binary."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from absl import flags

import os
import subprocess
import sys

import tensorflow as tf

# Cloud TPU Cluster Resolvers
flags.DEFINE_string(
    'gcp_project', None,
    'Project name for the Cloud TPU-enabled project. If not specified, we '
    'will attempt to automatically detect the GCE project from metadata.')
flags.DEFINE_string(
    'tpu_zone',
    None,
    help='GCE zone where the Cloud TPU is located in. If not specified, we '
    'will attempt to automatically detect the GCE project from metadata.')
flags.DEFINE_string('tpu_name', None,
                    'Name of the Cloud TPU for Cluster Resolvers. You must '
                    'specify either this flag or --master.')

# Tool specific parameters
flags.DEFINE_string(
    'service_addr', None, 'Address of TPU profiler service e.g. '
    'localhost:8466, you must specify either this flag or --tpu_name.')
flags.DEFINE_string('logdir', None,
                    'Path of TensorBoard log directory e.g. /tmp/tb_log, '
                    'gs://tb_bucket')
flags.DEFINE_integer('duration_ms', 2000, 'Duration of tracing in ms.')
flags.DEFINE_integer('num_tracing_attempts', 3,
                     'Automatically retry N times when no trace '
                     'event is collected.')
flags.DEFINE_boolean('include_dataset_ops', True,
                     'Set to false to profile longer TPU '
                     'device traces.')

FLAGS = flags.FLAGS
EXECUTABLE = 'data/capture_tpu_profile'


def run_main():
  tf.app.run(main)


def main(unused_argv=None):
  tf.logging.set_verbosity(tf.logging.INFO)

  if FLAGS.service_addr is None and FLAGS.tpu_name is None:
    sys.exit('You must specify either --service_addr or --tpu_name.')

  if FLAGS.service_addr is not None:
    if FLAGS.tpu_name is not None:
      tf.logging.warn('Both --service_addr and --tpu_name are set. Ignoring '
                      '--tpu_name and using --service_addr.')
    service_addr = FLAGS.service_addr
  else:
    tpu_cluster_resolver = (
        tf.contrib.cluster_resolver.TPUClusterResolver(
            tpu_names=[FLAGS.tpu_name],
            zone=FLAGS.tpu_zone,
            project=FLAGS.gcp_project))
    service_addr = tpu_cluster_resolver.get_master()
  service_addr = service_addr.replace('grpc://', '').replace(':8470', ':8466')

  if not FLAGS.logdir:
    sys.exit('logdir must be provided.')
  executable_path = os.path.join(os.path.dirname(__file__), EXECUTABLE)
  logdir = os.path.expandvars(os.path.expanduser(FLAGS.logdir))
  cmd = [executable_path]
  cmd.append('--logdir=' + logdir)
  cmd.append('--service_addr=' + service_addr)
  cmd.append('--duration_ms=' + str(FLAGS.duration_ms))
  cmd.append('--num_tracing_attempts=' + str(FLAGS.num_tracing_attempts))
  cmd.append('--include_dataset_ops=' + str(FLAGS.include_dataset_ops).lower())
  subprocess.call(cmd)


if __name__ == '__main__':
  run_main()
