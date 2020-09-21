#!/usr/bin/python
#pylint: disable-msg=C0111
__author__ = "raphtee@google.com (Travis Miller)"

import unittest, os, tempfile, logging

import common
from autotest_lib.server import autotest, utils, hosts, server_job, profilers
from autotest_lib.client.bin import sysinfo
from autotest_lib.client.common_lib import packages
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.test_utils import mock


class TestAutotest(unittest.TestCase):
    def setUp(self):
        # create god
        self.god = mock.mock_god()

        # create mock host object
        self.host = self.god.create_mock_class(hosts.RemoteHost, "host")
        self.host.hostname = "hostname"
        self.host.job = self.god.create_mock_class(server_job.server_job,
                                                   "job")
        self.host.job.run_test_cleanup = True
        self.host.job.sysinfo = self.god.create_mock_class(
            sysinfo.sysinfo, "sysinfo")
        self.host.job.profilers = self.god.create_mock_class(
            profilers.profilers, "profilers")
        self.host.job.profilers.add_log = {}
        self.host.job.tmpdir = "/job/tmp"
        self.host.job.default_profile_only = False
        self.host.job.args = []
        self.host.job.record = lambda *args: None

        # stubs
        self.god.stub_function(utils, "get_server_dir")
        self.god.stub_function(utils, "run")
        self.god.stub_function(utils, "get")
        self.god.stub_function(utils, "read_keyval")
        self.god.stub_function(utils, "write_keyval")
        self.god.stub_function(utils, "system")
        self.god.stub_function(tempfile, "mkstemp")
        self.god.stub_function(tempfile, "mktemp")
        self.god.stub_function(os, "getcwd")
        self.god.stub_function(os, "system")
        self.god.stub_function(os, "chdir")
        self.god.stub_function(os, "makedirs")
        self.god.stub_function(os, "remove")
        self.god.stub_function(os, "fdopen")
        self.god.stub_function(os.path, "exists")
        self.god.stub_function(autotest, "open")
        self.god.stub_function(autotest.global_config.global_config,
                               "get_config_value")
        self.god.stub_function(logging, "exception")
        self.god.stub_class(autotest, "_Run")
        self.god.stub_class(autotest, "log_collector")


    def tearDown(self):
        self.god.unstub_all()


    def construct(self):
        # setup
        self.serverdir = "serverdir"

        # record
        utils.get_server_dir.expect_call().and_return(self.serverdir)

        # create the autotest object
        self.autotest = autotest.Autotest(self.host)
        self.autotest.job = self.host.job
        self.god.stub_function(self.autotest, "_install_using_send_file")

        # stub out abspath
        self.god.stub_function(os.path, "abspath")

        # check
        self.god.check_playback()


    def record_install_prologue(self):
        self.construct()

        # setup
        self.god.stub_class(packages, "PackageManager")
        self.autotest.got = False
        location = os.path.join(self.serverdir, '../client')
        location = os.path.abspath.expect_call(location).and_return(location)

        # record
        utils.get.expect_call(os.path.join(self.serverdir,
            '../client')).and_return('source_material')

        self.host.wait_up.expect_call(timeout=30)
        self.host.setup.expect_call()
        self.host.get_autodir.expect_call().and_return("autodir")
        self.host.set_autodir.expect_call("autodir")
        self.host.run.expect_call('mkdir -p autodir')
        self.host.run.expect_call('rm -rf autodir/results/*',
                                  ignore_status=True)


    def test_constructor(self):
        self.construct()

        # we should check the calls
        self.god.check_playback()


    def test_full_client_install(self):
        self.record_install_prologue()

        self.host.run.expect_call('rm -f "autodir/packages.checksum"')
        c = autotest.global_config.global_config
        c.get_config_value.expect_call('PACKAGES',
                                       'serve_packages_from_autoserv',
                                       type=bool).and_return(False)
        self.host.send_file.expect_call('source_material', 'autodir',
                                        delete_dest=True)
        self.god.stub_function(autotest.Autotest, "_send_shadow_config")
        autotest.Autotest._send_shadow_config.expect_call()

        # run and check
        self.autotest.install_full_client()
        self.god.check_playback()


    def test_autoserv_install(self):
        self.record_install_prologue()

        c = autotest.global_config.global_config
        c.get_config_value.expect_call('PACKAGES',
            'fetch_location', type=list, default=[]).and_return([])

        os.path.exists.expect_call('/etc/cros_chroot_version').and_return(True)
        c.get_config_value.expect_call('PACKAGES',
                                       'serve_packages_from_autoserv',
                                       type=bool).and_return(True)
        self.autotest._install_using_send_file.expect_call(self.host,
                                                           'autodir')
        self.god.stub_function(autotest.Autotest, "_send_shadow_config")
        autotest.Autotest._send_shadow_config.expect_call()
        # run and check
        self.autotest.install()
        self.god.check_playback()


    def test_packaging_install(self):
        self.record_install_prologue()

        c = autotest.global_config.global_config
        c.get_config_value.expect_call('PACKAGES',
            'fetch_location', type=list, default=[]).and_return(['repo'])
        os.path.exists.expect_call('/etc/cros_chroot_version').and_return(True)
        pkgmgr = packages.PackageManager.expect_new('autodir',
            repo_urls=['repo'], hostname='hostname', do_locking=False,
            run_function=self.host.run, run_function_dargs=dict(timeout=600))
        pkg_dir = os.path.join('autodir', 'packages')
        cmd = ('cd autodir && ls | grep -v "^packages$" | '
               'grep -v "^result_tools$" | '
               'xargs rm -rf && rm -rf .[!.]*')
        self.host.run.expect_call(cmd)
        pkgmgr.install_pkg.expect_call('autotest', 'client', pkg_dir,
                                       'autodir', preserve_install_dir=True)

        # run and check
        self.autotest.install()
        self.god.check_playback()


    def test_run(self):
        self.construct()

        # setup
        control = "control"

        # stub out install
        self.god.stub_function(self.autotest, "install")

        # record
        self.autotest.install.expect_call(self.host, use_packaging=True)
        self.host.wait_up.expect_call(timeout=30)
        os.path.abspath.expect_call('.').and_return('.')
        run_obj = autotest._Run.expect_new(self.host, '.', None, False, False)
        tag = None
        run_obj.manual_control_file = os.path.join('autodir',
                                                   'control.%s' % tag)
        run_obj.remote_control_file = os.path.join('autodir',
                                                   'control.%s.autoserv' % tag)
        run_obj.tag = tag
        run_obj.autodir = 'autodir'
        run_obj.verify_machine.expect_call()
        run_obj.background = False
        debug = os.path.join('.', 'debug')
        os.makedirs.expect_call(debug)
        delete_file_list = [run_obj.remote_control_file,
                            run_obj.remote_control_file + '.state',
                            run_obj.manual_control_file,
                            run_obj.manual_control_file + '.state']
        cmd = ';'.join('rm -f ' + control for control in delete_file_list)
        self.host.run.expect_call(cmd, ignore_status=True)

        utils.get.expect_call(control, local_copy=True).and_return("temp")

        c = autotest.global_config.global_config
        c.get_config_value.expect_call("PACKAGES",
            'fetch_location', type=list, default=[]).and_return(['repo'])

        cfile = self.god.create_mock_class(file, "file")
        cfile_orig = "original control file"
        cfile_new = "args = []\njob.add_repository(['repo'])\n"
        cfile_new += cfile_orig

        os.path.exists.expect_call('/etc/cros_chroot_version').and_return(True)
        autotest.open.expect_call("temp").and_return(cfile)
        cfile.read.expect_call().and_return(cfile_orig)
        autotest.open.expect_call("temp", 'w').and_return(cfile)
        cfile.write.expect_call(cfile_new)

        self.host.job.preprocess_client_state.expect_call().and_return(
            '/job/tmp/file1')
        self.host.send_file.expect_call(
            "/job/tmp/file1", "autodir/control.None.autoserv.init.state")
        os.remove.expect_call("/job/tmp/file1")

        self.host.send_file.expect_call("temp", run_obj.remote_control_file)
        os.path.abspath.expect_call('temp').and_return('control_file')
        os.path.abspath.expect_call('control').and_return('control')
        os.remove.expect_call("temp")

        run_obj.execute_control.expect_call(timeout=30,
                                            client_disconnect_timeout=240)

        # run and check output
        self.autotest.run(control, timeout=30)
        self.god.check_playback()


    def _stub_get_client_autodir_paths(self):
        def mock_get_client_autodir_paths(cls, host):
            return ['/some/path', '/another/path']
        self.god.stub_with(autotest.Autotest, 'get_client_autodir_paths',
                           classmethod(mock_get_client_autodir_paths))


    def _expect_failed_run(self, command):
        (self.host.run.expect_call(command)
         .and_raises(error.AutoservRunError('dummy', object())))


    def test_get_installed_autodir(self):
        self._stub_get_client_autodir_paths()
        self.host.get_autodir.expect_call().and_return(None)
        self._expect_failed_run('test -x /some/path/bin/autotest')
        self.host.run.expect_call('test -x /another/path/bin/autotest')
        self.host.run.expect_call('test -w /another/path')

        autodir = autotest.Autotest.get_installed_autodir(self.host)
        self.assertEquals(autodir, '/another/path')


    def test_get_install_dir(self):
        self._stub_get_client_autodir_paths()
        self.host.get_autodir.expect_call().and_return(None)
        self._expect_failed_run('test -x /some/path/bin/autotest')
        self._expect_failed_run('test -x /another/path/bin/autotest')
        self._expect_failed_run('mkdir -p /some/path')
        self.host.run.expect_call('mkdir -p /another/path')
        self.host.run.expect_call('test -w /another/path')

        install_dir = autotest.Autotest.get_install_dir(self.host)
        self.assertEquals(install_dir, '/another/path')


    def test_client_logger_process_line_log_copy_collection_failure(self):
        collector = autotest.log_collector.expect_new(self.host, '', '')
        logger = autotest.client_logger(self.host, '', '')
        collector.collect_client_job_results.expect_call().and_raises(
                Exception('log copy failure'))
        logging.exception.expect_call(mock.is_string_comparator())
        logger._process_line('AUTOTEST_TEST_COMPLETE:/autotest/fifo1')


    def test_client_logger_process_line_log_copy_fifo_failure(self):
        collector = autotest.log_collector.expect_new(self.host, '', '')
        logger = autotest.client_logger(self.host, '', '')
        collector.collect_client_job_results.expect_call()
        self.host.run.expect_call('echo A > /autotest/fifo2').and_raises(
                Exception('fifo failure'))
        logging.exception.expect_call(mock.is_string_comparator())
        logger._process_line('AUTOTEST_TEST_COMPLETE:/autotest/fifo2')


    def test_client_logger_process_line_package_install_fifo_failure(self):
        collector = autotest.log_collector.expect_new(self.host, '', '')
        logger = autotest.client_logger(self.host, '', '')
        self.god.stub_function(logger, '_send_tarball')

        c = autotest.global_config.global_config
        c.get_config_value.expect_call('PACKAGES',
                                       'serve_packages_from_autoserv',
                                       type=bool).and_return(True)
        c.get_config_value.expect_call('PACKAGES',
                                       'serve_packages_from_autoserv',
                                       type=bool).and_return(True)
        logger._send_tarball.expect_call('pkgname.tar.bz2', '/autotest/dest/')

        self.host.run.expect_call('echo B > /autotest/fifo3').and_raises(
                Exception('fifo failure'))
        logging.exception.expect_call(mock.is_string_comparator())
        logger._process_line('AUTOTEST_FETCH_PACKAGE:pkgname.tar.bz2:'
                             '/autotest/dest/:/autotest/fifo3')


if __name__ == "__main__":
    unittest.main()
