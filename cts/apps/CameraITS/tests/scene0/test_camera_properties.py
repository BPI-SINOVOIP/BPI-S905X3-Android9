# Copyright 2013 The Android Open Source Project
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

import its.caps
import its.device
import its.objects
import pprint

def main():
    """Basic test to query and print out camera properties.
    """

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()

        pprint.pprint(props)

        its.caps.skip_unless(its.caps.manual_sensor(props))

        # Test that a handful of required keys are present.
        assert(props.has_key('android.sensor.info.sensitivityRange'))
        assert(props.has_key('android.sensor.orientation'))
        assert(props.has_key('android.scaler.streamConfigurationMap'))
        assert(props.has_key('android.lens.facing'))

        print "JPG sizes:", its.objects.get_available_output_sizes("jpg", props)
        print "RAW sizes:", its.objects.get_available_output_sizes("raw", props)
        print "YUV sizes:", its.objects.get_available_output_sizes("yuv", props)

if __name__ == '__main__':
    main()

