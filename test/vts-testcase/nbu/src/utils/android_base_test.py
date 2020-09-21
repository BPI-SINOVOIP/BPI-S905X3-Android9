"""Base class for automated android tests."""



from mobly import base_test
from mobly import logger
from mobly.controllers import android_device
from mobly.controllers.android_device_lib import adb

ANDROID_SNIPPET_PACKAGE = 'com.google.android.test'


class AndroidBaseTest(base_test.BaseTestClass):

  def log_bt_stack_conf(self, device):
    try:
      bt_stack_conf = device.adb.shell(['cat', '/etc/bluetooth/bt_stack.conf'])
    except adb.AdbError:
      device.log.debug('No /etc/bluetooth/bt_stack.conf file found on device.')
    else:
      device.log.debug('Content of /etc/bluetooth/bt_stack.conf: %s',
                       bt_stack_conf)

  def setup_class(self):
    # Registering android_device controller module, and declaring that the test
    # requires at least two Android devices.
    self.ads = self.register_controller(android_device, min_number=2)
    self.dut_a = self.ads[0]
    self.dut_b = self.ads[1]
    self.dut_a.load_snippet('android', ANDROID_SNIPPET_PACKAGE)
    self.dut_b.load_snippet('android', ANDROID_SNIPPET_PACKAGE)
    self.log_bt_stack_conf(self.dut_a)
    self.log_bt_stack_conf(self.dut_b)

  def on_fail(self, record):
    """Only executed after a test fails.

    Collect debug info here.

    Args:
      record: a copy of the test result record of the test.
    """
    begin_time = logger.epoch_to_log_line_timestamp(record.begin_time)
    android_device.take_bug_reports([self.dut_a, self.dut_b], record.test_name,
                                    begin_time)
