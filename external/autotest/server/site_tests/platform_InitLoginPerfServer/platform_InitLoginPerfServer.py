# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import numpy
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server import test, autotest

CLIENT_TEST_NAME = 'platform_InitLoginPerf'
STAGE_OOBE = 0
STAGE_REGULAR = 1
STAGE_NAME = ['oobe', 'regular']
BENCHMARKS = {
        'initial_login': {'stage': STAGE_OOBE,
                          'name': 'login-duration',
                          'display': '1stLogin',
                          'units': 'seconds',
                          'upload': True},
        'regular_login': {'stage': STAGE_REGULAR,
                          'name': 'login-duration',
                          'display': 'RegLogin',
                          'units': 'seconds',
                          'upload': True},
        'prepare_attestation': {'stage': STAGE_OOBE,
                                'name': 'attestation-duration',
                                'display': 'PrepAttn',
                                'units': 'seconds',
                                'upload': True},
        'take_ownership': {'stage': STAGE_OOBE,
                           'name': 'ownership-duration',
                           'display': 'TakeOwnp',
                           'units': 'seconds',
                           'upload': True},
        }

class platform_InitLoginPerfServer(test.test):
    """Test to exercise and gather perf data for initialization and login."""

    version = 1

    def initialize(self):
        """Run before the first iteration."""
        self.perf_results = {}
        for bmname in BENCHMARKS:
            self.perf_results[bmname] = []

    def stage_args(self, stage):
        """Build arguments for the client-side test.

        @param stage  Stage of the test to get arguments for.
        @return       Dictionary of arguments.

        """
        if stage == 0:
            return {'perform_init': True,
                    'pre_init_delay': self.pre_init_delay}
        else:
            return {'perform_init': False}

    def run_stage(self, stage):
        """Run the client-side test.

        @param stage: Stage of the test to run.

        """
        full_stage = 'iteration.%s/%s' % (self.iteration, STAGE_NAME[stage])
        logging.info('Run stage %s', full_stage)
        self.client_at.run_test(test_name=self.client_test,
                                results_dir=full_stage,
                                check_client_result=True,
                                **self.stage_args(stage))
        client_keyval = os.path.join(self.outputdir, full_stage,
                self.client_test, 'results', 'keyval')
        self.client_results[stage] = utils.read_keyval(client_keyval,
                type_tag='perf')

    def save_perf_data(self):
        """Extract perf data from client-side test results."""
        for bmname, bm in BENCHMARKS.iteritems():
            try:
                self.perf_results[bmname].append(
                        self.client_results[bm['stage']][bm['name']])
            except:
                logging.warning('Failed to extract %s from client results',
                                bmname)
                self.perf_results[bmname].append(None)
                pass

    def output_benchmark(self, bmname):
        """Output a benchmark.

        @param bmname: Name of the benchmark.

        """
        bm = BENCHMARKS[bmname]
        values = self.perf_results[bmname]
        if not bm.get('upload', True):
            return
        self.output_perf_value(
                description=bmname,
                value=[x for x in values if x is not None],
                units=bm.get('units', 'seconds'),
                higher_is_better=False,
                graph=self.graph_name)

    def display_perf_headers(self):
        """Add headers for the results table to the info log."""
        hdr = "# "
        for bm in BENCHMARKS.itervalues():
            hdr += bm['display'] + ' '
        logging.info('# Results for delay = %.2f sec', self.pre_init_delay)
        logging.info(hdr)

    def display_perf_line(self, n):
        """Add one iteration results line to the info log.

        @param n: Number of the iteration.

        """
        line = "# "
        for bmname in BENCHMARKS:
            value = self.perf_results[bmname][n]
            if value is None:
                line += '    None '
            else:
                line += '%8.2f ' % value
        logging.info(line)

    def display_perf_stats(self, name, func):
        """ Add results statistics line to the info log.

        @param name: Name of the statistic.
        @param func: Function to reduce the list of results.

        """
        line = "# "
        for bmname in BENCHMARKS:
            line += '%8.2f ' % func(self.perf_results[bmname])
        logging.info('# %s:', name)
        logging.info(line)

    def process_perf_data(self):
        """Process performance data from all iterations."""
        logging.info('Process perf data')
        logging.debug('Results: %s', self.perf_results)

        if self.upload_perf:
            for bmname in BENCHMARKS:
                self.output_benchmark(bmname)

        logging.info('##############################################')
        self.display_perf_headers()
        for iter in range(self.iteration - 1):
            self.display_perf_line(iter)
        self.display_perf_stats('Average', numpy.mean)
        self.display_perf_stats('Min', min)
        self.display_perf_stats('Max', max)
        self.display_perf_stats('StdDev', lambda x: numpy.std(x, ddof=1))
        logging.info('##############################################')

    def run_once(self, host, pre_init_delay=0,
                 upload_perf=False, graph_name=None):
        """Run a single iteration.

        @param pre_init_delay: Delay before initialization during first boot.
        @param upload_perf: Do we need to upload the results?
        @param graph_name: Graph name to use when uploading the results.

        """
        if self.iteration is None:
            self.iteration = 1
        logging.info('Start iteration %s', self.iteration)

        self.client = host
        self.pre_init_delay = pre_init_delay
        self.upload_perf = upload_perf
        self.graph_name = graph_name
        self.client_results = {}
        self.client_test = CLIENT_TEST_NAME
        self.client_at = autotest.Autotest(self.client)

        logging.info('Clear the owner before the test')
        tpm_utils.ClearTPMOwnerRequest(self.client, wait_for_ready=False)

        self.run_stage(STAGE_OOBE)
        self.client.reboot()
        self.run_stage(STAGE_REGULAR)
        self.save_perf_data()

    def postprocess(self):
        """Run after all iterations in case of success."""
        self.process_perf_data()

    def cleanup(self):
        """Run at the end regardless of success."""
        logging.info('Cleanup')
        tpm_utils.ClearTPMOwnerRequest(self.client, wait_for_ready=False)
