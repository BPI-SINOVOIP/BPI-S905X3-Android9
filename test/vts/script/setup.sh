#!/bin/bash
#
# Copyright 2016 The Android Open Source Project
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

# Only tested on linux so far
echo "Install Python SDK"
sudo apt-get -y install python-dev

echo "Install Protocol Buffer packages"
sudo apt-get -y install python-protobuf
sudo apt-get -y install protobuf-compiler

echo "Install Python virtualenv and pip tools for VTS TradeFed and Runner"
sudo apt-get -y install python-setuptools
sudo apt-get -y install python-pip
sudo apt-get -y install python3-pip
sudo apt-get -y install python-virtualenv
sudo apt-get -y install build-essential

echo "Install packages for Camera ITS tests"
sudo apt-get -y install python-tk
sudo apt-get -y install libjpeg-dev
sudo apt-get -y install libtiff-dev

echo "Install packaged for usb gadget tests"
sudo pip install --upgrade libusb1
