# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utility to run a Brillo emulator programmatically.

Requires system.img, userdata.img and kernel to be in imagedir. If running an
arm emulator kernel.dtb (or another dtb file) must also be in imagedir.

WARNING: Processes created by this utility may not die unless
EmulatorManager.stop is called. Call EmulatorManager.verify_stop to
confirm process has stopped and port is free.
"""

import os
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils


class EmulatorManagerException(Exception):
    """Bad port, missing artifact or non-existant imagedir."""
    pass


class EmulatorManager(object):
    """Manage an instance of a device emulator.

    @param imagedir: directory of emulator images.
    @param port: Port number for emulator's adbd. Note this port is one higher
                 than the port in the emulator's serial number.
    @param run: Function used to execute shell commands.
    """
    def __init__(self, imagedir, port, run=utils.run):
        if not port % 2 or port < 5555 or port > 5585:
             raise EmulatorManagerException('Port must be an odd number '
                                            'between 5555 and 5585.')
        try:
            run('test -f %s' % os.path.join(imagedir, 'system.img'))
        except error.GenericHostRunError:
            raise EmulatorManagerException('Image directory must exist and '
                                           'contain emulator images.')

        self.port = port
        self.imagedir = imagedir
        self.run = run


    def verify_stop(self, timeout_secs=3):
        """Wait for emulator on our port to stop.

        @param timeout_secs: Max seconds to wait for the emulator to stop.

        @return: Bool - True if emulator stops.
        """
        cycles = 0
        pid = self.find()
        while pid:
            cycles += 1
            time.sleep(0.1)
            pid = self.find()
            if cycles >= timeout_secs*10 and pid:
                return False
        return True


    def _find_dtb(self):
        """Detect a dtb file in the image directory

        @return: Path to dtb file or None.
        """
        cmd_result = self.run('find "%s" -name "*.dtb"' % self.imagedir)
        dtb = cmd_result.stdout.split('\n')[0]
        return dtb.strip() or None


    def start(self):
        """Start an emulator with the images and port specified.

        If an emulator is already running on the port it will be killed.
        """
        self.force_stop()
        time.sleep(1) # Wait for port to be free
        # TODO(jgiorgi): Add support for x86 / x64 emulators
        args = [
            '-dmS', 'emulator-%s' % self.port, 'qemu-system-arm',
            '-M', 'vexpress-a9',
            '-m', '1024M',
            '-kernel', os.path.join(self.imagedir, 'kernel'),
            '-append', ('"console=ttyAMA0 ro root=/dev/sda '
                        'androidboot.hardware=qemu qemu=1 rootwait noinitrd '
                        'init=/init androidboot.selinux=enforcing"'),
            '-nographic',
            '-device', 'virtio-scsi-device,id=scsi',
            '-device', 'scsi-hd,drive=system',
            '-drive', ('file="%s,if=none,id=system,format=raw"'
                       % os.path.join(self.imagedir, 'system.img')),
            '-device', 'scsi-hd,drive=userdata',
            '-drive', ('file="%s,if=none,id=userdata,format=raw"'
                       % os.path.join(self.imagedir, 'userdata.img')),
            '-redir', 'tcp:%s::5555' % self.port,
        ]

        # DTB file produced and required for arm but not x86 emulators
        dtb = self._find_dtb()
        if dtb:
            args += ['-dtb', dtb]
        else:
            raise EmulatorManagerException('DTB file missing. Required for arm '
                                           'emulators.')

        self.run(' '.join(['screen'] + args))


    def find(self):
        """Detect the PID of a qemu process running on our port.

        @return: PID or None
        """
        running = self.run('netstat -nlpt').stdout
        for proc in running.split('\n'):
            if ':%s' % self.port in proc:
                process = proc.split()[-1]
                if '/' in process: # Program identified, we started and can kill
                    return process.split('/')[0]


    def stop(self, kill=False):
        """Send signal to stop emulator process.

        Signal is sent to any running qemu process on our port regardless of how
        it was started. Silent no-op if no running qemu processes on the port.

        @param kill: Send SIGKILL signal instead of SIGTERM.
        """
        pid = self.find()
        if pid:
            cmd = 'kill -9 %s' if kill else 'kill %s'
            self.run(cmd % pid)


    def force_stop(self):
        """Attempt graceful shutdown, kill if not dead after 3 seconds.
        """
        self.stop()
        if not self.verify_stop(timeout_secs=3):
            self.stop(kill=True)
        if not self.verify_stop():
            raise RuntimeError('Emulator running on port %s failed to stop.'
                               % self.port)

