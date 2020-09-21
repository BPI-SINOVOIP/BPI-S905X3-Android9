# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This class defines the TestBed class."""

import logging
import re
import sys
import threading
import traceback
from multiprocessing import pool

import common

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import logging_config
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server import autoserv_parser
from autotest_lib.server import utils
from autotest_lib.server.cros import provision
from autotest_lib.server.hosts import adb_host
from autotest_lib.server.hosts import base_label
from autotest_lib.server.hosts import host_info
from autotest_lib.server.hosts import testbed_label
from autotest_lib.server.hosts import teststation_host


# Thread pool size to provision multiple devices in parallel.
_POOL_SIZE = 4

# Pattern for the image name when used to provision a dut connected to testbed.
# It should follow the naming convention of
# branch/target/build_id[:serial][#count],
# where serial and count are optional. Count is the number of devices to
# provision to.
_IMAGE_NAME_PATTERN = '(.*/.*/[^:#]*)(?::(.*))?(?:#(\d+))?'

class TestBed(object):
    """This class represents a collection of connected teststations and duts."""

    _parser = autoserv_parser.autoserv_parser
    VERSION_PREFIX = provision.TESTBED_BUILD_VERSION_PREFIX
    support_devserver_provision = False

    def __init__(self, hostname='localhost', afe_host=None, adb_serials=None,
                 host_info_store=None, **dargs):
        """Initialize a TestBed.

        This will create the Test Station Host and connected hosts (ADBHost for
        now) and allow the user to retrieve them.

        @param hostname: Hostname of the test station connected to the duts.
        @param adb_serials: List of adb device serials.
        @param host_info_store: A CachingHostInfoStore object.
        @param afe_host: The host object attained from the AFE (get_hosts).
        """
        logging.info('Initializing TestBed centered on host: %s', hostname)
        self.hostname = hostname
        self._afe_host = afe_host or utils.EmptyAFEHost()
        self.host_info_store = (host_info_store or
                                host_info.InMemoryHostInfoStore())
        self.labels = base_label.LabelRetriever(testbed_label.TESTBED_LABELS)
        self.teststation = teststation_host.create_teststationhost(
                hostname=hostname, afe_host=self._afe_host, **dargs)
        self.is_client_install_supported = False
        serials_from_attributes = self._afe_host.attributes.get('serials')
        if serials_from_attributes:
            serials_from_attributes = serials_from_attributes.split(',')

        self.adb_device_serials = (adb_serials or
                                   serials_from_attributes or
                                   self.query_adb_device_serials())
        self.adb_devices = {}
        for adb_serial in self.adb_device_serials:
            self.adb_devices[adb_serial] = adb_host.ADBHost(
                hostname=hostname, teststation=self.teststation,
                adb_serial=adb_serial, afe_host=self._afe_host,
                host_info_store=self.host_info_store, **dargs)


    def query_adb_device_serials(self):
        """Get a list of devices currently attached to the test station.

        @returns a list of adb devices.
        """
        return adb_host.ADBHost.parse_device_serials(
                self.teststation.run('adb devices').stdout)


    def get_all_hosts(self):
        """Return a list of all the hosts in this testbed.

        @return: List of the hosts which includes the test station and the adb
                 devices.
        """
        device_list = [self.teststation]
        device_list.extend(self.adb_devices.values())
        return device_list


    def get_test_station(self):
        """Return the test station host object.

        @return: The test station host object.
        """
        return self.teststation


    def get_adb_devices(self):
        """Return the adb host objects.

        @return: A dict of adb device serials to their host objects.
        """
        return self.adb_devices


    def get_labels(self):
        """Return a list of the labels gathered from the devices connected.

        @return: A list of strings that denote the labels from all the devices
                 connected.
        """
        return self.labels.get_labels(self)


    def update_labels(self):
        """Update the labels on the testbed."""
        return self.labels.update_labels(self)


    def get_platform(self):
        """Return the platform of the devices.

        @return: A string representing the testbed platform.
        """
        return 'testbed'


    def repair(self):
        """Run through repair on all the devices."""
        # board name is needed for adb_host to repair as the adb_host objects
        # created for testbed doesn't have host label and attributes retrieved
        # from AFE.
        info = self.host_info_store.get()
        board = info.board
        # Remove the tailing -# in board name as it can be passed in from
        # testbed board labels
        match = re.match(r'^(.*)-\d+$', board)
        if match:
            board = match.group(1)
        failures = []
        for adb_device in self.get_adb_devices().values():
            try:
                adb_device.repair(board=board, os=info.os)
            except:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                failures.append((adb_device.adb_serial, exc_type, exc_value,
                                 exc_traceback))
        if failures:
            serials = []
            for serial, exc_type, exc_value, exc_traceback in failures:
                serials.append(serial)
                details = ''.join(traceback.format_exception(
                        exc_type, exc_value, exc_traceback))
                logging.error('Failed to repair device with serial %s, '
                              'error:\n%s', serial, details)
            raise error.AutoservRepairTotalFailure(
                    'Fail to repair %d devices: %s' %
                    (len(serials), ','.join(serials)))


    def verify(self):
        """Run through verify on all the devices."""
        for device in self.get_all_hosts():
            device.verify()


    def cleanup(self):
        """Run through cleanup on all the devices."""
        for adb_device in self.get_adb_devices().values():
            adb_device.cleanup()


    def _parse_image(self, image_string):
        """Parse the image string to a dictionary.

        Sample value of image_string:
        Provision dut with serial ZX1G2 to build `branch1/shamu-userdebug/111`,
        and provision another shamu with build `branch2/shamu-userdebug/222`
        branch1/shamu-userdebug/111:ZX1G2,branch2/shamu-userdebug/222

        Provision 10 shamu with build `branch1/shamu-userdebug/LATEST`
        branch1/shamu-userdebug/LATEST#10

        @param image_string: A comma separated string of images. The image name
                is in the format of branch/target/build_id[:serial]. Serial is
                optional once testbed machine_install supports allocating DUT
                based on board.

        @returns: A list of tuples of (build, serial). serial could be None if
                  it's not specified.
        """
        images = []
        for image in image_string.split(','):
            match = re.match(_IMAGE_NAME_PATTERN, image)
            # The image string cannot specify both serial and count.
            if not match or (match.group(2) and match.group(3)):
                raise error.InstallError(
                        'Image name of "%s" has invalid format. It should '
                        'follow naming convention of '
                        'branch/target/build_id[:serial][#count]', image)
            if match.group(3):
                images.extend([(match.group(1), None)]*int(match.group(3)))
            else:
                images.append((match.group(1), match.group(2)))
        return images


    @staticmethod
    def _install_device(inputs):
        """Install build to a device with the given inputs.

        @param inputs: A dictionary of the arguments needed to install a device.
            Keys include:
            host: An ADBHost object of the device.
            build_url: Devserver URL to the build to install.
        """
        host = inputs['host']
        build_url = inputs['build_url']
        build_local_path = inputs['build_local_path']

        # Set the thread name with the serial so logging for installing
        # different devices can have different thread name.
        threading.current_thread().name = host.adb_serial
        logging.info('Starting installing device %s:%s from build url %s',
                     host.hostname, host.adb_serial, build_url)
        host.machine_install(build_url=build_url,
                             build_local_path=build_local_path)
        logging.info('Finished installing device %s:%s from build url %s',
                     host.hostname, host.adb_serial, build_url)


    def locate_devices(self, images):
        """Locate device for each image in the given images list.

        If the given images all have no serial associated and have the same
        image for the same board, testbed will assign all devices with the
        desired board to the image. This allows tests to randomly pick devices
        to run.
        As an example, a testbed with 4 devices, 2 for board_1 and 2 for
        board_2. If the given images value is:
        [('board_1_build', None), ('board_2_build', None)]
        The testbed will return following device allocation:
        {'serial_1_board_1': 'board_1_build',
         'serial_2_board_1': 'board_1_build',
         'serial_1_board_2': 'board_2_build',
         'serial_2_board_2': 'board_2_build',
        }
        That way, all board_1 duts will be installed with board_1_build, and
        all board_2 duts will be installed with board_2_build. Test can pick
        any dut from board_1 duts and same applies to board_2 duts.

        @param images: A list of tuples of (build, serial). serial could be None
                if it's not specified. Following are some examples:
                [('branch1/shamu-userdebug/100', None),
                 ('branch1/shamu-userdebug/100', None)]
                [('branch1/hammerhead-userdebug/100', 'XZ123'),
                 ('branch1/hammerhead-userdebug/200', None)]
                where XZ123 is serial of one of the hammerheads connected to the
                testbed.

        @return: A dictionary of (serial, build). Note that build here should
                 not have a serial specified in it.
        @raise InstallError: If not enough duts are available to install the
                given images. Or there are more duts with the same board than
                the images list specified.
        """
        # The map between serial and build to install in that dut.
        serial_build_pairs = {}
        builds_without_serial = [build for build, serial in images
                                 if not serial]
        for build, serial in images:
            if serial:
                serial_build_pairs[serial] = build
        # Return the mapping if all builds have serial specified.
        if not builds_without_serial:
            return serial_build_pairs

        # serials grouped by the board of duts.
        duts_by_name = {}
        for serial, host in self.get_adb_devices().iteritems():
            # Excluding duts already assigned to a build.
            if serial in serial_build_pairs:
                continue
            aliases = host.get_device_aliases()
            for alias in aliases:
                duts_by_name.setdefault(alias, []).append(serial)

        # Builds grouped by the board name.
        builds_by_name = {}
        for build in builds_without_serial:
            match = re.match(adb_host.BUILD_REGEX, build)
            if not match:
                raise error.InstallError('Build %s is invalid. Failed to parse '
                                         'the board name.' % build)
            name = match.group('BUILD_TARGET')
            builds_by_name.setdefault(name, []).append(build)

        # Pair build with dut with matching board.
        for name, builds in builds_by_name.iteritems():
            duts = duts_by_name.get(name, [])
            if len(duts) < len(builds):
                raise error.InstallError(
                        'Expected number of DUTs for name %s is %d, got %d' %
                        (name, len(builds), len(duts) if duts else 0))
            elif len(duts) == len(builds):
                serial_build_pairs.update(dict(zip(duts, builds)))
            else:
                # In this cases, available dut number is greater than the number
                # of builds.
                if len(set(builds)) > 1:
                    raise error.InstallError(
                            'Number of available DUTs are greater than builds '
                            'needed, testbed cannot allocate DUTs for testing '
                            'deterministically.')
                # Set all DUTs to the same build.
                for serial in duts:
                    serial_build_pairs[serial] = builds[0]

        return serial_build_pairs


    def save_info(self, results_dir):
        """Saves info about the testbed to a directory.

        @param results_dir: The directory to save to.
        """
        for device in self.get_adb_devices().values():
            device.save_info(results_dir, include_build_info=True)


    def _stage_shared_build(self, serial_build_map):
        """Try to stage build on teststation to be shared by all provision jobs.

        This logic only applies to the case that multiple devices are
        provisioned to the same build. If the provision job does not fit this
        requirement, this method will not stage any build.

        @param serial_build_map: A map between dut's serial and the build to be
                installed.

        @return: A tuple of (build_url, build_local_path, teststation), where
                build_url: url to the build on devserver
                build_local_path: Path to a local directory in teststation that
                                  contains the build.
                teststation: A teststation object that is used to stage the
                             build.
                If there are more than one build need to be staged or only one
                device is used for the test, return (None, None, None)
        """
        build_local_path = None
        build_url = None
        teststation = None
        same_builds = set([build for build in serial_build_map.values()])
        if len(same_builds) == 1 and len(serial_build_map.values()) > 1:
            same_build = same_builds.pop()
            logging.debug('All devices will be installed with build %s, stage '
                          'the shared build to be used for all provision jobs.',
                          same_build)
            stage_host = self.get_adb_devices()[serial_build_map.keys()[0]]
            teststation = stage_host.teststation
            build_url, _ = stage_host.stage_build_for_install(same_build)
            if stage_host.get_os_type() == adb_host.OS_TYPE_ANDROID:
                build_local_path = stage_host.stage_android_image_files(
                        build_url)
            else:
                build_local_path = stage_host.stage_brillo_image_files(
                        build_url)
        elif len(same_builds) > 1:
            logging.debug('More than one build need to be staged, leave the '
                          'staging build tasks to individual provision job.')
        else:
            logging.debug('Only one device needs to be provisioned, leave the '
                          'staging build task to individual provision job.')

        return build_url, build_local_path, teststation


    def machine_install(self, image=None):
        """Install the DUT.

        @param image: Image we want to install on this testbed, e.g.,
                      `branch1/shamu-eng/1001,branch2/shamu-eng/1002`

        @returns A tuple of (the name of the image installed, None), where None
                is a placeholder for update_url. Testbed does not have a single
                update_url, thus it's set to None.
        @returns A tuple of (image_name, host_attributes).
                image_name is the name of images installed, e.g.,
                `branch1/shamu-eng/1001,branch2/shamu-eng/1002`
                host_attributes is a dictionary of (attribute, value), which
                can be saved to afe_host_attributes table in database. This
                method returns a dictionary with entries of job_repo_urls for
                each provisioned devices:
                `job_repo_url_[adb_serial]`: devserver_url, where devserver_url
                is a url to the build staged on devserver.
                For example:
                {'job_repo_url_XZ001': 'http://10.1.1.3/branch1/shamu-eng/1001',
                 'job_repo_url_XZ002': 'http://10.1.1.3/branch2/shamu-eng/1002'}
        """
        image = image or self._parser.options.image
        if not image:
            raise error.InstallError('No image string is provided to test bed.')
        images = self._parse_image(image)
        host_attributes = {}

        # Change logging formatter to include thread name. This is to help logs
        # from each provision runs have the dut's serial, which is set as the
        # thread name.
        logging_config.add_threadname_in_log()

        serial_build_map = self.locate_devices(images)

        build_url, build_local_path, teststation = self._stage_shared_build(
                serial_build_map)

        thread_pool = None
        try:
            arguments = []
            for serial, build in serial_build_map.iteritems():
                logging.info('Installing build %s on DUT with serial %s.',
                             build, serial)
                host = self.get_adb_devices()[serial]
                if build_url:
                    device_build_url = build_url
                else:
                    device_build_url, _ = host.stage_build_for_install(build)
                arguments.append({'host': host,
                                  'build_url': device_build_url,
                                  'build_local_path': build_local_path})
                attribute_name = '%s_%s' % (constants.JOB_REPO_URL,
                                            host.adb_serial)
                host_attributes[attribute_name] = device_build_url

            thread_pool = pool.ThreadPool(_POOL_SIZE)
            thread_pool.map(self._install_device, arguments)
            thread_pool.close()
        except Exception as err:
            logging.error(err.message)
        finally:
            if thread_pool:
                thread_pool.join()

            if build_local_path:
                logging.debug('Clean up build artifacts %s:%s',
                              teststation.hostname, build_local_path)
                teststation.run('rm -rf %s' % build_local_path)

        return image, host_attributes


    def get_attributes_to_clear_before_provision(self):
        """Get a list of attribute to clear before machine_install starts.
        """
        return [host.job_repo_url_attribute for host in
                self.adb_devices.values()]
