# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import cr50_utils
from autotest_lib.server.cros.faft.cr50_test import Cr50Test


class firmware_Cr50Update(Cr50Test):
    """
    Verify a dut can update to the given image.

    Copy the new image onto the device and clear the update state to force
    cr50-update to run. The test will fail if Cr50 does not update or if the
    update script encounters any errors.

    @param image: the location of the update image
    @param image_type: string representing the image type. If it is "dev" then
                       don't check the RO versions when comparing versions.
    """
    version = 1
    UPDATE_TIMEOUT = 20
    ERASE_NVMEM = "erase_nvmem"

    DEV_NAME = "dev_image"
    OLD_RELEASE_NAME = "old_release_image"
    RELEASE_NAME = "release_image"
    SUCCESS = 0
    UPDATE_OK = 1


    def initialize(self, host, cmdline_args, release_path="", release_ver="",
                   old_release_path="", old_release_ver="", dev_path="",
                   test=""):
        """Initialize servo and process the given images"""
        super(firmware_Cr50Update, self).initialize(host, cmdline_args,
                                                    restore_cr50_state=True,
                                                    cr50_dev_path=dev_path)

        if not release_ver and not os.path.isfile(release_path):
            raise error.TestError('Need to specify a release version or path')

        self.devid = self.servo.get('cr50_devid')

        # Make sure ccd is disabled so it won't interfere with the update
        self.cr50.ccd_disable()

        self.rootfs_verification_disable()

        self.host = host
        self.erase_nvmem = test.lower() == self.ERASE_NVMEM

        # A dict used to store relevant information for each image
        self.images = {}

        # Process the given images in order of oldest to newest. Get the version
        # info and add them to the update order
        self.update_order = []
        if not self.erase_nvmem and (old_release_path or old_release_ver):
            self.add_image_to_update_order(self.OLD_RELEASE_NAME,
                                           old_release_path, old_release_ver)
        self.add_image_to_update_order(self.RELEASE_NAME, release_path,
                                       release_ver)
        self.add_image_to_update_order(self.DEV_NAME, dev_path)
        self.verify_update_order()
        logging.info("Update %s", self.update_order)

        self.chip_bid = None
        self.chip_flags = None
        chip_bid_info = cr50_utils.GetChipBoardId(self.host)
        if chip_bid_info != cr50_utils.ERASED_CHIP_BID:
            self.chip_bid, _, self.chip_flags = chip_bid_info
            logging.info('chip board id will be erased during rollback. %x:%x '
                'will be restored after rollback.',  self.chip_bid,
                self.chip_flags)
        else:
            logging.info('No chip board id is set. This test will not attempt '
                'to restore anything during rollback.')

        self.device_update_path = cr50_utils.GetActiveCr50ImagePath(self.host)
        # Update to the dev image
        self.run_update(self.DEV_NAME)

    def cleanup(self):
        """Update Cr50 to the image it was running at the start of the test"""
        super(firmware_Cr50Update, self).cleanup()

        logging.warning('rootfs verification is disabled')

        # Make sure keepalive is disabled
        self.cr50.ccd_enable()
        self.cr50.send_command("ccd keepalive disable")

        # Running usb_update commands stops trunksd. Reboot the device to reset
        # it
        self.host.run('reboot')


    def run_update(self, image_name, use_usb_update=False):
        """Copy the image to the DUT and upate to it.

        Normal updates will use the cr50-update script to update. If a rollback
        is True, use usb_update to flash the image and then use the 'rw'
        commands to force a rollback.

        @param image_name: the key in the images dict that can be used to
                           retrieve the image info.
        @param use_usb_update: True if usb_updater should be used directly
                               instead of running the update script.
        """
        self.cr50.ccd_disable()
        # Get the current update information
        image_ver, image_ver_str, image_path = self.images[image_name]

        dest, ver = cr50_utils.InstallImage(self.host, image_path,
                self.device_update_path)
        assert ver == image_ver, "Install failed"
        image_rw = image_ver[1]

        running_ver = cr50_utils.GetRunningVersion(self.host)
        running_ver_str = cr50_utils.GetVersionString(running_ver)

        # If the given image is older than the running one, then we will need
        # to do a rollback to complete the update.
        rollback = (cr50_utils.GetNewestVersion(running_ver[1], image_rw) !=
                    image_rw)
        logging.info("Attempting %s from %s to %s",
                     "rollback" if rollback else "update", running_ver_str,
                     image_ver_str)

        # If a rollback is needed, flash the image into the inactive partition,
        # on or use usb_update to update to the new image if it is requested.
        if use_usb_update or rollback:
            self.cr50_update(image_path, rollback=rollback,
                chip_bid=self.chip_bid, chip_flags=self.chip_flags,
                erase_nvmem=self.erase_nvmem)
            self.check_state((self.checkers.crossystem_checker,
                              {'mainfw_type': 'normal'}))

        # Cr50 is going to reject an update if it hasn't been up for more than
        # 60 seconds. Wait until that passes before trying to run the update.
        self.cr50.wait_until_update_is_allowed()

        # Running the usb update or rollback will enable ccd. Disable it again.
        self.cr50.ccd_disable()

        # Get the last cr50 update related message from /var/log/messages
        last_message = cr50_utils.CheckForFailures(self.host, '')

        # Clear the update state and reboot, so cr50-update will run again.
        cr50_utils.ClearUpdateStateAndReboot(self.host)

        # The cr50 updates happen over /dev/tpm0. It takes a while. After
        # cr50-update has finished, cr50 should reboot. Wait until this happens
        # before sending anymore commands.
        self.cr50.wait_for_reboot()

        # Verify the system boots normally after the update
        self.check_state((self.checkers.crossystem_checker,
                          {'mainfw_type': 'normal'}))

        # Verify the version has been updated and that there have been no
        # unexpected usb_updater exit codes.
        cr50_utils.VerifyUpdate(self.host, image_ver, last_message)

        logging.info("Successfully updated from %s to %s %s", running_ver_str,
                     image_name, image_ver_str)


    def fetch_image(self, ver=None):
        """Fetch the image from gs and copy it to the host.

        @param ver: The rw version of the prod image. If it is not None then the
                    image will be retrieved from chromeos-localmirror otherwise
                    it will be gotten from chromeos-localmirror-private using
                    the devids
        """
        if ver:
            bid = None
            if '/' in ver:
                ver, bid = ver.split('/', 1)
            return self.download_cr50_release_image(ver, bid)
        return self.download_cr50_debug_image(self.devid)


    def add_image_to_update_order(self, image_name, image_path, ver=None):
        """Process the image. Add it to the update_order list and images dict.

        Copy the image to the DUT and get version information.

        Store the image information in the images dictionary and add it to the
        update_order list.

        @param image_name: string that is what the image should be called. Used
                           as the key in the images dict.
        @param image_path: the path for the image.
        @param ver: If the image path isn't specified, this will be used to find
                    the cr50 image in gs://chromeos-localmirror/distfiles.
        """
        tmp_file = '/tmp/%s.bin' % image_name

        if not os.path.isfile(image_path):
            image_path, ver = self.fetch_image(ver)
        else:
            _, ver = cr50_utils.InstallImage(self.host, image_path, tmp_file)

        ver_str = cr50_utils.GetVersionString(ver)

        self.update_order.append(image_name)
        self.images[image_name] = (ver, ver_str, image_path)
        logging.info("%s stored at %s with version %s", image_name, image_path,
                     ver_str)


    def verify_update_order(self):
        """Verify each image in the update order has a higher version than the
        last.

        The test uses the cr50 update script to update to the next image in the
        update order. If the versions are not in ascending order then the update
        won't work. Cr50 cannot update to an older version using the standard
        update process.

        Raises:
            TestError if the versions are not in ascending order.
        """
        for i, name in enumerate(self.update_order[1::]):
            rw = self.images[name][0][1]

            last_name = self.update_order[i]
            last_rw = self.images[last_name][0][1]
            if cr50_utils.GetNewestVersion(last_rw, rw) != rw:
                raise error.TestError("%s is version %s. %s needs to have a "
                                      "higher version, but it has %s" %
                                      (last_name, last_rw, name, rw))


    def after_run_once(self):
        """Add log printing what iteration we just completed"""
        logging.info("Update iteration %s ran successfully", self.iteration)


    def run_once(self):
        """Update to each image in update_order"""
        for name in self.update_order:
            self.run_update(name)
