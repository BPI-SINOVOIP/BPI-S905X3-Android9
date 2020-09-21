"""This class defines the Remote host class."""

import os, logging, urllib, time
from autotest_lib.client.common_lib import error
from autotest_lib.server import utils
from autotest_lib.server.hosts import base_classes


class RemoteHost(base_classes.Host):
    """
    This class represents a remote machine on which you can run
    programs.

    It may be accessed through a network, a serial line, ...
    It is not the machine autoserv is running on.

    Implementation details:
    This is an abstract class, leaf subclasses must implement the methods
    listed here and in parent classes which have no implementation. They
    may reimplement methods which already have an implementation. You
    must not instantiate this class but should instantiate one of those
    leaf subclasses.
    """

    DEFAULT_REBOOT_TIMEOUT = base_classes.Host.DEFAULT_REBOOT_TIMEOUT
    DEFAULT_HALT_TIMEOUT = 2 * 60
    _LABEL_FUNCTIONS = []
    _DETECTABLE_LABELS = []

    VAR_LOG_MESSAGES_COPY_PATH = "/var/tmp/messages.autotest_start"


    def _initialize(self, hostname, autodir=None, *args, **dargs):
        super(RemoteHost, self)._initialize(*args, **dargs)

        self.hostname = hostname
        self.autodir = autodir
        self.tmp_dirs = []


    def __repr__(self):
        return "<remote host: %s>" % self.hostname


    def close(self):
        super(RemoteHost, self).close()
        self.stop_loggers()

        if hasattr(self, 'tmp_dirs'):
            for dir in self.tmp_dirs:
                try:
                    self.run('rm -rf "%s"' % (utils.sh_escape(dir)))
                except error.AutoservRunError:
                    pass


    def job_start(self):
        """
        Abstract method, called the first time a remote host object
        is created for a specific host after a job starts.

        This method depends on the create_host factory being used to
        construct your host object. If you directly construct host objects
        you will need to call this method yourself (and enforce the
        single-call rule).
        """
        try:
            cmd = ('test ! -e /var/log/messages || cp -f /var/log/messages '
                   '%s') % self.VAR_LOG_MESSAGES_COPY_PATH
            self.run(cmd)
        except Exception, e:
            # Non-fatal error
            logging.info('Failed to copy /var/log/messages at startup: %s', e)


    def get_autodir(self):
        return self.autodir


    def set_autodir(self, autodir):
        """
        This method is called to make the host object aware of the
        where autotest is installed. Called in server/autotest.py
        after a successful install
        """
        self.autodir = autodir


    def sysrq_reboot(self):
        self.run_background('echo b > /proc/sysrq-trigger')


    def halt(self, timeout=DEFAULT_HALT_TIMEOUT, wait=True):
        """
        Shut down the remote host.

        N.B.  This method makes no provision to bring the target back
        up.  The target will be offline indefinitely if there's no
        independent hardware (servo, RPM, etc.) to force the target to
        power on.

        @param timeout  Maximum time to wait for host down, in seconds.
        @param wait  Whether to wait for the host to go offline.
        """
        self.run_background('sleep 1 ; halt')
        if wait:
            self.wait_down(timeout=timeout)


    def reboot(self, timeout=DEFAULT_REBOOT_TIMEOUT, wait=True,
               fastsync=False, reboot_cmd=None, **dargs):
        """
        Reboot the remote host.

        Args:
                timeout - How long to wait for the reboot.
                wait - Should we wait to see if the machine comes back up.
                       If this is set to True, ignores reboot_cmd's error
                       even if occurs.
                fastsync - Don't wait for the sync to complete, just start one
                        and move on. This is for cases where rebooting prompty
                        is more important than data integrity and/or the
                        machine may have disks that cause sync to never return.
                reboot_cmd - Reboot command to execute.
        """
        self.reboot_setup(**dargs)
        if not reboot_cmd:
            reboot_cmd = ('sync & sleep 5; '
                          'reboot & sleep 60; '
                          'reboot -f & sleep 10; '
                          'reboot -nf & sleep 10; '
                          'telinit 6')

        def reboot():
            # pylint: disable=missing-docstring
            self.record("GOOD", None, "reboot.start")
            try:
                current_boot_id = self.get_boot_id()

                # sync before starting the reboot, so that a long sync during
                # shutdown isn't timed out by wait_down's short timeout
                if not fastsync:
                    self.run('sync; sync', timeout=timeout, ignore_status=True)

                self.run_background(reboot_cmd)
            except error.AutoservRunError:
                # If wait is set, ignore the error here, and rely on the
                # wait_for_restart() for stability, instead.
                # reboot_cmd sometimes causes an error even if reboot is
                # successfully in progress. This is difficult to be avoided,
                # because we have no much control on remote machine after
                # "reboot" starts.
                if not wait:
                    # TODO(b/37652392): Revisit no-wait case, later.
                    self.record("ABORT", None, "reboot.start",
                                "reboot command failed")
                    raise
            if wait:
                self.wait_for_restart(timeout, old_boot_id=current_boot_id,
                                      **dargs)

        # if this is a full reboot-and-wait, run the reboot inside a group
        if wait:
            self.log_op(self.OP_REBOOT, reboot)
        else:
            reboot()

    def suspend(self, timeout, suspend_cmd, **dargs):
        """
        Suspend the remote host.

        Args:
                timeout - How long to wait for the suspend.
                susped_cmd - suspend command to execute.
        """
        # define a function for the supend and run it in a group
        def suspend():
            # pylint: disable=missing-docstring
            self.record("GOOD", None, "suspend.start for %d seconds" % (timeout))
            try:
                self.run_background(suspend_cmd)
            except error.AutoservRunError:
                self.record("ABORT", None, "suspend.start",
                            "suspend command failed")
                raise error.AutoservSuspendError("suspend command failed")

            # Wait for some time, to ensure the machine is going to sleep.
            # Not too long to check if the machine really suspended.
            time_slice = min(timeout / 2, 300)
            time.sleep(time_slice)
            time_counter = time_slice
            while time_counter < timeout + 60:
                # Check if the machine is back. We check regularely to
                # ensure the machine was suspended long enough.
                if utils.ping(self.hostname, tries=1, deadline=1) == 0:
                    return
                else:
                    if time_counter > timeout - 10:
                        time_slice = 5
                    time.sleep(time_slice)
                    time_counter += time_slice

            if utils.ping(self.hostname, tries=1, deadline=1) != 0:
                raise error.AutoservSuspendError(
                    "DUT is not responding after %d seconds" % (time_counter))

        start_time = time.time()
        self.log_op(self.OP_SUSPEND, suspend)
        lasted = time.time() - start_time
        if (lasted < timeout):
            raise error.AutoservSuspendError(
                "Suspend did not last long enough: %d instead of %d" % (
                    lasted, timeout))

    def reboot_followup(self, *args, **dargs):
        super(RemoteHost, self).reboot_followup(*args, **dargs)
        if self.job:
            self.job.profilers.handle_reboot(self)


    def wait_for_restart(self, timeout=DEFAULT_REBOOT_TIMEOUT, **dargs):
        """
        Wait for the host to come back from a reboot. This wraps the
        generic wait_for_restart implementation in a reboot group.
        """
        def op_func():
            # pylint: disable=missing-docstring
            super(RemoteHost, self).wait_for_restart(timeout=timeout, **dargs)
        self.log_op(self.OP_REBOOT, op_func)


    def cleanup(self):
        super(RemoteHost, self).cleanup()
        self.reboot()


    def get_tmp_dir(self, parent='/tmp'):
        """
        Return the pathname of a directory on the host suitable
        for temporary file storage.

        The directory and its content will be deleted automatically
        on the destruction of the Host object that was used to obtain
        it.
        """
        self.run("mkdir -p %s" % parent)
        template = os.path.join(parent, 'autoserv-XXXXXX')
        dir_name = self.run("mktemp -d %s" % template).stdout.rstrip()
        self.tmp_dirs.append(dir_name)
        return dir_name


    def get_platform_label(self):
        """
        Return the platform label, or None if platform label is not set.
        """

        if self.job:
            keyval_path = os.path.join(self.job.resultdir, 'host_keyvals',
                                       self.hostname)
            keyvals = utils.read_keyval(keyval_path)
            return keyvals.get('platform', None)
        else:
            return None


    def get_all_labels(self):
        """
        Return all labels, or empty list if label is not set.
        """
        if self.job:
            keyval_path = os.path.join(self.job.resultdir, 'host_keyvals',
                                       self.hostname)
            keyvals = utils.read_keyval(keyval_path)
            all_labels = keyvals.get('labels', '')
            if all_labels:
                all_labels = all_labels.split(',')
                return [urllib.unquote(label) for label in all_labels]
        return []


    def delete_tmp_dir(self, tmpdir):
        """
        Delete the given temporary directory on the remote machine.

        @param tmpdir The directory to delete.
        """
        self.run('rm -rf "%s"' % utils.sh_escape(tmpdir), ignore_status=True)
        self.tmp_dirs.remove(tmpdir)


    def check_uptime(self):
        """
        Check that uptime is available and monotonically increasing.
        """
        if not self.is_up():
            raise error.AutoservHostError('Client does not appear to be up')
        result = self.run("/bin/cat /proc/uptime", 30)
        return result.stdout.strip().split()[0]


    def check_for_lkdtm(self):
        """
        Check for kernel dump test module. return True if exist.
        """
        cmd = 'ls /sys/kernel/debug/provoke-crash/DIRECT'
        return self.run(cmd, ignore_status=True).exit_status == 0


    def are_wait_up_processes_up(self):
        """
        Checks if any HOSTS waitup processes are running yet on the
        remote host.

        Returns True if any the waitup processes are running, False
        otherwise.
        """
        processes = self.get_wait_up_processes()
        if len(processes) == 0:
            return True # wait up processes aren't being used
        for procname in processes:
            exit_status = self.run("{ ps -e || ps; } | grep '%s'" % procname,
                                   ignore_status=True).exit_status
            if exit_status == 0:
                return True
        return False


    def get_labels(self):
        """Return a list of labels for this given host.

        This is the main way to retrieve all the automatic labels for a host
        as it will run through all the currently implemented label functions.
        """
        labels = []
        for label_function in self._LABEL_FUNCTIONS:
            try:
                label = label_function(self)
            except Exception:
                logging.exception('Label function %s failed; ignoring it.',
                                  label_function.__name__)
                label = None
            if label:
                if type(label) is str:
                    labels.append(label)
                elif type(label) is list:
                    labels.extend(label)
        return labels
