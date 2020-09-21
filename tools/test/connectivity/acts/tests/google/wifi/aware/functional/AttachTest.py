#!/usr/bin/python3.4
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time

from acts import asserts
from acts import utils
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class AttachTest(AwareBaseTest):
  def __init__(self, controllers):
    AwareBaseTest.__init__(self, controllers)

  @test_tracker_info(uuid="cdafd1e0-bcf5-4fe8-ae32-f55483db9925")
  def test_attach(self):
    """Functional test case / Attach test cases / attach

    Validates that attaching to the Wi-Fi Aware service works (receive
    the expected callback).
    """
    dut = self.android_devices[0]
    dut.droid.wifiAwareAttach(False)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)
    autils.fail_on_event(dut, aconsts.EVENT_CB_ON_IDENTITY_CHANGED)

  @test_tracker_info(uuid="82f2a8bc-a62b-49c2-ac8a-fe8460010ba2")
  def test_attach_with_identity(self):
    """Functional test case / Attach test cases / attach with identity callback

    Validates that attaching to the Wi-Fi Aware service works (receive
    the expected callbacks).
    """
    dut = self.android_devices[0]
    dut.droid.wifiAwareAttach(True)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_IDENTITY_CHANGED)

  @test_tracker_info(uuid="d2714d14-f330-47d4-b8e9-ee4d5e5b7ea0")
  def test_attach_multiple_sessions(self):
    """Functional test case / Attach test cases / multiple attach sessions

    Validates that when creating multiple attach sessions each can be
    configured independently as to whether or not to receive an identity
    callback.
    """
    dut = self.android_devices[0]

    # Create 3 attach sessions: 2 without identity callback, 1 with
    id1 = dut.droid.wifiAwareAttach(False, None, True)
    time.sleep(10) # to make sure all calls and callbacks are done
    id2 = dut.droid.wifiAwareAttach(True, None, True)
    time.sleep(10) # to make sure all calls and callbacks are done
    id3 = dut.droid.wifiAwareAttach(False, None, True)
    dut.log.info('id1=%d, id2=%d, id3=%d', id1, id2, id3)

    # Attach session 1: wait for attach, should not get identity
    autils.wait_for_event(dut,
                          autils.decorate_event(aconsts.EVENT_CB_ON_ATTACHED,
                                                id1))
    autils.fail_on_event(dut,
                         autils.decorate_event(
                             aconsts.EVENT_CB_ON_IDENTITY_CHANGED, id1))

    # Attach session 2: wait for attach and for identity callback
    autils.wait_for_event(dut,
                          autils.decorate_event(aconsts.EVENT_CB_ON_ATTACHED,
                                                id2))
    autils.wait_for_event(dut,
                          autils.decorate_event(
                              aconsts.EVENT_CB_ON_IDENTITY_CHANGED, id2))

    # Attach session 3: wait for attach, should not get identity
    autils.wait_for_event(dut,
                          autils.decorate_event(aconsts.EVENT_CB_ON_ATTACHED,
                                                id3))
    autils.fail_on_event(dut,
                         autils.decorate_event(
                             aconsts.EVENT_CB_ON_IDENTITY_CHANGED, id3))

  @test_tracker_info(uuid="b8ea4d02-ae23-42a7-a85e-def52932c858")
  def test_attach_with_no_wifi(self):
    """Function test case / Attach test cases / attempt to attach with wifi off

    Validates that if trying to attach with Wi-Fi disabled will receive the
    expected failure callback. As a side-effect also validates that the
    broadcast for Aware unavailable is received.
    """
    dut = self.android_devices[0]
    wutils.wifi_toggle_state(dut, False)
    autils.wait_for_event(dut, aconsts.BROADCAST_WIFI_AWARE_NOT_AVAILABLE)
    dut.droid.wifiAwareAttach()
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACH_FAILED)

  @test_tracker_info(uuid="7dcc4530-c936-4447-9d22-a7c5b315e2ce")
  def test_attach_with_doze(self):
    """Function test case / Attach test cases / attempt to attach with doze on

    Validates that if trying to attach with device in doze mode will receive the
    expected failure callback. As a side-effect also validates that the
    broadcast for Aware unavailable is received.
    """
    dut = self.android_devices[0]
    asserts.assert_true(utils.enable_doze(dut), "Can't enable doze")
    autils.wait_for_event(dut, aconsts.BROADCAST_WIFI_AWARE_NOT_AVAILABLE)
    dut.droid.wifiAwareAttach()
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACH_FAILED)
    asserts.assert_true(utils.disable_doze(dut), "Can't disable doze")
    autils.wait_for_event(dut, aconsts.BROADCAST_WIFI_AWARE_AVAILABLE)

  @test_tracker_info(uuid="2574fd01-8974-4dd0-aeb8-a7194461140e")
  def test_attach_with_location_off(self):
    """Function test case / Attach test cases / attempt to attach with location
    mode off.

    Validates that if trying to attach with device location mode off will
    receive the expected failure callback. As a side-effect also validates that
    the broadcast for Aware unavailable is received.
    """
    dut = self.android_devices[0]
    utils.set_location_service(dut, False)
    autils.wait_for_event(dut, aconsts.BROADCAST_WIFI_AWARE_NOT_AVAILABLE)
    dut.droid.wifiAwareAttach()
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACH_FAILED)
    utils.set_location_service(dut, True)
    autils.wait_for_event(dut, aconsts.BROADCAST_WIFI_AWARE_AVAILABLE)

  @test_tracker_info(uuid="7ffde8e7-a010-4b77-97f5-959f263b5249")
  def test_attach_apm_toggle_attach_again(self):
    """Validates that enabling Airplane mode while Aware is on resets it
    correctly, and allows it to be re-enabled when Airplane mode is then
    disabled."""
    dut = self.android_devices[0]

    # enable Aware (attach)
    dut.droid.wifiAwareAttach()
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)

    # enable airplane mode
    utils.force_airplane_mode(dut, True)
    autils.wait_for_event(dut, aconsts.BROADCAST_WIFI_AWARE_NOT_AVAILABLE)

    # wait a few seconds and disable airplane mode
    time.sleep(10)
    utils.force_airplane_mode(dut, False)
    autils.wait_for_event(dut, aconsts.BROADCAST_WIFI_AWARE_AVAILABLE)

    # try enabling Aware again (attach)
    dut.droid.wifiAwareAttach()
    autils.wait_for_event(dut, aconsts.EVENT_CB_ON_ATTACHED)
