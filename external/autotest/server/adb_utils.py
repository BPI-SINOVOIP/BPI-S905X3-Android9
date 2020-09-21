# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions for interacting with devices supporting adb commands. The
functions only work with an Autotest instance setup, e.g., in a lab.
"""


import os
import logging

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dev_server


def install_apk_from_build(host, apk, build_artifact, package_name=None,
                           force_reinstall=False, build_name=None):
    """Install the specific apk from given build artifact.

    @param host: An ADBHost object to install apk.
    @param apk: Name of the apk to install, e.g., sl4a.apk
    @param build_artifact: Name of the build artifact, e.g., test_zip. Note
            that it's not the name of the artifact file. Refer to
            artifact_info.py in devserver code for the mapping between
            artifact name to a build artifact.
    @param package_name: Name of the package, e.g., com.android.phone.
            If package_name is given, it checks if the package exists before
            trying to install it, unless force_reinstall is set to True.
    @param force_reinstall: True to reinstall the apk even if it's already
            installed. Default is set to False.
    @param build_name: None unless DUT is CrOS with ARC++ container. build_name
            points to ARC++ build artifacts.
    """
    # Check if apk is already installed.
    if package_name and not force_reinstall:
        if host.is_apk_installed(package_name):
            logging.info('Package %s is already installed.', package_name)
            return
    if build_name:
        # Pull devserver_url given ARC++ enabled host
        host_devserver_url = dev_server.AndroidBuildServer.resolve(build_name,
                host.hostname).url()
        # Return devserver_url given Android build path
        job_repo_url = os.path.join(host_devserver_url, build_name)
    else:
        info = host.host_info_store.get()
        job_repo_url = info.attributes.get(host.job_repo_url_attribute, '')
    if not job_repo_url:
        raise error.AutoservError(
                'The host %s has no attribute %s. `install_apk_from_build` '
                'only works for test with image specified.' %
                (host.hostname, host.job_repo_url_attribute))
    devserver_url = dev_server.AndroidBuildServer.get_server_url(job_repo_url)
    devserver = dev_server.AndroidBuildServer(devserver_url)
    build_info = host.get_build_info_from_build_url(job_repo_url)
    devserver.trigger_download(build_info['target'], build_info['build_id'],
                               build_info['branch'], build_artifact,
                               synchronous=True)
    build_info['os_type'] = 'android'
    apk_url = devserver.locate_file(apk, build_artifact, None, build_info)
    logging.debug('Found apk at: %s', apk_url)

    tmp_dir = host.teststation.get_tmp_dir()
    try:
        host.download_file(apk_url, apk, tmp_dir)
        result = host.install_apk(os.path.join(tmp_dir, apk),
                                  force_reinstall=force_reinstall)
        logging.debug(result.stdout)
        if package_name and not host.is_apk_installed(package_name):
            raise error.AutoservError('No package found with name of %s',
                                      package_name)
        logging.info('%s is installed successfully.', apk)
    finally:
        host.teststation.run('rm -rf %s' % tmp_dir)
