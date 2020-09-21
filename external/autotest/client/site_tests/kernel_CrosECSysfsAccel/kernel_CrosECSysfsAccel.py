# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging, os
import math
from autotest_lib.client.bin import utils, test
from autotest_lib.client.common_lib import error


class kernel_CrosECSysfsAccel(test.test):
    '''Make sure the EC sysfs accel interface provides meaningful output'''
    version = 1


    # For EC accelerometer, define the number of counts in 1G, and the number
    # of counts that the magnitude of each sensor is allowed to be off from a
    # magnitude of 1G. These values are not sensor dependent, they are based
    # on the EC sysfs interface, which specifies number of counts in 1G.
    _ACCEL_1G_IN_G = 1024
    _ACCEL_1G_IN_MS2 = 9.8185
    _ACCEL_MAG_VALID_OFFSET = .25

    _ACCEL_BASE_LOC = 'base'
    _ACCEL_LID_LOC = 'lid'
    _ACCEL_LOCS = [_ACCEL_BASE_LOC, _ACCEL_LID_LOC]


    sysfs_accel_search_path = '/sys/bus/iio/devices'
    sysfs_accel_paths = {}
    sysfs_accel_old_path = ''
    new_sysfs_layout = True

    @classmethod
    def _read_sysfs_accel_file(cls, fullpath):
        """
        Read the contents of the given accel sysfs file or fail

        @param fullpath Name of the file within the accel sysfs interface
        directory
        """
        try:
            content = utils.read_file(fullpath)
        except Exception as err:
            raise error.TestFail('sysfs file problem: %s' % err)
        return content


    def _find_sysfs_accel_dir(self):
        """
        Return the sysfs directory for accessing EC accels
        """
        for _, dirs, _ in os.walk(self.sysfs_accel_search_path):
            for d in dirs:
                dirpath = os.path.join(self.sysfs_accel_search_path, d)
                namepath = os.path.join(dirpath, 'name')

                try:
                    content = utils.read_file(namepath)
                except IOError as err:
                    # errno 2 is code for file does not exist, which is ok
                    # here, just continue on to next directory. Any other
                    # error is a problem, raise an error.
                    if err.errno == 2:
                        continue
                    raise error.TestFail('IOError %d while searching for accel'
                                         'sysfs dir in %s', err.errno, namepath)

                # Correct directory has a file called 'name' with contents
                # 'cros-ec-accel'
                if content.strip() != 'cros-ec-accel':
                    continue

                locpath = os.path.join(dirpath, 'location')
                try:
                    location = utils.read_file(locpath)
                except IOError as err:
                    if err.errno == 2:
                        # We have an older scheme
                        self.new_sysfs_layout = False
                        self.sysfs_accel_old_path = dirpath
                        return
                    raise error.TestFail('IOError %d while reading %s',
                                         err.errno, locpath)
                loc = location.strip()
                if loc in self._ACCEL_LOCS:
                    self.sysfs_accel_paths[loc] = dirpath

        if (not self.sysfs_accel_old_path and
            len(self.sysfs_accel_paths) == 0):
            raise error.TestFail('No sysfs interface to EC accels (cros-ec-accel)')

    def _verify_accel_data(self, name):
        """
        Verify one of the EC accelerometers through the sysfs interface.
        """
        if self.new_sysfs_layout:
            accel_scale = float(self._read_sysfs_accel_file(
                os.path.join(self.sysfs_accel_paths[name],
                             'scale')))
            exp = self._ACCEL_1G_IN_MS2
        else:
            accel_scale = 1
            exp = self._ACCEL_1G_IN_G

        err = exp * self._ACCEL_MAG_VALID_OFFSET
        value = {}
        mag = 0
        for axis in ['x', 'y', 'z']:
            name_list = ['in', 'accel', axis]
            if self.new_sysfs_layout:
                base_path = self.sysfs_accel_paths[name]
            else:
                base_path = self.sysfs_accel_old_path
                name_list.append(name)
            name_list.append('raw')
            axis_path = os.path.join(base_path, '_'.join(name_list))
            value[axis] = int(self._read_sysfs_accel_file(axis_path))
            value[axis] *= accel_scale
            mag += value[axis] * value[axis]

        mag = math.sqrt(mag)

        # Accel data is out of range if magnitude is not close to 1G.
        # Note, this means test will fail on the moon.
        if abs(mag - exp) <= err:
            logging.info("%s accel passed. Magnitude is %f.", name, mag)
        else:
            logging.info("%s accel bad data. Magnitude is %f, expected "
                         "%f +/-%f. Raw data is x:%f, y:%f, z:%f.", name,
                         mag, exp, err, value['x'], value['y'], value['z'])
            raise error.TestFail("Accel magnitude out of range.")


    def run_once(self):
        """
        Check for accelerometers, and if present, check data is valid
        """
        # First make sure that the motion sensors are active. If this
        # check fails it means the EC motion sense task is not running and
        # therefore not updating acceleration values in shared memory.
        active = utils.system_output('ectool motionsense active')
        if active == "0":
            raise error.TestFail("Motion sensing is inactive")

        # Find the iio sysfs directory for EC accels
        self._find_sysfs_accel_dir()

        if self.sysfs_accel_old_path:
            # Get all accelerometer data
            accel_info = utils.system_output('ectool motionsense')
            info = accel_info.splitlines()

            # If the base accelerometer is present, then verify data
            if 'None' not in info[1]:
                self._verify_accel_data(self._ACCEL_BASE_LOC)

            # If the lid accelerometer is present, then verify data
            if 'None' not in info[2]:
                self._verify_accel_data(self._ACCEL_LID_LOC)
        else:
            for loc in self.sysfs_accel_paths.keys():
                self._verify_accel_data(loc)
