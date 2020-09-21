#!/usr/bin/env python3.4
#
# Copyright 2016 - The Android Open Source Project
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
import pip
import setuptools
import sys
import subprocess
import os

README_FILE = os.path.join(os.path.dirname(__file__), 'README.md')

install_requires = [
    # Future needs to have a newer version that contains urllib.
    'future>=0.16.0',
    'psutil',
    'shellescape',
    'IPy',
]

if sys.version_info < (3, ):
    # "futures" is needed for py2 compatibility and it only works in 2.7
    install_requires.append('futures')
    install_requires.append('subprocess32')

try:
    # Need to install python-dev to work with Python2.7 installing.
    subprocess.check_call(['apt-get', 'install', '-y', 'python-dev'])
except subprocess.CalledProcessError as cpe:
    print('Could not install python-dev: %s' % cpe.output)


class LabHealthInstallDependencies(cmd.Command):
    """Installs only required packages

    Installs all required packages for acts to work. Rather than using the
    normal install system which creates links with the python egg, pip is
    used to install the packages.
    """

    description = 'Install dependencies needed for lab health.'
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        pip.main(['install', '--upgrade', 'pip'])

        required_packages = self.distribution.install_requires
        for package in required_packages:
            self.announce('Installing %s...' % package, log.INFO)
            pip.main(['install', package])

        self.announce('Dependencies installed.')


def main():
    setuptools.setup(
        name='LabHealth',
        version='0.3',
        description='Android Test Lab Health',
        license='Apache2.0',
        cmdclass={
            'install_deps': LabHealthInstallDependencies,
        },
        packages=setuptools.find_packages(),
        include_package_data=False,
        install_requires=install_requires,
        url="http://www.android.com/",
        long_description=open(README_FILE).read())


if __name__ == '__main__':
    main()
