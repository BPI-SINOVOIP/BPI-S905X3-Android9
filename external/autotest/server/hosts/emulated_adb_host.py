# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import sys

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import utils
from autotest_lib.server.hosts import adb_host
from autotest_lib.utils import emulator_manager

OS_TYPE_EMULATED_BRILLO = 'emulated_brillo'
OS_TYPE_DEFAULT = OS_TYPE_EMULATED_BRILLO
BOARD_DEFAULT = "brilloemulator_arm"
EMULATED_BRILLO_ARTIFACT_FORMAT = (
    '%(build_target)s-target_files-%(build_id)s.zip')
EMULATED_BRILLO_DTB_FORMAT = (
    '%(build_target)s-dtb-%(build_id)s.zip')


class EmulatedADBHost(adb_host.ADBHost):
    """Run an emulator as an ADB device preserving the API and assumptions of
    ADBHost.

    Currently supported emulators:
    * Brillo
        * brilloemulator_arm
    """

    def _initialize(self, *args, **kwargs):
        """Intialize an emulator so that existing assumptions that the host is
        always ready ar satisfied.

        @param args: pass through to ADBHost
        @param kwargs: pass through to ADBHost
        """
        super(EmulatedADBHost, self)._initialize(*args, **kwargs)

        # Verify serial
        m = re.match('emulator-(\d{4})', self.adb_serial)
        if not m:
            raise ValueError('Emulator serial must be in the format '
                             'emulator-PORT.')
        self.port = int(m.group(1)) + 1

        # Determine directory for images (needs to be persistent)
        tmp_dir = self.teststation.get_tmp_dir()
        self.imagedir = os.path.join(os.path.dirname(tmp_dir), self.adb_serial)
        self.teststation.run('rm -rf %s' % tmp_dir)
        self.teststation.run('mkdir -p %s' % self.imagedir)

        # Boot the emulator, if not already booted, since ADBHost assumes the
        # device is always available
        self._emulator = emulator_manager.EmulatorManager(
                self.imagedir, self.port, run=self.teststation.run)
        self._start_emulator_if_not_started()


    def _start_emulator_if_not_started(self):
        """Boot or reboot the emulator if necessary.

        If the emulator is not started boot the emulator. Otherwise leave it
        alone. Ensure emulator is running and ready before returning.
        """
        host_os = self.get_os_type()
        board = self.get_board()

        # Check that images exist in imagedir
        try:
            self.teststation.run('test -f %s' % os.path.join(self.imagedir,
                                                            'system.img'))

        # Use default images
        except error.GenericHostRunError:
            self.teststation.run('cp %s/* %s/' % (
                os.path.join('/usr/local/emulator_images', host_os, board),
                self.imagedir
            ))

        if not self._emulator.find():
            self._emulator.start()
        self.wait_up()
        self._reset_adbd_connection()


    def get_os_type(self):
        """Determine the OS type from afe_host object or use the default if
        no os label / no afe_host object.

        @return: os type as str
        """
        info = self.host_info_store.get()
        return info.os or OS_TYPE_DEFAULT


    def get_board(self):
        """Determine the board from afe_host object or use the default if
        no board label / no afe_host object.

        @return: board as str
        """
        info = self.host_info_store.get()
        return info.board or BOARD_DEFAULT


    @staticmethod
    def check_host(host, timeout=10):
        """No dynamic host checking. Must be explicit.

        @param host: ignored
        @param timeout: ignored

        @return: False
        """
        return False


    def stage_emulator_artifact(self, build_url):
        """Download required build artifact from the given build_url to a
        local directory in the machine running the emulator.

        @param build_url: The url to use for downloading Android artifacts.
                          pattern: http://$devserv/static/branch/target/build_id

        @return: Path to the directory contains image files.
        """
        build_info = self.get_build_info_from_build_url(build_url)

        zipped_artifact = EMULATED_BRILLO_ARTIFACT_FORMAT % build_info
        dtb_artifact = EMULATED_BRILLO_DTB_FORMAT % build_info
        image_dir = self.teststation.get_tmp_dir()

        try:
            self.download_file(build_url, zipped_artifact, image_dir,
                               unzip=True)
            self.download_file(build_url, dtb_artifact, image_dir,
                               unzip=True)
            return image_dir
        except:
            self.teststation.run('rm -rf %s' % image_dir)
            raise


    def setup_brillo_emulator(self, build_url, build_local_path=None):
        """Install the Brillo DUT.

        Following are the steps used here to provision an android device:
        1. If build_local_path is not set, download the target_files zip, e.g.,
        brilloemulator_arm-target_files-123456.zip, and unzip it.
        2. Move the necessary images to a new directory.
        3. Determine port for ADB from serial.
        4. Use EmulatorManager to start the emulator.

        @param build_url: The url to use for downloading Android artifacts.
                          pattern: http://$devserver:###/static/$build
        @param build_local_path: The path to a local folder that contains the
                                 image files needed to provision the device.
                                 Note that the folder is in the machine running
                                 adb command, rather than the drone.

        @raises AndroidInstallError if any error occurs.
        """
        # If the build is not staged in local server yet, clean up the temp
        # folder used to store image files after the provision is completed.
        delete_build_folder = bool(not build_local_path)

        try:
            # Download image files needed for provision to a local directory.
            if not build_local_path:
                build_local_path = self.stage_emulator_artifact(build_url)

            # Create directory with required files
            self.teststation.run('rm -rf %s && mkdir %s' % (self.imagedir,
                                                            self.imagedir))
            self.teststation.run('mv %s %s' % (
                    os.path.join(build_local_path, 'IMAGES', 'system.img'),
                    os.path.join(self.imagedir, 'system.img')
            ))
            self.teststation.run('mv %s %s' % (
                    os.path.join(build_local_path, 'IMAGES', 'userdata.img'),
                    os.path.join(self.imagedir, 'userdata.img')
            ))
            self.teststation.run('mv %s %s' % (
                    os.path.join(build_local_path, 'BOOT', 'kernel'),
                    os.path.join(self.imagedir, 'kernel')
            ))
            self.teststation.run('mv %s/*.dtb %s' % (build_local_path,
                                                     self.imagedir))

            # Start the emulator
            self._emulator.force_stop()
            self._start_emulator_if_not_started()

        except Exception as e:
            logging.error('Install Brillo build failed with error: %s', e)
            # Re-raise the exception with type of AndroidInstallError.
            raise (adb_host.AndroidInstallError, sys.exc_info()[1],
                   sys.exc_info()[2])
        finally:
            if delete_build_folder:
                self.teststation.run('rm -rf %s' % build_local_path)
                logging.info('Successfully installed Android build staged at '
                             '%s.', build_url)


    def machine_install(self, build_url=None, build_local_path=None, wipe=True,
                        flash_all=False, os_type=None):
        """Install the DUT.

        @param build_url: The url to use for downloading Android artifacts.
                          pattern: http://$devserver:###/static/$build.
                          If build_url is set to None, the code may try
                          _parser.options.image to do the installation. If none
                          of them is set, machine_install will fail.
        @param build_local_path: The path to a local directory that contains the
                                 image files needed to provision the device.
        @param wipe: No-op
        @param flash_all: No-op
        @param os_type: OS to install (overrides label).

        @returns A tuple of (image_name, host_attributes). image_name is the
                 name of image installed, e.g.,
                 git_mnc-release/shamu-userdebug/1234
                 host_attributes is a dictionary of (attribute, value), which
                 can be saved to afe_host_attributes table in database. This
                 method returns a dictionary with a single entry of
                 `job_repo_url_[adb_serial]`: devserver_url, where devserver_url
                 is a url to the build staged on devserver.
        """
        os_type = os_type or self.get_os_type()
        if not build_url and self._parser.options.image:
            build_url, _ = self.stage_build_for_install(
                    self._parser.options.image, os_type=os_type)
        if os_type == OS_TYPE_EMULATED_BRILLO:
            self.setup_brillo_emulator(
                    build_url=build_url, build_local_path=build_local_path)
            self.ensure_adb_mode()
        else:
            raise error.InstallError(
                    'Installation of os type %s is not supported.' %
                    os_type)
        return (build_url.split('static/')[-1],
                {self.job_repo_url_attribute: build_url})


    def repair(self):
        """No-op. No repair procedures for emulated devices.
        """
        pass


    def verify_software(self):
        """Verify commands are available on teststation.

        @return: Bool - teststation has necessary software installed.
        """
        adb = self.teststation.run('which adb')
        qemu = self.teststation.run('which qemu-system-arm')
        unzip = self.teststation.run('which unzip')

        return bool(adb and qemu and unzip)


    def fastboot_run(self, command, **kwargs):
        """No-op, emulators do not support fastboot.

        @param command: command to not execute
        @param kwargs: additional arguments to ignore

        @return: empty CmdResult object
        """
        return utils.CmdResult()


    def get_labels(self):
        """No-op, emulators do not have any detectable labels.

        @return: empty list
        """
        return []


    def get_platform(self):
        """@return: emulated_adb
        """
        return 'emulated_adb'

