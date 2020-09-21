#!/usr/bin/env python3.4
#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from distutils import cmd
from distutils import log
import os
import shutil
import setuptools
from setuptools.command import test
import sys

install_requires = [
    # Future needs to have a newer version that contains urllib.
    'future>=0.16.0',
    # mock-1.0.1 is the last version compatible with setuptools <17.1,
    # which is what comes with Ubuntu 14.04 LTS.
    'mock<=1.0.1',
    'numpy',
    'pyserial',
    'shellescape',
    'protobuf',
    'roman',
    'scapy-python3',
    'pylibftdi',
    'xlsxwriter'
]

if sys.version_info < (3, ):
    install_requires.append('enum34')
    install_requires.append('statistics')
    # "futures" is needed for py2 compatibility and it only works in 2.7
    install_requires.append('futures')
    install_requires.append('py2-ipaddress')
    install_requires.append('subprocess32')


class PyTest(test.test):
    """Class used to execute unit tests using PyTest. This allows us to execute
    unit tests without having to install the package.
    """

    def finalize_options(self):
        test.test.finalize_options(self)
        self.test_args = ['-x', "tests"]
        self.test_suite = True

    def run_tests(self):
        import pytest
        errno = pytest.main(self.test_args)
        sys.exit(errno)


class ActsInstallDependencies(cmd.Command):
    """Installs only required packages

    Installs all required packages for acts to work. Rather than using the
    normal install system which creates links with the python egg, pip is
    used to install the packages.
    """

    description = 'Install dependencies needed for acts to run on this machine.'
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        import pip
        pip.main(['install', '--upgrade', 'pip'])
        required_packages = self.distribution.install_requires

        for package in required_packages:
            self.announce('Installing %s...' % package, log.INFO)
            pip.main(['install', '-v', '--no-cache-dir', package])

        self.announce('Dependencies installed.')


class ActsUninstall(cmd.Command):
    """Acts uninstaller.

    Uninstalls acts from the current version of python. This will attempt to
    import acts from any of the python egg locations. If it finds an import
    it will use the modules file location to delete it. This is repeated until
    acts can no longer be imported and thus is uninstalled.
    """

    description = 'Uninstall acts from the local machine.'
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def uninstall_acts_module(self, acts_module):
        """Uninstalls acts from a module.

        Args:
            acts_module: The acts module to uninstall.
        """
        for acts_install_dir in acts_module.__path__:
            self.announce('Deleting acts from: %s' % acts_install_dir,
                          log.INFO)
            shutil.rmtree(acts_install_dir)

    def run(self):
        """Entry point for the uninstaller."""
        # Remove the working directory from the python path. This ensures that
        # Source code is not accidentally targeted.
        our_dir = os.path.abspath(os.path.dirname(__file__))
        if our_dir in sys.path:
            sys.path.remove(our_dir)

        if os.getcwd() in sys.path:
            sys.path.remove(os.getcwd())

        try:
            import acts as acts_module
        except ImportError:
            self.announce(
                'Acts is not installed, nothing to uninstall.',
                level=log.ERROR)
            return

        while acts_module:
            self.uninstall_acts_module(acts_module)
            try:
                del sys.modules['acts']
                import acts as acts_module
            except ImportError:
                acts_module = None

        self.announce('Finished uninstalling acts.')


def main():
    setuptools.setup(
        name='acts',
        version='0.9',
        description='Android Comms Test Suite',
        license='Apache2.0',
        packages=setuptools.find_packages(),
        include_package_data=False,
        tests_require=['pytest'],
        install_requires=install_requires,
        scripts=['acts/bin/act.py', 'acts/bin/monsoon.py'],
        cmdclass={
            'test': PyTest,
            'install_deps': ActsInstallDependencies,
            'uninstall': ActsUninstall
        },
        url="http://www.android.com/")

    if {'-u', '--uninstall', 'uninstall'}.intersection(sys.argv):
        act_path = '/usr/local/bin/act.py'
        if os.path.islink(act_path):
            os.unlink(act_path)
        elif os.path.exists(act_path):
            os.remove(act_path)


if __name__ == '__main__':
    main()
