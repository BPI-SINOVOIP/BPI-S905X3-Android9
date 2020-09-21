import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class platform_ActivateDate(test.test):
    """
    Tests that activate date is run correctly from a clean system state.
    For context, activate date is an upstart script located in:
        chromeos-base/chromeos-activate-date
    It attempts to set the initial date when a new chromebook is first used
    by the customer, which drives things like battery warranty issues.
    """
    version = 1

    def run_once(self, host):
        host.run('truncate -s 0 /var/log/messages')
        host.run('activate_date --clean')

        # Since the activate_date value was cleaned, this reboot will cause it
        # to execute the activation sequence.
        host.reboot()

        # The activation sequence polls with a 200 second sleep, which seems
        # pretty high, but we'll wait for at least one of those sleep cycles.
        poll_interval = 10
        max_num_attempts = 21

        current_attempt = 1
        success = False
        while current_attempt <= max_num_attempts and not success:
          # Autotest logs the grep command to the system log, so we're just
          # going to search for it locally.
          get_log_cmd = 'cat /var/log/messages'
          success = 'Activation date set' in host.run(get_log_cmd).stdout
          if not success:
            current_attempt = current_attempt + 1
            time.sleep(poll_interval)

        if not success:
          error.TestFail('Failed to set activation date')

        # After this reboot, vpd_get_value ActivateDate should be set already
        # because this is what prevents the init script from executing again.
        host.reboot()

        if not host.run('vpd_get_value ActivateDate').stdout:
          error.TestFail('ActivateDate should be set correctly after reboot')
