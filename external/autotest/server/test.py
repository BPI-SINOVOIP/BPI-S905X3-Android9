# Copyright Martin J. Bligh, Andy Whitcroft, 2007
#
# Define the server-side test class
#
# pylint: disable=missing-docstring

import logging
import os
import tempfile

from autotest_lib.client.common_lib import log
from autotest_lib.client.common_lib import test as common_test
from autotest_lib.client.common_lib import utils


class test(common_test.base_test):
    disable_sysinfo_install_cache = False
    host_parameter = None


_sysinfo_before_test_script = """\
import pickle
from autotest_lib.client.bin import test
mytest = test.test(job, '', %r)
job.sysinfo.log_before_each_test(mytest)
sysinfo_pickle = os.path.join(mytest.outputdir, 'sysinfo.pickle')
pickle.dump(job.sysinfo, open(sysinfo_pickle, 'w'))
job.record('GOOD', '', 'sysinfo.before')
"""

_sysinfo_after_test_script = """\
import pickle
from autotest_lib.client.bin import test
mytest = test.test(job, '', %r)
# success is passed in so diffable_logdir can decide if or not to collect
# full log content.
mytest.success = %s
sysinfo_pickle = os.path.join(mytest.outputdir, 'sysinfo.pickle')
if os.path.exists(sysinfo_pickle):
    job.sysinfo = pickle.load(open(sysinfo_pickle))
    job.sysinfo.__init__(job.resultdir)
job.sysinfo.log_after_each_test(mytest)
job.record('GOOD', '', 'sysinfo.after')
"""

# this script is ran after _sysinfo_before_test_script and before
# _sysinfo_after_test_script which means the pickle file exists
# already and should be dumped with updated state for
# _sysinfo_after_test_script to pick it up later
_sysinfo_iteration_script = """\
import pickle
from autotest_lib.client.bin import test
mytest = test.test(job, '', %r)
sysinfo_pickle = os.path.join(mytest.outputdir, 'sysinfo.pickle')
if os.path.exists(sysinfo_pickle):
    job.sysinfo = pickle.load(open(sysinfo_pickle))
    job.sysinfo.__init__(job.resultdir)
job.sysinfo.%s(mytest, iteration=%d)
pickle.dump(job.sysinfo, open(sysinfo_pickle, 'w'))
job.record('GOOD', '', 'sysinfo.iteration.%s')
"""


def install_autotest_and_run(func):
    def wrapper(self, mytest):
        host, at, outputdir = self._install()
        # TODO(kevcheng): remove when host client install is supported for
        # ADBHost. crbug.com/543702
        if not host.is_client_install_supported:
            logging.debug('host client install not supported, skipping %s:',
                          func.__name__)
            return

        try:
            host.erase_dir_contents(outputdir)
            func(self, mytest, host, at, outputdir)
        finally:
            # the test class can define this flag to make us remove the
            # sysinfo install files and outputdir contents after each run
            if mytest.disable_sysinfo_install_cache:
                self.cleanup(host_close=False)

    return wrapper


class _sysinfo_logger(object):
    def __init__(self, job):
        self.job = job
        self.pickle = None

        # for now support a single host
        self.host = None
        self.autotest = None
        self.outputdir = None

        if len(job.machines) != 1:
            # disable logging on multi-machine tests
            self.before_hook = self.after_hook = None
            self.before_iteration_hook = self.after_iteration_hook = None


    def _install(self):
        if not self.host:
            from autotest_lib.server import hosts, autotest
            self.host = hosts.create_target_machine(
                    self.job.machine_dict_list[0])
            # TODO(kevcheng): remove when host client install is supported for
            # ADBHost. crbug.com/543702
            if not self.host.is_client_install_supported:
                return self.host, None, None
            try:
                tmp_dir = self.host.get_tmp_dir(parent="/tmp/sysinfo")
                self.autotest = autotest.Autotest(self.host)
                self.autotest.install(autodir=tmp_dir)
                self.outputdir = self.host.get_tmp_dir()
            except:
                # if installation fails roll back the host
                try:
                    self.host.close()
                except:
                    logging.exception("Unable to close host %s",
                                      self.host.hostname)
                self.host = None
                self.autotest = None
                raise
        else:
            # TODO(kevcheng): remove when host client install is supported for
            # ADBHost. crbug.com/543702
            if not self.host.is_client_install_supported:
                return self.host, None, None

            # if autotest client dir does not exist, reinstall (it may have
            # been removed by the test code)
            autodir = self.host.get_autodir()
            if not autodir or not self.host.path_exists(autodir):
                self.autotest.install(autodir=autodir)

            # if the output dir does not exist, recreate it
            if not self.host.path_exists(self.outputdir):
                self.host.run('mkdir -p %s' % self.outputdir)

        return self.host, self.autotest, self.outputdir


    def _pull_pickle(self, host, outputdir):
        """Pulls from the client the pickle file with the saved sysinfo state.
        """
        fd, path = tempfile.mkstemp(dir=self.job.tmpdir)
        os.close(fd)
        host.get_file(os.path.join(outputdir, "sysinfo.pickle"), path)
        self.pickle = path


    def _push_pickle(self, host, outputdir):
        """Pushes the server saved sysinfo pickle file to the client.
        """
        if self.pickle:
            host.send_file(self.pickle,
                           os.path.join(outputdir, "sysinfo.pickle"))
            os.remove(self.pickle)
            self.pickle = None


    def _pull_sysinfo_keyval(self, host, outputdir, mytest):
        """Pulls sysinfo and keyval data from the client.
        """
        # pull the sysinfo data back on to the server
        host.get_file(os.path.join(outputdir, "sysinfo"), mytest.outputdir)

        # pull the keyval data back into the local one
        fd, path = tempfile.mkstemp(dir=self.job.tmpdir)
        os.close(fd)
        host.get_file(os.path.join(outputdir, "keyval"), path)
        keyval = utils.read_keyval(path)
        os.remove(path)
        mytest.write_test_keyval(keyval)


    @log.log_and_ignore_errors("pre-test server sysinfo error:")
    @install_autotest_and_run
    def before_hook(self, mytest, host, at, outputdir):
        # run the pre-test sysinfo script
        at.run(_sysinfo_before_test_script % outputdir,
               results_dir=self.job.resultdir)

        self._pull_pickle(host, outputdir)


    @log.log_and_ignore_errors("pre-test iteration server sysinfo error:")
    @install_autotest_and_run
    def before_iteration_hook(self, mytest, host, at, outputdir):
        # this function is called after before_hook() se we have sysinfo state
        # to push to the server
        self._push_pickle(host, outputdir);
        # run the pre-test iteration sysinfo script
        at.run(_sysinfo_iteration_script %
               (outputdir, 'log_before_each_iteration', mytest.iteration,
                'before'),
               results_dir=self.job.resultdir)

        # get the new sysinfo state from the client
        self._pull_pickle(host, outputdir)


    @log.log_and_ignore_errors("post-test iteration server sysinfo error:")
    @install_autotest_and_run
    def after_iteration_hook(self, mytest, host, at, outputdir):
        # push latest sysinfo state to the client
        self._push_pickle(host, outputdir);
        # run the post-test iteration sysinfo script
        at.run(_sysinfo_iteration_script %
               (outputdir, 'log_after_each_iteration', mytest.iteration,
                'after'),
               results_dir=self.job.resultdir)

        # get the new sysinfo state from the client
        self._pull_pickle(host, outputdir)


    @log.log_and_ignore_errors("post-test server sysinfo error:")
    @install_autotest_and_run
    def after_hook(self, mytest, host, at, outputdir):
        self._push_pickle(host, outputdir);
        # run the post-test sysinfo script
        at.run(_sysinfo_after_test_script % (outputdir, mytest.success),
               results_dir=self.job.resultdir)

        self._pull_sysinfo_keyval(host, outputdir, mytest)


    def cleanup(self, host_close=True):
        if self.host and self.autotest:
            try:
                try:
                    self.autotest.uninstall()
                finally:
                    if host_close:
                        self.host.close()
                    else:
                        self.host.erase_dir_contents(self.outputdir)

            except Exception:
                # ignoring exceptions here so that we don't hide the true
                # reason of failure from runtest
                logging.exception('Error cleaning up the sysinfo autotest/host '
                                  'objects, ignoring it')


def runtest(job, url, tag, args, dargs):
    """Server-side runtest.

    @param job: A server_job instance.
    @param url: URL to the test.
    @param tag: Test tag that will be appended to the test name.
                See client/common_lib/test.py:runtest
    @param args: args to pass to the test.
    @param dargs: key-val based args to pass to the test.
    """

    disable_before_test_hook = dargs.pop('disable_before_test_sysinfo', False)
    disable_after_test_hook = dargs.pop('disable_after_test_sysinfo', False)
    disable_before_iteration_hook = dargs.pop(
            'disable_before_iteration_sysinfo', False)
    disable_after_iteration_hook = dargs.pop(
            'disable_after_iteration_sysinfo', False)

    disable_sysinfo = dargs.pop('disable_sysinfo', False)
    if job.fast and not disable_sysinfo:
        # Server job will be executed in fast mode, which means
        # 1) if job succeeds, no hook will be executed.
        # 2) if job failed, after_hook will be executed.
        logger = _sysinfo_logger(job)
        logging_args = [None, logger.after_hook, None,
                        logger.after_iteration_hook]
    elif not disable_sysinfo:
        logger = _sysinfo_logger(job)
        logging_args = [
            logger.before_hook if not disable_before_test_hook else None,
            logger.after_hook if not disable_after_test_hook else None,
            (logger.before_iteration_hook
                 if not disable_before_iteration_hook else None),
            (logger.after_iteration_hook
                 if not disable_after_iteration_hook else None),
        ]
    else:
        logger = None
        logging_args = [None, None, None, None]

    # add in a hook that calls host.log_kernel if we can
    def log_kernel_hook(mytest, existing_hook=logging_args[0]):
        if mytest.host_parameter:
            host = dargs[mytest.host_parameter]
            if host:
                host.log_kernel()
        # chain this call with any existing hook
        if existing_hook:
            existing_hook(mytest)
    logging_args[0] = log_kernel_hook

    try:
        common_test.runtest(job, url, tag, args, dargs, locals(), globals(),
                            *logging_args)
    finally:
        if logger:
            logger.cleanup()
