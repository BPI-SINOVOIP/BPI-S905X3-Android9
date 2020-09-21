# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import importlib
import logging
import os
import re

import yaml

from autotest_lib.client.common_lib import error

class DeviceCapability(object):
    """
    Generate capabilities status on DUT from yaml files in a given path.
    Answer from the capabilities whether some capability is satisfied on DUT.
    """

    def __init__(self, settings_path='/usr/local/etc/autotest-capability'):
        """
        @param settings_path: string, the base directory for autotest
                              capability. There should be yaml files.
        """
        self.capabilities = self.__get_autotest_capability(settings_path)
        logging.info("Capabilities:\n%r", self.capabilities)


    def __get_autotest_capability(self, settings_path):
        """
        Generate and summarize capabilities from yaml files in
        settings_path with detectors.

        @param settings_path: string, the base directory for autotest
                              capability. There should be yaml files.
        @returns dict:
            The capabilities on DUT.
            Its key is string denoting a capability. Its value is 'yes', 'no' or
            'disable.'
        """

        def run_detector(name):
            """
            Run a detector in the detector directory. (i.e.
            autotest/files/client/cros/video/detectors)
            Return the result of the detector.

            @param name: string, the name of running detector.
            @returns string, a result of detect() in the detector script.
            """
            if name not in detect_results:
                detector = importlib.import_module(
                    "autotest_lib.client.cros.video.detectors.%s"
                    % name)
                detect_results[name] = detector.detect()
                logging.info("Detector result (%s): %s",
                             name, detect_results[name])
            return detect_results[name]

        managed_cap_fpath = os.path.join(settings_path,
                                         'managed-capabilities.yaml')
        if not os.path.exists(managed_cap_fpath):
            raise error.TestFail("%s is not installed" % managed_cap_fpath)
        managed_caps = yaml.load(file(managed_cap_fpath))

        cap_files = [f for f in os.listdir(settings_path)
                     if re.match(r'^[0-9]+-.*\.yaml$', f)]
        cap_files.sort(key=lambda f: int(f.split('-', 1)[0]))

        detect_results = {}
        autotest_caps = dict.fromkeys(managed_caps, 'no')
        for fname in cap_files:
            logging.debug('Processing caps: %s', fname)
            fname = os.path.join(settings_path, fname)
            for rule in yaml.load(file(fname)):
                # The type of rule is string or dict
                # If the type is a string, it is a capability (e.g. webcam).
                # If a specific condition (e.g. kepler, cpu type) is required,
                # rule would be dict, for example,
                # {'detector': 'intel_cpu',
                #  'match': ['intel_celeron_1007U'],
                #  'capabilities': ['no hw_h264_enc_1080_30'] }.
                logging.debug("%r", rule)
                caps = []
                if isinstance(rule, dict):
                    if run_detector(rule['detector']) in rule['match']:
                        caps = rule['capabilities']
                else:
                    caps = [rule]

                for capability in caps:
                    m = re.match(r'(?:(disable|no)\s+)?([\w\-]+)$', capability)
                    prefix, capability = m.groups()
                    if capability in managed_caps:
                        autotest_caps[capability] = prefix or 'yes'
                    else:
                        raise error.TestFail(
                            "Unexpected capability: %s" % capability)

        return autotest_caps


    def get_managed_caps(self):
        return self.capabilities.keys()


    def get_capability_results(self):
        return self.capabilities


    def get_capability(self, cap):
        """
        Decide if a device satisfies a required capability for an autotest.

        @param cap: string, denoting one capability. It must be one in
                    settings_path + 'managed-capabilities.yaml.'
        @returns 'yes', 'no', or 'disable.'
        """
        try:
            return self.capabilities[cap]
        except KeyError:
            raise error.TestFail("Unexpected capability: %s" % cap)


    def ensure_capability(self, cap):
        """
        Raise TestNAError if a device doesn't satisfy cap.
        """
        if self.get_capability(cap) != 'yes':
            raise error.TestNAError("Missing Capability: %s" % cap)


    def have_capability(self, cap):
        """
        Return whether cap is available.
        """
        return self.get_capability(cap) == 'yes'
