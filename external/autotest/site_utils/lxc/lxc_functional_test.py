#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Function tests of lxc module. To be able to run this test, following setup
is required:
  1. lxc is installed.
  2. Autotest code exists in /usr/local/autotest, with site-packages installed.
     (run utils/build_externals.py)
  3. The user runs the test should have sudo access. Run the test with sudo.
Note that the test does not require Autotest database and frontend.
"""


import argparse
import logging
import os
import tempfile

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import unittest_setup


TEST_JOB_ID = 123
TEST_JOB_FOLDER = '123-debug_user'
# Create a temp directory for functional tests. The directory is not under /tmp
# for Moblab to be able to run the test.
TEMP_DIR = tempfile.mkdtemp(dir=lxc.DEFAULT_CONTAINER_PATH,
                            prefix='container_test_')
RESULT_PATH = os.path.join(TEMP_DIR, 'results', str(TEST_JOB_ID))
# Link to download a test package of autotest server package.
# Ideally the test should stage a build on devserver and download the
# autotest_server_package from devserver. This test is focused on testing
# container, so it's prefered to avoid dependency on devserver.
AUTOTEST_SERVER_PKG = ('http://storage.googleapis.com/abci-ssp/'
                       'autotest-containers/autotest_server_package.tar.bz2')

# Test log file to be created in result folder, content is `test`.
TEST_LOG = 'test.log'
# Name of test script file to run in container.
TEST_SCRIPT = 'test.py'
# Test script to run in container to verify autotest code setup.
TEST_SCRIPT_CONTENT = """
import socket
import sys

# Test import
import common
import chromite

# This test has to be before the import of autotest_lib, because ts_mon requires
# httplib2 module in chromite/third_party. The one in Autotest site-packages is
# out dated.
%(ts_mon_test)s

from autotest_lib.server import utils
from autotest_lib.site_utils import lxc

with open(sys.argv[1], 'w') as f:
    f.write('test')

# Confirm hostname starts with `test-`
if not socket.gethostname().startswith('test-'):
    raise Exception('The container\\\'s hostname must start with `test-`.')

# Test installing packages
lxc.install_packages(['atop'], ['acora'])

"""

TEST_SCRIPT_CONTENT_TS_MON = """
# Test ts_mon metrics can be set up.
from chromite.lib import ts_mon_config
ts_mon_config.SetupTsMonGlobalState('some_test', suppress_exception=False)
"""

CREATE_FAKE_TS_MON_CONFIG_SCRIPT = 'create_fake_key.py'

CREATE_FAKE_TS_MON_CONFIG_SCRIPT_CONTENT = """
import os
import rsa

EXPECTED_TS_MON_CONFIG_NAME = '/etc/chrome-infra/ts-mon.json'

FAKE_TS_MON_CONFIG_CONTENT = '''
    {
        "credentials":"/tmp/service_account_prodx_mon.json",
        "endpoint":"https://xxx.googleapis.com/v1:insert",
        "use_new_proto": true
    }'''

FAKE_SERVICE_ACCOUNT_CRED_JSON = '''
    {
        "type": "service_account",
        "project_id": "test_project",
        "private_key_id": "aaa",
        "private_key": "%s",
        "client_email": "xxx",
        "client_id": "111",
        "auth_uri": "https://accounts.google.com/o/oauth2/auth",
        "token_uri": "https://accounts.google.com/o/oauth2/token",
        "auth_provider_x509_cert_url":
                "https://www.googleapis.com/oauth2/v1/certs",
        "client_x509_cert_url":
                "https://www.googleapis.com/robot/v1/metadata/x509/xxx"
    }'''


TEST_KEY = '''------BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCzg4K2SXqf9LAM
52a/t2HfpY5y49sbrgRb1llP6c8RVWhUX/pGdjbcIM97+1CJEWBN8Vmraoe4+71o
1idTPehJfHRNeyXQUnro8CmnSxE9tLHtdKj0pzvO+yqT66O6Iw1aUAIX+dG4Us9Q
Z22ypFHaJ74lKw9JFwAFTJ/TF1rXUXqgufYTNNqP3Ra7wCHF8BmtjwRYAlvsR9CO
c4eVC1+qhq/8/EOMCgF/rsbZW93r/nz5xgsSX0k6WkAz5WX2mniHfmBFpmr039jZ
0eI1mEMGDAYuUn05++dNveo/ZOZj3wBlFzyfNSeeWJB5SdKPTvN3H/Iu0Aw+Rtb6
szwNClaFAgMBAAECggEAHZ8cjVRUJ/tiJorzlTyfKZ6hwhsPv4JIRVg6LhnceZWA
jPW2cHSWyl2epyx55lhH7iyeeY7vXOqrX1aBMDb1stSWw2dH/tdxYSkqEmksa+R6
fL6kl5RV5epjpPt77Z3VmPq9UbP/M310qKWcgB8lw4wN0AfKMqsZLYauk9BVhNRu
Bgah9O7BmcXS+mp49w0Xyfo1UBvzW8R6UnBhHbf9aOY8ObMD0Jj/wDjlYMqSSIKR
9/8GZWQEKe6q0PyRRdNNtdzbpBrR0fIw6/T9pfDR2fBAcpNvD50eJk2jRiRDTWFJ
rVSc0bvZFb74Rc3LbMSXW/6Kb7I2IG1XsWw7nxp92QKBgQDgzdIxZrkNZ3Tbuzng
SG4atjnaCXoekOHK7VZVYd30S0AAizeGu1sjpUVQgsf+qkFskXAQp2/2f+Wiuq2G
+nJYvXwZ/r9IcUs/oD3Fa2ezCVz1N/HOSPFAZK9XZuZbL8sXEYIPGJWH5F8Sanmb
xNp9IUynlpwgM2JlZNeTCkv4PQKBgQDMbL/AF3LSpKvwi+QvYVkX/gChQmNMr4pP
TM/GI4D03tNrzsut3oerKMUw0c5MxonkAJpuACN6baRyBOBxRYQSt8wWkORg9iqy
a7aHnQqIGRafydW1/Snhr2DJSSaViHfO0oaA1r61zgMUTnSGb3UjyxJQp65dvPac
BhpR9wpz6QKBgQDR2S/CL8rEqXObfi1roREu3DYqw7f8enBb1qtFrsLbPbd0CoD9
wz0zjB6lJj/9CP9jkmwTD8njR8ab3jkIDBfboJ4NQhFbVW7R6QpglH9L0Iy2189g
KhUScCqBoyubqYSidxR6dQ94uATLkxsL/nmaXxBITL5XDMBoN/dIak86XQKBgDqa
oo4LKtvAYZpgQFZk7gm2w693PMhrOpdpSddfrkSE7M9nRXTe6r3ivkU0oJPaBwXa
Nmt6lrEuZYpaY42VhDtpfZSqjQ5PBAaKYpWWK8LAjn/YeO/nV+5fPLv3wJv1t4MP
T4f4CExOdwuHQliX81kDioicyZwN5BTumvUMgW6hAoGAF29kI1KthKaHN9P1DchI
qqoHb9FPdZ5I6HDQpn6fr9ut7+9kVqexUrQ2AMvcVei6gDWW6P3yDCdTKcV9qtts
1JOP2aSmXvibflx/bNfnhu988qJDhJ3CCjfc79fjwntUIXNPsFmwC9W5lnlSMKHM
rH4RdmnjeCIG1PZ35m/yUSU=
-----END PRIVATE KEY-----'''

if not os.path.exists(EXPECTED_TS_MON_CONFIG_NAME):
    try:
        os.makedirs(os.path.dirname(EXPECTED_TS_MON_CONFIG_NAME))
    except OSError:
        # Directory already exists.
        pass

    with open(EXPECTED_TS_MON_CONFIG_NAME, 'w') as f:
        f.write(FAKE_TS_MON_CONFIG_CONTENT)
    with open ('/tmp/service_account_prodx_mon.json', 'w') as f:
        f.write(FAKE_SERVICE_ACCOUNT_CRED_JSON % repr(TEST_KEY)[2:-1])
"""

# Name of the test control file.
TEST_CONTROL_FILE = 'attach.1'
TEST_DUT = '172.27.213.193'
TEST_RESULT_PATH = lxc.RESULT_DIR_FMT % TEST_JOB_FOLDER
# Test autoserv command.
AUTOSERV_COMMAND = (('/usr/bin/python -u /usr/local/autotest/server/autoserv '
                     '-p -r %(result_path)s/%(test_dut)s -m %(test_dut)s '
                     '-u debug_user -l test -s -P %(job_id)s-debug_user/'
                     '%(test_dut)s -n %(result_path)s/%(test_control_file)s '
                     '--verify_job_repo_url') %
                     {'job_id': TEST_JOB_ID,
                      'result_path': TEST_RESULT_PATH,
                      'test_dut': TEST_DUT,
                      'test_control_file': TEST_CONTROL_FILE})
# Content of the test control file.
TEST_CONTROL_CONTENT = """
def run(machine):
    job.run_test('dummy_PassServer',
                 host=hosts.create_host(machine))

parallel_simple(run, machines)
"""


def setup_base(container_path):
    """Test setup base container works.

    @param bucket: ContainerBucket to interact with containers.
    """
    logging.info('Rebuild base container in folder %s.', container_path)
    image = lxc.BaseImage(container_path)
    image.setup()
    logging.info('Base container created: %s', image.get().name)


def setup_test(bucket, container_id, skip_cleanup):
    """Test container can be created from base container.

    @param bucket: ContainerBucket to interact with containers.
    @param container_id: ID of the test container.
    @param skip_cleanup: Set to True to skip cleanup, used to troubleshoot
                         container failures.

    @return: A Container object created for the test container.
    """
    logging.info('Create test container.')
    os.makedirs(RESULT_PATH)
    container = bucket.setup_test(container_id, TEST_JOB_ID,
                                  AUTOTEST_SERVER_PKG, RESULT_PATH,
                                  skip_cleanup=skip_cleanup,
                                  job_folder=TEST_JOB_FOLDER,
                                  dut_name='192.168.0.3')

    # Inject "AUTOSERV/testing_mode: True" in shadow config to test autoserv.
    container.attach_run('echo $\'[AUTOSERV]\ntesting_mode: True\' >>'
                         ' /usr/local/autotest/shadow_config.ini')

    if not utils.is_moblab():
        # Create fake '/etc/chrome-infra/ts-mon.json' if it doesn't exist.
        create_key_script = os.path.join(
                RESULT_PATH, CREATE_FAKE_TS_MON_CONFIG_SCRIPT)
        with open(create_key_script, 'w') as script:
            script.write(CREATE_FAKE_TS_MON_CONFIG_SCRIPT_CONTENT)
        container_result_path = lxc.RESULT_DIR_FMT % TEST_JOB_FOLDER
        container_create_key_script = os.path.join(
                container_result_path, CREATE_FAKE_TS_MON_CONFIG_SCRIPT)
        container.attach_run('python %s' % container_create_key_script)

    return container


def test_share(container):
    """Test container can share files with the host.

    @param container: The test container.
    """
    logging.info('Test files written to result directory can be accessed '
                 'from the host running the container..')
    host_test_script = os.path.join(RESULT_PATH, TEST_SCRIPT)
    with open(host_test_script, 'w') as script:
        if utils.is_moblab():
            script.write(TEST_SCRIPT_CONTENT % {'ts_mon_test': ''})
        else:
            script.write(TEST_SCRIPT_CONTENT %
                         {'ts_mon_test': TEST_SCRIPT_CONTENT_TS_MON})

    container_result_path = lxc.RESULT_DIR_FMT % TEST_JOB_FOLDER
    container_test_script = os.path.join(container_result_path, TEST_SCRIPT)
    container_test_script_dest = os.path.join('/usr/local/autotest/utils/',
                                              TEST_SCRIPT)
    container_test_log = os.path.join(container_result_path, TEST_LOG)
    host_test_log = os.path.join(RESULT_PATH, TEST_LOG)
    # Move the test script out of result folder as it needs to import common.
    container.attach_run('mv %s %s' % (container_test_script,
                                       container_test_script_dest))
    container.attach_run('python %s %s' % (container_test_script_dest,
                                           container_test_log))
    if not os.path.exists(host_test_log):
        raise Exception('Results created in container can not be accessed from '
                        'the host.')
    with open(host_test_log, 'r') as log:
        if log.read() != 'test':
            raise Exception('Failed to read the content of results in '
                            'container.')


def test_autoserv(container):
    """Test container can run autoserv command.

    @param container: The test container.
    """
    logging.info('Test autoserv command.')
    logging.info('Create test control file.')
    host_control_file = os.path.join(RESULT_PATH, TEST_CONTROL_FILE)
    with open(host_control_file, 'w') as control_file:
        control_file.write(TEST_CONTROL_CONTENT)

    logging.info('Run autoserv command.')
    container.attach_run(AUTOSERV_COMMAND)

    logging.info('Confirm results are available from host.')
    # Read status.log to check the content is not empty.
    container_status_log = os.path.join(TEST_RESULT_PATH, TEST_DUT,
                                        'status.log')
    status_log = container.attach_run(command='cat %s' % container_status_log
                                      ).stdout
    if len(status_log) < 10:
        raise Exception('Failed to read status.log in container.')


def test_package_install(container):
    """Test installing package in container.

    @param container: The test container.
    """
    # Packages are installed in TEST_SCRIPT_CONTENT. Verify the packages in
    # this method.
    container.attach_run('which atop')
    container.attach_run('python -c "import acora"')


def test_ssh(container, remote):
    """Test container can run ssh to remote server.

    @param container: The test container.
    @param remote: The remote server to ssh to.

    @raise: error.CmdError if container can't ssh to remote server.
    """
    logging.info('Test ssh to %s.', remote)
    container.attach_run('ssh %s -a -x -o StrictHostKeyChecking=no '
                         '-o BatchMode=yes -o UserKnownHostsFile=/dev/null '
                         '-p 22 "true"' % remote)


def parse_options():
    """Parse command line inputs.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--dut', type=str,
                        help='Test device to ssh to.',
                        default=None)
    parser.add_argument('-r', '--devserver', type=str,
                        help='Test devserver to ssh to.',
                        default=None)
    parser.add_argument('-v', '--verbose', action='store_true',
                        default=False,
                        help='Print out ALL entries.')
    parser.add_argument('-s', '--skip_cleanup', action='store_true',
                        default=False,
                        help='Skip deleting test containers.')
    return parser.parse_args()


def main(options):
    """main script.

    @param options: Options to run the script.
    """
    # Verify that the test is running as the correct user.
    unittest_setup.verify_user()

    log_level=(logging.DEBUG if options.verbose else logging.INFO)
    unittest_setup.setup_logging(log_level)

    setup_base(TEMP_DIR)
    bucket = lxc.ContainerBucket(TEMP_DIR)

    container_id = lxc.ContainerId.create(TEST_JOB_ID)
    container = setup_test(bucket, container_id, options.skip_cleanup)
    test_share(container)
    test_autoserv(container)
    if options.dut:
        test_ssh(container, options.dut)
    if options.devserver:
        test_ssh(container, options.devserver)
    # Packages are installed in TEST_SCRIPT, verify the packages are installed.
    test_package_install(container)
    logging.info('All tests passed.')


if __name__ == '__main__':
    options = parse_options()
    try:
        main(options)
    except:
        # If the cleanup code below raises additional errors, they obfuscate the
        # actual error in the test.  Highlight the error to aid in debugging.
        logging.exception('ERROR:\n%s', error.format_error())
        raise
    finally:
        if not options.skip_cleanup:
            logging.info('Cleaning up temporary directory %s.', TEMP_DIR)
            try:
                lxc.ContainerBucket(TEMP_DIR).destroy_all()
            finally:
                utils.run('sudo rm -rf "%s"' % TEMP_DIR)
