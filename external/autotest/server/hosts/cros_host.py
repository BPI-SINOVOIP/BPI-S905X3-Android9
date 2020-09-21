# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import sys
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import autotemp
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import hosts
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.client.common_lib.cros import autoupdater
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants as client_constants
from autotest_lib.client.cros import cros_ui
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.input_playback import input_playback
from autotest_lib.client.cros.video import constants as video_test_constants
from autotest_lib.server import afe_utils
from autotest_lib.server import autoserv_parser
from autotest_lib.server import autotest
from autotest_lib.server import constants
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants as ds_constants
from autotest_lib.server.cros.dynamic_suite import tools, frontend_wrappers
from autotest_lib.server.cros.faft.config.config import Config as FAFTConfig
from autotest_lib.server.cros.servo import firmware_programmer
from autotest_lib.server.cros.servo import plankton
from autotest_lib.server.hosts import abstract_ssh
from autotest_lib.server.hosts import base_label
from autotest_lib.server.hosts import cros_label
from autotest_lib.server.hosts import chameleon_host
from autotest_lib.server.hosts import cros_repair
from autotest_lib.server.hosts import plankton_host
from autotest_lib.server.hosts import servo_host
from autotest_lib.site_utils.rpm_control_system import rpm_client

# In case cros_host is being ran via SSP on an older Moblab version with an
# older chromite version.
try:
    from chromite.lib import metrics
except ImportError:
    metrics =  utils.metrics_mock


CONFIG = global_config.global_config
ENABLE_DEVSERVER_TRIGGER_AUTO_UPDATE = CONFIG.get_config_value(
        'CROS', 'enable_devserver_trigger_auto_update', type=bool,
        default=False)


class FactoryImageCheckerException(error.AutoservError):
    """Exception raised when an image is a factory image."""
    pass


class CrosHost(abstract_ssh.AbstractSSHHost):
    """Chromium OS specific subclass of Host."""

    VERSION_PREFIX = provision.CROS_VERSION_PREFIX

    _parser = autoserv_parser.autoserv_parser
    _AFE = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
    support_devserver_provision = ENABLE_DEVSERVER_TRIGGER_AUTO_UPDATE

    # Timeout values (in seconds) associated with various Chrome OS
    # state changes.
    #
    # In general, a good rule of thumb is that the timeout can be up
    # to twice the typical measured value on the slowest platform.
    # The times here have not necessarily been empirically tested to
    # meet this criterion.
    #
    # SLEEP_TIMEOUT:  Time to allow for suspend to memory.
    # RESUME_TIMEOUT: Time to allow for resume after suspend, plus
    #   time to restart the netwowrk.
    # SHUTDOWN_TIMEOUT: Time to allow for shut down.
    # BOOT_TIMEOUT: Time to allow for boot from power off.  Among
    #   other things, this must account for the 30 second dev-mode
    #   screen delay, time to start the network on the DUT, and the
    #   ssh timeout of 120 seconds.
    # USB_BOOT_TIMEOUT: Time to allow for boot from a USB device,
    #   including the 30 second dev-mode delay and time to start the
    #   network.
    # INSTALL_TIMEOUT: Time to allow for chromeos-install.
    # POWERWASH_BOOT_TIMEOUT: Time to allow for a reboot that
    #   includes powerwash.

    SLEEP_TIMEOUT = 2
    RESUME_TIMEOUT = 10
    SHUTDOWN_TIMEOUT = 10
    BOOT_TIMEOUT = 150
    USB_BOOT_TIMEOUT = 300
    INSTALL_TIMEOUT = 480
    POWERWASH_BOOT_TIMEOUT = 60

    # Minimum OS version that supports server side packaging. Older builds may
    # not have server side package built or with Autotest code change to support
    # server-side packaging.
    MIN_VERSION_SUPPORT_SSP = CONFIG.get_config_value(
            'AUTOSERV', 'min_version_support_ssp', type=int)

    # REBOOT_TIMEOUT: How long to wait for a reboot.
    #
    # We have a long timeout to ensure we don't flakily fail due to other
    # issues. Shorter timeouts are vetted in platform_RebootAfterUpdate.
    # TODO(sbasi - crbug.com/276094) Restore to 5 mins once the 'host did not
    # return from reboot' bug is solved.
    REBOOT_TIMEOUT = 480

    # _USB_POWER_TIMEOUT: Time to allow for USB to power toggle ON and OFF.
    # _POWER_CYCLE_TIMEOUT: Time to allow for manual power cycle.
    _USB_POWER_TIMEOUT = 5
    _POWER_CYCLE_TIMEOUT = 10

    _LAB_MACHINE_FILE = '/mnt/stateful_partition/.labmachine'
    _RPM_HOSTNAME_REGEX = ('chromeos(\d+)(-row(\d+))?-rack(\d+[a-z]*)'
                           '-host(\d+)')
    _LIGHTSENSOR_FILES = [ "in_illuminance0_input",
                           "in_illuminance_input",
                           "in_illuminance0_raw",
                           "in_illuminance_raw",
                           "illuminance0_input"]
    _LIGHTSENSOR_SEARCH_DIR = '/sys/bus/iio/devices'

    # Constants used in ping_wait_up() and ping_wait_down().
    #
    # _PING_WAIT_COUNT is the approximate number of polling
    # cycles to use when waiting for a host state change.
    #
    # _PING_STATUS_DOWN and _PING_STATUS_UP are names used
    # for arguments to the internal _ping_wait_for_status()
    # method.
    _PING_WAIT_COUNT = 40
    _PING_STATUS_DOWN = False
    _PING_STATUS_UP = True

    # Allowed values for the power_method argument.

    # POWER_CONTROL_RPM: Passed as default arg for power_off/on/cycle() methods.
    # POWER_CONTROL_SERVO: Used in set_power() and power_cycle() methods.
    # POWER_CONTROL_MANUAL: Used in set_power() and power_cycle() methods.
    POWER_CONTROL_RPM = 'RPM'
    POWER_CONTROL_SERVO = 'servoj10'
    POWER_CONTROL_MANUAL = 'manual'

    POWER_CONTROL_VALID_ARGS = (POWER_CONTROL_RPM,
                                POWER_CONTROL_SERVO,
                                POWER_CONTROL_MANUAL)

    _RPM_OUTLET_CHANGED = 'outlet_changed'

    # URL pattern to download firmware image.
    _FW_IMAGE_URL_PATTERN = CONFIG.get_config_value(
            'CROS', 'firmware_url_pattern', type=str)


    # A flag file to indicate provision failures.  The file is created
    # at the start of any AU procedure (see `machine_install()`).  The
    # file's location in stateful means that on successul update it will
    # be removed.  Thus, if this file exists, it indicates that we've
    # tried and failed in a previous attempt to update.
    PROVISION_FAILED = '/var/tmp/provision_failed'


    @staticmethod
    def check_host(host, timeout=10):
        """
        Check if the given host is a chrome-os host.

        @param host: An ssh host representing a device.
        @param timeout: The timeout for the run command.

        @return: True if the host device is chromeos.

        """
        try:
            result = host.run(
                    'grep -q CHROMEOS /etc/lsb-release && '
                    '! test -f /mnt/stateful_partition/.android_tester && '
                    '! grep -q moblab /etc/lsb-release',
                    ignore_status=True, timeout=timeout)
            if result.exit_status == 0:
                lsb_release_content = host.run(
                    'grep CHROMEOS_RELEASE_BOARD /etc/lsb-release',
                    timeout=timeout).stdout
                return not (
                    lsbrelease_utils.is_jetstream(
                        lsb_release_content=lsb_release_content) or
                    lsbrelease_utils.is_gce_board(
                        lsb_release_content=lsb_release_content))

        except (error.AutoservRunError, error.AutoservSSHTimeout):
            return False

        return False


    @staticmethod
    def get_chameleon_arguments(args_dict):
        """Extract chameleon options from `args_dict` and return the result.

        Recommended usage:
        ~~~~~~~~
            args_dict = utils.args_to_dict(args)
            chameleon_args = hosts.CrosHost.get_chameleon_arguments(args_dict)
            host = hosts.create_host(machine, chameleon_args=chameleon_args)
        ~~~~~~~~

        @param args_dict Dictionary from which to extract the chameleon
          arguments.
        """
        return {key: args_dict[key]
                for key in ('chameleon_host', 'chameleon_port')
                if key in args_dict}


    @staticmethod
    def get_plankton_arguments(args_dict):
        """Extract chameleon options from `args_dict` and return the result.

        Recommended usage:
        ~~~~~~~~
            args_dict = utils.args_to_dict(args)
            plankton_args = hosts.CrosHost.get_plankton_arguments(args_dict)
            host = hosts.create_host(machine, plankton_args=plankton_args)
        ~~~~~~~~

        @param args_dict Dictionary from which to extract the plankton
          arguments.
        """
        return {key: args_dict[key]
                for key in ('plankton_host', 'plankton_port')
                if key in args_dict}


    @staticmethod
    def get_servo_arguments(args_dict):
        """Extract servo options from `args_dict` and return the result.

        Recommended usage:
        ~~~~~~~~
            args_dict = utils.args_to_dict(args)
            servo_args = hosts.CrosHost.get_servo_arguments(args_dict)
            host = hosts.create_host(machine, servo_args=servo_args)
        ~~~~~~~~

        @param args_dict Dictionary from which to extract the servo
          arguments.
        """
        servo_attrs = (servo_host.SERVO_HOST_ATTR,
                       servo_host.SERVO_PORT_ATTR,
                       servo_host.SERVO_BOARD_ATTR)
        servo_args = {key: args_dict[key]
                      for key in servo_attrs
                      if key in args_dict}
        return (
            None
            if servo_host.SERVO_HOST_ATTR in servo_args
                and not servo_args[servo_host.SERVO_HOST_ATTR]
            else servo_args)


    def _initialize(self, hostname, chameleon_args=None, servo_args=None,
                    plankton_args=None, try_lab_servo=False,
                    try_servo_repair=False,
                    ssh_verbosity_flag='', ssh_options='',
                    *args, **dargs):
        """Initialize superclasses, |self.chameleon|, and |self.servo|.

        This method will attempt to create the test-assistant object
        (chameleon/servo) when it is needed by the test. Check
        the docstring of chameleon_host.create_chameleon_host and
        servo_host.create_servo_host for how this is determined.

        @param hostname: Hostname of the dut.
        @param chameleon_args: A dictionary that contains args for creating
                               a ChameleonHost. See chameleon_host for details.
        @param servo_args: A dictionary that contains args for creating
                           a ServoHost object. See servo_host for details.
        @param try_lab_servo: When true, indicates that an attempt should
                              be made to create a ServoHost for a DUT in
                              the test lab, even if not required by
                              `servo_args`. See servo_host for details.
        @param try_servo_repair: If a servo host is created, check it
                              with `repair()` rather than `verify()`.
                              See servo_host for details.
        @param ssh_verbosity_flag: String, to pass to the ssh command to control
                                   verbosity.
        @param ssh_options: String, other ssh options to pass to the ssh
                            command.
        """
        super(CrosHost, self)._initialize(hostname=hostname,
                                          *args, **dargs)
        self._repair_strategy = cros_repair.create_cros_repair_strategy()
        self.labels = base_label.LabelRetriever(cros_label.CROS_LABELS)
        # self.env is a dictionary of environment variable settings
        # to be exported for commands run on the host.
        # LIBC_FATAL_STDERR_ can be useful for diagnosing certain
        # errors that might happen.
        self.env['LIBC_FATAL_STDERR_'] = '1'
        self._ssh_verbosity_flag = ssh_verbosity_flag
        self._ssh_options = ssh_options
        # TODO(fdeng): We need to simplify the
        # process of servo and servo_host initialization.
        # crbug.com/298432
        self._servo_host = servo_host.create_servo_host(
                dut=self, servo_args=servo_args,
                try_lab_servo=try_lab_servo,
                try_servo_repair=try_servo_repair)
        if self._servo_host is not None:
            self.servo = self._servo_host.get_servo()
        else:
            self.servo = None

        # TODO(waihong): Do the simplication on Chameleon too.
        self._chameleon_host = chameleon_host.create_chameleon_host(
                dut=self.hostname, chameleon_args=chameleon_args)
        # Add plankton host if plankton args were added on command line
        self._plankton_host = plankton_host.create_plankton_host(plankton_args)

        if self._chameleon_host:
            self.chameleon = self._chameleon_host.create_chameleon_board()
        else:
            self.chameleon = None

        if self._plankton_host:
            self.plankton_servo = self._plankton_host.get_servo()
            logging.info('plankton_servo: %r', self.plankton_servo)
            # Create the plankton object used to access the ec uart
            self.plankton = plankton.Plankton(self.plankton_servo,
                    self._plankton_host.get_servod_server_proxy())
        else:
            self.plankton = None


    def _get_cros_repair_image_name(self):
        info = self.host_info_store.get()
        if info.board is None:
            raise error.AutoservError('Cannot obtain repair image name. '
                                      'No board label value found')

        return afe_utils.get_stable_cros_image_name(info.board)


    def host_version_prefix(self, image):
        """Return version label prefix.

        In case the CrOS provisioning version is something other than the
        standard CrOS version e.g. CrOS TH version, this function will
        find the prefix from provision.py.

        @param image: The image name to find its version prefix.
        @returns: A prefix string for the image type.
        """
        return provision.get_version_label_prefix(image)


    def verify_job_repo_url(self, tag=''):
        """
        Make sure job_repo_url of this host is valid.

        Eg: The job_repo_url "http://lmn.cd.ab.xyx:8080/static/\
        lumpy-release/R29-4279.0.0/autotest/packages" claims to have the
        autotest package for lumpy-release/R29-4279.0.0. If this isn't the case,
        download and extract it. If the devserver embedded in the url is
        unresponsive, update the job_repo_url of the host after staging it on
        another devserver.

        @param job_repo_url: A url pointing to the devserver where the autotest
            package for this build should be staged.
        @param tag: The tag from the server job, in the format
                    <job_id>-<user>/<hostname>, or <hostless> for a server job.

        @raises DevServerException: If we could not resolve a devserver.
        @raises AutoservError: If we're unable to save the new job_repo_url as
            a result of choosing a new devserver because the old one failed to
            respond to a health check.
        @raises urllib2.URLError: If the devserver embedded in job_repo_url
                                  doesn't respond within the timeout.
        """
        info = self.host_info_store.get()
        job_repo_url = info.attributes.get(ds_constants.JOB_REPO_URL, '')
        if not job_repo_url:
            logging.warning('No job repo url set on host %s', self.hostname)
            return

        logging.info('Verifying job repo url %s', job_repo_url)
        devserver_url, image_name = tools.get_devserver_build_from_package_url(
            job_repo_url)

        ds = dev_server.ImageServer(devserver_url)

        logging.info('Staging autotest artifacts for %s on devserver %s',
            image_name, ds.url())

        start_time = time.time()
        ds.stage_artifacts(image_name, ['autotest_packages'])
        stage_time = time.time() - start_time

        # Record how much of the verification time comes from a devserver
        # restage. If we're doing things right we should not see multiple
        # devservers for a given board/build/branch path.
        try:
            board, build_type, branch = server_utils.ParseBuildName(
                                                image_name)[:3]
        except server_utils.ParseBuildNameException:
            pass
        else:
            devserver = devserver_url[
                devserver_url.find('/') + 2:devserver_url.rfind(':')]
            stats_key = {
                'board': board,
                'build_type': build_type,
                'branch': branch,
                'devserver': devserver.replace('.', '_'),
            }

            monarch_fields = {
                'board': board,
                'build_type': build_type,
                # TODO(akeshet): To be consistent with most other metrics,
                # consider changing the following field to be named
                # 'milestone'.
                'branch': branch,
                'dev_server': devserver,
            }
            metrics.Counter(
                    'chromeos/autotest/provision/verify_url'
                    ).increment(fields=monarch_fields)
            metrics.SecondsDistribution(
                    'chromeos/autotest/provision/verify_url_duration'
                    ).add(stage_time, fields=monarch_fields)


    def stage_server_side_package(self, image=None):
        """Stage autotest server-side package on devserver.

        @param image: Full path of an OS image to install or a build name.

        @return: A url to the autotest server-side package.

        @raise: error.AutoservError if fail to locate the build to test with, or
                fail to stage server-side package.
        """
        # If enable_drone_in_restricted_subnet is False, do not set hostname
        # in devserver.resolve call, so a devserver in non-restricted subnet
        # is picked to stage autotest server package for drone to download.
        hostname = self.hostname
        if not server_utils.ENABLE_DRONE_IN_RESTRICTED_SUBNET:
            hostname = None
        if image:
            image_name = tools.get_build_from_image(image)
            if not image_name:
                raise error.AutoservError(
                        'Failed to parse build name from %s' % image)
            ds = dev_server.ImageServer.resolve(image_name, hostname)
        else:
            info = self.host_info_store.get()
            job_repo_url = info.attributes.get(ds_constants.JOB_REPO_URL, '')
            if job_repo_url:
                devserver_url, image_name = (
                    tools.get_devserver_build_from_package_url(job_repo_url))
                # If enable_drone_in_restricted_subnet is True, use the
                # existing devserver. Otherwise, resolve a new one in
                # non-restricted subnet.
                if server_utils.ENABLE_DRONE_IN_RESTRICTED_SUBNET:
                    ds = dev_server.ImageServer(devserver_url)
                else:
                    ds = dev_server.ImageServer.resolve(image_name)
            elif info.build is not None:
                ds = dev_server.ImageServer.resolve(info.build, hostname)
                image_name = info.build
            else:
                raise error.AutoservError(
                        'Failed to stage server-side package. The host has '
                        'no job_report_url attribute or version label.')

        # Get the OS version of the build, for any build older than
        # MIN_VERSION_SUPPORT_SSP, server side packaging is not supported.
        match = re.match('.*/R\d+-(\d+)\.', image_name)
        if match and int(match.group(1)) < self.MIN_VERSION_SUPPORT_SSP:
            raise error.AutoservError(
                    'Build %s is older than %s. Server side packaging is '
                    'disabled.' % (image_name, self.MIN_VERSION_SUPPORT_SSP))

        ds.stage_artifacts(image_name, ['autotest_server_package'])
        return '%s/static/%s/%s' % (ds.url(), image_name,
                                    'autotest_server_package.tar.bz2')


    def _try_stateful_update(self, update_url, force_update, updater):
        """Try to use stateful update to initialize DUT.

        When DUT is already running the same version that machine_install
        tries to install, stateful update is a much faster way to clean up
        the DUT for testing, compared to a full reimage. It is implemeted
        by calling autoupdater.run_update, but skipping updating root, as
        updating the kernel is time consuming and not necessary.

        @param update_url: url of the image.
        @param force_update: Set to True to update the image even if the DUT
            is running the same version.
        @param updater: ChromiumOSUpdater instance used to update the DUT.
        @returns: True if the DUT was updated with stateful update.

        """
        self.prepare_for_update()

        # TODO(jrbarnette):  Yes, I hate this re.match() test case.
        # It's better than the alternative:  see crbug.com/360944.
        image_name = autoupdater.url_to_image_name(update_url)
        release_pattern = r'^.*-release/R[0-9]+-[0-9]+\.[0-9]+\.0$'
        if not re.match(release_pattern, image_name):
            return False
        if not updater.check_version():
            return False
        if not force_update:
            logging.info('Canceling stateful update because the new and '
                         'old versions are the same.')
            return False
        # Following folders should be rebuilt after stateful update.
        # A test file is used to confirm each folder gets rebuilt after
        # the stateful update.
        folders_to_check = ['/var', '/home', '/mnt/stateful_partition']
        test_file = '.test_file_to_be_deleted'
        paths = [os.path.join(folder, test_file) for folder in folders_to_check]
        self.run('touch %s' % ' '.join(paths))

        updater.run_update(update_root=False)

        # Reboot to complete stateful update.
        self.reboot(timeout=self.REBOOT_TIMEOUT, wait=True)

        # After stateful update and a reboot, all of the test_files shouldn't
        # exist any more. Otherwise the stateful update is failed.
        return not any(
            self.path_exists(os.path.join(folder, test_file))
            for folder in folders_to_check)


    def _post_update_processing(self, updater, expected_kernel=None):
        """After the DUT is updated, confirm machine_install succeeded.

        @param updater: ChromiumOSUpdater instance used to update the DUT.
        @param expected_kernel: kernel expected to be active after reboot,
            or `None` to skip rollback checking.

        """
        # Touch the lab machine file to leave a marker that
        # distinguishes this image from other test images.
        # Afterwards, we must re-run the autoreboot script because
        # it depends on the _LAB_MACHINE_FILE.
        autoreboot_cmd = ('FILE="%s" ; [ -f "$FILE" ] || '
                          '( touch "$FILE" ; start autoreboot )')
        self.run(autoreboot_cmd % self._LAB_MACHINE_FILE)
        updater.verify_boot_expectations(
                expected_kernel, rollback_message=
                'Build %s failed to boot on %s; system rolled back to previous '
                'build' % (updater.update_version, self.hostname))
        # Check that we've got the build we meant to install.
        if not updater.check_version_to_confirm_install():
            raise autoupdater.ChromiumOSError(
                'Failed to update %s to build %s; found build '
                '%s instead' % (self.hostname,
                                updater.update_version,
                                self.get_release_version()))

        logging.debug('Cleaning up old autotest directories.')
        try:
            installed_autodir = autotest.Autotest.get_installed_autodir(self)
            self.run('rm -rf ' + installed_autodir)
        except autotest.AutodirNotFoundError:
            logging.debug('No autotest installed directory found.')


    def _stage_image_for_update(self, image_name=None):
        """Stage a build on a devserver and return the update_url and devserver.

        @param image_name: a name like lumpy-release/R27-3837.0.0
        @returns a tuple with an update URL like:
            http://172.22.50.205:8082/update/lumpy-release/R27-3837.0.0
            and the devserver instance.
        """
        if not image_name:
            image_name = self._get_cros_repair_image_name()
        logging.info('Staging build for AU: %s', image_name)
        devserver = dev_server.ImageServer.resolve(image_name, self.hostname)
        devserver.trigger_download(image_name, synchronous=False)
        return (tools.image_url_pattern() % (devserver.url(), image_name),
                devserver)


    def stage_image_for_servo(self, image_name=None):
        """Stage a build on a devserver and return the update_url.

        @param image_name: a name like lumpy-release/R27-3837.0.0
        @returns an update URL like:
            http://172.22.50.205:8082/update/lumpy-release/R27-3837.0.0
        """
        if not image_name:
            image_name = self._get_cros_repair_image_name()
        logging.info('Staging build for servo install: %s', image_name)
        devserver = dev_server.ImageServer.resolve(image_name, self.hostname)
        devserver.stage_artifacts(image_name, ['test_image'])
        return devserver.get_test_image_url(image_name)


    def stage_factory_image_for_servo(self, image_name):
        """Stage a build on a devserver and return the update_url.

        @param image_name: a name like <baord>/4262.204.0

        @return: An update URL, eg:
            http://<devserver>/static/canary-channel/\
            <board>/4262.204.0/factory_test/chromiumos_factory_image.bin

        @raises: ValueError if the factory artifact name is missing from
                 the config.

        """
        if not image_name:
            logging.error('Need an image_name to stage a factory image.')
            return

        factory_artifact = CONFIG.get_config_value(
                'CROS', 'factory_artifact', type=str, default='')
        if not factory_artifact:
            raise ValueError('Cannot retrieve the factory artifact name from '
                             'autotest config, and hence cannot stage factory '
                             'artifacts.')

        logging.info('Staging build for servo install: %s', image_name)
        devserver = dev_server.ImageServer.resolve(image_name, self.hostname)
        devserver.stage_artifacts(
                image_name,
                [factory_artifact],
                archive_url=None)

        return tools.factory_image_url_pattern() % (devserver.url(), image_name)


    def _get_au_monarch_fields(self, devserver, build):
        """Form monarch fields by given devserve & build for auto-update.

        @param devserver: the devserver (ImageServer instance) for auto-update.
        @param build: the build to be updated.

        @return A dictionary of monach fields.
        """
        try:
            board, build_type, milestone, _ = server_utils.ParseBuildName(build)
        except server_utils.ParseBuildNameException:
            logging.warning('Unable to parse build name %s. Something is '
                            'likely broken, but will continue anyway.',
                            build)
            board, build_type, milestone = ('', '', '')

        monarch_fields = {
                'dev_server': devserver.hostname,
                'board': board,
                'build_type': build_type,
                'milestone': milestone
        }
        return monarch_fields


    def _retry_auto_update_with_new_devserver(self, build, last_devserver,
                                              force_update, force_full_update,
                                              force_original, quick_provision):
        """Kick off auto-update by devserver and send metrics.

        @param build: the build to update.
        @param last_devserver: the last devserver that failed to provision.
        @param force_update: see |machine_install_by_devserver|'s force_udpate
                             for details.
        @param force_full_update: see |machine_install_by_devserver|'s
                                  force_full_update for details.
        @param force_original: Whether to force stateful update with the
                               original payload.
        @param quick_provision: Attempt to use quick provision path first.

        @return the result of |auto_update| in dev_server.
        """
        devserver = dev_server.resolve(
                build, self.hostname, ban_list=[last_devserver.url()])
        devserver.trigger_download(build, synchronous=False)
        monarch_fields = self._get_au_monarch_fields(devserver, build)
        logging.debug('Retry auto_update: resolved devserver for '
                      'auto-update: %s', devserver.url())

        # Add metrics
        install_with_dev_counter = metrics.Counter(
                'chromeos/autotest/provision/install_with_devserver')
        install_with_dev_counter.increment(fields=monarch_fields)
        c = metrics.Counter(
                'chromeos/autotest/provision/retry_by_devserver')
        monarch_fields['last_devserver'] = last_devserver.hostname
        monarch_fields['host'] = self.hostname
        c.increment(fields=monarch_fields)

        # Won't retry auto_update in a retry of auto-update.
        # In other words, we only retry auto-update once with a different
        # devservers.
        devserver.auto_update(
                self.hostname, build,
                original_board=self.get_board().replace(
                        ds_constants.BOARD_PREFIX, ''),
                original_release_version=self.get_release_version(),
                log_dir=self.job.resultdir,
                force_update=force_update,
                full_update=force_full_update,
                force_original=force_original,
                quick_provision=quick_provision)


    def machine_install_by_devserver(self, update_url=None, force_update=False,
                    local_devserver=False, repair=False,
                    force_full_update=False):
        """Ultiize devserver to install the DUT.

        (TODO) crbugs.com/627269: The logic in this function has some overlap
        with those in function machine_install. The merge will be done later,
        not in the same CL.

        @param update_url: The update_url or build for the host to update.
        @param force_update: Force an update even if the version installed
                is the same. Default:False
        @param local_devserver: Used by test_that to allow people to
                use their local devserver. Default: False
        @param repair: Forces update to repair image. Implies force_update.
        @param force_full_update: If True, do not attempt to run stateful
                update, force a full reimage. If False, try stateful update
                first when the dut is already installed with the same version.
        @raises autoupdater.ChromiumOSError

        @returns A tuple of (image_name, host_attributes).
                image_name is the name of image installed, e.g.,
                veyron_jerry-release/R50-7871.0.0
                host_attributes is a dictionary of (attribute, value), which
                can be saved to afe_host_attributes table in database. This
                method returns a dictionary with a single entry of
                `job_repo_url`: repo_url, where repo_url is a devserver url to
                autotest packages.
        """
        if repair:
            update_url = self._get_cros_repair_image_name()
            force_update = True

        if not update_url and not self._parser.options.image:
            raise error.AutoservError(
                    'There is no update URL, nor a method to get one.')

        if not update_url and self._parser.options.image:
            update_url = self._parser.options.image


        # Get build from parameter or AFE.
        # If the build is not a URL, let devserver to stage it first.
        # Otherwise, choose a devserver to trigger auto-update.
        build = None
        devserver = None
        logging.debug('Resolving a devserver for auto-update')
        previously_resolved = False
        if update_url.startswith('http://'):
            build = autoupdater.url_to_image_name(update_url)
            previously_resolved = True
        else:
            build = update_url
        devserver = dev_server.resolve(build, self.hostname)
        server_name = devserver.hostname

        monarch_fields = self._get_au_monarch_fields(devserver, build)

        if previously_resolved:
            # Make sure devserver for Auto-Update has staged the build.
            if server_name not in update_url:
              logging.debug('Resolved to devserver that does not match '
                            'update_url. The previously resolved devserver '
                            'must be unhealthy. Switching to use devserver %s,'
                            ' and re-staging.',
                            server_name)
              logging.info('Staging build for AU: %s', update_url)
              devserver.trigger_download(build, synchronous=False)
              c = metrics.Counter(
                  'chromeos/autotest/provision/failover_download')
              c.increment(fields=monarch_fields)
        else:
            logging.info('Staging build for AU: %s', update_url)
            devserver.trigger_download(build, synchronous=False)
            c = metrics.Counter('chromeos/autotest/provision/trigger_download')
            c.increment(fields=monarch_fields)

        # Report provision stats.
        install_with_dev_counter = metrics.Counter(
                'chromeos/autotest/provision/install_with_devserver')
        install_with_dev_counter.increment(fields=monarch_fields)
        logging.debug('Resolved devserver for auto-update: %s', devserver.url())

        # and other metrics from this function.
        metrics.Counter('chromeos/autotest/provision/resolve'
                        ).increment(fields=monarch_fields)

        force_original = self.get_chromeos_release_milestone() is None

        build_re = CONFIG.get_config_value(
                'CROS', 'quick_provision_build_regex', type=str, default='')
        quick_provision = (len(build_re) != 0 and
                           re.match(build_re, build) is not None)

        try:
            devserver.auto_update(
                    self.hostname, build,
                    original_board=self.get_board().replace(
                            ds_constants.BOARD_PREFIX, ''),
                    original_release_version=self.get_release_version(),
                    log_dir=self.job.resultdir,
                    force_update=force_update,
                    full_update=force_full_update,
                    force_original=force_original,
                    quick_provision=quick_provision)
        except dev_server.RetryableProvisionException:
            # It indicates that last provision failed due to devserver load
            # issue, so another devserver is resolved to kick off provision
            # job once again and only once.
            logging.debug('Provision failed due to devserver issue,'
                          'retry it with another devserver.')

            # Check first whether this DUT is completely offline. If so, skip
            # the following provision tries.
            logging.debug('Checking whether host %s is online.', self.hostname)
            if utils.ping(self.hostname, tries=1, deadline=1) == 0:
                self._retry_auto_update_with_new_devserver(
                        build, devserver, force_update, force_full_update,
                        force_original, quick_provision)
            else:
                raise error.AutoservError(
                        'No answer to ping from %s' % self.hostname)

        # The reason to resolve a new devserver in function machine_install
        # is mostly because that the update_url there may has a strange format,
        # and it's hard to parse the devserver url from it.
        # Since we already resolve a devserver to trigger auto-update, the same
        # devserver is used to form JOB_REPO_URL here. Verified in local test.
        repo_url = tools.get_package_url(devserver.url(), build)
        return build, {ds_constants.JOB_REPO_URL: repo_url}


    def prepare_for_update(self):
        """Prepares the DUT for an update.

        Subclasses may override this to perform any special actions
        required before updating.
        """
        pass


    def machine_install(self, update_url=None, force_update=False,
                        local_devserver=False, repair=False,
                        force_full_update=False):
        """Install the DUT.

        Use stateful update if the DUT is already running the same build.
        Stateful update does not update kernel and tends to run much faster
        than a full reimage. If the DUT is running a different build, or it
        failed to do a stateful update, full update, including kernel update,
        will be applied to the DUT.

        Once a host enters machine_install its host attribute job_repo_url
        (used for package install) will be removed and then updated.

        @param update_url: The url to use for the update
                pattern: http://$devserver:###/update/$build
                If update_url is None and repair is True we will install the
                stable image listed in afe_stable_versions table. If the table
                is not setup, global_config value under CROS.stable_cros_version
                will be used instead.
        @param force_update: Force an update even if the version installed
                is the same. Default:False
        @param local_devserver: Used by test_that to allow people to
                use their local devserver. Default: False
        @param repair: Forces update to repair image. Implies force_update.
        @param force_full_update: If True, do not attempt to run stateful
                update, force a full reimage. If False, try stateful update
                first when the dut is already installed with the same version.
        @raises autoupdater.ChromiumOSError

        @returns A tuple of (image_name, host_attributes).
                image_name is the name of image installed, e.g.,
                veyron_jerry-release/R50-7871.0.0
                host_attributes is a dictionary of (attribute, value), which
                can be saved to afe_host_attributes table in database. This
                method returns a dictionary with a single entry of
                `job_repo_url`: repo_url, where repo_url is a devserver url to
                autotest packages.
        """
        devserver = None
        if repair:
            update_url, devserver = self._stage_image_for_update()
            force_update = True

        if not update_url and not self._parser.options.image:
            raise error.AutoservError(
                    'There is no update URL, nor a method to get one.')

        if not update_url and self._parser.options.image:
            # This is the base case where we have no given update URL i.e.
            # dynamic suites logic etc. This is the most flexible case where we
            # can serve an update from any of our fleet of devservers.
            requested_build = self._parser.options.image
            if not requested_build.startswith('http://'):
                logging.debug('Update will be staged for this installation')
                update_url, devserver = self._stage_image_for_update(
                        requested_build)
            else:
                update_url = requested_build

        logging.debug('Update URL is %s', update_url)

        # Report provision stats.
        server_name = dev_server.get_hostname(update_url)
        (metrics.Counter('chromeos/autotest/provision/install')
         .increment(fields={'devserver': server_name}))

        # Create a file to indicate if provision fails. The file will be removed
        # by stateful update or full install.
        self.run('touch %s' % self.PROVISION_FAILED)

        update_complete = False
        updater = autoupdater.ChromiumOSUpdater(
                 update_url, host=self, local_devserver=local_devserver)
        if not force_full_update:
            try:
                # If the DUT is already running the same build, try stateful
                # update first as it's much quicker than a full re-image.
                update_complete = self._try_stateful_update(
                        update_url, force_update, updater)
            except Exception as e:
                logging.exception(e)

        inactive_kernel = None
        if update_complete or (not force_update and updater.check_version()):
            logging.info('Install complete without full update')
        else:
            logging.info('DUT requires full update.')
            self.reboot(timeout=self.REBOOT_TIMEOUT, wait=True)
            self.prepare_for_update()

            num_of_attempts = provision.FLAKY_DEVSERVER_ATTEMPTS

            while num_of_attempts > 0:
                num_of_attempts -= 1
                try:
                    updater.run_update()
                except Exception:
                    logging.warn('Autoupdate did not complete.')
                    # Do additional check for the devserver health. Ideally,
                    # the autoupdater.py could raise an exception when it
                    # detected network flake but that would require
                    # instrumenting the update engine and parsing it log.
                    if (num_of_attempts <= 0 or
                            devserver is None or
                            dev_server.ImageServer.devserver_healthy(
                                    devserver.url())):
                        raise

                    logging.warn('Devserver looks unhealthy. Trying another')
                    update_url, devserver = self._stage_image_for_update(
                            requested_build)
                    logging.debug('New Update URL is %s', update_url)
                    updater = autoupdater.ChromiumOSUpdater(
                            update_url, host=self,
                            local_devserver=local_devserver)
                else:
                    break

            # Give it some time in case of IO issues.
            time.sleep(10)

            # Figure out active and inactive kernel.
            active_kernel, inactive_kernel = updater.get_kernel_state()

            # Ensure inactive kernel has higher priority than active.
            if (updater.get_kernel_priority(inactive_kernel)
                    < updater.get_kernel_priority(active_kernel)):
                raise autoupdater.ChromiumOSError(
                    'Update failed. The priority of the inactive kernel'
                    ' partition is less than that of the active kernel'
                    ' partition.')

            # Updater has returned successfully; reboot the host.
            #
            # Regarding the 'crossystem' command: In some cases, the
            # TPM gets into a state such that it fails verification.
            # We don't know why.  However, this call papers over the
            # problem by clearing the TPM during the reboot.
            #
            # We ignore failures from 'crossystem'.  Although failure
            # here is unexpected, and could signal a bug, the point
            # of the exercise is to paper over problems; allowing
            # this to fail would defeat the purpose.
            self.run('crossystem clear_tpm_owner_request=1',
                     ignore_status=True)
            self.reboot(timeout=self.REBOOT_TIMEOUT, wait=True)

        self._post_update_processing(updater, inactive_kernel)
        image_name = autoupdater.url_to_image_name(update_url)
        # update_url is different from devserver url needed to stage autotest
        # packages, therefore, resolve a new devserver url here.
        devserver_url = dev_server.ImageServer.resolve(image_name,
                                                       self.hostname).url()
        repo_url = tools.get_package_url(devserver_url, image_name)
        return image_name, {ds_constants.JOB_REPO_URL: repo_url}


    def _clear_fw_version_labels(self, rw_only):
        """Clear firmware version labels from the machine.

        @param rw_only: True to only clear fwrw_version; otherewise, clear
                        both fwro_version and fwrw_version.
        """
        labels = self._AFE.get_labels(
                name__startswith=provision.FW_RW_VERSION_PREFIX,
                host__hostname=self.hostname)
        if not rw_only:
            labels = labels + self._AFE.get_labels(
                    name__startswith=provision.FW_RO_VERSION_PREFIX,
                    host__hostname=self.hostname)
        for label in labels:
            label.remove_hosts(hosts=[self.hostname])


    def _add_fw_version_label(self, build, rw_only):
        """Add firmware version label to the machine.

        @param build: Build of firmware.
        @param rw_only: True to only add fwrw_version; otherwise, add both
                        fwro_version and fwrw_version.

        """
        fw_label = provision.fwrw_version_to_label(build)
        self._AFE.run('label_add_hosts', id=fw_label, hosts=[self.hostname])
        if not rw_only:
            fw_label = provision.fwro_version_to_label(build)
            self._AFE.run('label_add_hosts', id=fw_label, hosts=[self.hostname])


    def firmware_install(self, build=None, rw_only=False):
        """Install firmware to the DUT.

        Use stateful update if the DUT is already running the same build.
        Stateful update does not update kernel and tends to run much faster
        than a full reimage. If the DUT is running a different build, or it
        failed to do a stateful update, full update, including kernel update,
        will be applied to the DUT.

        Once a host enters firmware_install its fw[ro|rw]_version label will
        be removed. After the firmware is updated successfully, a new
        fw[ro|rw]_version label will be added to the host.

        @param build: The build version to which we want to provision the
                      firmware of the machine,
                      e.g. 'link-firmware/R22-2695.1.144'.
        @param rw_only: True to only install firmware to its RW portions. Keep
                        the RO portions unchanged.

        TODO(dshi): After bug 381718 is fixed, update here with corresponding
                    exceptions that could be raised.

        """
        if not self.servo:
            raise error.TestError('Host %s does not have servo.' %
                                  self.hostname)

        # Get the DUT board name from servod.
        board = self.servo.get_board()

        # If build is not set, try to install firmware from stable CrOS.
        if not build:
            build = afe_utils.get_stable_faft_version(board)
            if not build:
                raise error.TestError(
                        'Failed to find stable firmware build for %s.',
                        self.hostname)
            logging.info('Will install firmware from build %s.', build)

        config = FAFTConfig(board)
        if config.use_u_boot:
            ap_image = 'image-%s.bin' % board
        else: # Depthcharge platform
            ap_image = 'image.bin'
        ec_image = 'ec.bin'
        ds = dev_server.ImageServer.resolve(build, self.hostname)
        ds.stage_artifacts(build, ['firmware'])

        tmpd = autotemp.tempdir(unique_id='fwimage')
        try:
            fwurl = self._FW_IMAGE_URL_PATTERN % (ds.url(), build)
            local_tarball = os.path.join(tmpd.name, os.path.basename(fwurl))
            ds.download_file(fwurl, local_tarball)

            self._clear_fw_version_labels(rw_only)
            if config.chrome_ec:
                logging.info('Will re-program EC %snow', 'RW ' if rw_only else '')
                server_utils.system('tar xf %s -C %s %s' %
                                    (local_tarball, tmpd.name, ec_image),
                                    timeout=60)
                self.servo.program_ec(os.path.join(tmpd.name, ec_image), rw_only)
            else:
                logging.info('Not a Chrome EC, ignore re-programing it')
            logging.info('Will re-program BIOS %snow', 'RW ' if rw_only else '')
            server_utils.system('tar xf %s -C %s %s' %
                                (local_tarball, tmpd.name, ap_image),
                                timeout=60)
            self.servo.program_bios(os.path.join(tmpd.name, ap_image), rw_only)
            self.servo.get_power_state_controller().reset()
            time.sleep(self.servo.BOOT_DELAY)
            if utils.host_is_in_lab_zone(self.hostname):
                self._add_fw_version_label(build, rw_only)
        finally:
            tmpd.clean()


    def program_base_ec(self, image_path):
        """Program Base EC on DUT with the given image.

        @param image_path: a string, file name of the EC image to program
                           on the DUT.

        """
        dest_path = os.path.join('/tmp', os.path.basename(image_path))
        self.send_file(image_path, dest_path)
        programmer = firmware_programmer.ProgrammerDfu(self.servo, self)
        programmer.program_ec(dest_path)


    def show_update_engine_log(self):
        """Output update engine log."""
        logging.debug('Dumping %s', client_constants.UPDATE_ENGINE_LOG)
        self.run('cat %s' % client_constants.UPDATE_ENGINE_LOG)


    def servo_install(self, image_url=None, usb_boot_timeout=USB_BOOT_TIMEOUT,
                      install_timeout=INSTALL_TIMEOUT):
        """
        Re-install the OS on the DUT by:
        1) installing a test image on a USB storage device attached to the Servo
                board,
        2) booting that image in recovery mode, and then
        3) installing the image with chromeos-install.

        @param image_url: If specified use as the url to install on the DUT.
                otherwise boot the currently staged image on the USB stick.
        @param usb_boot_timeout: The usb_boot_timeout to use during reimage.
                Factory images need a longer usb_boot_timeout than regular
                cros images.
        @param install_timeout: The timeout to use when installing the chromeos
                image. Factory images need a longer install_timeout.

        @raises AutoservError if the image fails to boot.

        """
        logging.info('Downloading image to USB, then booting from it. Usb boot '
                     'timeout = %s', usb_boot_timeout)
        with metrics.SecondsTimer(
                'chromeos/autotest/provision/servo_install/boot_duration'):
            self.servo.install_recovery_image(image_url)
            if not self.wait_up(timeout=usb_boot_timeout):
                raise hosts.AutoservRepairError(
                        'DUT failed to boot from USB after %d seconds' %
                        usb_boot_timeout)

        # The new chromeos-tpm-recovery has been merged since R44-7073.0.0.
        # In old CrOS images, this command fails. Skip the error.
        logging.info('Resetting the TPM status')
        try:
            self.run('chromeos-tpm-recovery')
        except error.AutoservRunError:
            logging.warn('chromeos-tpm-recovery is too old.')


        with metrics.SecondsTimer(
                'chromeos/autotest/provision/servo_install/install_duration'):
            logging.info('Installing image through chromeos-install.')
            self.run('chromeos-install --yes', timeout=install_timeout)
            self.halt()

        logging.info('Power cycling DUT through servo.')
        self.servo.get_power_state_controller().power_off()
        self.servo.switch_usbkey('off')
        # N.B. The Servo API requires that we use power_on() here
        # for two reasons:
        #  1) After turning on a DUT in recovery mode, you must turn
        #     it off and then on with power_on() once more to
        #     disable recovery mode (this is a Parrot specific
        #     requirement).
        #  2) After power_off(), the only way to turn on is with
        #     power_on() (this is a Storm specific requirement).
        self.servo.get_power_state_controller().power_on()

        logging.info('Waiting for DUT to come back up.')
        if not self.wait_up(timeout=self.BOOT_TIMEOUT):
            raise error.AutoservError('DUT failed to reboot installed '
                                      'test image after %d seconds' %
                                      self.BOOT_TIMEOUT)


    def repair_servo(self):
        """
        Confirm that servo is initialized and verified.

        If the servo object is missing, attempt to repair the servo
        host.  Repair failures are passed back to the caller.

        @raise AutoservError: If there is no servo host for this CrOS
                              host.
        """
        if self.servo:
            return
        if not self._servo_host:
            raise error.AutoservError('No servo host for %s.' %
                                      self.hostname)
        self._servo_host.repair()
        self.servo = self._servo_host.get_servo()


    def repair(self):
        """Attempt to get the DUT to pass `self.verify()`.

        This overrides the base class function for repair; it does
        not call back to the parent class, but instead relies on
        `self._repair_strategy` to coordinate the verification and
        repair steps needed to get the DUT working.
        """
        self._repair_strategy.repair(self)
        # Sometimes, hosts with certain ethernet dongles get stuck in a
        # bad network state where they're reachable from this code, but
        # not from the devservers during provisioning.  Rebooting the
        # DUT fixes it.
        #
        # TODO(jrbarnette):  Ideally, we'd get rid of the problem
        # dongles, and drop this code.  Failing that, we could be smart
        # enough not to reboot if repair rebooted the DUT (e.g. by
        # looking at DUT uptime after repair completes).

        self.reboot()


    def close(self):
        """Close connection."""
        super(CrosHost, self).close()
        if self._chameleon_host:
            self._chameleon_host.close()

        if self._servo_host:
            self._servo_host.close()


    def get_power_supply_info(self):
        """Get the output of power_supply_info.

        power_supply_info outputs the info of each power supply, e.g.,
        Device: Line Power
          online:                  no
          type:                    Mains
          voltage (V):             0
          current (A):             0
        Device: Battery
          state:                   Discharging
          percentage:              95.9276
          technology:              Li-ion

        Above output shows two devices, Line Power and Battery, with details of
        each device listed. This function parses the output into a dictionary,
        with key being the device name, and value being a dictionary of details
        of the device info.

        @return: The dictionary of power_supply_info, e.g.,
                 {'Line Power': {'online': 'yes', 'type': 'main'},
                  'Battery': {'vendor': 'xyz', 'percentage': '100'}}
        @raise error.AutoservRunError if power_supply_info tool is not found in
               the DUT. Caller should handle this error to avoid false failure
               on verification.
        """
        result = self.run('power_supply_info').stdout.strip()
        info = {}
        device_name = None
        device_info = {}
        for line in result.split('\n'):
            pair = [v.strip() for v in line.split(':')]
            if len(pair) != 2:
                continue
            if pair[0] == 'Device':
                if device_name:
                    info[device_name] = device_info
                device_name = pair[1]
                device_info = {}
            else:
                device_info[pair[0]] = pair[1]
        if device_name and not device_name in info:
            info[device_name] = device_info
        return info


    def get_battery_percentage(self):
        """Get the battery percentage.

        @return: The percentage of battery level, value range from 0-100. Return
                 None if the battery info cannot be retrieved.
        """
        try:
            info = self.get_power_supply_info()
            logging.info(info)
            return float(info['Battery']['percentage'])
        except (KeyError, ValueError, error.AutoservRunError):
            return None


    def is_ac_connected(self):
        """Check if the dut has power adapter connected and charging.

        @return: True if power adapter is connected and charging.
        """
        try:
            info = self.get_power_supply_info()
            return info['Line Power']['online'] == 'yes'
        except (KeyError, error.AutoservRunError):
            return None


    def _cleanup_poweron(self):
        """Special cleanup method to make sure hosts always get power back."""
        afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
        hosts = afe.get_hosts(hostname=self.hostname)
        if not hosts or not (self._RPM_OUTLET_CHANGED in
                             hosts[0].attributes):
            return
        logging.debug('This host has recently interacted with the RPM'
                      ' Infrastructure. Ensuring power is on.')
        try:
            self.power_on()
            afe.set_host_attribute(self._RPM_OUTLET_CHANGED, None,
                                   hostname=self.hostname)
        except rpm_client.RemotePowerException:
            logging.error('Failed to turn Power On for this host after '
                          'cleanup through the RPM Infrastructure.')

            battery_percentage = self.get_battery_percentage()
            if battery_percentage and battery_percentage < 50:
                raise
            elif self.is_ac_connected():
                logging.info('The device has power adapter connected and '
                             'charging. No need to try to turn RPM on '
                             'again.')
                afe.set_host_attribute(self._RPM_OUTLET_CHANGED, None,
                                       hostname=self.hostname)
            logging.info('Battery level is now at %s%%. The device may '
                         'still have enough power to run test, so no '
                         'exception will be raised.', battery_percentage)


    def _is_factory_image(self):
        """Checks if the image on the DUT is a factory image.

        @return: True if the image on the DUT is a factory image.
                 False otherwise.
        """
        result = self.run('[ -f /root/.factory_test ]', ignore_status=True)
        return result.exit_status == 0


    def _restart_ui(self):
        """Restart the Chrome UI.

        @raises: FactoryImageCheckerException for factory images, since
                 we cannot attempt to restart ui on them.
                 error.AutoservRunError for any other type of error that
                 occurs while restarting ui.
        """
        if self._is_factory_image():
            raise FactoryImageCheckerException('Cannot restart ui on factory '
                                               'images')

        # TODO(jrbarnette):  The command to stop/start the ui job
        # should live inside cros_ui, too.  However that would seem
        # to imply interface changes to the existing start()/restart()
        # functions, which is a bridge too far (for now).
        prompt = cros_ui.get_chrome_session_ident(self)
        self.run('stop ui; start ui')
        cros_ui.wait_for_chrome_ready(prompt, self)


    def _get_lsb_release_content(self):
        """Return the content of lsb-release file of host."""
        return self.run(
                'cat "%s"' % client_constants.LSB_RELEASE).stdout.strip()


    def get_release_version(self):
        """Get the value of attribute CHROMEOS_RELEASE_VERSION from lsb-release.

        @returns The version string in lsb-release, under attribute
                 CHROMEOS_RELEASE_VERSION.
        """
        return lsbrelease_utils.get_chromeos_release_version(
                lsb_release_content=self._get_lsb_release_content())


    def get_release_builder_path(self):
        """Get the value of CHROMEOS_RELEASE_BUILDER_PATH from lsb-release.

        @returns The version string in lsb-release, under attribute
                 CHROMEOS_RELEASE_BUILDER_PATH.
        """
        return lsbrelease_utils.get_chromeos_release_builder_path(
                lsb_release_content=self._get_lsb_release_content())


    def get_chromeos_release_milestone(self):
        """Get the value of attribute CHROMEOS_RELEASE_BUILD_TYPE
        from lsb-release.

        @returns The version string in lsb-release, under attribute
                 CHROMEOS_RELEASE_BUILD_TYPE.
        """
        return lsbrelease_utils.get_chromeos_release_milestone(
                lsb_release_content=self._get_lsb_release_content())


    def verify_cros_version_label(self):
        """ Make sure host's cros-version label match the actual image in dut.

        Remove any cros-version: label that doesn't match that installed in
        the dut.

        @param raise_error: Set to True to raise exception if any mismatch found

        @raise error.AutoservError: If any mismatch between cros-version label
                                    and the build installed in dut is found.
        """
        labels = self._AFE.get_labels(
                name__startswith=ds_constants.VERSION_PREFIX,
                host__hostname=self.hostname)
        mismatch_found = False
        if labels:
            # Ask the DUT for its canonical image name.  This will be in
            # a form like this:  kevin-release/R66-10405.0.0
            release_builder_path = self.get_release_builder_path()
            host_list = [self.hostname]
            for label in labels:
                # Remove any cros-version label that does not match
                # the DUT's installed image.
                #
                # TODO(jrbarnette):  Tests sent to the `arc-presubmit`
                # pool install images matching the format above, but
                # then apply a label with `-cheetsth` appended.  Probably,
                # it's wrong for ARC presubmit testing to make that change,
                # but until it's fixed, this code specifically excuses that
                # behavior.
                build_version = label.name[len(ds_constants.VERSION_PREFIX):]
                if build_version.endswith('-cheetsth'):
                    build_version = build_version[:-len('-cheetsth')]
                if build_version != release_builder_path:
                    logging.warn(
                        'cros-version label "%s" does not match '
                        'release_builder_path %s. Removing the label.',
                        label.name, release_builder_path)
                    label.remove_hosts(hosts=host_list)
                    mismatch_found = True
        if mismatch_found:
            raise error.AutoservError('The host has wrong cros-version label.')


    def cleanup_services(self):
        """Reinitializes the device for cleanup.

        Subclasses may override this to customize the cleanup method.

        To indicate failure of the reset, the implementation may raise
        any of:
            error.AutoservRunError
            error.AutotestRunError
            FactoryImageCheckerException

        @raises error.AutoservRunError
        @raises error.AutotestRunError
        @raises error.FactoryImageCheckerException
        """
        self._restart_ui()


    def cleanup(self):
        """Cleanup state on device."""
        self.run('rm -f %s' % client_constants.CLEANUP_LOGS_PAUSED_FILE)
        try:
            self.cleanup_services()
        except (error.AutotestRunError, error.AutoservRunError,
                FactoryImageCheckerException):
            logging.warning('Unable to restart ui, rebooting device.')
            # Since restarting the UI fails fall back to normal Autotest
            # cleanup routines, i.e. reboot the machine.
            super(CrosHost, self).cleanup()
        # Check if the rpm outlet was manipulated.
        if self.has_power():
            self._cleanup_poweron()
        self.verify_cros_version_label()


    def reboot(self, **dargs):
        """
        This function reboots the site host. The more generic
        RemoteHost.reboot() performs sync and sleeps for 5
        seconds. This is not necessary for Chrome OS devices as the
        sync should be finished in a short time during the reboot
        command.
        """
        if 'reboot_cmd' not in dargs:
            reboot_timeout = dargs.get('reboot_timeout', 10)
            dargs['reboot_cmd'] = ('sleep 1; '
                                   'reboot & sleep %d; '
                                   'reboot -f' % reboot_timeout)
        # Enable fastsync to avoid running extra sync commands before reboot.
        if 'fastsync' not in dargs:
            dargs['fastsync'] = True

        # For purposes of logging reboot times:
        # Get the board name i.e. 'daisy_spring'
        board_fullname = self.get_board()

        # Strip the prefix and add it to dargs.
        dargs['board'] = board_fullname[board_fullname.find(':')+1:]
        # Record who called us
        orig = sys._getframe(1).f_code
        metric_fields = {'board' : dargs['board'],
                         'dut_host_name' : self.hostname,
                         'success' : True}
        metric_debug_fields = {'board' : dargs['board'],
                               'caller' : "%s:%s" % (orig.co_filename, orig.co_name),
                               'success' : True,
                               'error' : ''}

        t0 = time.time()
        try:
            super(CrosHost, self).reboot(**dargs)
        except Exception as e:
            metric_fields['success'] = False
            metric_debug_fields['success'] = False
            metric_debug_fields['error'] = type(e).__name__
            raise
        finally:
            duration = int(time.time() - t0)
            metrics.Counter(
                    'chromeos/autotest/autoserv/reboot_count').increment(
                    fields=metric_fields)
            metrics.Counter(
                    'chromeos/autotest/autoserv/reboot_debug').increment(
                    fields=metric_debug_fields)
            metrics.SecondsDistribution(
                    'chromeos/autotest/autoserv/reboot_duration').add(
                    duration, fields=metric_fields)


    def suspend(self, **dargs):
        """
        This function suspends the site host.
        """
        suspend_time = dargs.get('suspend_time', 60)
        dargs['timeout'] = suspend_time
        if 'suspend_cmd' not in dargs:
            dargs['suspend_cmd'] = ' && '.join([
                'echo 0 > /sys/class/rtc/rtc0/wakealarm',
                'echo +%d > /sys/class/rtc/rtc0/wakealarm' % suspend_time,
                'powerd_dbus_suspend --delay=0'])
        super(CrosHost, self).suspend(**dargs)


    def upstart_status(self, service_name):
        """Check the status of an upstart init script.

        @param service_name: Service to look up.

        @returns True if the service is running, False otherwise.
        """
        return 'start/running' in self.run('status %s' % service_name,
                                           ignore_status=True).stdout


    def verify_software(self):
        """Verify working software on a Chrome OS system.

        Tests for the following conditions:
         1. All conditions tested by the parent version of this
            function.
         2. Sufficient space in /mnt/stateful_partition.
         3. Sufficient space in /mnt/stateful_partition/encrypted.
         4. update_engine answers a simple status request over DBus.

        """
        super(CrosHost, self).verify_software()
        default_kilo_inodes_required = CONFIG.get_config_value(
                'SERVER', 'kilo_inodes_required', type=int, default=100)
        board = self.get_board().replace(ds_constants.BOARD_PREFIX, '')
        kilo_inodes_required = CONFIG.get_config_value(
                'SERVER', 'kilo_inodes_required_%s' % board,
                type=int, default=default_kilo_inodes_required)
        self.check_inodes('/mnt/stateful_partition', kilo_inodes_required)
        self.check_diskspace(
            '/mnt/stateful_partition',
            CONFIG.get_config_value(
                'SERVER', 'gb_diskspace_required', type=float,
                default=20.0))
        encrypted_stateful_path = '/mnt/stateful_partition/encrypted'
        # Not all targets build with encrypted stateful support.
        if self.path_exists(encrypted_stateful_path):
            self.check_diskspace(
                encrypted_stateful_path,
                CONFIG.get_config_value(
                    'SERVER', 'gb_encrypted_diskspace_required', type=float,
                    default=0.1))

        self.wait_for_system_services()

        # Factory images don't run update engine,
        # goofy controls dbus on these DUTs.
        if not self._is_factory_image():
            self.run('update_engine_client --status')

        self.verify_cros_version_label()


    @retry.retry(error.AutoservError, timeout_min=5, delay_sec=10)
    def wait_for_system_services(self):
        """Waits for system-services to be running.

        Sometimes, update_engine will take a while to update firmware, so we
        should give this some time to finish. See crbug.com/765686#c38 for
        details.
        """
        if not self.upstart_status('system-services'):
            raise error.AutoservError('Chrome failed to reach login. '
                                      'System services not running.')


    def verify(self):
        """Verify Chrome OS system is in good state."""
        self._repair_strategy.verify(self)


    def make_ssh_command(self, user='root', port=22, opts='', hosts_file=None,
                         connect_timeout=None, alive_interval=None,
                         alive_count_max=None, connection_attempts=None):
        """Override default make_ssh_command to use options tuned for Chrome OS.

        Tuning changes:
          - ConnectTimeout=30; maximum of 30 seconds allowed for an SSH
          connection failure.  Consistency with remote_access.sh.

          - ServerAliveInterval=900; which causes SSH to ping connection every
          900 seconds. In conjunction with ServerAliveCountMax ensures
          that if the connection dies, Autotest will bail out.
          Originally tried 60 secs, but saw frequent job ABORTS where
          the test completed successfully. Later increased from 180 seconds to
          900 seconds to account for tests where the DUT is suspended for
          longer periods of time.

          - ServerAliveCountMax=3; consistency with remote_access.sh.

          - ConnectAttempts=4; reduce flakiness in connection errors;
          consistency with remote_access.sh.

          - UserKnownHostsFile=/dev/null; we don't care about the keys.
          Host keys change with every new installation, don't waste
          memory/space saving them.

          - SSH protocol forced to 2; needed for ServerAliveInterval.

        @param user User name to use for the ssh connection.
        @param port Port on the target host to use for ssh connection.
        @param opts Additional options to the ssh command.
        @param hosts_file Ignored.
        @param connect_timeout Ignored.
        @param alive_interval Ignored.
        @param alive_count_max Ignored.
        @param connection_attempts Ignored.
        """
        options = ' '.join([opts, '-o Protocol=2'])
        return super(CrosHost, self).make_ssh_command(
            user=user, port=port, opts=options, hosts_file='/dev/null',
            connect_timeout=30, alive_interval=900, alive_count_max=3,
            connection_attempts=4)


    def syslog(self, message, tag='autotest'):
        """Logs a message to syslog on host.

        @param message String message to log into syslog
        @param tag String tag prefix for syslog

        """
        self.run('logger -t "%s" "%s"' % (tag, message))


    def _ping_check_status(self, status):
        """Ping the host once, and return whether it has a given status.

        @param status Check the ping status against this value.
        @return True iff `status` and the result of ping are the same
                (i.e. both True or both False).

        """
        ping_val = utils.ping(self.hostname, tries=1, deadline=1)
        return not (status ^ (ping_val == 0))

    def _ping_wait_for_status(self, status, timeout):
        """Wait for the host to have a given status (UP or DOWN).

        Status is checked by polling.  Polling will not last longer
        than the number of seconds in `timeout`.  The polling
        interval will be long enough that only approximately
        _PING_WAIT_COUNT polling cycles will be executed, subject
        to a maximum interval of about one minute.

        @param status Waiting will stop immediately if `ping` of the
                      host returns this status.
        @param timeout Poll for at most this many seconds.
        @return True iff the host status from `ping` matched the
                requested status at the time of return.

        """
        # _ping_check_status() takes about 1 second, hence the
        # "- 1" in the formula below.
        # FIXME: if the ping command errors then _ping_check_status()
        # returns instantly. If timeout is also smaller than twice
        # _PING_WAIT_COUNT then the while loop below forks many
        # thousands of ping commands (see /tmp/test_that_results_XXXXX/
        # /results-1-logging_YYY.ZZZ/debug/autoserv.DEBUG) and hogs one
        # CPU core for 60 seconds.
        poll_interval = min(int(timeout / self._PING_WAIT_COUNT), 60) - 1
        end_time = time.time() + timeout
        while time.time() <= end_time:
            if self._ping_check_status(status):
                return True
            if poll_interval > 0:
                time.sleep(poll_interval)

        # The last thing we did was sleep(poll_interval), so it may
        # have been too long since the last `ping`.  Check one more
        # time, just to be sure.
        return self._ping_check_status(status)

    def ping_wait_up(self, timeout):
        """Wait for the host to respond to `ping`.

        N.B.  This method is not a reliable substitute for
        `wait_up()`, because a host that responds to ping will not
        necessarily respond to ssh.  This method should only be used
        if the target DUT can be considered functional even if it
        can't be reached via ssh.

        @param timeout Minimum time to allow before declaring the
                       host to be non-responsive.
        @return True iff the host answered to ping before the timeout.

        """
        return self._ping_wait_for_status(self._PING_STATUS_UP, timeout)

    def ping_wait_down(self, timeout):
        """Wait until the host no longer responds to `ping`.

        This function can be used as a slightly faster version of
        `wait_down()`, by avoiding potentially long ssh timeouts.

        @param timeout Minimum time to allow for the host to become
                       non-responsive.
        @return True iff the host quit answering ping before the
                timeout.

        """
        return self._ping_wait_for_status(self._PING_STATUS_DOWN, timeout)

    def test_wait_for_sleep(self, sleep_timeout=None):
        """Wait for the client to enter low-power sleep mode.

        The test for "is asleep" can't distinguish a system that is
        powered off; to confirm that the unit was asleep, it is
        necessary to force resume, and then call
        `test_wait_for_resume()`.

        This function is expected to be called from a test as part
        of a sequence like the following:

        ~~~~~~~~
            boot_id = host.get_boot_id()
            # trigger sleep on the host
            host.test_wait_for_sleep()
            # trigger resume on the host
            host.test_wait_for_resume(boot_id)
        ~~~~~~~~

        @param sleep_timeout time limit in seconds to allow the host sleep.

        @exception TestFail The host did not go to sleep within
                            the allowed time.
        """
        if sleep_timeout is None:
            sleep_timeout = self.SLEEP_TIMEOUT

        if not self.ping_wait_down(timeout=sleep_timeout):
            raise error.TestFail(
                'client failed to sleep after %d seconds' % sleep_timeout)


    def test_wait_for_resume(self, old_boot_id, resume_timeout=None):
        """Wait for the client to resume from low-power sleep mode.

        The `old_boot_id` parameter should be the value from
        `get_boot_id()` obtained prior to entering sleep mode.  A
        `TestFail` exception is raised if the boot id changes.

        See @ref test_wait_for_sleep for more on this function's
        usage.

        @param old_boot_id A boot id value obtained before the
                               target host went to sleep.
        @param resume_timeout time limit in seconds to allow the host up.

        @exception TestFail The host did not respond within the
                            allowed time.
        @exception TestFail The host responded, but the boot id test
                            indicated a reboot rather than a sleep
                            cycle.
        """
        if resume_timeout is None:
            resume_timeout = self.RESUME_TIMEOUT

        if not self.wait_up(timeout=resume_timeout):
            raise error.TestFail(
                'client failed to resume from sleep after %d seconds' %
                    resume_timeout)
        else:
            new_boot_id = self.get_boot_id()
            if new_boot_id != old_boot_id:
                logging.error('client rebooted (old boot %s, new boot %s)',
                              old_boot_id, new_boot_id)
                raise error.TestFail(
                    'client rebooted, but sleep was expected')


    def test_wait_for_shutdown(self, shutdown_timeout=None):
        """Wait for the client to shut down.

        The test for "has shut down" can't distinguish a system that
        is merely asleep; to confirm that the unit was down, it is
        necessary to force boot, and then call test_wait_for_boot().

        This function is expected to be called from a test as part
        of a sequence like the following:

        ~~~~~~~~
            boot_id = host.get_boot_id()
            # trigger shutdown on the host
            host.test_wait_for_shutdown()
            # trigger boot on the host
            host.test_wait_for_boot(boot_id)
        ~~~~~~~~

        @param shutdown_timeout time limit in seconds to allow the host down.
        @exception TestFail The host did not shut down within the
                            allowed time.
        """
        if shutdown_timeout is None:
            shutdown_timeout = self.SHUTDOWN_TIMEOUT

        if not self.ping_wait_down(timeout=shutdown_timeout):
            raise error.TestFail(
                'client failed to shut down after %d seconds' %
                    shutdown_timeout)


    def test_wait_for_boot(self, old_boot_id=None):
        """Wait for the client to boot from cold power.

        The `old_boot_id` parameter should be the value from
        `get_boot_id()` obtained prior to shutting down.  A
        `TestFail` exception is raised if the boot id does not
        change.  The boot id test is omitted if `old_boot_id` is not
        specified.

        See @ref test_wait_for_shutdown for more on this function's
        usage.

        @param old_boot_id A boot id value obtained before the
                               shut down.

        @exception TestFail The host did not respond within the
                            allowed time.
        @exception TestFail The host responded, but the boot id test
                            indicated that there was no reboot.
        """
        if not self.wait_up(timeout=self.REBOOT_TIMEOUT):
            raise error.TestFail(
                'client failed to reboot after %d seconds' %
                    self.REBOOT_TIMEOUT)
        elif old_boot_id:
            if self.get_boot_id() == old_boot_id:
                logging.error('client not rebooted (boot %s)',
                              old_boot_id)
                raise error.TestFail(
                    'client is back up, but did not reboot')


    @staticmethod
    def check_for_rpm_support(hostname):
        """For a given hostname, return whether or not it is powered by an RPM.

        @param hostname: hostname to check for rpm support.

        @return None if this host does not follows the defined naming format
                for RPM powered DUT's in the lab. If it does follow the format,
                it returns a regular expression MatchObject instead.
        """
        return re.match(CrosHost._RPM_HOSTNAME_REGEX, hostname)


    def has_power(self):
        """For this host, return whether or not it is powered by an RPM.

        @return True if this host is in the CROS lab and follows the defined
                naming format.
        """
        return CrosHost.check_for_rpm_support(self.hostname)


    def _set_power(self, state, power_method):
        """Sets the power to the host via RPM, Servo or manual.

        @param state Specifies which power state to set to DUT
        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        ACCEPTABLE_STATES = ['ON', 'OFF']

        if state.upper() not in ACCEPTABLE_STATES:
            raise error.TestError('State must be one of: %s.'
                                   % (ACCEPTABLE_STATES,))

        if power_method == self.POWER_CONTROL_SERVO:
            logging.info('Setting servo port J10 to %s', state)
            self.servo.set('prtctl3_pwren', state.lower())
            time.sleep(self._USB_POWER_TIMEOUT)
        elif power_method == self.POWER_CONTROL_MANUAL:
            logging.info('You have %d seconds to set the AC power to %s.',
                         self._POWER_CYCLE_TIMEOUT, state)
            time.sleep(self._POWER_CYCLE_TIMEOUT)
        else:
            if not self.has_power():
                raise error.TestFail('DUT does not have RPM connected.')
            afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
            afe.set_host_attribute(self._RPM_OUTLET_CHANGED, True,
                                   hostname=self.hostname)
            rpm_client.set_power(self.hostname, state.upper(), timeout_mins=5)


    def power_off(self, power_method=POWER_CONTROL_RPM):
        """Turn off power to this host via RPM, Servo or manual.

        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        self._set_power('OFF', power_method)


    def power_on(self, power_method=POWER_CONTROL_RPM):
        """Turn on power to this host via RPM, Servo or manual.

        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        self._set_power('ON', power_method)


    def power_cycle(self, power_method=POWER_CONTROL_RPM):
        """Cycle power to this host by turning it OFF, then ON.

        @param power_method Specifies which method of power control to
                            use. By default "RPM" will be used. Valid values
                            are the strings "RPM", "manual", "servoj10".

        """
        if power_method in (self.POWER_CONTROL_SERVO,
                            self.POWER_CONTROL_MANUAL):
            self.power_off(power_method=power_method)
            time.sleep(self._POWER_CYCLE_TIMEOUT)
            self.power_on(power_method=power_method)
        else:
            rpm_client.set_power(self.hostname, 'CYCLE')


    def get_platform(self):
        """Determine the correct platform label for this host.

        @returns a string representing this host's platform.
        """
        cmd = 'mosys platform model'
        result = self.run(command=cmd, ignore_status=True)
        if result.exit_status == 0:
            return result.stdout.strip()
        else:
            # $(mosys platform model) should support all platforms, but it
            # currently doesn't, so this reverts to parsing the fw
            # for any unsupported mosys platforms.
            crossystem = utils.Crossystem(self)
            crossystem.init()
            # Extract fwid value and use the leading part as the platform id.
            # fwid generally follow the format of {platform}.{firmware version}
            # Example: Alex.X.YYY.Z or Google_Alex.X.YYY.Z
            platform = crossystem.fwid().split('.')[0].lower()
            # Newer platforms start with 'Google_' while the older ones do not.
            return platform.replace('google_', '')


    def get_architecture(self):
        """Determine the correct architecture label for this host.

        @returns a string representing this host's architecture.
        """
        crossystem = utils.Crossystem(self)
        crossystem.init()
        return crossystem.arch()


    def get_chrome_version(self):
        """Gets the Chrome version number and milestone as strings.

        Invokes "chrome --version" to get the version number and milestone.

        @return A tuple (chrome_ver, milestone) where "chrome_ver" is the
            current Chrome version number as a string (in the form "W.X.Y.Z")
            and "milestone" is the first component of the version number
            (the "W" from "W.X.Y.Z").  If the version number cannot be parsed
            in the "W.X.Y.Z" format, the "chrome_ver" will be the full output
            of "chrome --version" and the milestone will be the empty string.

        """
        version_string = self.run(client_constants.CHROME_VERSION_COMMAND).stdout
        return utils.parse_chrome_version(version_string)


    def get_ec_version(self):
        """Get the ec version as strings.

        @returns a string representing this host's ec version.
        """
        command = 'mosys ec info -s fw_version'
        result = self.run(command, ignore_status=True)
        if result.exit_status != 0:
            return ''
        return result.stdout.strip()


    def get_firmware_version(self):
        """Get the firmware version as strings.

        @returns a string representing this host's firmware version.
        """
        crossystem = utils.Crossystem(self)
        crossystem.init()
        return crossystem.fwid()


    def get_hardware_revision(self):
        """Get the hardware revision as strings.

        @returns a string representing this host's hardware revision.
        """
        command = 'mosys platform version'
        result = self.run(command, ignore_status=True)
        if result.exit_status != 0:
            return ''
        return result.stdout.strip()


    def get_kernel_version(self):
        """Get the kernel version as strings.

        @returns a string representing this host's kernel version.
        """
        return self.run('uname -r').stdout.strip()


    def is_chrome_switch_present(self, switch):
        """Returns True if the specified switch was provided to Chrome.

        @param switch The chrome switch to search for.
        """

        command = 'pgrep -x -f -c "/opt/google/chrome/chrome.*%s.*"' % switch
        return self.run(command, ignore_status=True).exit_status == 0


    def oobe_triggers_update(self):
        """Returns True if this host has an OOBE flow during which
        it will perform an update check and perhaps an update.
        One example of such a flow is Hands-Off Zero-Touch Enrollment.
        As more such flows are developed, code handling them needs
        to be added here.

        @return Boolean indicating whether this host's OOBE triggers an update.
        """
        return self.is_chrome_switch_present(
            '--enterprise-enable-zero-touch-enrollment=hands-off')


    # TODO(kevcheng): change this to just return the board without the
    # 'board:' prefix and fix up all the callers.  Also look into removing the
    # need for this method.
    def get_board(self):
        """Determine the correct board label for this host.

        @returns a string representing this host's board.
        """
        release_info = utils.parse_cmd_output('cat /etc/lsb-release',
                                              run_method=self.run)
        return (ds_constants.BOARD_PREFIX +
                release_info['CHROMEOS_RELEASE_BOARD'])

    def get_channel(self):
        """Determine the correct channel label for this host.

        @returns: a string represeting this host's build channel.
                  (stable, dev, beta). None on fail.
        """
        return lsbrelease_utils.get_chromeos_channel(
                lsb_release_content=self._get_lsb_release_content())

    def has_lightsensor(self):
        """Determine the correct board label for this host.

        @returns the string 'lightsensor' if this host has a lightsensor or
                 None if it does not.
        """
        search_cmd = "find -L %s -maxdepth 4 | egrep '%s'" % (
            self._LIGHTSENSOR_SEARCH_DIR, '|'.join(self._LIGHTSENSOR_FILES))
        try:
            # Run the search cmd following the symlinks. Stderr_tee is set to
            # None as there can be a symlink loop, but the command will still
            # execute correctly with a few messages printed to stderr.
            self.run(search_cmd, stdout_tee=None, stderr_tee=None)
            return 'lightsensor'
        except error.AutoservRunError:
            # egrep exited with a return code of 1 meaning none of the possible
            # lightsensor files existed.
            return None


    def has_bluetooth(self):
        """Determine the correct board label for this host.

        @returns the string 'bluetooth' if this host has bluetooth or
                 None if it does not.
        """
        try:
            self.run('test -d /sys/class/bluetooth/hci0')
            # test exited with a return code of 0.
            return 'bluetooth'
        except error.AutoservRunError:
            # test exited with a return code 1 meaning the directory did not
            # exist.
            return None


    def get_accels(self):
        """
        Determine the type of accelerometers on this host.

        @returns a string representing this host's accelerometer type.
        At present, it only returns "accel:cros-ec", for accelerometers
        attached to a Chrome OS EC, or none, if no accelerometers.
        """
        # Check to make sure we have ectool
        rv = self.run('which ectool', ignore_status=True)
        if rv.exit_status:
            logging.info("No ectool cmd found, assuming no EC accelerometers")
            return None

        # Check that the EC supports the motionsense command
        rv = self.run('ectool motionsense', ignore_status=True)
        if rv.exit_status:
            logging.info("EC does not support motionsense command "
                         "assuming no EC accelerometers")
            return None

        # Check that EC motion sensors are active
        active = self.run('ectool motionsense active').stdout.split('\n')
        if active[0] == "0":
            logging.info("Motion sense inactive, assuming no EC accelerometers")
            return None

        logging.info("EC accelerometers found")
        return 'accel:cros-ec'


    def has_chameleon(self):
        """Determine if a Chameleon connected to this host.

        @returns a list containing two strings ('chameleon' and
                 'chameleon:' + label, e.g. 'chameleon:hdmi') if this host
                 has a Chameleon or None if it has not.
        """
        if self._chameleon_host:
            return ['chameleon', 'chameleon:' + self.chameleon.get_label()]
        else:
            return None


    def has_loopback_dongle(self):
        """Determine if an audio loopback dongle is plugged to this host.

        @returns 'audio_loopback_dongle' when there is an audio loopback dongle
                                         plugged to this host.
                 None                    when there is no audio loopback dongle
                                         plugged to this host.
        """
        nodes_info = self.run(command=cras_utils.get_cras_nodes_cmd(),
                              ignore_status=True).stdout
        if (cras_utils.node_type_is_plugged('HEADPHONE', nodes_info) and
            cras_utils.node_type_is_plugged('MIC', nodes_info)):
                return 'audio_loopback_dongle'
        else:
                return None


    def get_power_supply(self):
        """
        Determine what type of power supply the host has

        @returns a string representing this host's power supply.
                 'power:battery' when the device has a battery intended for
                        extended use
                 'power:AC_primary' when the device has a battery not intended
                        for extended use (for moving the machine, etc)
                 'power:AC_only' when the device has no battery at all.
        """
        psu = self.run(command='mosys psu type', ignore_status=True)
        if psu.exit_status:
            # The psu command for mosys is not included for all platforms. The
            # assumption is that the device will have a battery if the command
            # is not found.
            return 'power:battery'

        psu_str = psu.stdout.strip()
        if psu_str == 'unknown':
            return None

        return 'power:%s' % psu_str


    def get_storage(self):
        """
        Determine the type of boot device for this host.

        Determine if the internal device is SCSI or dw_mmc device.
        Then check that it is SSD or HDD or eMMC or something else.

        @returns a string representing this host's internal device type.
                 'storage:ssd' when internal device is solid state drive
                 'storage:hdd' when internal device is hard disk drive
                 'storage:mmc' when internal device is mmc drive
                 None          When internal device is something else or
                               when we are unable to determine the type
        """
        # The output should be /dev/mmcblk* for SD/eMMC or /dev/sd* for scsi
        rootdev_cmd = ' '.join(['. /usr/sbin/write_gpt.sh;',
                                '. /usr/share/misc/chromeos-common.sh;',
                                'load_base_vars;',
                                'get_fixed_dst_drive'])
        rootdev = self.run(command=rootdev_cmd, ignore_status=True)
        if rootdev.exit_status:
            logging.info("Fail to run %s", rootdev_cmd)
            return None
        rootdev_str = rootdev.stdout.strip()

        if not rootdev_str:
            return None

        rootdev_base = os.path.basename(rootdev_str)

        mmc_pattern = '/dev/mmcblk[0-9]'
        if re.match(mmc_pattern, rootdev_str):
            # Use type to determine if the internal device is eMMC or somthing
            # else. We can assume that MMC is always an internal device.
            type_cmd = 'cat /sys/block/%s/device/type' % rootdev_base
            type = self.run(command=type_cmd, ignore_status=True)
            if type.exit_status:
                logging.info("Fail to run %s", type_cmd)
                return None
            type_str = type.stdout.strip()

            if type_str == 'MMC':
                return 'storage:mmc'

        scsi_pattern = '/dev/sd[a-z]+'
        if re.match(scsi_pattern, rootdev.stdout):
            # Read symlink for /sys/block/sd* to determine if the internal
            # device is connected via ata or usb.
            link_cmd = 'readlink /sys/block/%s' % rootdev_base
            link = self.run(command=link_cmd, ignore_status=True)
            if link.exit_status:
                logging.info("Fail to run %s", link_cmd)
                return None
            link_str = link.stdout.strip()
            if 'usb' in link_str:
                return None

            # Read rotation to determine if the internal device is ssd or hdd.
            rotate_cmd = str('cat /sys/block/%s/queue/rotational'
                              % rootdev_base)
            rotate = self.run(command=rotate_cmd, ignore_status=True)
            if rotate.exit_status:
                logging.info("Fail to run %s", rotate_cmd)
                return None
            rotate_str = rotate.stdout.strip()

            rotate_dict = {'0':'storage:ssd', '1':'storage:hdd'}
            return rotate_dict.get(rotate_str)

        # All other internal device / error case will always fall here
        return None


    def get_servo(self):
        """Determine if the host has a servo attached.

        If the host has a working servo attached, it should have a servo label.

        @return: string 'servo' if the host has servo attached. Otherwise,
                 returns None.
        """
        return 'servo' if self._servo_host else None


    def get_video_labels(self):
        """Run /usr/local/bin/avtest_label_detect to get a list of video labels.

        Sample output of avtest_label_detect:
        Detected label: hw_video_acc_vp8
        Detected label: webcam

        @return: A list of labels detected by tool avtest_label_detect.
        """
        try:
            result = self.run('/usr/local/bin/avtest_label_detect').stdout
            return re.findall('^Detected label: (\w+)$', result, re.M)
        except error.AutoservRunError:
            # The tool is not installed.
            return []


    def is_video_glitch_detection_supported(self):
        """ Determine if a board under test is supported for video glitch
        detection tests.

        @return: 'video_glitch_detection' if board is supported, None otherwise.
        """
        board = self.get_board().replace(ds_constants.BOARD_PREFIX, '')

        if board in video_test_constants.SUPPORTED_BOARDS:
            return 'video_glitch_detection'

        return None


    def get_touch(self):
        """
        Determine whether board under test has a touchpad or touchscreen.

        @return: A list of some combination of 'touchscreen' and 'touchpad',
            depending on what is present on the device.

        """
        labels = []
        looking_for = ['touchpad', 'touchscreen']
        player = input_playback.InputPlayback()
        input_events = self.run('ls /dev/input/event*').stdout.strip().split()
        filename = '/tmp/touch_labels'
        for event in input_events:
            self.run('evtest %s > %s' % (event, filename), timeout=1,
                     ignore_timeout=True)
            properties = self.run('cat %s' % filename).stdout
            input_type = player._determine_input_type(properties)
            if input_type in looking_for:
                labels.append(input_type)
                looking_for.remove(input_type)
            if len(looking_for) == 0:
                break
        self.run('rm %s' % filename)

        return labels


    def has_internal_display(self):
        """Determine if the device under test is equipped with an internal
        display.

        @return: 'internal_display' if one is present; None otherwise.
        """
        from autotest_lib.client.cros.graphics import graphics_utils
        from autotest_lib.client.common_lib import utils as common_utils

        def __system_output(cmd):
            return self.run(cmd).stdout

        def __read_file(remote_path):
            return self.run('cat %s' % remote_path).stdout

        # Hijack the necessary client functions so that we can take advantage
        # of the client lib here.
        # FIXME: find a less hacky way than this
        original_system_output = utils.system_output
        original_read_file = common_utils.read_file
        utils.system_output = __system_output
        common_utils.read_file = __read_file
        try:
            return ('internal_display' if graphics_utils.has_internal_display()
                                   else None)
        finally:
            utils.system_output = original_system_output
            common_utils.read_file = original_read_file


    def is_boot_from_usb(self):
        """Check if DUT is boot from USB.

        @return: True if DUT is boot from usb.
        """
        device = self.run('rootdev -s -d').stdout.strip()
        removable = int(self.run('cat /sys/block/%s/removable' %
                                 os.path.basename(device)).stdout.strip())
        return removable == 1


    def read_from_meminfo(self, key):
        """Return the memory info from /proc/meminfo

        @param key: meminfo requested

        @return the memory value as a string

        """
        meminfo = self.run('grep %s /proc/meminfo' % key).stdout.strip()
        logging.debug('%s', meminfo)
        return int(re.search(r'\d+', meminfo).group(0))


    def get_cpu_arch(self):
        """Returns CPU arch of the device.

        @return CPU architecture of the DUT.
        """
        # Add CPUs by following logic in client/bin/utils.py.
        if self.run("grep '^flags.*:.* lm .*' /proc/cpuinfo",
                ignore_status=True).stdout:
            return 'x86_64'
        if self.run("grep -Ei 'ARM|CPU implementer' /proc/cpuinfo",
                ignore_status=True).stdout:
            return 'arm'
        return 'i386'


    def get_board_type(self):
        """
        Get the DUT's device type from /etc/lsb-release.
        DEVICETYPE can be one of CHROMEBOX, CHROMEBASE, CHROMEBOOK or more.

        @return value of DEVICETYPE param from lsb-release.
        """
        device_type = self.run('grep DEVICETYPE /etc/lsb-release',
                               ignore_status=True).stdout
        if device_type:
            return device_type.split('=')[-1].strip()
        return ''


    def get_arc_version(self):
        """Return ARC version installed on the DUT.

        @returns ARC version as string if the CrOS build has ARC, else None.
        """
        arc_version = self.run('grep CHROMEOS_ARC_VERSION /etc/lsb-release',
                               ignore_status=True).stdout
        if arc_version:
            return arc_version.split('=')[-1].strip()
        return None


    def get_os_type(self):
        return 'cros'


    def enable_adb_testing(self):
        """Mark this host as an adb tester."""
        self.run('touch %s' % constants.ANDROID_TESTER_FILEFLAG)


    def get_labels(self):
        """Return the detected labels on the host."""
        return self.labels.get_labels(self)


    def update_labels(self):
        """Update the labels on the host."""
        self.labels.update_labels(self)
