# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
import unittest
import logging
import os
import sys
import tempfile
import shutil
from importlib import import_module

from google.protobuf import text_format

from acts.libs.proto.proto_utils import compile_proto
from acts.libs.proto.proto_utils import compile_import_proto

TEST_PROTO_NAME = "acts_proto_utils_test.proto"
TEST_PROTO_GENERATED_NAME = "acts_proto_utils_test_pb2"
TEST_PROTO_DATA_NAME = "acts_proto_utils_test_data.txt"
TEST_NAME = "test_name"
TEST_ID = 42


class BtMetricsUtilsTest(unittest.TestCase):
    """This test class has unit tests for the implementation of everything
    under acts.controllers.android_device.
    """

    def setUp(self):
        # Set log_path to logging since acts logger setup is not called.
        if not hasattr(logging, "log_path"):
            setattr(logging, "log_path", "/tmp/logs")
        # Creates a temp dir to be used by tests in this test class.
        self.tmp_dir = tempfile.mkdtemp()

    def tearDown(self):
        """Removes the temp dir.
        """
        shutil.rmtree(self.tmp_dir)

    def getResource(self, relative_path_to_test):
        return os.path.join(
            os.path.dirname(os.path.realpath(__file__)), relative_path_to_test)

    def compare_test_entry(self, entry, name, id, nested):
        self.assertEqual(entry.name, name)
        self.assertEqual(entry.id, id)
        self.assertEqual(len(entry.nested), len(nested))
        for i in range(len(entry.nested)):
            self.assertEqual(entry.nested[i].name, nested[i][0])
            self.assertEqual(entry.nested[i].type, nested[i][1])

    def test_compile_proto(self):
        proto_path = self.getResource(TEST_PROTO_NAME)
        output_module_name = compile_proto(proto_path, self.tmp_dir)
        self.assertIsNotNone(output_module_name)
        self.assertEqual(output_module_name, TEST_PROTO_GENERATED_NAME)
        self.assertTrue(
            os.path.exists(
                os.path.join(self.tmp_dir, output_module_name) + ".py"))
        sys.path.append(self.tmp_dir)
        bt_metrics_utils_test_pb2 = None
        try:
            bt_metrics_utils_test_pb2 = import_module(output_module_name)
        except ImportError:
            self.fail("Cannot import generated py-proto %s" %
                      (output_module_name))
        test_proto = bt_metrics_utils_test_pb2.TestProto()
        test_proto_entry = test_proto.entries.add()
        test_proto_entry.id = TEST_ID
        test_proto_entry.name = TEST_NAME
        self.assertEqual(test_proto.entries[0].id, TEST_ID)
        self.assertEqual(test_proto.entries[0].name, TEST_NAME)

    def test_parse_proto(self):
        proto_path = self.getResource(TEST_PROTO_NAME)
        output_module = compile_import_proto(self.tmp_dir, proto_path)
        self.assertIsNotNone(output_module)
        AAA = output_module.TestProtoEntry.NestedType.AAA
        BBB = output_module.TestProtoEntry.NestedType.BBB
        test_proto = output_module.TestProto()
        proto_data_path = self.getResource(TEST_PROTO_DATA_NAME)
        self.assertTrue(os.path.exists(proto_data_path))
        self.assertTrue(os.path.isfile(proto_data_path))
        with open(proto_data_path) as f:
            text_format.Merge(f.read(), test_proto)
        self.assertEqual(len(test_proto.entries), 2)
        entry1 = test_proto.entries[0]
        self.compare_test_entry(entry1, "TestName1", 42,
                                [("NestedA", AAA), ("NestedB", BBB),
                                 ("NestedA", AAA)])
        entry2 = test_proto.entries[1]
        self.compare_test_entry(entry2, "TestName2", 43,
                                [("NestedB", BBB), ("NestedA", AAA),
                                 ("NestedB", BBB)])
