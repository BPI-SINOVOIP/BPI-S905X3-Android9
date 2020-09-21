# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
CM_07 Validation of Status in Case of an Unsupported CID in MBIM_COMMAND_MSG

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 40
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""
import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_channel
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_control
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.tests import test


class CM07Test(test.Test):
    """ Implement the CM_07 test. """

    def run_internal(self):
        """ Run CM_07 test. """
        # Precondition
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.test_context).run()

        # Step 1
        # 255 is an unsupported CID.
        command_message = mbim_control.MBIMCommandMessage(
                message_length=48,
                total_fragments=1,
                current_fragment=0,
                device_service_id=mbim_constants.UUID_BASIC_CONNECT.bytes,
                cid=255,
                command_type=mbim_constants.COMMAND_TYPE_QUERY,
                information_buffer_length=0)
        packets = command_message.generate_packets()
        channel = mbim_channel.MBIMChannel(
                {'idVendor': self.test_context.id_vendor,
                 'idProduct': self.test_context.id_product},
                self.test_context.mbim_communication_interface.bInterfaceNumber,
                self.test_context.interrupt_endpoint.bEndpointAddress,
                self.test_context.mbim_functional.wMaxControlMessage)
        response_packets = channel.bidirectional_transaction(*packets)
        channel.close()

        # Step 2
        response_message = mbim_control.parse_response_packets(response_packets)

        # Step 3
        if (response_message.message_type != mbim_constants.MBIM_COMMAND_DONE or
            (response_message.status_codes !=
             mbim_constants.MBIM_STATUS_NO_DEVICE_SUPPORT)):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.4.5#2')
