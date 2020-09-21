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
# ==============================================================================
"""Tests for data input for speech commands."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.examples.speech_commands import freeze
from tensorflow.python.platform import test


class FreezeTest(test.TestCase):

  def testCreateInferenceGraph(self):
    with self.test_session() as sess:
      freeze.create_inference_graph('a,b,c,d', 16000, 1000.0, 30.0, 30.0, 10.0,
                                    40, 'conv')
      self.assertIsNotNone(sess.graph.get_tensor_by_name('wav_data:0'))
      self.assertIsNotNone(
          sess.graph.get_tensor_by_name('decoded_sample_data:0'))
      self.assertIsNotNone(sess.graph.get_tensor_by_name('labels_softmax:0'))


if __name__ == '__main__':
  test.main()
