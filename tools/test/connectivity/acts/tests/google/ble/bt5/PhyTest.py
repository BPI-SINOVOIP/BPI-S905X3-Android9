#/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
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
"""
This test script exercises set PHY and read PHY procedures.
"""

from queue import Empty

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.GattConnectedBaseTest import GattConnectedBaseTest
from acts.test_utils.bt.bt_constants import gatt_connection_priority
from acts.test_utils.bt.bt_constants import gatt_event
from acts.test_utils.bt.bt_constants import gatt_phy
from acts import signals

CONNECTION_PRIORITY_HIGH = gatt_connection_priority['high']
PHY_LE_1M = gatt_phy['1m']
PHY_LE_2M = gatt_phy['2m']


def lfmt(txPhy, rxPhy):
    return '(' + list(gatt_phy.keys())[list(gatt_phy.values()).index(
        txPhy)] + ', ' + list(gatt_phy.keys())[list(gatt_phy.values()).index(
            rxPhy)] + ')'


class PhyTest(GattConnectedBaseTest):
    def setup_class(self):
        if not self.cen_ad.droid.bluetoothIsLe2MPhySupported():
            raise signals.TestSkipClass(
                "Central device does not support LE 2M PHY")

        if not self.per_ad.droid.bluetoothIsLe2MPhySupported():
            raise signals.TestSkipClass(
                "Peripheral device does not support LE 2M PHY")

    # Some controllers auto-update PHY to 2M, and both client and server
    # might receive PHY Update event right after connection, if the
    # connection was established over 1M PHY. We will ignore this event, but
    # must pop it from queue.
    def pop_initial_phy_update(self):
        try:
            maybe_event = gatt_event['phy_update']['evt'].format(
                self.gatt_callback)
            self.cen_ad.ed.pop_event(maybe_event, 0)
        except Empty:
            pass

        try:
            maybe_event = gatt_event['serv_phy_update']['evt'].format(
                self.gatt_server_callback)
            self.per_ad.ed.pop_event(maybe_event, 0)
        except Empty:
            pass

    # this helper method checks wether both client and server received PHY
    # update event with proper txPhy and rxPhy
    def ensure_both_updated_phy(self, clientTxPhy, clientRxPhy):
        event = self._client_wait(gatt_event['phy_update'])
        txPhy = event['data']['TxPhy']
        rxPhy = event['data']['RxPhy']
        self.log.info("\tClient PHY updated: " + lfmt(txPhy, rxPhy))
        self.assertEqual(0, event['data']['Status'], "Status should be 0")
        self.assertEqual(clientTxPhy, event['data']['TxPhy'])
        self.assertEqual(clientRxPhy, event['data']['RxPhy'])

        bt_device_id = 0
        event = self._server_wait(gatt_event['serv_phy_update'])
        txPhy = event['data']['TxPhy']
        rxPhy = event['data']['RxPhy']
        self.log.info("\tServer PHY updated: " + lfmt(txPhy, rxPhy))
        self.assertEqual(0, event['data']['Status'], "Status should be 0")
        self.assertEqual(clientRxPhy, event['data']['TxPhy'])
        self.assertEqual(clientTxPhy, event['data']['RxPhy'])

    # read the client phy, return (txPhy, rxPhy)
    def read_client_phy(self):
        self.cen_ad.droid.gattClientReadPhy(self.bluetooth_gatt)
        event = self._client_wait(gatt_event['phy_read'])
        self.assertEqual(0, event['data']['Status'], "Status should be 0")
        return (event['data']['TxPhy'], event['data']['RxPhy'])

    # read the server phy, return (txPhy, rxPhy)
    def read_server_phy(self):
        bt_device_id = 0
        self.per_ad.droid.gattServerReadPhy(self.gatt_server, bt_device_id)
        event = self._server_wait(gatt_event['serv_phy_read'])
        self.assertEqual(0, event['data']['Status'], "Status should be 0")
        return (event['data']['TxPhy'], event['data']['RxPhy'])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='edb95ae1-97e5-4337-9a60-1e113aa43a4d')
    def test_phy_read(self):
        """Test LE read PHY.

        Test LE read PHY.

        Steps:
        1. Central, Peripheral : read PHY, make sure values are same.
        2. Central: update PHY.
        3. Ensure both Central and Peripheral received PHY update event.
        4. Central, Peripheral: read PHY, make sure values are same.

        Expected Result:
        Verify that read PHY works properly.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, PHY
        Priority: 0
        """
        self.cen_ad.droid.gattClientRequestConnectionPriority(
            self.bluetooth_gatt, CONNECTION_PRIORITY_HIGH)
        self.pop_initial_phy_update()

        # read phy from client and server, make sure they're same
        cTxPhy, cRxPhy = self.read_client_phy()
        sTxPhy, sRxPhy = self.read_server_phy()
        self.assertEqual(cTxPhy, sTxPhy)
        self.assertEqual(cRxPhy, sRxPhy)

        self.log.info("Initial connection PHY was: " + lfmt(cTxPhy, cRxPhy))

        nextTxPhy = (cTxPhy == PHY_LE_1M) and PHY_LE_2M or PHY_LE_1M
        nextRxPhy = (cRxPhy == PHY_LE_1M) and PHY_LE_2M or PHY_LE_1M

        # try to update PHY from Client
        self.log.info("Will try to set PHY to: " + lfmt(nextTxPhy, nextRxPhy))
        self.cen_ad.droid.gattClientSetPreferredPhy(self.bluetooth_gatt,
                                                    nextTxPhy, nextRxPhy, 0)
        self.ensure_both_updated_phy(nextTxPhy, nextRxPhy)

        # read phy on client and server, make sure values are same and equal
        # the newly set value
        cTxPhy, cRxPhy = self.read_client_phy()
        sTxPhy, sRxPhy = self.read_server_phy()
        self.assertEqual(cTxPhy, sTxPhy)
        self.assertEqual(cRxPhy, sRxPhy)

        self.assertEqual(nextTxPhy, cTxPhy)
        self.assertEqual(nextRxPhy, cRxPhy)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6b66af0a-35eb-42af-acd5-9634684f275d')
    def test_phy_change_20_times(self):
        """Test PHY update.

        Test LE PHY update.

        Steps:
        1. Central: read PHY.
        2. Central: update PHY to 1M, 2M, 1M... 20 times, each time ensuring
                    both client and server received PHY update event.

        Expected Result:
        Verify that read update PHY worked properly each time.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, PHY
        Priority: 0
        """
        self.cen_ad.droid.gattClientRequestConnectionPriority(
            self.bluetooth_gatt, CONNECTION_PRIORITY_HIGH)
        self.pop_initial_phy_update()

        txPhyB, rxPhyB = self.read_client_phy()
        txPhyA = (txPhyB == PHY_LE_1M) and PHY_LE_2M or PHY_LE_1M
        rxPhyA = (rxPhyB == PHY_LE_1M) and PHY_LE_2M or PHY_LE_1M

        self.log.info("Initial connection PHY was: " + lfmt(txPhyB, rxPhyB))

        for i in range(20):
            #swap values between iterations
            txPhy = (i & 1) and txPhyB or txPhyA
            rxPhy = (i & 1) and rxPhyB or rxPhyA

            self.log.info("Will try to set PHY to: " + lfmt(txPhy, rxPhy))
            self.cen_ad.droid.gattClientSetPreferredPhy(self.bluetooth_gatt,
                                                        txPhy, rxPhy, 0)
            self.ensure_both_updated_phy(txPhy, rxPhy)
        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='13f28de4-07f4-458c-a3e5-3ba95318616f')
    def test_phy_change_asym(self):
        """Test PHY update with asymetric rx and tx PHY.

        Test PHY update with asymetric rx and tx PHY.

        Steps:
        1. Central: read PHY.
        2. Central: update PHY to tx: 1M, rx: 2M, ensure both devices received
                    the asymetric update.
        3. Central: update PHY to tx: 2M, rx: 1M, ensure both devices received
                    the asymetric update.

        Expected Result:
        Verify that read update PHY worked properly each time.

        Returns:
          Pass if True
          Fail if False

        TAGS: LE, PHY
        Priority: 0
        """
        self.cen_ad.droid.gattClientRequestConnectionPriority(
            self.bluetooth_gatt, CONNECTION_PRIORITY_HIGH)
        self.pop_initial_phy_update()

        txPhy, rxPhy = self.read_client_phy()

        self.log.info("Initial connection PHY was: " + lfmt(txPhy, rxPhy))
        self.log.info("will try to set PHY to: PHY_LE_1M, PHY_LE_2M")

        #try to update PHY to tx 1M, rx 2M from Client
        self.cen_ad.droid.gattClientSetPreferredPhy(self.bluetooth_gatt,
                                                    PHY_LE_1M, PHY_LE_2M, 0)
        self.ensure_both_updated_phy(PHY_LE_1M, PHY_LE_2M)

        #try to update PHY to TX 2M, RX 1M from Client
        self.log.info("will try to set PHY to: PHY_LE_2M, PHY_LE_1M")
        self.cen_ad.droid.gattClientSetPreferredPhy(self.bluetooth_gatt,
                                                    PHY_LE_2M, PHY_LE_1M, 0)
        self.ensure_both_updated_phy(PHY_LE_2M, PHY_LE_1M)

        return True
