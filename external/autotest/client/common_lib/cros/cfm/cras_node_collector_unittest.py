import unittest
import mock

from autotest_lib.client.common_lib.cros.cfm import cras_node_collector

# pylint: disable=missing-docstring

class CrasNodeCollectorTest(unittest.TestCase):
    """Unit tests for cras_node_collector."""

    def test_collect_output(self):
        output_devices = (
            'Output Devices:\n'
            'ID Name\n'
            '13 HDA Intel PCH: ALC283 Analog:3,0\n'
            '12 HDA Intel HDMI: HDMI 2:2,8\n'
            '11 HDA Intel HDMI: HDMI 1:2,7\n'
            '10 HDA Intel HDMI: HDMI 0:2,3\n'
            '8  Jabra SPEAK 410 USB: USB Audio:1,0\n'
            '6  Jabra SPEAK 410 USB: USB Audio:0,0\n'
            'Output Nodes:')

        output_nodes = (
            'Output Nodes:\n'
            'Stable Id   ID  Vol   Plugged  L/R swapped       Time Hotword '
            'Type        Name\n'
            '(d5da8553) 13:0      100       no              no           0 '
            'HEADPHONE        Headphone Jack\n'
            '(5280b26d) 12:0      100       no              no           0 '
            'HDMI             HDMI/DP,pcm=8 Jack\n'
            '(78297d51) 11:0      100       no              no           0 '
            'HDMI             HDMI/DP,pcm=7 Jack\n'
            '(f2070728) 10:0      100       no              no  1511264933 '
            'HDMI             HDMI/DP,pcm=3 Jack\n'
            '(2fb77ef4) 10:1      100       no              no           0 '
            'HDMI             HDMI\n'
            '(051d1a79) 8:0    75      yes              no  1511264933 '
            'USB             *(default)\n'
            '(051d1a79) 6:0   100      yes              no  1511264933 '
            'USB              (default)\n'
            'Input Devices:')

        mock_host = mock.Mock()
        mock_host.run_output.side_effect = [output_devices, output_nodes]
        collector = cras_node_collector.CrasNodeCollector(mock_host)

        nodes = collector.get_output_nodes()
        self.assertEquals(7, len(nodes))
        node_ids = set([node.node_id for node in nodes])
        node_types = set([node.node_type for node in nodes])
        device_names = set([node.device_name for node in nodes])
        self.assertEquals(node_ids, set(
            ['13:0', '12:0', '11:0', '10:0', '10:1', '8:0', '6:0']))
        self.assertEquals(node_types, set(['HEADPHONE', 'USB', 'HDMI' ]))

    def test_collect_input(self):
        input_devices = (
          'Input Devices:\n'
          'ID   Name\n'
          '14   HDA Intel PCH: ALC283 Analog:3,0\n'
          '9    Jabra SPEAK 410 USB: USB Audio:1,0\n'
          '7    Jabra SPEAK 410 USB: USB Audio:0,0\n'
          '5    Post DSP Loopback\n'
          '4    Post Mix Pre DSP Loopback\n'
          'Input Nodes:'
        )
        input_nodes = (
          'Input Nodes:\n'
          'Stable Id     ID Gain   Plugged  L/R swapped       Time Hotword '
          'Type      Name\n'
          '(ca711b33)   14:0        0       no              no           0 '
          'MIC              Mic Jack\n'
          '(4b5d44d7)   9:0     0      yes              no  1511264933 '
          'USB              Jabra SPEAK 410 USB: USB Audio:1,0: Mic\n'
          '(ca24f0a3)   7:0     0      yes              no  1511264933 '
          'USB             *Jabra SPEAK 410 USB: USB Audio:0,0: Mic\n'
          '(8c74f766)   5:0     0      yes              no           0 '
          'POST_DSP_LOOPBACK Post DSP Loopback\n'
          '(8a04af91)   4:0     0      yes              no           0 '
          'POST_MIX_LOOPBACK Post Mix Pre DSP Loopback\n'
          'Attached clients:'
        )

        mock_host = mock.Mock()
        mock_host.run_output.side_effect = [input_devices, input_nodes]
        collector = cras_node_collector.CrasNodeCollector(mock_host)

        nodes = collector.get_input_nodes()
        self.assertEquals(5, len(nodes))
        node_ids = set([node.node_id for node in nodes])
        node_types = set([node.node_type for node in nodes])
        device_names = set([node.device_name for node in nodes])
        self.assertEquals(node_ids, set(
            ['4:0', '5:0', '7:0', '9:0', '14:0']))
        self.assertEquals(node_types, set(
            ['MIC', 'USB', 'POST_DSP_LOOPBACK', 'POST_MIX_LOOPBACK']))
        self.assertEquals(device_names, set(
            ['HDA Intel PCH: ALC283 Analog:3,0',
             'Jabra SPEAK 410 USB: USB Audio:1,0',
             'Jabra SPEAK 410 USB: USB Audio:0,0',
             'Post DSP Loopback',
             'Post Mix Pre DSP Loopback',]))

if __name__ == "__main__":
    unittest.main()
