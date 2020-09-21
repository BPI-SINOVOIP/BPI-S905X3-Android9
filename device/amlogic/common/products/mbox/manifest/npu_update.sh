#opyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if cat $1 | grep "android.hardware.neuralnetworks">/dev/null
then
  exit
else
    sed -i '/<manifest version="1.0" type="device" target-level="3">/a\     <hal format="hidl">\n        <name>android.hardware.neuralnetworks</name>\n        <transport>hwbinder</transport>\n        <version>1.1</version>\n        <interface>\n        <name>IDevice</name>\n        <instance>ovx-driver</instance>\n        </interface>\n        <fqname>@1.1::IDevice/ovx-driver</fqname>\n        </hal>' $1
fi

