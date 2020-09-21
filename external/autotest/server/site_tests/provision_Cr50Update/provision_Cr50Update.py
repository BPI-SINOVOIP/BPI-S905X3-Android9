# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


""" The autotest performing Cr50 update."""


import os
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import cr50_utils
from autotest_lib.server.cros.faft.cr50_test import Cr50Test


class provision_Cr50Update(Cr50Test):
    """A test that can provision a machine to the correct cr50 version.

    Take value and split it into image_rw_ver, image_bid, and chip bid if it
    is specifying a prod signed image. If it requests a selfsigned image split
    it into the dev server build info and chip board id.

    Update Cr50 so it is running the correct image with the right chip baord id.

    value=prodsigned-0.0.23(/ZZAF:ffffffff:7f00/ZZAF:7f80)?
    value=selfsigned-reef-release/R61-9704.0.0(/ZZAF:7f80)?

    """

    version = 1

    def initialize(self, host, cmdline_args, value='', release_path='',
                   chip_bid_str='', dev_path=''):
        """Initialize get the cr50 update version information"""
        super(provision_Cr50Update, self).initialize(host, cmdline_args,
            provision_update=True, cr50_dev_path=dev_path)
        self.host = host
        self.chip_bid_str = chip_bid_str

        image_info = None

        if os.path.isfile(release_path):
            image_info = self.get_local_image(release_path)
        else:
            source, test_info = value.split('-', 1)

            # The chip board id is optional so value can be
            # 'ver_part_1/ver_part_2/chip_bid' or 'ver_part_1/ver_part_2'.
            #
            # Get the chip bid from test_info if it was included.
            if test_info.count('/') == 2:
                test_info, self.chip_bid_str = test_info.rsplit('/', 1)

            if source == 'prodsigned':
                image_info = self.get_prodsigned_image(test_info)
            if source == 'selfsigned':
                image_info = self.get_selfsigned_image(test_info)

        if not image_info:
            raise error.TestError('Could not find new cr50 image')

        self.local_path, self.image_ver = image_info
        self.image_rw = self.image_ver[1]
        self.image_bid = self.image_ver[2]


    def get_local_image(self, release_path):
        """Get the version of the local image.

        Args:
            release_path: The local path to the cr50 image

        Returns:
            the local path, image version tuple
        """
        ver = cr50_utils.InstallImage(self.host, release_path,
            '/tmp/release.bin')[1]
        return release_path, ver


    def get_prodsigned_image(self, value):
        """Find the release image.

        Args:
            value: The Cr50 image version info rw_ver/image_bid. The image_bid
                   is optional

        Returns:
            the local path, image version tuple
        """
        release_bid_str = None
        values = value.split('/')
        release_ver = values[0]

        if len(values) > 1 and values[1]:
            release_bid_str = values[1]

        return self.download_cr50_release_image(release_ver,
            release_bid_str)


    def get_selfsigned_image(self, value):
        """Find the selfsigned image image.

        The value should be something like reef-release/R61-9704.0.0

        Args:
            value: A string with the build to extract the cr50 image from.

        Returns:
            the local path, image version tuple

        Raises:
            TestError because it is not yet implemented
        """
        # TODO(mruthven): Add support for downloading the image from the
        # devserver and extracting the cr50 image.
        raise error.TestError('No support for finding %s', value)


    def check_bid_settings(self, chip_bid_info, image_bid_str):
        """Compare the chip and image board ids.

        Compare the image and chip board ids before changing the cr50 state.
        Raise an error if the image will not be able to run with the requested
        chip board id.

        Args:
            chip_bid_info: The chip board id info tuple
            image_bid_str: the image board_id:mask:flags.

        Raises:
            TestFail if we will not be able to update to the image with the
            given chip board id.
        """
        # If the image isn't board id locked, it will run on all devices.
        if chip_bid_info == cr50_utils.ERASED_CHIP_BID:
            logging.info('Chip has no board id. It will run any image.')
            return
        if not image_bid_str:
            logging.info('Image is not board id locked. It will run on all '
                         'devices.')
            return

        chip_bid, chip_bid_inv, chip_flags = chip_bid_info
        chip_bid_str = cr50_utils.GetBoardIdInfoString(chip_bid_info,
            symbolic=True)

        image_bid, image_mask, image_flags = image_bid_str.split(':')

        # Convert the image board id to integers
        image_mask = int(image_mask, 16)
        image_bid = cr50_utils.GetIntBoardId(image_bid)
        image_flags = int(image_flags, 16)

        errors = []
        # All bits in the image mask must match between the image and chip board
        # ids.
        image_bid = image_bid & image_mask
        chip_bid = chip_bid & image_mask
        if image_bid != chip_bid:
            errors.append('board id')
        # All 1s in the image flags must also be 1 in the chip flags
        chip_flags = chip_flags & image_flags
        if image_flags != chip_flags:
            errors.append('flags')
        if len(errors):
            raise error.TestFail('Image will not be able to run with the '
                'given %s: chip %s image %s' % (' and '.join(errors),
                chip_bid_str, image_bid_str))


    def get_new_chip_bid(self):
        """Get the new chip board id and flags.

        Returns:
            a tuple chip bid info, a bool True if the chip board id needs to
            change.
        """
        chip_bid_info = self.chip_bid_str.split(':')
        running_bid_info = cr50_utils.GetChipBoardId(self.host)

        # If no board id was specified, restore the original board id
        if len(chip_bid_info) != 2 or not chip_bid_info[0]:
            logging.info('No board id given. Using the current chip settings '
                         '%s', running_bid_info)
            return running_bid_info, False

        chip_bid = cr50_utils.GetIntBoardId(chip_bid_info[0])
        chip_flags = int(chip_bid_info[1], 16)

        chip_bid_info = (chip_bid, 0xffffffff ^ chip_bid, chip_flags)
        set_bid = chip_bid_info != running_bid_info
        return chip_bid_info, set_bid


    def check_final_state(self, chip_bid_info):
        """Verify the update checking the chip board id and running image

        Args:
            chip_bid_info: A tuple of ints: chip_board_id, ~chip_board_id,
                           and flags.

        Raises:
            TestFail if the device did not update to the correct state
        """
        state = self.get_cr50_device_state()
        image_bid = cr50_utils.GetBoardIdInfoString(self.image_bid)

        failed = []
        if chip_bid_info != state['chip_bid']:
            failed.append('cr50 chip board id')
        if image_bid != state['cr50_image_bid']:
            failed.append('cr50 image board id')
        if self.image_rw != state['running_ver'][1]:
            failed.append('cr50 image version')
        if self.image_ver != state['device_prod_ver']:
            failed.append('device prod image')
        if self.image_ver != state['device_prepvt_ver']:
            failed.append('device prepvt image')
        if len(failed):
            raise error.TestFail('Update failures: %s', ', '.join(failed))


    def run_once(self):
        """The method called by the control file to start the update."""
        chip_bid_info, set_bid = self.get_new_chip_bid()

        logging.info('Updating to image %s with chip board id %s',
            self.image_ver, cr50_utils.GetBoardIdInfoString(chip_bid_info))

        # Make sure the image will be able to run with the given chip board id.
        self.check_bid_settings(chip_bid_info, self.image_bid)

        # If the release version is not newer than the running rw version, we
        # have to do a rollback.
        running_rw = cr50_utils.GetRunningVersion(self.host)[1]
        rollback = (cr50_utils.GetNewestVersion(running_rw, self.image_rw) !=
            self.image_rw)

        # You can only update the board id or update to an old image by rolling
        # back from a dev image.
        need_rollback = rollback or set_bid
        if need_rollback and not self.has_saved_cr50_dev_path():
            raise error.TestFail('Need a dev image to rollback to %s or update'
                                 'the board id')
        # Copy the image onto the DUT. cr50-update uses both cr50.bin.prod and
        # cr50.bin.prepvt in /opt/google/cr50/firmware/, so copy it to both
        # places. Rootfs verification has to be disabled to do the copy.
        self.rootfs_verification_disable()
        cr50_utils.InstallImage(self.host, self.local_path,
                cr50_utils.CR50_PREPVT)
        cr50_utils.InstallImage(self.host, self.local_path,
                cr50_utils.CR50_PROD)

        # Update to the dev image if there needs to be a rollback.
        if need_rollback:
            dev_path = self.get_saved_cr50_dev_path()
            self.cr50_update(dev_path)

        # If we aren't changing the board id, don't pass any values into the bid
        # args.
        chip_bid = chip_bid_info[0] if need_rollback else None
        chip_flags = chip_bid_info[2] if need_rollback else None
        # Update to the new image, setting the chip board id and rolling back if
        # necessary.
        self.cr50_update(self.local_path, rollback=need_rollback,
            chip_bid=chip_bid, chip_flags=chip_flags)

        cr50_utils.ClearUpdateStateAndReboot(self.host)

        # Verify everything updated correctly
        self.check_final_state(chip_bid_info)
