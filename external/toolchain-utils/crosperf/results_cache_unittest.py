#!/usr/bin/env python2

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Module of result cache unittest."""

from __future__ import print_function

import mock
import os
import tempfile
import unittest

import image_checksummer
import machine_manager
import test_flag

from label import MockLabel
from results_cache import CacheConditions
from results_cache import Result
from results_cache import ResultsCache
from results_cache import TelemetryResult
from cros_utils import command_executer
from cros_utils import logger
from cros_utils import misc

OUTPUT = """CMD (True): ./test_that.sh\
 --remote=172.17.128.241  --board=lumpy   LibCBench
CMD (None): cd /usr/local/google/home/yunlian/gd/src/build/images/lumpy/latest/../../../../..; cros_sdk  -- ./in_chroot_cmd6X7Cxu.sh
Identity added: /tmp/test_that.PO1234567/autotest_key (/tmp/test_that.PO1234567/autotest_key)
INFO    : Using emerged autotests already installed at /build/lumpy/usr/local/autotest.

INFO    : Running the following control files 1 times:
INFO    :  * 'client/site_tests/platform_LibCBench/control'

INFO    : Running client test client/site_tests/platform_LibCBench/control
./server/autoserv -m 172.17.128.241 --ssh-port 22 -c client/site_tests/platform_LibCBench/control -r /tmp/test_that.PO1234567/platform_LibCBench --test-retry=0 --args 
ERROR:root:import statsd failed, no stats will be reported.
14:20:22 INFO | Results placed in /tmp/test_that.PO1234567/platform_LibCBench
14:20:22 INFO | Processing control file
14:20:23 INFO | Starting master ssh connection '/usr/bin/ssh -a -x -N -o ControlMaster=yes -o ControlPath=/tmp/_autotmp_VIIP67ssh-master/socket -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o BatchMode=yes -o ConnectTimeout=30 -o ServerAliveInterval=180 -o ServerAliveCountMax=3 -o ConnectionAttempts=4 -o Protocol=2 -l root -p 22 172.17.128.241'
14:20:23 ERROR| [stderr] Warning: Permanently added '172.17.128.241' (RSA) to the list of known hosts.
14:20:23 INFO | INFO	----	----	kernel=3.8.11	localtime=May 22 14:20:23	timestamp=1369257623	
14:20:23 INFO | Installing autotest on 172.17.128.241
14:20:23 INFO | Using installation dir /usr/local/autotest
14:20:23 WARNI| No job_repo_url for <remote host: 172.17.128.241>
14:20:23 INFO | Could not install autotest using the packaging system: No repos to install an autotest client from. Trying other methods
14:20:23 INFO | Installation of autotest completed
14:20:24 WARNI| No job_repo_url for <remote host: 172.17.128.241>
14:20:24 INFO | Executing /usr/local/autotest/bin/autotest /usr/local/autotest/control phase 0
14:20:24 INFO | Entered autotestd_monitor.
14:20:24 INFO | Finished launching tail subprocesses.
14:20:24 INFO | Finished waiting on autotestd to start.
14:20:26 INFO | START	----	----	timestamp=1369257625	localtime=May 22 14:20:25	
14:20:26 INFO | 	START	platform_LibCBench	platform_LibCBench	timestamp=1369257625	localtime=May 22 14:20:25	
14:20:30 INFO | 		GOOD	platform_LibCBench	platform_LibCBench	timestamp=1369257630	localtime=May 22 14:20:30	completed successfully
14:20:30 INFO | 	END GOOD	platform_LibCBench	platform_LibCBench	timestamp=1369257630	localtime=May 22 14:20:30	
14:20:31 INFO | END GOOD	----	----	timestamp=1369257630	localtime=May 22 14:20:30	
14:20:31 INFO | Got lock of exit_code_file.
14:20:31 INFO | Released lock of exit_code_file and closed it.
OUTPUT: ==============================
OUTPUT: Current time: 2013-05-22 14:20:32.818831 Elapsed: 0:01:30 ETA: Unknown
Done: 0% [                                                  ]
OUTPUT: Thread Status:
RUNNING:  1 ('ttt: LibCBench (1)' 0:01:21)
Machine Status:
Machine                        Thread     Lock Status                    Checksum                        
172.17.128.241                 ttt: LibCBench (1) True RUNNING                   3ba9f2ecbb222f20887daea5583d86ba

OUTPUT: ==============================
14:20:33 INFO | Killing child processes.
14:20:33 INFO | Client complete
14:20:33 INFO | Finished processing control file
14:20:33 INFO | Starting master ssh connection '/usr/bin/ssh -a -x -N -o ControlMaster=yes -o ControlPath=/tmp/_autotmp_aVJUgmssh-master/socket -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o BatchMode=yes -o ConnectTimeout=30 -o ServerAliveInterval=180 -o ServerAliveCountMax=3 -o ConnectionAttempts=4 -o Protocol=2 -l root -p 22 172.17.128.241'
14:20:33 ERROR| [stderr] Warning: Permanently added '172.17.128.241' (RSA) to the list of known hosts.

INFO    : Test results:
-------------------------------------------------------------------
platform_LibCBench                                      [  PASSED  ]
platform_LibCBench/platform_LibCBench                   [  PASSED  ]
platform_LibCBench/platform_LibCBench                     b_malloc_big1__0_                                     0.00375231466667
platform_LibCBench/platform_LibCBench                     b_malloc_big2__0_                                     0.002951359
platform_LibCBench/platform_LibCBench                     b_malloc_bubble__0_                                   0.015066374
platform_LibCBench/platform_LibCBench                     b_malloc_sparse__0_                                   0.015053784
platform_LibCBench/platform_LibCBench                     b_malloc_thread_local__0_                             0.01138439
platform_LibCBench/platform_LibCBench                     b_malloc_thread_stress__0_                            0.0367894733333
platform_LibCBench/platform_LibCBench                     b_malloc_tiny1__0_                                    0.000768474333333
platform_LibCBench/platform_LibCBench                     b_malloc_tiny2__0_                                    0.000581407333333
platform_LibCBench/platform_LibCBench                     b_pthread_create_serial1__0_                          0.0291785246667
platform_LibCBench/platform_LibCBench                     b_pthread_createjoin_serial1__0_                      0.031907936
platform_LibCBench/platform_LibCBench                     b_pthread_createjoin_serial2__0_                      0.043485347
platform_LibCBench/platform_LibCBench                     b_pthread_uselesslock__0_                             0.0294113346667
platform_LibCBench/platform_LibCBench                     b_regex_compile____a_b_c__d_b__                       0.00529833933333
platform_LibCBench/platform_LibCBench                     b_regex_search____a_b_c__d_b__                        0.00165455066667
platform_LibCBench/platform_LibCBench                     b_regex_search___a_25_b__                             0.0496191923333
platform_LibCBench/platform_LibCBench                     b_stdio_putcgetc__0_                                  0.100005711667
platform_LibCBench/platform_LibCBench                     b_stdio_putcgetc_unlocked__0_                         0.0371443833333
platform_LibCBench/platform_LibCBench                     b_string_memset__0_                                   0.00275405066667
platform_LibCBench/platform_LibCBench                     b_string_strchr__0_                                   0.00456903
platform_LibCBench/platform_LibCBench                     b_string_strlen__0_                                   0.044893587
platform_LibCBench/platform_LibCBench                     b_string_strstr___aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac__ 0.118360778
platform_LibCBench/platform_LibCBench                     b_string_strstr___aaaaaaaaaaaaaaaaaaaaaaaaac__        0.068957325
platform_LibCBench/platform_LibCBench                     b_string_strstr___aaaaaaaaaaaaaacccccccccccc__        0.0135694476667
platform_LibCBench/platform_LibCBench                     b_string_strstr___abcdefghijklmnopqrstuvwxyz__        0.0134553343333
platform_LibCBench/platform_LibCBench                     b_string_strstr___azbycxdwevfugthsirjqkplomn__        0.0133123556667
platform_LibCBench/platform_LibCBench                     b_utf8_bigbuf__0_                                     0.0473772253333
platform_LibCBench/platform_LibCBench                     b_utf8_onebyone__0_                                   0.130938538333
-------------------------------------------------------------------
Total PASS: 2/2 (100%)

INFO    : Elapsed time: 0m16s 
"""

error = """
ERROR: Identity added: /tmp/test_that.Z4Ld/autotest_key (/tmp/test_that.Z4Ld/autotest_key)
INFO    : Using emerged autotests already installed at /build/lumpy/usr/local/autotest.
INFO    : Running the following control files 1 times:
INFO    :  * 'client/site_tests/platform_LibCBench/control'
INFO    : Running client test client/site_tests/platform_LibCBench/control
INFO    : Test results:
INFO    : Elapsed time: 0m18s
"""

keyvals = {
    '': 'PASS',
    'b_stdio_putcgetc__0_': '0.100005711667',
    'b_string_strstr___azbycxdwevfugthsirjqkplomn__': '0.0133123556667',
    'b_malloc_thread_local__0_': '0.01138439',
    'b_string_strlen__0_': '0.044893587',
    'b_malloc_sparse__0_': '0.015053784',
    'b_string_memset__0_': '0.00275405066667',
    'platform_LibCBench': 'PASS',
    'b_pthread_uselesslock__0_': '0.0294113346667',
    'b_string_strchr__0_': '0.00456903',
    'b_pthread_create_serial1__0_': '0.0291785246667',
    'b_string_strstr___aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac__': '0.118360778',
    'b_string_strstr___aaaaaaaaaaaaaacccccccccccc__': '0.0135694476667',
    'b_pthread_createjoin_serial1__0_': '0.031907936',
    'b_malloc_thread_stress__0_': '0.0367894733333',
    'b_regex_search____a_b_c__d_b__': '0.00165455066667',
    'b_malloc_bubble__0_': '0.015066374',
    'b_malloc_big2__0_': '0.002951359',
    'b_stdio_putcgetc_unlocked__0_': '0.0371443833333',
    'b_pthread_createjoin_serial2__0_': '0.043485347',
    'b_regex_search___a_25_b__': '0.0496191923333',
    'b_utf8_bigbuf__0_': '0.0473772253333',
    'b_malloc_big1__0_': '0.00375231466667',
    'b_regex_compile____a_b_c__d_b__': '0.00529833933333',
    'b_string_strstr___aaaaaaaaaaaaaaaaaaaaaaaaac__': '0.068957325',
    'b_malloc_tiny2__0_': '0.000581407333333',
    'b_utf8_onebyone__0_': '0.130938538333',
    'b_malloc_tiny1__0_': '0.000768474333333',
    'b_string_strstr___abcdefghijklmnopqrstuvwxyz__': '0.0134553343333'
}

TMP_DIR1 = '/tmp/tmpAbcXyz'


class MockResult(Result):
  """Mock result class."""

  def __init__(self, mylogger, label, logging_level, machine):
    super(MockResult, self).__init__(mylogger, label, logging_level, machine)

  def FindFilesInResultsDir(self, find_args):
    return ''

  # pylint: disable=arguments-differ
  def GetKeyvals(self, temp=False):
    if temp:
      pass
    return keyvals


class ResultTest(unittest.TestCase):
  """Result test class."""

  def __init__(self, *args, **kwargs):
    super(ResultTest, self).__init__(*args, **kwargs)
    self.callFakeProcessResults = False
    self.fakeCacheReturnResult = None
    self.callGetResultsDir = False
    self.callProcessResults = False
    self.callGetPerfReportFiles = False
    self.kv_dict = None
    self.tmpdir = ''
    self.callGetNewKeyvals = False
    self.callGetResultsFile = False
    self.callGetPerfDataFiles = False
    self.args = None
    self.callGatherPerfResults = False
    self.mock_logger = mock.Mock(spec=logger.Logger)
    self.mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)
    self.mock_label = MockLabel('mock_label', 'chromeos_image', 'autotest_dir',
                                '/tmp', 'lumpy', 'remote', 'image_args',
                                'cache_dir', 'average', 'gcc', None)

  def testCreateFromRun(self):
    result = MockResult.CreateFromRun(logger.GetLogger(), 'average',
                                      self.mock_label, 'remote1', OUTPUT, error,
                                      0, True, 0)
    self.assertEqual(result.keyvals, keyvals)
    self.assertEqual(result.chroot_results_dir,
                     '/tmp/test_that.PO1234567/platform_LibCBench')
    self.assertEqual(result.results_dir,
                     '/tmp/chroot/tmp/test_that.PO1234567/platform_LibCBench')
    self.assertEqual(result.retval, 0)

  def setUp(self):
    self.result = Result(self.mock_logger, self.mock_label, 'average',
                         self.mock_cmd_exec)

  @mock.patch.object(os.path, 'isdir')
  @mock.patch.object(command_executer.CommandExecuter, 'RunCommand')
  @mock.patch.object(command_executer.CommandExecuter, 'CopyFiles')
  def test_copy_files_to(self, mock_copyfiles, mock_runcmd, mock_isdir):

    files = ['src_file_1', 'src_file_2', 'src_file_3']
    dest_dir = '/tmp/test'
    self.mock_cmd_exec.RunCommand = mock_runcmd
    self.mock_cmd_exec.CopyFiles = mock_copyfiles

    mock_copyfiles.return_value = 0

    #test 1. dest_dir exists; CopyFiles returns 0.
    mock_isdir.return_value = True
    self.result.CopyFilesTo(dest_dir, files)
    self.assertEqual(mock_runcmd.call_count, 0)
    self.assertEqual(mock_copyfiles.call_count, 3)
    first_args = mock_copyfiles.call_args_list[0][0]
    second_args = mock_copyfiles.call_args_list[1][0]
    third_args = mock_copyfiles.call_args_list[2][0]
    self.assertEqual(first_args, ('src_file_1', '/tmp/test/src_file_1.0'))
    self.assertEqual(second_args, ('src_file_2', '/tmp/test/src_file_2.0'))
    self.assertEqual(third_args, ('src_file_3', '/tmp/test/src_file_3.0'))

    mock_runcmd.reset_mock()
    mock_copyfiles.reset_mock()
    #test 2. dest_dir does not exist; CopyFiles returns 0.
    mock_isdir.return_value = False
    self.result.CopyFilesTo(dest_dir, files)
    self.assertEqual(mock_runcmd.call_count, 3)
    self.assertEqual(mock_copyfiles.call_count, 3)
    self.assertEqual(mock_runcmd.call_args_list[0],
                     mock_runcmd.call_args_list[1])
    self.assertEqual(mock_runcmd.call_args_list[0],
                     mock_runcmd.call_args_list[2])
    self.assertEqual(mock_runcmd.call_args_list[0][0], ('mkdir -p /tmp/test',))

    #test 3. CopyFiles returns 1 (fails).
    mock_copyfiles.return_value = 1
    self.assertRaises(Exception, self.result.CopyFilesTo, dest_dir, files)

  @mock.patch.object(Result, 'CopyFilesTo')
  def test_copy_results_to(self, mockCopyFilesTo):
    perf_data_files = [
        '/tmp/perf.data.0', '/tmp/perf.data.1', '/tmp/perf.data.2'
    ]
    perf_report_files = [
        '/tmp/perf.report.0', '/tmp/perf.report.1', '/tmp/perf.report.2'
    ]

    self.result.perf_data_files = perf_data_files
    self.result.perf_report_files = perf_report_files

    self.result.CopyFilesTo = mockCopyFilesTo
    self.result.CopyResultsTo('/tmp/results/')
    self.assertEqual(mockCopyFilesTo.call_count, 2)
    self.assertEqual(len(mockCopyFilesTo.call_args_list), 2)
    self.assertEqual(mockCopyFilesTo.call_args_list[0][0], ('/tmp/results/',
                                                            perf_data_files))
    self.assertEqual(mockCopyFilesTo.call_args_list[1][0], ('/tmp/results/',
                                                            perf_report_files))

  def test_get_new_keyvals(self):
    kv_dict = {}

    def FakeGetDataMeasurementsFiles():
      filename = os.path.join(os.getcwd(), 'unittest_keyval_file.txt')
      return [filename]

    self.result.GetDataMeasurementsFiles = FakeGetDataMeasurementsFiles
    kv_dict2, udict = self.result.GetNewKeyvals(kv_dict)
    self.assertEqual(kv_dict2, {
        u'Box2D__Box2D': 4775,
        u'Mandreel__Mandreel': 6620,
        u'Gameboy__Gameboy': 9901,
        u'Crypto__Crypto': 8737,
        u'telemetry_page_measurement_results__num_errored': 0,
        u'telemetry_page_measurement_results__num_failed': 0,
        u'PdfJS__PdfJS': 6455,
        u'Total__Score': 7918,
        u'EarleyBoyer__EarleyBoyer': 14340,
        u'MandreelLatency__MandreelLatency': 5188,
        u'CodeLoad__CodeLoad': 6271,
        u'DeltaBlue__DeltaBlue': 14401,
        u'Typescript__Typescript': 9815,
        u'SplayLatency__SplayLatency': 7653,
        u'zlib__zlib': 16094,
        u'Richards__Richards': 10358,
        u'RegExp__RegExp': 1765,
        u'NavierStokes__NavierStokes': 9815,
        u'Splay__Splay': 4425,
        u'RayTrace__RayTrace': 16600
    })
    self.assertEqual(udict, {
        u'Box2D__Box2D': u'score',
        u'Mandreel__Mandreel': u'score',
        u'Gameboy__Gameboy': u'score',
        u'Crypto__Crypto': u'score',
        u'telemetry_page_measurement_results__num_errored': u'count',
        u'telemetry_page_measurement_results__num_failed': u'count',
        u'PdfJS__PdfJS': u'score',
        u'Total__Score': u'score',
        u'EarleyBoyer__EarleyBoyer': u'score',
        u'MandreelLatency__MandreelLatency': u'score',
        u'CodeLoad__CodeLoad': u'score',
        u'DeltaBlue__DeltaBlue': u'score',
        u'Typescript__Typescript': u'score',
        u'SplayLatency__SplayLatency': u'score',
        u'zlib__zlib': u'score',
        u'Richards__Richards': u'score',
        u'RegExp__RegExp': u'score',
        u'NavierStokes__NavierStokes': u'score',
        u'Splay__Splay': u'score',
        u'RayTrace__RayTrace': u'score'
    })

  def test_append_telemetry_units(self):
    kv_dict = {
        u'Box2D__Box2D': 4775,
        u'Mandreel__Mandreel': 6620,
        u'Gameboy__Gameboy': 9901,
        u'Crypto__Crypto': 8737,
        u'PdfJS__PdfJS': 6455,
        u'Total__Score': 7918,
        u'EarleyBoyer__EarleyBoyer': 14340,
        u'MandreelLatency__MandreelLatency': 5188,
        u'CodeLoad__CodeLoad': 6271,
        u'DeltaBlue__DeltaBlue': 14401,
        u'Typescript__Typescript': 9815,
        u'SplayLatency__SplayLatency': 7653,
        u'zlib__zlib': 16094,
        u'Richards__Richards': 10358,
        u'RegExp__RegExp': 1765,
        u'NavierStokes__NavierStokes': 9815,
        u'Splay__Splay': 4425,
        u'RayTrace__RayTrace': 16600
    }
    units_dict = {
        u'Box2D__Box2D': u'score',
        u'Mandreel__Mandreel': u'score',
        u'Gameboy__Gameboy': u'score',
        u'Crypto__Crypto': u'score',
        u'PdfJS__PdfJS': u'score',
        u'Total__Score': u'score',
        u'EarleyBoyer__EarleyBoyer': u'score',
        u'MandreelLatency__MandreelLatency': u'score',
        u'CodeLoad__CodeLoad': u'score',
        u'DeltaBlue__DeltaBlue': u'score',
        u'Typescript__Typescript': u'score',
        u'SplayLatency__SplayLatency': u'score',
        u'zlib__zlib': u'score',
        u'Richards__Richards': u'score',
        u'RegExp__RegExp': u'score',
        u'NavierStokes__NavierStokes': u'score',
        u'Splay__Splay': u'score',
        u'RayTrace__RayTrace': u'score'
    }

    results_dict = self.result.AppendTelemetryUnits(kv_dict, units_dict)
    self.assertEqual(results_dict, {
        u'Box2D__Box2D': [4775, u'score'],
        u'Splay__Splay': [4425, u'score'],
        u'Gameboy__Gameboy': [9901, u'score'],
        u'Crypto__Crypto': [8737, u'score'],
        u'PdfJS__PdfJS': [6455, u'score'],
        u'Total__Score': [7918, u'score'],
        u'EarleyBoyer__EarleyBoyer': [14340, u'score'],
        u'MandreelLatency__MandreelLatency': [5188, u'score'],
        u'DeltaBlue__DeltaBlue': [14401, u'score'],
        u'SplayLatency__SplayLatency': [7653, u'score'],
        u'Mandreel__Mandreel': [6620, u'score'],
        u'Richards__Richards': [10358, u'score'],
        u'zlib__zlib': [16094, u'score'],
        u'CodeLoad__CodeLoad': [6271, u'score'],
        u'Typescript__Typescript': [9815, u'score'],
        u'RegExp__RegExp': [1765, u'score'],
        u'RayTrace__RayTrace': [16600, u'score'],
        u'NavierStokes__NavierStokes': [9815, u'score']
    })

  @mock.patch.object(misc, 'GetInsideChrootPath')
  @mock.patch.object(tempfile, 'mkdtemp')
  @mock.patch.object(command_executer.CommandExecuter, 'RunCommand')
  @mock.patch.object(command_executer.CommandExecuter,
                     'ChrootRunCommandWOutput')
  def test_get_keyvals(self, mock_chrootruncmd, mock_runcmd, mock_mkdtemp,
                       mock_getpath):

    self.kv_dict = {}
    self.callGetNewKeyvals = False

    def reset():
      self.kv_dict = {}
      self.callGetNewKeyvals = False
      mock_chrootruncmd.reset_mock()
      mock_runcmd.reset_mock()
      mock_mkdtemp.reset_mock()
      mock_getpath.reset_mock()

    def FakeGetNewKeyvals(kv_dict):
      self.kv_dict = kv_dict
      self.callGetNewKeyvals = True
      return_kvdict = {'first_time': 680, 'Total': 10}
      return_udict = {'first_time': 'ms', 'Total': 'score'}
      return return_kvdict, return_udict

    mock_mkdtemp.return_value = TMP_DIR1
    mock_chrootruncmd.return_value = [
        '', ('%s,PASS\n%s/telemetry_Crosperf,PASS\n') % (TMP_DIR1, TMP_DIR1), ''
    ]
    mock_getpath.return_value = TMP_DIR1
    self.result.ce.ChrootRunCommandWOutput = mock_chrootruncmd
    self.result.ce.RunCommand = mock_runcmd
    self.result.GetNewKeyvals = FakeGetNewKeyvals
    self.result.suite = 'telemetry_Crosperf'
    self.result.results_dir = '/tmp/test_that_resultsNmq'

    # Test 1. no self.temp_dir.
    res = self.result.GetKeyvals()
    self.assertTrue(self.callGetNewKeyvals)
    self.assertEqual(self.kv_dict, {'': 'PASS', 'telemetry_Crosperf': 'PASS'})
    self.assertEqual(mock_runcmd.call_count, 1)
    self.assertEqual(mock_runcmd.call_args_list[0][0],
                     ('cp -r /tmp/test_that_resultsNmq/* %s' % TMP_DIR1,))
    self.assertEqual(mock_chrootruncmd.call_count, 1)
    self.assertEqual(
        mock_chrootruncmd.call_args_list[0][0],
        ('/tmp',
         ('python generate_test_report --no-color --csv %s') % TMP_DIR1))
    self.assertEqual(mock_getpath.call_count, 1)
    self.assertEqual(mock_mkdtemp.call_count, 1)
    self.assertEqual(res, {'Total': [10, 'score'], 'first_time': [680, 'ms']})

    # Test 2. self.temp_dir
    reset()
    mock_chrootruncmd.return_value = [
        '', ('/tmp/tmpJCajRG,PASS\n/tmp/tmpJCajRG/'
             'telemetry_Crosperf,PASS\n'), ''
    ]
    mock_getpath.return_value = '/tmp/tmpJCajRG'
    self.result.temp_dir = '/tmp/tmpJCajRG'
    res = self.result.GetKeyvals()
    self.assertEqual(mock_runcmd.call_count, 0)
    self.assertEqual(mock_mkdtemp.call_count, 0)
    self.assertEqual(mock_chrootruncmd.call_count, 1)
    self.assertTrue(self.callGetNewKeyvals)
    self.assertEqual(self.kv_dict, {'': 'PASS', 'telemetry_Crosperf': 'PASS'})
    self.assertEqual(res, {'Total': [10, 'score'], 'first_time': [680, 'ms']})

    # Test 3. suite != telemetry_Crosperf.  Normally this would be for
    # running non-Telemetry autotests, such as BootPerfServer.  In this test
    # case, the keyvals we have set up were returned from a Telemetry test run;
    # so this pass is basically testing that we don't append the units to the
    # test results (which we do for Telemetry autotest runs).
    reset()
    self.result.suite = ''
    res = self.result.GetKeyvals()
    self.assertEqual(res, {'Total': 10, 'first_time': 680})

  def test_get_results_dir(self):

    self.result.out = ''
    self.assertRaises(Exception, self.result.GetResultsDir)

    self.result.out = OUTPUT
    resdir = self.result.GetResultsDir()
    self.assertEqual(resdir, '/tmp/test_that.PO1234567/platform_LibCBench')

  @mock.patch.object(command_executer.CommandExecuter, 'RunCommandGeneric')
  def test_find_files_in_results_dir(self, mock_runcmd):

    self.result.results_dir = None
    res = self.result.FindFilesInResultsDir('-name perf.data')
    self.assertIsNone(res)

    self.result.ce.RunCommand = mock_runcmd
    self.result.results_dir = '/tmp/test_results'
    mock_runcmd.return_value = [0, '/tmp/test_results/perf.data', '']
    res = self.result.FindFilesInResultsDir('-name perf.data')
    self.assertEqual(mock_runcmd.call_count, 1)
    self.assertEqual(mock_runcmd.call_args_list[0][0],
                     ('find /tmp/test_results -name perf.data',))
    self.assertEqual(res, '/tmp/test_results/perf.data')

    mock_runcmd.reset_mock()
    mock_runcmd.return_value = [1, '', '']
    self.assertRaises(Exception, self.result.FindFilesInResultsDir,
                      '-name perf.data')

  @mock.patch.object(Result, 'FindFilesInResultsDir')
  def test_get_perf_data_files(self, mock_findfiles):
    self.args = None

    mock_findfiles.return_value = 'line1\nline1\n'
    self.result.FindFilesInResultsDir = mock_findfiles
    res = self.result.GetPerfDataFiles()
    self.assertEqual(res, ['line1', 'line1'])
    self.assertEqual(mock_findfiles.call_args_list[0][0], ('-name perf.data',))

  def test_get_perf_report_files(self):
    self.args = None

    def FakeFindFiles(find_args):
      self.args = find_args
      return 'line1\nline1\n'

    self.result.FindFilesInResultsDir = FakeFindFiles
    res = self.result.GetPerfReportFiles()
    self.assertEqual(res, ['line1', 'line1'])
    self.assertEqual(self.args, '-name perf.data.report')

  def test_get_data_measurement_files(self):
    self.args = None

    def FakeFindFiles(find_args):
      self.args = find_args
      return 'line1\nline1\n'

    self.result.FindFilesInResultsDir = FakeFindFiles
    res = self.result.GetDataMeasurementsFiles()
    self.assertEqual(res, ['line1', 'line1'])
    self.assertEqual(self.args, '-name perf_measurements')

  @mock.patch.object(misc, 'GetInsideChrootPath')
  @mock.patch.object(command_executer.CommandExecuter, 'ChrootRunCommand')
  def test_generate_perf_report_files(self, mock_chrootruncmd, mock_getpath):
    fake_file = '/usr/chromeos/chroot/tmp/results/fake_file'
    self.result.perf_data_files = ['/tmp/results/perf.data']
    self.result.board = 'lumpy'
    mock_getpath.return_value = fake_file
    self.result.ce.ChrootRunCommand = mock_chrootruncmd
    tmp = self.result.GeneratePerfReportFiles()
    self.assertEqual(tmp, ['/tmp/chroot%s' % fake_file])
    self.assertEqual(mock_chrootruncmd.call_args_list[0][0],
                     ('/tmp',
                      ('/usr/sbin/perf report -n --symfs /build/lumpy '
                       '--vmlinux /build/lumpy/usr/lib/debug/boot/vmlinux '
                       '--kallsyms /build/lumpy/boot/System.map-* -i '
                       '%s --stdio > %s') % (fake_file, fake_file)))

  @mock.patch.object(misc, 'GetOutsideChrootPath')
  def test_populate_from_run(self, mock_getpath):

    def FakeGetResultsDir():
      self.callGetResultsDir = True
      return '/tmp/results_dir'

    def FakeGetResultsFile():
      self.callGetResultsFile = True
      return []

    def FakeGetPerfDataFiles():
      self.callGetPerfDataFiles = True
      return []

    def FakeGetPerfReportFiles():
      self.callGetPerfReportFiles = True
      return []

    def FakeProcessResults(show_results=False):
      if show_results:
        pass
      self.callProcessResults = True

    if mock_getpath:
      pass
    mock.get_path = '/tmp/chromeos/tmp/results_dir'
    self.result.chromeos_root = '/tmp/chromeos'

    self.callGetResultsDir = False
    self.callGetResultsFile = False
    self.callGetPerfDataFiles = False
    self.callGetPerfReportFiles = False
    self.callProcessResults = False

    self.result.GetResultsDir = FakeGetResultsDir
    self.result.GetResultsFile = FakeGetResultsFile
    self.result.GetPerfDataFiles = FakeGetPerfDataFiles
    self.result.GeneratePerfReportFiles = FakeGetPerfReportFiles
    self.result.ProcessResults = FakeProcessResults

    self.result.PopulateFromRun(OUTPUT, '', 0, 'test', 'telemetry_Crosperf')
    self.assertTrue(self.callGetResultsDir)
    self.assertTrue(self.callGetResultsFile)
    self.assertTrue(self.callGetPerfDataFiles)
    self.assertTrue(self.callGetPerfReportFiles)
    self.assertTrue(self.callProcessResults)

  def test_process_results(self):

    def FakeGetKeyvals(show_all=False):
      if show_all:
        return {'first_time': 680, 'Total': 10}
      else:
        return {'Total': 10}

    def FakeGatherPerfResults():
      self.callGatherPerfResults = True

    self.callGatherPerfResults = False

    self.result.GetKeyvals = FakeGetKeyvals
    self.result.GatherPerfResults = FakeGatherPerfResults

    self.result.retval = 0
    self.result.ProcessResults()
    self.assertTrue(self.callGatherPerfResults)
    self.assertEqual(len(self.result.keyvals), 2)
    self.assertEqual(self.result.keyvals, {'Total': 10, 'retval': 0})

    self.result.retval = 1
    self.result.ProcessResults()
    self.assertEqual(len(self.result.keyvals), 2)
    self.assertEqual(self.result.keyvals, {'Total': 10, 'retval': 1})

  @mock.patch.object(misc, 'GetInsideChrootPath')
  @mock.patch.object(command_executer.CommandExecuter,
                     'ChrootRunCommandWOutput')
  def test_populate_from_cache_dir(self, mock_runchrootcmd, mock_getpath):

    # pylint: disable=redefined-builtin
    def FakeMkdtemp(dir=None):
      if dir:
        pass
      return self.tmpdir

    current_path = os.getcwd()
    cache_dir = os.path.join(current_path, 'test_cache/test_input')
    self.result.ce = command_executer.GetCommandExecuter(log_level='average')
    self.result.ce.ChrootRunCommandWOutput = mock_runchrootcmd
    mock_runchrootcmd.return_value = [
        '', ('%s,PASS\n%s/\telemetry_Crosperf,PASS\n') % (TMP_DIR1, TMP_DIR1),
        ''
    ]
    mock_getpath.return_value = TMP_DIR1
    self.tmpdir = tempfile.mkdtemp()
    save_real_mkdtemp = tempfile.mkdtemp
    tempfile.mkdtemp = FakeMkdtemp

    self.result.PopulateFromCacheDir(cache_dir, 'sunspider',
                                     'telemetry_Crosperf')
    self.assertEqual(self.result.keyvals, {
        u'Total__Total': [444.0, u'ms'],
        u'regexp-dna__regexp-dna': [16.2, u'ms'],
        u'telemetry_page_measurement_results__num_failed': [0, u'count'],
        u'telemetry_page_measurement_results__num_errored': [0, u'count'],
        u'string-fasta__string-fasta': [23.2, u'ms'],
        u'crypto-sha1__crypto-sha1': [11.6, u'ms'],
        u'bitops-3bit-bits-in-byte__bitops-3bit-bits-in-byte': [3.2, u'ms'],
        u'access-nsieve__access-nsieve': [7.9, u'ms'],
        u'bitops-nsieve-bits__bitops-nsieve-bits': [9.4, u'ms'],
        u'string-validate-input__string-validate-input': [19.3, u'ms'],
        u'3d-raytrace__3d-raytrace': [24.7, u'ms'],
        u'3d-cube__3d-cube': [28.0, u'ms'],
        u'string-unpack-code__string-unpack-code': [46.7, u'ms'],
        u'date-format-tofte__date-format-tofte': [26.3, u'ms'],
        u'math-partial-sums__math-partial-sums': [22.0, u'ms'],
        '\telemetry_Crosperf': ['PASS', ''],
        u'crypto-aes__crypto-aes': [15.2, u'ms'],
        u'bitops-bitwise-and__bitops-bitwise-and': [8.4, u'ms'],
        u'crypto-md5__crypto-md5': [10.5, u'ms'],
        u'string-tagcloud__string-tagcloud': [52.8, u'ms'],
        u'access-nbody__access-nbody': [8.5, u'ms'],
        'retval': 0,
        u'math-spectral-norm__math-spectral-norm': [6.6, u'ms'],
        u'math-cordic__math-cordic': [8.7, u'ms'],
        u'access-binary-trees__access-binary-trees': [4.5, u'ms'],
        u'controlflow-recursive__controlflow-recursive': [4.4, u'ms'],
        u'access-fannkuch__access-fannkuch': [17.8, u'ms'],
        u'string-base64__string-base64': [16.0, u'ms'],
        u'date-format-xparb__date-format-xparb': [20.9, u'ms'],
        u'3d-morph__3d-morph': [22.1, u'ms'],
        u'bitops-bits-in-byte__bitops-bits-in-byte': [9.1, u'ms']
    })

    # Clean up after test.
    tempfile.mkdtemp = save_real_mkdtemp
    command = 'rm -Rf %s' % self.tmpdir
    self.result.ce.RunCommand(command)

  @mock.patch.object(misc, 'GetRoot')
  @mock.patch.object(command_executer.CommandExecuter, 'RunCommand')
  def test_cleanup(self, mock_runcmd, mock_getroot):

    # Test 1. 'rm_chroot_tmp' is True; self.results_dir exists;
    # self.temp_dir exists; results_dir name contains 'test_that_results_'.
    mock_getroot.return_value = ['/tmp/tmp_AbcXyz', 'test_that_results_fake']
    self.result.ce.RunCommand = mock_runcmd
    self.result.results_dir = 'test_results_dir'
    self.result.temp_dir = 'testtemp_dir'
    self.result.CleanUp(True)
    self.assertEqual(mock_getroot.call_count, 1)
    self.assertEqual(mock_runcmd.call_count, 2)
    self.assertEqual(mock_runcmd.call_args_list[0][0],
                     ('rm -rf test_results_dir',))
    self.assertEqual(mock_runcmd.call_args_list[1][0], ('rm -rf testtemp_dir',))

    # Test 2. Same, except ath results_dir name does not contain
    # 'test_that_results_'
    mock_getroot.reset_mock()
    mock_runcmd.reset_mock()
    mock_getroot.return_value = ['/tmp/tmp_AbcXyz', 'other_results_fake']
    self.result.ce.RunCommand = mock_runcmd
    self.result.results_dir = 'test_results_dir'
    self.result.temp_dir = 'testtemp_dir'
    self.result.CleanUp(True)
    self.assertEqual(mock_getroot.call_count, 1)
    self.assertEqual(mock_runcmd.call_count, 2)
    self.assertEqual(mock_runcmd.call_args_list[0][0],
                     ('rm -rf /tmp/tmp_AbcXyz',))
    self.assertEqual(mock_runcmd.call_args_list[1][0], ('rm -rf testtemp_dir',))

    # Test 3. mock_getroot returns nothing; 'rm_chroot_tmp' is False.
    mock_getroot.reset_mock()
    mock_runcmd.reset_mock()
    self.result.CleanUp(False)
    self.assertEqual(mock_getroot.call_count, 0)
    self.assertEqual(mock_runcmd.call_count, 1)
    self.assertEqual(mock_runcmd.call_args_list[0][0], ('rm -rf testtemp_dir',))

    # Test 4. 'rm_chroot_tmp' is True, but result_dir & temp_dir are None.
    mock_getroot.reset_mock()
    mock_runcmd.reset_mock()
    self.result.results_dir = None
    self.result.temp_dir = None
    self.result.CleanUp(True)
    self.assertEqual(mock_getroot.call_count, 0)
    self.assertEqual(mock_runcmd.call_count, 0)

  @mock.patch.object(misc, 'GetInsideChrootPath')
  @mock.patch.object(command_executer.CommandExecuter, 'ChrootRunCommand')
  def test_store_to_cache_dir(self, mock_chrootruncmd, mock_getpath):

    def FakeMkdtemp(directory=''):
      if directory:
        pass
      return self.tmpdir

    if mock_chrootruncmd or mock_getpath:
      pass
    current_path = os.getcwd()
    cache_dir = os.path.join(current_path, 'test_cache/test_output')

    self.result.ce = command_executer.GetCommandExecuter(log_level='average')
    self.result.out = OUTPUT
    self.result.err = error
    self.result.retval = 0
    self.tmpdir = tempfile.mkdtemp()
    if not os.path.exists(self.tmpdir):
      os.makedirs(self.tmpdir)
    self.result.results_dir = os.path.join(os.getcwd(), 'test_cache')
    save_real_mkdtemp = tempfile.mkdtemp
    tempfile.mkdtemp = FakeMkdtemp

    mock_mm = machine_manager.MockMachineManager('/tmp/chromeos_root', 0,
                                                 'average', '')
    mock_mm.machine_checksum_string['mock_label'] = 'fake_machine_checksum123'

    mock_keylist = ['key1', 'key2', 'key3']
    test_flag.SetTestMode(True)
    self.result.StoreToCacheDir(cache_dir, mock_mm, mock_keylist)

    # Check that the correct things were written to the 'cache'.
    test_dir = os.path.join(os.getcwd(), 'test_cache/test_output')
    base_dir = os.path.join(os.getcwd(), 'test_cache/compare_output')
    self.assertTrue(os.path.exists(os.path.join(test_dir, 'autotest.tbz2')))
    self.assertTrue(os.path.exists(os.path.join(test_dir, 'machine.txt')))
    self.assertTrue(os.path.exists(os.path.join(test_dir, 'results.txt')))

    f1 = os.path.join(test_dir, 'machine.txt')
    f2 = os.path.join(base_dir, 'machine.txt')
    cmd = 'diff %s %s' % (f1, f2)
    [_, out, _] = self.result.ce.RunCommandWOutput(cmd)
    self.assertEqual(len(out), 0)

    f1 = os.path.join(test_dir, 'results.txt')
    f2 = os.path.join(base_dir, 'results.txt')
    cmd = 'diff %s %s' % (f1, f2)
    [_, out, _] = self.result.ce.RunCommandWOutput(cmd)
    self.assertEqual(len(out), 0)

    # Clean up after test.
    tempfile.mkdtemp = save_real_mkdtemp
    command = 'rm %s/*' % test_dir
    self.result.ce.RunCommand(command)


TELEMETRY_RESULT_KEYVALS = {
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'math-cordic (ms)':
        '11.4',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'access-nbody (ms)':
        '6.9',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'access-fannkuch (ms)':
        '26.3',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'math-spectral-norm (ms)':
        '6.3',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'bitops-nsieve-bits (ms)':
        '9.3',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'math-partial-sums (ms)':
        '32.8',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'regexp-dna (ms)':
        '16.1',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    '3d-cube (ms)':
        '42.7',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'crypto-md5 (ms)':
        '10.8',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'crypto-sha1 (ms)':
        '12.4',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'string-tagcloud (ms)':
        '47.2',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'string-fasta (ms)':
        '36.3',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'access-binary-trees (ms)':
        '7.3',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'date-format-xparb (ms)':
        '138.1',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'crypto-aes (ms)':
        '19.2',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'Total (ms)':
        '656.5',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'string-base64 (ms)':
        '17.5',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'string-validate-input (ms)':
        '24.8',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    '3d-raytrace (ms)':
        '28.7',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'controlflow-recursive (ms)':
        '5.3',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'bitops-bits-in-byte (ms)':
        '9.8',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    '3d-morph (ms)':
        '50.2',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'bitops-bitwise-and (ms)':
        '8.8',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'access-nsieve (ms)':
        '8.6',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'date-format-tofte (ms)':
        '31.2',
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'bitops-3bit-bits-in-byte (ms)':
        '3.5',
    'retval':
        0,
    'http://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html '
    'string-unpack-code (ms)':
        '45.0'
}

PURE_TELEMETRY_OUTPUT = """
page_name,3d-cube (ms),3d-morph (ms),3d-raytrace (ms),Total (ms),access-binary-trees (ms),access-fannkuch (ms),access-nbody (ms),access-nsieve (ms),bitops-3bit-bits-in-byte (ms),bitops-bits-in-byte (ms),bitops-bitwise-and (ms),bitops-nsieve-bits (ms),controlflow-recursive (ms),crypto-aes (ms),crypto-md5 (ms),crypto-sha1 (ms),date-format-tofte (ms),date-format-xparb (ms),math-cordic (ms),math-partial-sums (ms),math-spectral-norm (ms),regexp-dna (ms),string-base64 (ms),string-fasta (ms),string-tagcloud (ms),string-unpack-code (ms),string-validate-input (ms)\r\nhttp://www.webkit.org/perf/sunspider-1.0.2/sunspider-1.0.2/driver.html,42.7,50.2,28.7,656.5,7.3,26.3,6.9,8.6,3.5,9.8,8.8,9.3,5.3,19.2,10.8,12.4,31.2,138.1,11.4,32.8,6.3,16.1,17.5,36.3,47.2,45.0,24.8\r
"""


class TelemetryResultTest(unittest.TestCase):
  """Telemetry result test."""

  def __init__(self, *args, **kwargs):
    super(TelemetryResultTest, self).__init__(*args, **kwargs)
    self.callFakeProcessResults = False
    self.result = None
    self.mock_logger = mock.Mock(spec=logger.Logger)
    self.mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)
    self.mock_label = MockLabel('mock_label', 'chromeos_image', 'autotest_dir',
                                '/tmp', 'lumpy', 'remote', 'image_args',
                                'cache_dir', 'average', 'gcc', None)
    self.mock_machine = machine_manager.MockCrosMachine(
        'falco.cros', '/tmp/chromeos', 'average')

  def test_populate_from_run(self):

    def FakeProcessResults():
      self.callFakeProcessResults = True

    self.callFakeProcessResults = False
    self.result = TelemetryResult(self.mock_logger, self.mock_label, 'average',
                                  self.mock_cmd_exec)
    self.result.ProcessResults = FakeProcessResults
    self.result.PopulateFromRun(OUTPUT, error, 3, 'fake_test',
                                'telemetry_Crosperf')
    self.assertTrue(self.callFakeProcessResults)
    self.assertEqual(self.result.out, OUTPUT)
    self.assertEqual(self.result.err, error)
    self.assertEqual(self.result.retval, 3)

  def test_populate_from_cache_dir_and_process_results(self):

    self.result = TelemetryResult(self.mock_logger, self.mock_label, 'average',
                                  self.mock_machine)
    current_path = os.getcwd()
    cache_dir = os.path.join(current_path,
                             'test_cache/test_puretelemetry_input')
    self.result.PopulateFromCacheDir(cache_dir, '', '')
    self.assertEqual(self.result.out.strip(), PURE_TELEMETRY_OUTPUT.strip())
    self.assertEqual(self.result.err, '')
    self.assertEqual(self.result.retval, 0)
    self.assertEqual(self.result.keyvals, TELEMETRY_RESULT_KEYVALS)


class ResultsCacheTest(unittest.TestCase):
  """Resultcache test class."""

  def __init__(self, *args, **kwargs):
    super(ResultsCacheTest, self).__init__(*args, **kwargs)
    self.fakeCacheReturnResult = None
    self.mock_logger = mock.Mock(spec=logger.Logger)
    self.mock_label = MockLabel('mock_label', 'chromeos_image', 'autotest_dir',
                                '/tmp', 'lumpy', 'remote', 'image_args',
                                'cache_dir', 'average', 'gcc', None)

  def setUp(self):
    self.results_cache = ResultsCache()

    mock_machine = machine_manager.MockCrosMachine('falco.cros',
                                                   '/tmp/chromeos', 'average')

    mock_mm = machine_manager.MockMachineManager('/tmp/chromeos_root', 0,
                                                 'average', '')
    mock_mm.machine_checksum_string['mock_label'] = 'fake_machine_checksum123'

    self.results_cache.Init(
        self.mock_label.chromeos_image,
        self.mock_label.chromeos_root,
        'sunspider',
        1,  # benchmark_run.iteration,
        '',  # benchmark_run.test_args,
        '',  # benchmark_run.profiler_args,
        mock_mm,
        mock_machine,
        self.mock_label.board,
        [CacheConditions.CACHE_FILE_EXISTS, CacheConditions.CHECKSUMS_MATCH],
        self.mock_logger,
        'average',
        self.mock_label,
        '',  # benchmark_run.share_cache
        'telemetry_Crosperf',
        True,  # benchmark_run.show_all_results
        False)  # benchmark_run.run_local

  @mock.patch.object(image_checksummer.ImageChecksummer, 'Checksum')
  def test_get_cache_dir_for_write(self, mock_checksum):

    def FakeGetMachines(label):
      if label:
        pass
      m1 = machine_manager.MockCrosMachine(
          'lumpy1.cros', self.results_cache.chromeos_root, 'average')
      m2 = machine_manager.MockCrosMachine(
          'lumpy2.cros', self.results_cache.chromeos_root, 'average')
      return [m1, m2]

    mock_checksum.return_value = 'FakeImageChecksumabc123'
    self.results_cache.machine_manager.GetMachines = FakeGetMachines
    self.results_cache.machine_manager.machine_checksum['mock_label'] = \
        'FakeMachineChecksumabc987'
    # Based on the label, benchmark and machines, get the directory in which
    # to store the cache information for this test run.
    result_path = self.results_cache.GetCacheDirForWrite()
    # Verify that the returned directory is correct (since the label
    # contained a cache_dir, named 'cache_dir', that's what is expected in
    # the result, rather than '~/cros_scratch').
    comp_path = os.path.join(os.getcwd(),
                             'cache_dir/54524606abaae4fdf7b02f49f7ae7127_'
                             'sunspider_1_fda29412ceccb72977516c4785d08e2c_'
                             'FakeImageChecksumabc123_FakeMachineChecksum'
                             'abc987__6')
    self.assertEqual(result_path, comp_path)

  def test_form_cache_dir(self):
    # This is very similar to the previous test (FormCacheDir is called
    # from GetCacheDirForWrite).
    cache_key_list = ('54524606abaae4fdf7b02f49f7ae7127', 'sunspider', '1',
                      '7215ee9c7d9dc229d2921a40e899ec5f',
                      'FakeImageChecksumabc123', '*', '*', '6')
    path = self.results_cache.FormCacheDir(cache_key_list)
    self.assertEqual(len(path), 1)
    path1 = path[0]
    test_dirname = ('54524606abaae4fdf7b02f49f7ae7127_sunspider_1_7215ee9'
                    'c7d9dc229d2921a40e899ec5f_FakeImageChecksumabc123_*_*_6')
    comp_path = os.path.join(os.getcwd(), 'cache_dir', test_dirname)
    self.assertEqual(path1, comp_path)

  @mock.patch.object(image_checksummer.ImageChecksummer, 'Checksum')
  def test_get_cache_key_list(self, mock_checksum):
    # This tests the mechanism that generates the various pieces of the
    # cache directory name, based on various conditions.

    def FakeGetMachines(label):
      if label:
        pass
      m1 = machine_manager.MockCrosMachine(
          'lumpy1.cros', self.results_cache.chromeos_root, 'average')
      m2 = machine_manager.MockCrosMachine(
          'lumpy2.cros', self.results_cache.chromeos_root, 'average')
      return [m1, m2]

    mock_checksum.return_value = 'FakeImageChecksumabc123'
    self.results_cache.machine_manager.GetMachines = FakeGetMachines
    self.results_cache.machine_manager.machine_checksum['mock_label'] = \
        'FakeMachineChecksumabc987'

    # Test 1. Generating cache name for reading (not writing).
    key_list = self.results_cache.GetCacheKeyList(True)
    self.assertEqual(key_list[0], '*')  # Machine checksum value, for read.
    self.assertEqual(key_list[1], 'sunspider')
    self.assertEqual(key_list[2], '1')
    self.assertEqual(key_list[3], 'fda29412ceccb72977516c4785d08e2c')
    self.assertEqual(key_list[4], 'FakeImageChecksumabc123')
    self.assertEqual(key_list[5], '*')
    self.assertEqual(key_list[6], '*')
    self.assertEqual(key_list[7], '6')

    # Test 2. Generating cache name for writing, with local image type.
    key_list = self.results_cache.GetCacheKeyList(False)
    self.assertEqual(key_list[0], '54524606abaae4fdf7b02f49f7ae7127')
    self.assertEqual(key_list[1], 'sunspider')
    self.assertEqual(key_list[2], '1')
    self.assertEqual(key_list[3], 'fda29412ceccb72977516c4785d08e2c')
    self.assertEqual(key_list[4], 'FakeImageChecksumabc123')
    self.assertEqual(key_list[5], 'FakeMachineChecksumabc987')
    self.assertEqual(key_list[6], '')
    self.assertEqual(key_list[7], '6')

    # Test 3. Generating cache name for writing, with trybot image type.
    self.results_cache.label.image_type = 'trybot'
    key_list = self.results_cache.GetCacheKeyList(False)
    self.assertEqual(key_list[0], '54524606abaae4fdf7b02f49f7ae7127')
    self.assertEqual(key_list[3], 'fda29412ceccb72977516c4785d08e2c')
    self.assertEqual(key_list[4], '54524606abaae4fdf7b02f49f7ae7127')
    self.assertEqual(key_list[5], 'FakeMachineChecksumabc987')

    # Test 4. Generating cache name for writing, with official image type.
    self.results_cache.label.image_type = 'official'
    key_list = self.results_cache.GetCacheKeyList(False)
    self.assertEqual(key_list[0], '54524606abaae4fdf7b02f49f7ae7127')
    self.assertEqual(key_list[1], 'sunspider')
    self.assertEqual(key_list[2], '1')
    self.assertEqual(key_list[3], 'fda29412ceccb72977516c4785d08e2c')
    self.assertEqual(key_list[4], '*')
    self.assertEqual(key_list[5], 'FakeMachineChecksumabc987')
    self.assertEqual(key_list[6], '')
    self.assertEqual(key_list[7], '6')

    # Test 5. Generating cache name for writing, with local image type, and
    # specifying that the image path must match the cached image path.
    self.results_cache.label.image_type = 'local'
    self.results_cache.cache_conditions.append(CacheConditions.IMAGE_PATH_MATCH)
    key_list = self.results_cache.GetCacheKeyList(False)
    self.assertEqual(key_list[0], '54524606abaae4fdf7b02f49f7ae7127')
    self.assertEqual(key_list[3], 'fda29412ceccb72977516c4785d08e2c')
    self.assertEqual(key_list[4], 'FakeImageChecksumabc123')
    self.assertEqual(key_list[5], 'FakeMachineChecksumabc987')

  @mock.patch.object(command_executer.CommandExecuter, 'RunCommand')
  @mock.patch.object(os.path, 'isdir')
  @mock.patch.object(Result, 'CreateFromCacheHit')
  def test_read_result(self, mock_create, mock_isdir, mock_runcmd):

    self.fakeCacheReturnResult = None

    def FakeGetCacheDirForRead():
      return self.fakeCacheReturnResult

    def FakeGetCacheDirForWrite():
      return self.fakeCacheReturnResult

    mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)
    fake_result = Result(self.mock_logger, self.mock_label, 'average',
                         mock_cmd_exec)
    fake_result.retval = 0

    # Set up results_cache _GetCacheDirFor{Read,Write} to return
    # self.fakeCacheReturnResult, which is initially None (see above).
    # So initially, no cache dir is returned.
    self.results_cache.GetCacheDirForRead = FakeGetCacheDirForRead
    self.results_cache.GetCacheDirForWrite = FakeGetCacheDirForWrite

    mock_isdir.return_value = True
    save_cc = [
        CacheConditions.CACHE_FILE_EXISTS, CacheConditions.CHECKSUMS_MATCH
    ]
    self.results_cache.cache_conditions.append(CacheConditions.FALSE)

    # Test 1. CacheCondition.FALSE, which means do not read from the cache.
    # (force re-running of test).  Result should be None.
    res = self.results_cache.ReadResult()
    self.assertIsNone(res)
    self.assertEqual(mock_runcmd.call_count, 1)

    # Test 2. Remove CacheCondition.FALSE. Result should still be None,
    # because GetCacheDirForRead is returning None at the moment.
    mock_runcmd.reset_mock()
    self.results_cache.cache_conditions = save_cc
    res = self.results_cache.ReadResult()
    self.assertIsNone(res)
    self.assertEqual(mock_runcmd.call_count, 0)

    # Test 3. Now set up cache dir to be returned by GetCacheDirForRead.
    # Since cache_dir is found, will call Result.CreateFromCacheHit, which
    # which will actually all our mock_create and should return fake_result.
    self.fakeCacheReturnResult = 'fake/cache/dir'
    mock_create.return_value = fake_result
    res = self.results_cache.ReadResult()
    self.assertEqual(mock_runcmd.call_count, 0)
    self.assertEqual(res, fake_result)

    # Test 4. os.path.isdir(cache_dir) will now return false, so result
    # should be None again (no cache found).
    mock_isdir.return_value = False
    res = self.results_cache.ReadResult()
    self.assertEqual(mock_runcmd.call_count, 0)
    self.assertIsNone(res)

    # Test 5. os.path.isdir returns true, but mock_create now returns None
    # (the call to CreateFromCacheHit returns None), so overal result is None.
    mock_isdir.return_value = True
    mock_create.return_value = None
    res = self.results_cache.ReadResult()
    self.assertEqual(mock_runcmd.call_count, 0)
    self.assertIsNone(res)

    # Test 6. Everything works 'as expected', result should be fake_result.
    mock_create.return_value = fake_result
    res = self.results_cache.ReadResult()
    self.assertEqual(mock_runcmd.call_count, 0)
    self.assertEqual(res, fake_result)

    # Test 7. The run failed; result should be None.
    mock_create.return_value = fake_result
    fake_result.retval = 1
    self.results_cache.cache_conditions.append(CacheConditions.RUN_SUCCEEDED)
    res = self.results_cache.ReadResult()
    self.assertEqual(mock_runcmd.call_count, 0)
    self.assertIsNone(res)


if __name__ == '__main__':
  unittest.main()
