# Copyright 2014 The Android Open Source Project
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

import its.device
import its.objects
import its.caps
import time

def main():
    """Test if image and motion sensor events are in the same time domain.
    """

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()

        # Only run test if the appropriate caps are claimed.
        its.caps.skip_unless(its.caps.sensor_fusion(props))

        # Get the timestamp of a captured image.
        if its.caps.manual_sensor(props):
            req, fmt = its.objects.get_fastest_manual_capture_settings(props)
        else:
            req, fmt = its.objects.get_fastest_auto_capture_settings(props)
        cap = cam.do_capture(req, fmt)
        ts_image0 = cap['metadata']['android.sensor.timestamp']

        # Get the timestamps of motion events.
        print "Reading sensor measurements"
        sensors = cam.get_sensors()
        cam.start_sensor_events()
        time.sleep(2.0)
        events = cam.get_sensor_events()
        ts_sensor_first = {}
        ts_sensor_last = {}
        for sensor, existing in sensors.iteritems():
            if existing:
                assert(len(events[sensor]) > 0)
                ts_sensor_first[sensor] = events[sensor][0]["time"]
                ts_sensor_last[sensor] = events[sensor][-1]["time"]

        # Get the timestamp of another image.
        cap = cam.do_capture(req, fmt)
        ts_image1 = cap['metadata']['android.sensor.timestamp']

        print "Image timestamps:", ts_image0, ts_image1

        # The motion timestamps must be between the two image timestamps.
        for sensor, existing in sensors.iteritems():
            if existing:
                print "%s timestamps: %d %d" % (sensor, ts_sensor_first[sensor],
                                                ts_sensor_last[sensor])
                assert ts_image0 < ts_sensor_first[sensor] < ts_image1
                assert ts_image0 < ts_sensor_last[sensor] < ts_image1

if __name__ == '__main__':
    main()

