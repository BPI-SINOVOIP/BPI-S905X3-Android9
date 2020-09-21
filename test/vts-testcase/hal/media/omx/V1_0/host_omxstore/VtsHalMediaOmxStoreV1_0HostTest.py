#!/usr/bin/env python
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
#
"""This module is for VTS test cases involving IOmxStore and IOmx::listNodes().

VtsHalMediaOmxStoreV1_0Host derives from base_test.BaseTestClass. It contains
two independent tests: testListServiceAttributes() and
testQueryCodecInformation(). The first one tests
IOmxStore::listServiceAttributes() while the second one test multiple functions
in IOmxStore as well as check the consistency of the return values with
IOmx::listNodes().

"""

import logging
import re

from vts.runners.host import asserts
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_host_test import hal_hidl_host_test

OMXSTORE_V1_0_HAL = "android.hardware.media.omx@1.0::IOmxStore"

class VtsHalMediaOmxStoreV1_0Host(hal_hidl_host_test.HalHidlHostTest):
    """Host test class to run the Media_OmxStore HAL."""

    TEST_HAL_SERVICES = {OMXSTORE_V1_0_HAL}

    def setUpClass(self):
        super(VtsHalMediaOmxStoreV1_0Host, self).setUpClass()

        self.dut.hal.InitHidlHal(
            target_type='media_omx',
            target_basepaths=self.dut.libPaths,
            target_version=1.0,
            target_package='android.hardware.media.omx',
            target_component_name='IOmxStore',
            hw_binder_service_name=self.getHalServiceName(OMXSTORE_V1_0_HAL),
            bits=int(self.abi_bitness))

        self.omxstore = self.dut.hal.media_omx
        self.vtypes = self.omxstore.GetHidlTypeInterface('types')

    def testListServiceAttributes(self):
        """Test IOmxStore::listServiceAttributes().

        Tests that IOmxStore::listServiceAttributes() can be called
        successfully and returns sensible attributes.

        An attribute has a name (key) and a value. Known attributes (represented
        by variable "known" below) have certain specifications for valid values.
        Unknown attributes that start with 'supports-' should only have '0' or
        '1' as their value. Other unknown attributes do not cause the test to
        fail, but are reported as warnings in the host log.

        """

        status, attributes = self.omxstore.listServiceAttributes()
        asserts.assertEqual(self.vtypes.Status.OK, status,
                            'listServiceAttributes() fails.')

        # known is a dictionary whose keys are the known "key" for a service
        # attribute pair (see IOmxStore::Attribute), and whose values are the
        # corresponding regular expressions that will have to match with the
        # "value" of the attribute pair. If listServiceAttributes() returns an
        # attribute that has a matching key but an unmatched value, the test
        # will fail.
        known = {
            'max-video-encoder-input-buffers': re.compile('0|[1-9][0-9]*'),
            'supports-multiple-secure-codecs': re.compile('0|1'),
            'supports-secure-with-non-secure-codec': re.compile('0|1'),
        }
        # unknown is a list of pairs of regular expressions. For each attribute
        # whose key is not known (i.e., does not match any of the keys in the
        # "known" variable defined above), that key will be tried for a match
        # with the first element of each pair of the variable "unknown". If a
        # match occurs, the value of that same attribute will be tried for a
        # match with the second element of the pair. If this second match fails,
        # the test will fail.
        unknown = [
            (re.compile(r'supports-[a-z0-9\-]*'), re.compile('0|1')),
        ]

        # key_set is used to verify that listServiceAttributes() does not return
        # duplicate attribute names.
        key_set = set()
        for attr in attributes:
            attr_key = attr['key']
            attr_value = attr['value']

            # attr_key must not have been seen before.
            assert(
                attr_key not in key_set,
                'Service attribute "' + attr_key + '" has duplicates.')
            key_set.add(attr_key)

            if attr_key in known:
                asserts.assertTrue(
                    known[attr_key].match(attr_value),
                    'Service attribute "' + attr_key + '" has ' +
                    'invalid value "' + attr_value + '".')
            else:
                matched = False
                for key_re, value_re in unknown:
                    if key_re.match(attr_key):
                        asserts.assertTrue(
                            value_re.match(attr_value),
                            'Service attribute "' + attr_key + '" has ' +
                            'invalid value "' + attr_value + '".')
                        matched = True
                if not matched:
                    logging.warning(
                        'Unrecognized service attribute "' + attr_key + '" ' +
                        'with value "' + attr_value + '".')

    def testQueryCodecInformation(self):
        """Query and verify information from IOmxStore and IOmx::listNodes().

        This function performs three main checks:
         1. Information about roles and nodes returned from
            IOmxStore::listRoles() conforms to the specifications in
            IOmxStore.hal.
         2. Each node present in the information returned from
            IOmxStore::listRoles() must be supported by its owner. A node is
            considered "supported" by its owner if the IOmx instance
            corresponding to that owner returns that node and all associated
            roles when IOmx::listNodes() is called.
         3. The prefix string obtained form IOmxStore::getNodePrefix() must be
            sensible, and is indeed a prefix of all the node names.

        In step 1, node attributes are validated in the same manner as how
        service attributes are validated in testListServiceAttributes().
        Role names and mime types must be recognized by the function get_role()
        defined below.

        """

        # Basic patterns for matching
        class Pattern(object):
            toggle = '(0|1)'
            string = '(.*)'
            num = '(0|([1-9][0-9]*))'
            size = '(' + num + 'x' + num + ')'
            ratio = '(' + num + ':' + num + ')'
            range_num = '((' + num + '-' + num + ')|' + num + ')'
            range_size = '((' + size + '-' + size + ')|' + size + ')'
            range_ratio = '((' + ratio + '-' + ratio + ')|' + ratio + ')'
            list_range_num = '(' + range_num + '(,' + range_num + ')*)'

        # Matching rules for node attributes with fixed keys
        attr_re = {
            'alignment'                     : Pattern.size,
            'bitrate-range'                 : Pattern.range_num,
            'block-aspect-ratio-range'      : Pattern.range_ratio,
            'block-count-range'             : Pattern.range_num,
            'block-size'                    : Pattern.size,
            'blocks-per-second-range'       : Pattern.range_num,
            'complexity-default'            : Pattern.num,
            'complexity-range'              : Pattern.range_num,
            'feature-adaptive-playback'     : Pattern.toggle,
            'feature-bitrate-control'       : '(VBR|CBR|CQ)[,(VBR|CBR|CQ)]*',
            'feature-can-swap-width-height' : Pattern.toggle,
            'feature-intra-refresh'         : Pattern.toggle,
            'feature-partial-frame'         : Pattern.toggle,
            'feature-secure-playback'       : Pattern.toggle,
            'feature-tunneled-playback'     : Pattern.toggle,
            'frame-rate-range'              : Pattern.range_num,
            'max-channel-count'             : Pattern.num,
            'max-concurrent-instances'      : Pattern.num,
            'max-supported-instances'       : Pattern.num,
            'pixel-aspect-ratio-range'      : Pattern.range_ratio,
            'quality-default'               : Pattern.num,
            'quality-range'                 : Pattern.range_num,
            'quality-scale'                 : Pattern.string,
            'sample-rate-ranges'            : Pattern.list_range_num,
            'size-range'                    : Pattern.range_size,
        }

        # Matching rules for node attributes with key patterns
        attr_pattern_re = [
            ('measured-frame-rate-' + Pattern.size +
             '-range', Pattern.range_num),
            (r'feature-[a-zA-Z0-9_\-]+', Pattern.string),
        ]

        # Matching rules for node names and owners
        node_name_re = r'[a-zA-Z0-9.\-]+'
        node_owner_re = r'[a-zA-Z0-9._\-]+'

        # Compile all regular expressions
        for key in attr_re:
            attr_re[key] = re.compile(attr_re[key])
        for index, value in enumerate(attr_pattern_re):
            attr_pattern_re[index] = (re.compile(value[0]),
                                      re.compile(value[1]))
        node_name_re = re.compile(node_name_re)
        node_owner_re = re.compile(node_owner_re)

        # Mapping from mime types to roles.
        # These values come from MediaDefs.cpp and OMXUtils.cpp
        audio_mime_to_role = {
            '3gpp'          : 'amrnb',
            'ac3'           : 'ac3',
            'amr-wb'        : 'amrwb',
            'eac3'          : 'eac3',
            'flac'          : 'flac',
            'g711-alaw'     : 'g711alaw',
            'g711-mlaw'     : 'g711mlaw',
            'gsm'           : 'gsm',
            'mp4a-latm'     : 'aac',
            'mpeg'          : 'mp3',
            'mpeg-L1'       : 'mp1',
            'mpeg-L2'       : 'mp2',
            'opus'          : 'opus',
            'raw'           : 'raw',
            'vorbis'        : 'vorbis',
        }
        video_mime_to_role = {
            '3gpp'          : 'h263',
            'avc'           : 'avc',
            'dolby-vision'  : 'dolby-vision',
            'hevc'          : 'hevc',
            'mp4v-es'       : 'mpeg4',
            'mpeg2'         : 'mpeg2',
            'x-vnd.on2.vp8' : 'vp8',
            'x-vnd.on2.vp9' : 'vp9',
        }
        def get_role(is_encoder, mime):
            """Returns the role based on is_encoder and mime.

            The mapping from a pair (is_encoder, mime) to a role string is
            defined in frameworks/av/media/libmedia/MediaDefs.cpp and
            frameworks/av/media/libstagefright/omx/OMXUtils.cpp. This function
            does essentially the same work as GetComponentRole() in
            OMXUtils.cpp.

            Args:
              is_encoder: A boolean indicating whether the role is for an
                  encoder or a decoder.
              mime: A string of the desired mime type.

            Returns:
              A string for the requested role name, or None if mime is not
              recognized.
            """
            mime_suffix = mime[6:]
            middle = 'encoder.' if is_encoder else 'decoder.'
            if mime.startswith('audio/'):
                if mime_suffix not in audio_mime_to_role:
                    return None
                prefix = 'audio_'
                suffix = audio_mime_to_role[mime_suffix]
            elif mime.startswith('video/'):
                if mime_suffix not in video_mime_to_role:
                    return None
                prefix = 'video_'
                suffix = video_mime_to_role[mime_suffix]
            else:
                return None
            return prefix + middle + suffix

        # The test code starts here.
        roles = self.omxstore.listRoles()
        if len(roles) == 0:
            logging.warning('IOmxStore has an empty implementation. Skipping...')
            return

        # A map from a node name to a set of roles.
        node2roles = {}

        # A map from an owner to a set of node names.
        owner2nodes = {}

        logging.info('Testing IOmxStore::listRoles()...')
        # role_set is used for checking if there are duplicate roles.
        role_set = set()
        for role in roles:
            role_name = role['role']
            mime_type = role['type']
            is_encoder = role['isEncoder']
            nodes = role['nodes']

            # The role name must not have duplicates.
            asserts.assertFalse(
                role_name in role_set,
                'Role "' + role_name + '" has duplicates.')

            queried_role = get_role(is_encoder, mime_type)
            # type and isEncoder must be known.
            asserts.assertTrue(
                queried_role,
                'Invalid mime type "' + mime_type + '" for ' +
                ('an encoder.' if is_encoder else 'a decoder.'))
            # type and isEncoder must be consistent with role.
            asserts.assertEqual(
                role_name, queried_role,
                'Role "' + role_name + '" does not match ' +
                ('an encoder ' if is_encoder else 'a decoder ') +
                'for mime type "' + mime_type + '"')

            # Save the role name to check for duplicates.
            role_set.add(role_name)

            # Ignore role.preferPlatformNodes for now.

            # node_set is used for checking if there are duplicate node names
            # for each role.
            node_set = set()
            for node in nodes:
                node_name = node['name']
                owner = node['owner']
                attributes = node['attributes']

                # For each role, the node name must not have duplicates.
                asserts.assertFalse(
                    node_name in node_set,
                    'Node "' + node_name + '" has duplicates for the same ' +
                    'role "' + queried_role + '".')

                # Check the format of node name
                asserts.assertTrue(
                    node_name_re.match(node_name),
                    'Node name "' + node_name + '" is invalid.')
                # Check the format of node owner
                asserts.assertTrue(
                    node_owner_re.match(owner),
                    'Node owner "' + owner + '" is invalid.')

                attr_map = {}
                for attr in attributes:
                    attr_key = attr['key']
                    attr_value = attr['value']

                    # For each node and each role, the attribute key must not
                    # have duplicates.
                    asserts.assertFalse(
                        attr_key in attr_map,
                        'Attribute "' + attr_key +
                        '" for node "' + node_name +
                        '"has duplicates.')

                    # Check the value against the corresponding regular
                    # expression.
                    if attr_key in attr_re:
                        asserts.assertTrue(
                            attr_re[attr_key].match(attr_value),
                            'Attribute "' + attr_key + '" has ' +
                            'invalid value "' + attr_value + '".')
                    else:
                        key_found = False
                        for pattern_key, pattern_value in attr_pattern_re:
                            if pattern_key.match(attr_key):
                                asserts.assertTrue(
                                    pattern_value.match(attr_value),
                                    'Attribute "' + attr_key + '" has ' +
                                    'invalid value "' + attr_value + '".')
                                key_found = True
                                break
                        if not key_found:
                            logging.warning(
                                'Unknown attribute "' +
                                attr_key + '" with value "' +
                                attr_value + '".')

                    # Store the key-value pair
                    attr_map[attr_key] = attr_value

                if node_name not in node2roles:
                    node2roles[node_name] = {queried_role,}
                    if owner not in owner2nodes:
                        owner2nodes[owner] = {node_name,}
                    else:
                        owner2nodes[owner].add(node_name)
                else:
                    node2roles[node_name].add(queried_role)

        # Verify the information with IOmx::listNodes().
        # IOmxStore::listRoles() and IOmx::listNodes() should give consistent
        # information about nodes and roles.
        logging.info('Verifying with IOmx::listNodes()...')
        for owner in owner2nodes:
            # Obtain the IOmx instance for each "owner"
            omx = self.omxstore.getOmx(owner)
            asserts.assertTrue(
                omx,
                'Cannot obtain IOmx instance "' + owner + '".')

            # Invoke IOmx::listNodes()
            status, node_info_list = omx.listNodes()
            asserts.assertEqual(
                self.vtypes.Status.OK, status,
                'IOmx::listNodes() fails for IOmx instance "' + owner + '".')

            # Verify that roles for each node match with the information from
            # IOmxStore::listRoles().
            node_set = set()
            for node_info in node_info_list:
                node = node_info['mName']
                roles = node_info['mRoles']

                # IOmx::listNodes() should not list duplicate node names.
                asserts.assertFalse(
                    node in node_set,
                    'IOmx::listNodes() lists duplicate nodes "' + node + '".')
                node_set.add(node)

                # Skip "hidden" nodes, i.e. those that are not advertised by
                # IOmxStore::listRoles().
                if node not in owner2nodes[owner]:
                    logging.warning(
                        'IOmx::listNodes() lists unknown node "' + node +
                        '" for IOmx instance "' + owner + '".')
                    continue

                # All the roles advertised by IOmxStore::listRoles() for this
                # node must be included in role_set.
                role_set = set(roles)
                asserts.assertTrue(
                    node2roles[node] <= role_set,
                    'IOmx::listNodes() for IOmx instance "' + owner + '" ' +
                    'does not report some roles for node "' + node + '": ' +
                    ', '.join(node2roles[node] - role_set))

                # Try creating the node.
                status, omxNode = omx.allocateNode(node, None)
                asserts.assertEqual(
                    self.vtypes.Status.OK, status,
                    'IOmx::allocateNode() for IOmx instance "' + owner + '" ' +
                    'fails to allocate node "' + node +'".')


            # Check that all nodes obtained from IOmxStore::listRoles() are
            # supported by the their corresponding IOmx instances.
            node_set_diff = owner2nodes[owner] - node_set
            asserts.assertFalse(
                node_set_diff,
                'IOmx::listNodes() for IOmx instance "' + owner + '" ' +
                'does not report some expected nodes: ' +
                ', '.join(node_set_diff) + '.')

        # Call IOmxStore::getNodePrefix().
        prefix = self.omxstore.getNodePrefix()
        logging.info('Checking node prefix: ' +
                     'IOmxStore::getNodePrefix() returns "' + prefix + '".')

        # Check that the prefix is a sensible string.
        asserts.assertTrue(
            node_name_re.match(prefix),
            '"' + prefix + '" is not a valid prefix for node names.')

        # Check that all node names have the said prefix.
        for node in node2roles:
            asserts.assertTrue(
                node.startswith(prefix),
                'Node "' + node + '" does not start with ' +
                'prefix "' + prefix + '".')

if __name__ == '__main__':
    test_runner.main()
