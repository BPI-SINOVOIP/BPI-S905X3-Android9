#!/usr/bin/env python
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

from setuptools import setup
from setuptools import find_packages

install_requires = [
    'python-dateutil',
    'protobuf',
    'google-api-python-client'
]

setup(
    name='acloud',
    version='0.1',
    description='Cloud Android Driver',
    license='Apache2.0',
    packages=find_packages(),
    include_package_data=False,
    install_requires=install_requires,
    url="http://www.android.com/",
    entry_points={
        'console_scripts': [
            'acloud = acloud.public.__main__:main'
        ]
    }
)
