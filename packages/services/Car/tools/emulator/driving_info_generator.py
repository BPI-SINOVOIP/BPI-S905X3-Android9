# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import datetime
import time

from xml.dom import minidom

import diagnostic_sensors as s
import vhal_consts_2_0 as c

from diagnostic_builder import DiagnosticEventBuilder

# interval of generating driving information
SAMPLE_INTERVAL_SECONDS = 0.5

RPM_LOW = 1000
RPM_HIGH = 3000

REVERSE_DURATION_SECONDS = 10
PARK_DURATION_SECONDS = 10

# roughly 5 miles/hour
REVERSE_SPEED_METERS_PER_SECOND = 2.3

UTC_TIME_FORMAT = "%Y-%m-%dT%H:%M:%S"

# Diagnostics property constants. The value is based on the record from a test drive
FUEL_SYSTEM_STATUS_VALUE = 2
AMBIENT_AIR_TEMPERATURE_VALUE = 21
ENGINE_COOLANT_TEMPERATURE_VALUE = 75


def speed2Gear(speed):
    """
        Get the current gear based on speed of vehicle. The conversion may not be strictly real but
        are close enough to a normal vehicle. Assume the vehicle is moving forward.
    """
    if speed < 4.4:
        # 0 - 10 mph
        return c.VEHICLEGEAR_GEAR_1
    elif speed < 11.2:
        # 10 - 25 mph
        return c.VEHICLEGEAR_GEAR_2
    elif speed < 20.1:
        # 25 - 45 mph
        return c.VEHICLEGEAR_GEAR_3
    elif speed < 26.8:
        # 45 - 60 mph
        return c.VEHICLEGEAR_GEAR_4
    else:
        # > 60 mph
        return c.VEHICLEGEAR_GEAR_5

class GpxFrame(object):
    """
        A class representing a track point from GPX file
    """
    def __init__(self, trkptDom):
        timeElements = trkptDom.getElementsByTagName('time')
        if timeElements:
            # time value in GPX is in UTC format: YYYY-MM-DDTHH:MM:SS, need to parse it
            self.datetime = datetime.datetime.strptime(timeElements[0].firstChild.nodeValue,
                                                          UTC_TIME_FORMAT)
        speedElements = trkptDom.getElementsByTagName('speed')
        if speedElements:
            self.speedInMps = float(speedElements[0].firstChild.nodeValue)

class DrivingInfoGenerator(object):
    """
        A class that generates driving information like speed, odometer, rpm, diagnostics etc. It
        takes a GPX file which describes a real route that consists of a sequence of location data,
        and then derive driving information from those data.

        One assumption is that it is automatic transmission car, so that current gear does not
        necessarily match selected gear.
    """

    def __init__(self, gpxFile, vhal):
        self.gpxDom = minidom.parse(gpxFile)
        # Speed of vehicle (meter / second)
        self.speedInMps = 0
        # Fixed RPM with average value during driving
        self.rpm = RPM_LOW
        # Odometer (kilometer)
        self.odometerInKm = 0
        # Gear selection
        self.selectedGear = c.VEHICLEGEAR_GEAR_PARK
        # Current gear
        self.currentGear = c.VEHICLEGEAR_GEAR_PARK
        # Timestamp while driving on route defined in GPX file
        self.datetime = 0
        # Get Diagnostics live frame property configure
        vhal.getConfig(c.VEHICLEPROPERTY_OBD2_LIVE_FRAME)
        self.liveFrameConfig = vhal.rxMsg()


    def _generateFrame(self, listener):
        """
            Handle newly generated vehicle property with listener
        """
        listener.handle(c.VEHICLEPROPERTY_PERF_VEHICLE_SPEED, 0, self.speedInMps, "PERF_VEHICLE_SPEED")
        listener.handle(c.VEHICLEPROPERTY_ENGINE_RPM, 0, self.rpm, "ENGINE_RPM")
        listener.handle(c.VEHICLEPROPERTY_PERF_ODOMETER, 0, self.odometerInKm, "PERF_ODOMETER")
        listener.handle(c.VEHICLEPROPERTY_CURRENT_GEAR, 0, self.currentGear, "CURRENT_GEAR")
        listener.handle(c.VEHICLEPROPERTY_OBD2_LIVE_FRAME, 0,
                        self._buildDiagnosticLiveFrame(), "DIAGNOSTIC_LIVE_FRAME")

    def _buildDiagnosticLiveFrame(self):
        """
            Build a diagnostic live frame with a few sensor fields set
        """
        builder = DiagnosticEventBuilder(self.liveFrameConfig)
        builder.setStringValue('')
        builder.addIntSensor(s.DIAGNOSTIC_SENSOR_INTEGER_FUEL_SYSTEM_STATUS,
                             FUEL_SYSTEM_STATUS_VALUE)
        builder.addIntSensor(s.DIAGNOSTIC_SENSOR_INTEGER_AMBIENT_AIR_TEMPERATURE,
                             AMBIENT_AIR_TEMPERATURE_VALUE)
        builder.addFloatSensor(s.DIAGNOSTIC_SENSOR_FLOAT_ENGINE_COOLANT_TEMPERATURE,
                               ENGINE_COOLANT_TEMPERATURE_VALUE)
        builder.addFloatSensor(s.DIAGNOSTIC_SENSOR_FLOAT_ENGINE_RPM, self.rpm)
        builder.addFloatSensor(s.DIAGNOSTIC_SENSOR_FLOAT_VEHICLE_SPEED, self.speedInMps)
        return builder.build()

    def _generateFromGpxFrame(self, gpxFrame, listener):
        """
            Generate a sequence of vehicle property frames from current track point to the next one.
            The frequency of frames are pre-defined.

            Some assumptions here:
                - Two track points are very close to each other (e.g. 1 second driving distance)
                - It is a straight line between two track point
                - Speed is changing linearly between two track point

            Given the info:
                timestamp1 : speed1
                timestamp2 : speed2

            Vehicle properties in each frame are derived like this:
                - Speed is calculated based on linear model
                - Odometer is calculated based on speed and time
                - RPM will be set to a low value if not accelerating, otherwise set to a high value
                - Current gear will be set according to speed
        """

        duration = (gpxFrame.datetime - self.datetime).total_seconds()
        speedIncrement = (gpxFrame.speedInMps - self.speedInMps) / duration * SAMPLE_INTERVAL_SECONDS
        self.rpm = RPM_HIGH if speedIncrement > 0 else RPM_LOW

        timeElapsed = 0
        while timeElapsed < duration:
            self._generateFrame(listener)
            if timeElapsed + SAMPLE_INTERVAL_SECONDS < duration:
                self.odometerInKm += (self.speedInMps + speedIncrement / 2.0) * SAMPLE_INTERVAL_SECONDS / 1000
                self.speedInMps += speedIncrement
                time.sleep(SAMPLE_INTERVAL_SECONDS)
            else:
                timeLeft = duration - timeElapsed
                self.odometerInKm += (self.speedInMps + gpxFrame.speedInMps) / 2.0 * timeLeft / 1000
                self.speedInMps = gpxFrame.speedInMps
                time.sleep(timeLeft)

            self.currentGear = speed2Gear(self.speedInMps)
            timeElapsed += SAMPLE_INTERVAL_SECONDS

        self.datetime = gpxFrame.datetime

    def _generateInReverseMode(self, duration, listener):
        print "Vehicle is reversing"
        listener.handle(c.VEHICLEPROPERTY_GEAR_SELECTION, 0, c.VEHICLEGEAR_GEAR_REVERSE,
                        "GEAR_SELECTION")
        listener.handle(c.VEHICLEPROPERTY_CURRENT_GEAR, 0, c.VEHICLEGEAR_GEAR_REVERSE,
                        "CURRENT_GEAR")
        self.rpm = RPM_LOW
        self.speedInMps = REVERSE_SPEED_METERS_PER_SECOND
        curTime = 0
        while curTime < duration:
            self._generateFrame(listener)
            self.odometerInKm += self.speedInMps * SAMPLE_INTERVAL_SECONDS / 1000
            curTime += SAMPLE_INTERVAL_SECONDS
            time.sleep(SAMPLE_INTERVAL_SECONDS)
        # After reverse is done, set speed to 0
        self.speedInMps = .0

    def _generateInParkMode(self, duration, listener):
        print "Vehicle is parked"
        listener.handle(c.VEHICLEPROPERTY_GEAR_SELECTION, 0, c.VEHICLEGEAR_GEAR_PARK,
                        "GEAR_SELECTION")
        listener.handle(c.VEHICLEPROPERTY_CURRENT_GEAR, 0, c.VEHICLEGEAR_GEAR_PARK,
                        "CURRENT_GEAR")
        # Assume in park mode, engine is still on
        self.rpm = RPM_LOW
        self.speedInMps = .0
        curTime = 0
        while curTime < duration:
            self._generateFrame(listener)
            curTime += SAMPLE_INTERVAL_SECONDS
            time.sleep(SAMPLE_INTERVAL_SECONDS)

    def generate(self, listener):
        # First, car is parked (probably in garage)
        self._generateInParkMode(PARK_DURATION_SECONDS, listener)
        # Second, car will reverse (out of garage)
        self._generateInReverseMode(REVERSE_DURATION_SECONDS, listener)

        trk = self.gpxDom.getElementsByTagName('trk')[0]
        trkseg = trk.getElementsByTagName('trkseg')[0]
        trkpts = trkseg.getElementsByTagName('trkpt')

        print "Vehicle start moving forward"
        listener.handle(c.VEHICLEPROPERTY_GEAR_SELECTION, 0, c.VEHICLEGEAR_GEAR_DRIVE,
                        "GEAR_SELECTION")

        firstGpxFrame = GpxFrame(trkpts[0])
        self.speedInMps = firstGpxFrame.speedInMps
        self.datetime = firstGpxFrame.datetime

        for i in xrange(1, len(trkpts)):
            self._generateFromGpxFrame(GpxFrame(trkpts[i]), listener)
