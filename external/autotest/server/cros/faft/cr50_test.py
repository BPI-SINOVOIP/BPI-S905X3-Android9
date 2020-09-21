# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import pprint

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import cr50_utils, tpm_utils
from autotest_lib.server.cros import debugd_dev_tools, gsutil_wrapper
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class Cr50Test(FirmwareTest):
    """
    Base class that sets up helper objects/functions for cr50 tests.
    """
    version = 1

    CR50_GS_URL = 'gs://chromeos-localmirror-private/distfiles/chromeos-cr50-%s/'
    CR50_DEBUG_FILE = '*/cr50_dbg_%s.bin'
    CR50_PROD_FILE = 'cr50.%s.bin.prod'
    NONE = 0
    # Saved the original device state during init.
    INITIAL_STATE = 1 << 0
    # Saved the original image, the device image, and the debug image. These
    # images are needed to be able to restore the original image and board id.
    IMAGES = 1 << 1

    def initialize(self, host, cmdline_args, restore_cr50_state=False,
                   cr50_dev_path='', provision_update=False):
        self._saved_state = self.NONE
        self._raise_error_on_mismatch = not restore_cr50_state
        self._provision_update = provision_update
        super(Cr50Test, self).initialize(host, cmdline_args)

        if not hasattr(self, 'cr50'):
            raise error.TestNAError('Test can only be run on devices with '
                                    'access to the Cr50 console')

        self.host = host
        self._save_original_state()
        # We successfully saved the device state
        self._saved_state |= self.INITIAL_STATE
        try:
            self._save_node_locked_dev_image(cr50_dev_path)
            self._save_original_images()
            # We successfully saved the device images
            self._saved_state |= self.IMAGES
        except:
            if restore_cr50_state:
                raise


    def _save_node_locked_dev_image(self, cr50_dev_path):
        """Save or download the node locked dev image.

        Args:
            cr50_dev_path: The path to the node locked cr50 image.
        """
        if os.path.isfile(cr50_dev_path):
            self._node_locked_cr50_image = cr50_dev_path
        else:
            devid = self.servo.get('cr50_devid')
            self._node_locked_cr50_image = self.download_cr50_debug_image(
                devid)[0]


    def _save_original_images(self):
        """Use the saved state to find all of the device images.

        This will download running cr50 image and the device image.
        """
        # Copy the prod and prepvt images from the DUT
        _, prod_rw, prod_bid = self._original_state['device_prod_ver']
        filename = 'prod_device_image_' + prod_rw
        self._device_prod_image = os.path.join(self.resultsdir,
                filename)
        self.host.get_file(cr50_utils.CR50_PROD,
                self._device_prod_image)

        if cr50_utils.HasPrepvtImage(self.host):
            _, prepvt_rw, prepvt_bid = self._original_state['device_prepvt_ver']
            filename = 'prepvt_device_image_' + prepvt_rw
            self._device_prepvt_image = os.path.join(self.resultsdir,
                    filename)
            self.host.get_file(cr50_utils.CR50_PREPVT,
                    self._device_prepvt_image)
            prepvt_bid = cr50_utils.GetBoardIdInfoString(prepvt_bid)
        else:
            self._device_prepvt_image = None
            prepvt_rw = None
            prepvt_bid = None

        # If the running cr50 image version matches the image on the DUT use
        # the DUT image as the original image. If the versions don't match
        # download the image from google storage
        _, running_rw, running_bid = self.get_saved_cr50_original_version()

        # Make sure prod_bid and running_bid are in the same format
        prod_bid = cr50_utils.GetBoardIdInfoString(prod_bid)
        running_bid = cr50_utils.GetBoardIdInfoString(running_bid)
        if running_rw == prod_rw and running_bid == prod_bid:
            logging.info('Using device cr50 prod image %s %s', prod_rw,
                    prod_bid)
            self._original_cr50_image = self._device_prod_image
        elif running_rw == prepvt_rw and running_bid == prepvt_bid:
            logging.info('Using device cr50 prepvt image %s %s', prepvt_rw,
                    prepvt_bid)
            self._original_cr50_image = self._device_prepvt_image
        else:
            logging.info('Downloading cr50 image %s %s', running_rw,
                    running_bid)
            self._original_cr50_image = self.download_cr50_release_image(
                running_rw, running_bid)[0]


    def _save_original_state(self):
        """Save the cr50 related state.

        Save the device's current cr50 version, cr50 board id, rlz, and image
        at /opt/google/cr50/firmware/cr50.bin.prod. These will be used to
        restore the state during cleanup.
        """
        self._original_state = self.get_cr50_device_state()


    def get_saved_cr50_original_version(self):
        """Return (ro ver, rw ver, bid)"""
        if ('running_ver' not in self._original_state or 'cr50_image_bid' not in
            self._original_state):
            raise error.TestError('No record of original cr50 image version')
        return (self._original_state['running_ver'][0],
                self._original_state['running_ver'][1],
                self._original_state['cr50_image_bid'])


    def get_saved_cr50_original_path(self):
        """Return the local path for the original cr50 image"""
        if not hasattr(self, '_original_cr50_image'):
            raise error.TestError('No record of original image')
        return self._original_cr50_image


    def has_saved_cr50_dev_path(self):
        """Returns true if we saved the node locked debug image"""
        return hasattr(self, '_node_locked_cr50_image')


    def get_saved_cr50_dev_path(self):
        """Return the local path for the cr50 dev image"""
        if not self.has_saved_cr50_dev_path():
            raise error.TestError('No record of debug image')
        return self._node_locked_cr50_image


    def _restore_original_image(self, chip_bid, chip_flags):
        """Restore the cr50 image and erase the state.

        Make 3 attempts to update to the original image. Use a rollback from
        the DBG image to erase the state that can only be erased by a DBG image.
        Set the chip board id during rollback

        Args:
            chip_bid: the integer representation of chip board id or None if the
                      board id should be erased
            chip_flags: the integer representation of chip board id flags or
                        None if the board id should be erased
        """
        for i in range(3):
            try:
                # Update to the node-locked DBG image so we can erase all of
                # the state we are trying to reset
                self.cr50_update(self._node_locked_cr50_image)

                # Rollback to the original cr50 image.
                self.cr50_update(self._original_cr50_image, rollback=True,
                                 chip_bid=chip_bid, chip_flags=chip_flags)
                break
            except Exception, e:
                logging.warning('Failed to restore original image attempt %d: '
                                '%r', i, e)


    def rootfs_verification_disable(self):
        """Remove rootfs verification"""
        if not self._rootfs_verification_is_disabled():
            logging.debug('Removing rootfs verification.')
            self.rootfs_tool.enable()


    def _rootfs_verification_is_disabled(self):
        """Returns true if rootfs verification is enabled"""
        # Clear the TPM owner before trying to check rootfs verification
        tpm_utils.ClearTPMOwnerRequest(self.host, wait_for_ready=True)
        self.rootfs_tool = debugd_dev_tools.RootfsVerificationTool()
        self.rootfs_tool.initialize(self.host)
        # rootfs_tool.is_enabled is True, that means rootfs verification is
        # disabled.
        return self.rootfs_tool.is_enabled()


    def _restore_original_state(self):
        """Restore the original cr50 related device state"""
        if not (self._saved_state & self.IMAGES):
            logging.warning('Did not save the original images. Cannot restore '
                            'state')
            return
        # Remove the prepvt image if the test installed one.
        if (not self._original_state['has_prepvt'] and
            cr50_utils.HasPrepvtImage(self.host)):
            self.host.run('rm %s' % cr50_utils.CR50_PREPVT)
        # If rootfs verification has been disabled, copy the cr50 device image
        # back onto the DUT.
        if self._rootfs_verification_is_disabled():
            cr50_utils.InstallImage(self.host, self._device_prod_image,
                    cr50_utils.CR50_PROD)
            # Install the prepvt image if there was one.
            if self._device_prepvt_image:
                cr50_utils.InstallImage(self.host, self._device_prepvt_image,
                        cr50_utils.CR50_PREPVT)

        chip_bid_info = self._original_state['chip_bid']
        bid_is_erased = chip_bid_info == cr50_utils.ERASED_CHIP_BID
        chip_bid = None if bid_is_erased else chip_bid_info[0]
        chip_flags = None if bid_is_erased else chip_bid_info[2]
        # Update to the original image and erase the board id
        self._restore_original_image(chip_bid, chip_flags)

        # Set the RLZ code
        cr50_utils.SetRLZ(self.host, self._original_state['rlz'])

        # Verify everything is still the same
        mismatch = self._check_original_state()
        if mismatch:
            raise error.TestError('Could not restore state: %s' % mismatch)

        logging.info('Successfully restored the original cr50 state')


    def get_cr50_device_state(self):
        """Get a dict with the current device cr50 information

        The state dict will include the platform brand, rlz code, chip board id,
        the running cr50 image version, the running cr50 image board id, and the
        device cr50 image version.
        """
        state = {}
        state['mosys platform brand'] = self.host.run('mosys platform brand',
            ignore_status=True).stdout.strip()
        state['device_prod_ver'] = cr50_utils.GetBinVersion(self.host,
                cr50_utils.CR50_PROD)
        state['has_prepvt'] = cr50_utils.HasPrepvtImage(self.host)
        if state['has_prepvt']:
            state['device_prepvt_ver'] = cr50_utils.GetBinVersion(self.host,
                    cr50_utils.CR50_PREPVT)
        else:
            state['device_prepvt_ver'] = None
        state['rlz'] = cr50_utils.GetRLZ(self.host)
        state['chip_bid'] = cr50_utils.GetChipBoardId(self.host)
        state['chip_bid_str'] = '%08x:%08x:%08x' % state['chip_bid']
        state['running_ver'] = cr50_utils.GetRunningVersion(self.host)
        state['cr50_image_bid'] = self.cr50.get_active_board_id_str()

        logging.debug('Current Cr50 state:\n%s', pprint.pformat(state))
        return state


    def _check_original_state(self):
        """Compare the current cr50 state to the original state

        Returns:
            A dictionary with the state that is wrong as the key and
            the new and old state as the value
        """
        if not (self._saved_state & self.INITIAL_STATE):
            logging.warning('Did not save the original state. Cannot verify it '
                            'matches')
            return
        # Make sure the /var/cache/cr50* state is up to date.
        cr50_utils.ClearUpdateStateAndReboot(self.host)

        mismatch = {}
        new_state = self.get_cr50_device_state()

        for k, new_val in new_state.iteritems():
            original_val = self._original_state[k]
            if new_val != original_val:
                mismatch[k] = 'old: %s, new: %s' % (original_val, new_val)

        if mismatch:
            logging.warning('State Mismatch:\n%s', pprint.pformat(mismatch))
        else:
            logging.info('The device is in the original state')
        return mismatch


    def cleanup(self):
        """Make sure the device state is the same as the start of the test"""
        state_mismatch = self._check_original_state()

        if state_mismatch and not self._provision_update:
            self._restore_original_state()
            if self._raise_error_on_mismatch:
                raise error.TestError('Unexpected state mismatch during '
                                      'cleanup %s' % state_mismatch)
        super(Cr50Test, self).cleanup()


    def find_cr50_gs_image(self, filename, image_type=None):
        """Find the cr50 gs image name

        Args:
            filename: the cr50 filename to match to
            image_type: release or debug. If it is not specified we will search
                        both the release and debug directories
        Returns:
            a tuple of the gsutil bucket, filename
        """
        gs_url = self.CR50_GS_URL % (image_type if image_type else '*')
        gs_filename = os.path.join(gs_url, filename)
        bucket, gs_filename = utils.gs_ls(gs_filename)[0].rsplit('/', 1)
        return bucket, gs_filename


    def download_cr50_gs_image(self, filename, image_bid='', bucket=None,
                               image_type=None):
        """Get the image from gs and save it in the autotest dir

        Args:
            filename: The cr50 image basename
            image_bid: the board id info list or string. It will be added to the
                       filename.
            bucket: The gs bucket name
            image_type: 'debug' or 'release'. This will be used to determine
                        the bucket if the bucket is not given.
        Returns:
            A tuple with the local path and version
        """
        # Add the image bid string to the filename
        if image_bid:
            bid_str = cr50_utils.GetBoardIdInfoString(image_bid,
                                                       symbolic=True)
            filename += '.' + bid_str.replace(':', '_')

        if not bucket:
            bucket, filename = self.find_cr50_gs_image(filename, image_type)

        remote_temp_dir = '/tmp/'
        src = os.path.join(remote_temp_dir, filename)
        dest = os.path.join(self.resultsdir, filename)

        # Copy the image to the dut
        gsutil_wrapper.copy_private_bucket(host=self.host,
                                           bucket=bucket,
                                           filename=filename,
                                           destination=remote_temp_dir)

        self.host.get_file(src, dest)
        ver = cr50_utils.GetBinVersion(self.host, src)

        # Compare the image board id to the downloaded image to make sure we got
        # the right file
        downloaded_bid = cr50_utils.GetBoardIdInfoString(ver[2], symbolic=True)
        if image_bid and bid_str != downloaded_bid:
            raise error.TestError('Could not download image with matching '
                                  'board id wanted %s got %s' % (bid_str,
                                  downloaded_bid))
        return dest, ver


    def download_cr50_debug_image(self, devid, image_bid=''):
        """download the cr50 debug file

        Get the file with the matching devid and image board id info

        Args:
            devid: the cr50_devid string '${DEVID0} ${DEVID1}'
            image_bid: the image board id info string or list
        Returns:
            A tuple with the debug image local path and version
        """
        # Debug images are node locked with the devid. Add the devid to the
        # filename
        filename = self.CR50_DEBUG_FILE % (devid.replace(' ', '_'))

        # Download the image
        dest, ver = self.download_cr50_gs_image(filename, image_bid=image_bid,
                                                image_type='debug')

        return dest, ver


    def download_cr50_release_image(self, rw_ver, image_bid=''):
        """download the cr50 release file

        Get the file with the matching version and image board id info

        Args:
            rw_ver: the rw version string
            image_bid: the image board id info string or list
        Returns:
            A tuple with the release image local path and version
        """
        # Release images can be found using the rw version
        filename = self.CR50_PROD_FILE % rw_ver

        # Download the image
        dest, ver = self.download_cr50_gs_image(filename, image_bid=image_bid,
                                                image_type='release')

        # Compare the rw version and board id info to make sure the right image
        # was found
        if rw_ver != ver[1]:
            raise error.TestError('Could not download image with matching '
                                  'rw version')
        return dest, ver


    def _cr50_verify_update(self, expected_ver, expect_rollback):
        """Verify the expected version is running on cr50

        Args:
            expect_ver: The RW version string we expect to be running
            expect_rollback: True if cr50 should have rolled back during the
                             update

        Raises:
            TestFail if there is any unexpected update state
        """
        errors = []
        running_ver = self.cr50.get_version()
        if expected_ver != running_ver:
            errors.append('running %s not %s' % (running_ver, expected_ver))

        if expect_rollback != self.cr50.rolledback():
            errors.append('%srollback detected' %
                          'no ' if expect_rollback else '')
        if len(errors):
            raise error.TestFail('cr50_update failed: %s' % ', '.join(errors))
        logging.info('RUNNING %s after %s', expected_ver,
                     'rollback' if expect_rollback else 'update')


    def _cr50_run_update(self, path):
        """Install the image at path onto cr50

        Args:
            path: the location of the image to update to

        Returns:
            the rw version of the image
        """
        tmp_dest = '/tmp/' + os.path.basename(path)

        dest, image_ver = cr50_utils.InstallImage(self.host, path, tmp_dest)
        cr50_utils.UsbUpdater(self.host, ['-a', dest])
        return image_ver[1]


    def cr50_update(self, path, rollback=False, erase_nvmem=False,
                    expect_rollback=False, chip_bid=None, chip_flags=None):
        """Attempt to update to the given image.

        If rollback is True, we assume that cr50 is already running an image
        that can rollback.

        Args:
            path: the location of the update image
            rollback: True if we need to force cr50 to rollback to update to
                      the given image
            erase_nvmem: True if we need to erase nvmem during rollback
            expect_rollback: True if cr50 should rollback on its own
            chip_bid: the integer representation of chip board id or None if the
                      board id should be erased during rollback
            chip_flags: the integer representation of chip board id flags or
                        None if the board id should be erased during rollback

        Raises:
            TestFail if the update failed
        """
        original_ver = self.cr50.get_version()

        # Cr50 is going to reject an update if it hasn't been up for more than
        # 60 seconds. Wait until that passes before trying to run the update.
        self.cr50.wait_until_update_is_allowed()

        rw_ver = self._cr50_run_update(path)

        # Running the update may cause cr50 to reboot. Wait for that before
        # sending more commands. The reboot should happen quickly. Wait a
        # maximum of 10 seconds.
        self.cr50.wait_for_reboot(10)

        if erase_nvmem and rollback:
            self.cr50.erase_nvmem()

        if rollback:
            self.cr50.rollback(chip_bid=chip_bid, chip_flags=chip_flags)

        expected_ver = original_ver if expect_rollback else rw_ver
        # If we expect a rollback, the version should remain unchanged
        self._cr50_verify_update(expected_ver, rollback or expect_rollback)
