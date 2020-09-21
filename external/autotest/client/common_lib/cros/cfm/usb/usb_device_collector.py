import cStringIO

from autotest_lib.client.common_lib.cros import textfsm
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device


class UsbDeviceCollector(object):
    """Utility class for obtaining info about connected USB devices."""

    USB_DEVICES_TEMPLATE = (
        'Value Required Vendor ([0-9a-fA-F]+)\n'
        'Value Required ProdID ([0-9A-Fa-f]+)\n'
        'Value Required prev ([0-9a-fA-Z.]+)\n'
        'Value Required Bus ([0-9.]+)\n'
        'Value Required Port ([0-9.]+)\n'
        'Value Required Lev ([0-9.]+)\n'
        'Value Required Dev ([0-9.]+)\n'
        'Value Required Prnt ([0-9.]+)\n'
        'Value Manufacturer (.+)\n'
        'Value Product (.+)\n'
        'Value serialnumber ([0-9a-fA-Z\:\-]+)\n'
        'Value cinterfaces (\d)\n'
        'Value List intindex ([0-9])\n'
        'Value List intdriver ([A-Za-z-\(\)]+)\n\n'
        'Start\n'
        '  ^USB-Device -> Continue.Record\n'
        '  ^T:\s+Bus=${Bus}\s+Lev=${Lev}\s+Prnt=${Prnt}'
        '\s+Port=${Port}.*Dev#=\s*${Dev}.*\n'
        '  ^P:\s+Vendor=${Vendor}\s+ProdID=${ProdID}\sRev=${prev}\n'
        '  ^S:\s+Manufacturer=${Manufacturer}\n'
        '  ^S:\s+Product=${Product}\n'
        '  ^S:\s+SerialNumber=${serialnumber}\n'
        '  ^C:\s+\#Ifs=\s+${cinterfaces}\n'
        '  ^I:\s+If\#=\s+${intindex}.*Driver=${intdriver}\n'
    )

    def __init__(self, host):
        """
        Constructor
        @param host the DUT.
        """
        self._host = host

    def _extract_usb_data(self, rawdata):
      """
      Populate usb data into a list of dictionaries.
      @param rawdata The output of "usb-devices" on CfM.
      @returns list of dictionary, example dictionary:
      {'Manufacturer': 'USBest Technology',
      'Product': 'SiS HID Touch Controller',
      'Vendor': '266e',
      'intindex': ['0'],
      'tport': '00',
      'tcnt': '01',
      'serialnumber': '',
      'tlev': '03',
      'tdev': '18',
      'dver': '',
      'intdriver': ['usbhid'],
      'tbus': '01',
      'prev': '03.00',
      'cinterfaces': '1',
      'ProdID': '0110',
      'tprnt': '14'}
      """
      usbdata = []
      rawdata += '\n'
      re_table = textfsm.TextFSM(cStringIO.StringIO(self.USB_DEVICES_TEMPLATE))
      fsm_results = re_table.ParseText(rawdata)
      usbdata = [dict(zip(re_table.header, row)) for row in fsm_results]
      return usbdata

    def _collect_usb_device_data(self):
        """Collecting usb device data."""
        usb_devices = (self._host.run('usb-devices', ignore_status=True).
                       stdout.strip().split('\n\n'))
        return self._extract_usb_data(
            '\nUSB-Device\n'+'\nUSB-Device\n'.join(usb_devices))


    def _create_usb_device(self, usbdata):
        return usb_device.UsbDevice(
            vid=usbdata['Vendor'],
            pid=usbdata['ProdID'],
            product=usbdata.get('Product', 'Not available'),
            interfaces=usbdata['intdriver'],
            bus=int(usbdata['Bus']),
            level=int(usbdata['Lev']),
            # We increment here by 1 because usb-devices reports 0-indexed port
            # numbers where as lsusb reports 1-indexed. We opted to follow the
            # the lsusb standard.
            port=int(usbdata['Port']) + 1)

    def get_usb_devices(self):
        """
        Returns the list of UsbDevices connected to the DUT.
        @returns A list of UsbDevice instances.
        """
        usbdata = self._collect_usb_device_data()
        data_and_devices = []
        for data in usbdata:
            usb_device = self._create_usb_device(data)
            data_and_devices.append((data, usb_device))
        # Make a pass to populate parents of the UsbDevices.
        # We need parent ID and Device ID from the raw data since we do not
        # care about storing those in a UsbDevice. That's why we bother
        # iterating through the (data,UsbDevice) pairs.
        for data, usb_device in data_and_devices:
            parent_id = int(data['Prnt'])
            bus = usb_device.bus
            # Device IDs are not unique across busses. When finding a device's
            # parent we look for a device with the parent ID on the same bus.
            usb_device.parent = self._find_device_on_same_bus(
                    data_and_devices, parent_id, bus)
        return [x[1] for x in data_and_devices]

    def _find_device_on_same_bus(self, data_and_devices, device_id, bus):
        for data, usb_device in data_and_devices:
            if int(data['Dev']) == device_id and usb_device.bus == bus:
                return usb_device
        return None

    def get_devices_by_spec(self, *specs):
        """
        Returns all UsbDevices that match the any of the given specs.
        @param specs instances of UsbDeviceSpec.
        @returns a list UsbDevice instances.
        """
        spec_vid_pids = [spec.vid_pid for spec in specs]
        return [d for d in self.get_usb_devices()
                if d.vid_pid in spec_vid_pids]
