#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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
#

import logging
import time

from vts.runners.host import asserts
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_host_test import hal_hidl_host_test

VEHICLE_V2_0_HAL = "android.hardware.automotive.vehicle@2.0::IVehicle"

class VtsHalAutomotiveVehicleV2_0HostTest(hal_hidl_host_test.HalHidlHostTest):
    """A simple testcase for the VEHICLE HIDL HAL."""

    TEST_HAL_SERVICES = {
        VEHICLE_V2_0_HAL,
    }

    def setUpClass(self):
        """Creates a mirror and init vehicle hal."""
        super(VtsHalAutomotiveVehicleV2_0HostTest, self).setUpClass()

        results = self.shell.Execute("id -u system")
        system_uid = results[const.STDOUT][0].strip()
        logging.info("system_uid: %s", system_uid)

        self.dut.hal.InitHidlHal(
            target_type="vehicle",
            target_basepaths=self.dut.libPaths,
            target_version=2.0,
            target_package="android.hardware.automotive.vehicle",
            target_component_name="IVehicle",
            hw_binder_service_name=self.getHalServiceName(VEHICLE_V2_0_HAL),
            bits=int(self.abi_bitness))

        self.vehicle = self.dut.hal.vehicle  # shortcut
        self.vehicle.SetCallerUid(system_uid)
        self.vtypes = self.dut.hal.vehicle.GetHidlTypeInterface("types")
        logging.info("vehicle types: %s", self.vtypes)
        asserts.assertEqual(0x00ff0000, self.vtypes.VehiclePropertyType.MASK)
        asserts.assertEqual(0x0f000000, self.vtypes.VehicleArea.MASK)

    def setUp(self):
        super(VtsHalAutomotiveVehicleV2_0HostTest, self).setUp()
        self.propToConfig = {}
        for config in self.vehicle.getAllPropConfigs():
            self.propToConfig[config['prop']] = config
        self.configList = self.propToConfig.values()

    def testListProperties(self):
        """Checks whether some PropConfigs are returned.

        Verifies that call to getAllPropConfigs is not failing and
        it returns at least 1 vehicle property config.
        """
        logging.info("all supported properties: %s", self.configList)
        asserts.assertLess(0, len(self.configList))

    def emptyValueProperty(self, propertyId, areaId=0):
        """Creates a property structure for use with the Vehicle HAL.

        Args:
            propertyId: the numeric identifier of the output property.
            areaId: the numeric identifier of the vehicle area of the output
                    property. 0, or omitted, for global.

        Returns:
            a property structure for use with the Vehicle HAL.
        """
        return {
            'prop' : propertyId,
            'timestamp' : 0,
            'areaId' : areaId,
            'status' : self.vtypes.VehiclePropertyStatus.AVAILABLE,
            'value' : {
                'int32Values' : [],
                'floatValues' : [],
                'int64Values' : [],
                'bytes' : [],
                'stringValue' : ""
            }
        }

    def readVhalProperty(self, propertyId, areaId=0):
        """Reads a specified property from Vehicle HAL.

        Args:
            propertyId: the numeric identifier of the property to be read.
            areaId: the numeric identifier of the vehicle area to retrieve the
                    property for. 0, or omitted, for global.

        Returns:
            the value of the property as read from Vehicle HAL, or None
            if it could not read successfully.
        """
        vp = self.vtypes.Py2Pb("VehiclePropValue",
                               self.emptyValueProperty(propertyId, areaId))
        logging.info("0x%x get request: %s", propertyId, vp)
        status, value = self.vehicle.get(vp)
        logging.info("0x%x get response: %s, %s", propertyId, status, value)
        if self.vtypes.StatusCode.OK == status:
            return value
        else:
            logging.warning("attempt to read property 0x%x returned error %d",
                            propertyId, status)

    def setVhalProperty(self, propertyId, value, areaId=0,
                        expectedStatus=0):
        """Sets a specified property in the Vehicle HAL.

        Args:
            propertyId: the numeric identifier of the property to be set.
            value: the value of the property, formatted as per the Vehicle HAL
                   (use emptyValueProperty() as a helper).
            areaId: the numeric identifier of the vehicle area to set the
                    property for. 0, or omitted, for global.
            expectedStatus: the StatusCode expected to be returned from setting
                    the property. 0, or omitted, for OK.
        """
        propValue = self.emptyValueProperty(propertyId, areaId)
        for k in propValue["value"]:
            if k in value:
                if k == "stringValue":
                    propValue["value"][k] += value[k]
                else:
                    propValue["value"][k].extend(value[k])
        vp = self.vtypes.Py2Pb("VehiclePropValue", propValue)
        logging.info("0x%x set request: %s", propertyId, vp)
        status = self.vehicle.set(vp)
        logging.info("0x%x set response: %s", propertyId, status)
        if 0 == expectedStatus:
            expectedStatus = self.vtypes.StatusCode.OK
        asserts.assertEqual(expectedStatus, status, "Prop 0x%x" % propertyId)

    def setAndVerifyIntProperty(self, propertyId, value, areaId=0):
        """Sets a integer property in the Vehicle HAL and reads it back.

        Args:
            propertyId: the numeric identifier of the property to be set.
            value: the int32 value of the property to be set.
            areaId: the numeric identifier of the vehicle area to set the
                    property for. 0, or omitted, for global.
        """
        self.setVhalProperty(propertyId, {"int32Values" : [value]}, areaId=areaId)

        propValue = self.readVhalProperty(propertyId, areaId=areaId)
        asserts.assertEqual(1, len(propValue["value"]["int32Values"]))
        asserts.assertEqual(value, propValue["value"]["int32Values"][0])

    def extractZonesAsList(self, supportedAreas):
        """Converts bitwise area flags to list of zones"""
        allZones = [
            self.vtypes.VehicleAreaZone.ROW_1_LEFT,
            self.vtypes.VehicleAreaZone.ROW_1_CENTER,
            self.vtypes.VehicleAreaZone.ROW_1_RIGHT,
            self.vtypes.VehicleAreaZone.ROW_2_LEFT,
            self.vtypes.VehicleAreaZone.ROW_2_CENTER,
            self.vtypes.VehicleAreaZone.ROW_2_RIGHT,
            self.vtypes.VehicleAreaZone.ROW_3_LEFT,
            self.vtypes.VehicleAreaZone.ROW_3_CENTER,
            self.vtypes.VehicleAreaZone.ROW_3_RIGHT,
            self.vtypes.VehicleAreaZone.ROW_4_LEFT,
            self.vtypes.VehicleAreaZone.ROW_4_CENTER,
            self.vtypes.VehicleAreaZone.ROW_4_RIGHT,
        ]

        extractedZones = []
        for zone in allZones:
            if (zone & supportedAreas == zone):
                extractedZones.append(zone)
        return extractedZones


    def disableTestHvacPowerOn(self):
        # Disable this test for now.  HVAC Power On will no longer behave like this now that we've
        #   added the status field in VehiclePropValue.  Need to update the test for this.
        """Test power on/off and properties associated with it.

        Gets the list of properties that are affected by the HVAC power state
        and validates them.

        Turns power on to start in a defined state, verifies that power is on
        and properties are available.  State change from on->off and verifies
        that properties are no longer available, then state change again from
        off->on to verify properties are now available again.
        """

        # Checks that HVAC_POWER_ON property is supported and returns valid
        # result initially.
        hvacPowerOnConfig = self.propToConfig[self.vtypes.VehicleProperty.HVAC_POWER_ON]
        if hvacPowerOnConfig is None:
            logging.info("HVAC_POWER_ON not supported")
            return

        zones = self.extractZonesAsList(hvacPowerOnConfig['supportedAreas'])
        asserts.assertLess(0, len(zones), "supportedAreas for HVAC_POWER_ON property is invalid")

        # TODO(pavelm): consider to check for all zones
        zone = zones[0]

        propValue = self.readVhalProperty(
            self.vtypes.VehicleProperty.HVAC_POWER_ON, areaId=zone)

        asserts.assertEqual(1, len(propValue["value"]["int32Values"]))
        asserts.assertTrue(
            propValue["value"]["int32Values"][0] in [0, 1],
            "%d not a valid value for HVAC_POWER_ON" %
                propValue["value"]["int32Values"][0]
            )

        # Checks that HVAC_POWER_ON config string returns valid result.
        requestConfig = [self.vtypes.Py2Pb(
            "VehicleProperty", self.vtypes.VehicleProperty.HVAC_POWER_ON)]
        logging.info("HVAC power on config request: %s", requestConfig)
        responseConfig = self.vehicle.getPropConfigs(requestConfig)
        logging.info("HVAC power on config response: %s", responseConfig)
        hvacTypes = set([
            self.vtypes.VehicleProperty.HVAC_FAN_SPEED,
            self.vtypes.VehicleProperty.HVAC_FAN_DIRECTION,
            self.vtypes.VehicleProperty.HVAC_TEMPERATURE_CURRENT,
            self.vtypes.VehicleProperty.HVAC_TEMPERATURE_SET,
            self.vtypes.VehicleProperty.HVAC_DEFROSTER,
            self.vtypes.VehicleProperty.HVAC_AC_ON,
            self.vtypes.VehicleProperty.HVAC_MAX_AC_ON,
            self.vtypes.VehicleProperty.HVAC_MAX_DEFROST_ON,
            self.vtypes.VehicleProperty.HVAC_RECIRC_ON,
            self.vtypes.VehicleProperty.HVAC_DUAL_ON,
            self.vtypes.VehicleProperty.HVAC_AUTO_ON,
            self.vtypes.VehicleProperty.HVAC_ACTUAL_FAN_SPEED_RPM,
        ])
        status = responseConfig[0]
        asserts.assertEqual(self.vtypes.StatusCode.OK, status)
        configString = responseConfig[1][0]["configString"]
        configProps = []
        if configString != "":
            for prop in configString.split(","):
                configProps.append(int(prop, 16))
        for prop in configProps:
            asserts.assertTrue(prop in hvacTypes,
                               "0x%X not an HVAC type" % prop)

        # Turn power on.
        self.setAndVerifyIntProperty(
            self.vtypes.VehicleProperty.HVAC_POWER_ON, 1, areaId=zone)

        # Check that properties that require power to be on can be set.
        propVals = {}
        for prop in configProps:
            v = self.readVhalProperty(prop, areaId=zone)["value"]
            self.setVhalProperty(prop, v, areaId=zone)
            # Save the value for use later when trying to set the property when
            # HVAC is off.
            propVals[prop] = v

        # Turn power off.
        self.setAndVerifyIntProperty(
            self.vtypes.VehicleProperty.HVAC_POWER_ON, 0, areaId=zone)

        # Check that properties that require power to be on can't be set.
        for prop in configProps:
            self.setVhalProperty(
                prop, propVals[prop],
                areaId=zone,
                expectedStatus=self.vtypes.StatusCode.NOT_AVAILABLE)

        # Turn power on.
        self.setAndVerifyIntProperty(
            self.vtypes.VehicleProperty.HVAC_POWER_ON, 1, areaId=zone)

        # Check that properties that require power to be on can be set.
        for prop in configProps:
            self.setVhalProperty(prop, propVals[prop], areaId=zone)

    def testVehicleStaticProps(self):
        """Verifies that static properties are configured correctly"""
        staticProperties = set([
            self.vtypes.VehicleProperty.INFO_VIN,
            self.vtypes.VehicleProperty.INFO_MAKE,
            self.vtypes.VehicleProperty.INFO_MODEL,
            self.vtypes.VehicleProperty.INFO_MODEL_YEAR,
            self.vtypes.VehicleProperty.INFO_FUEL_CAPACITY,
            self.vtypes.VehicleProperty.INFO_FUEL_TYPE,
            self.vtypes.VehicleProperty.INFO_EV_BATTERY_CAPACITY,
            self.vtypes.VehicleProperty.INFO_EV_CONNECTOR_TYPE,
            self.vtypes.VehicleProperty.HVAC_FAN_DIRECTION_AVAILABLE,
            self.vtypes.VehicleProperty.AP_POWER_BOOTUP_REASON,
        ])
        for c in self.configList:
            prop = c['prop']
            msg = "Prop 0x%x" % prop
            if (c["prop"] in staticProperties):
                asserts.assertEqual(self.vtypes.VehiclePropertyChangeMode.STATIC, c["changeMode"], msg)
                asserts.assertEqual(self.vtypes.VehiclePropertyAccess.READ, c["access"], msg)
                propValue = self.readVhalProperty(prop)
                asserts.assertEqual(prop, propValue["prop"])
                self.setVhalProperty(prop, propValue["value"],
                    expectedStatus=self.vtypes.StatusCode.ACCESS_DENIED)
            else:  # Non-static property
                asserts.assertNotEqual(self.vtypes.VehiclePropertyChangeMode.STATIC, c["changeMode"], msg)

    def testPropertyRanges(self):
        """Retrieve the property ranges for all areas.

        This checks that the areas noted in the config all give valid area
        configs.  Once these are validated, the values for all these areas
        retrieved from the HIDL must be within the ranges defined."""

        enumProperties = {
            self.vtypes.VehicleProperty.ENGINE_OIL_LEVEL,
            self.vtypes.VehicleProperty.GEAR_SELECTION,
            self.vtypes.VehicleProperty.CURRENT_GEAR,
            self.vtypes.VehicleProperty.TURN_SIGNAL_STATE,
            self.vtypes.VehicleProperty.IGNITION_STATE,
            self.vtypes.VehicleProperty.HVAC_FAN_DIRECTION,
        }

        for c in self.configList:
            # Continuous properties need to have a sampling frequency.
            if c["changeMode"] & self.vtypes.VehiclePropertyChangeMode.CONTINUOUS != 0:
                asserts.assertLess(0.0, c["minSampleRate"],
                                   "minSampleRate should be > 0. Config list: %s" % c)
                asserts.assertLess(0.0, c["maxSampleRate"],
                                   "maxSampleRate should be > 0. Config list: %s" % c)
                asserts.assertFalse(c["minSampleRate"] > c["maxSampleRate"],
                                    "Prop 0x%x minSampleRate > maxSampleRate" %
                                        c["prop"])

            if c["prop"] & self.vtypes.VehiclePropertyType.BOOLEAN != 0:
                # Boolean types don't have ranges
                continue

            if c["prop"] in enumProperties:
                # This property does not use traditional min/max ranges
                continue

            asserts.assertTrue(c["areaConfigs"] != None, "Prop 0x%x must have areaConfigs" %
                               c["prop"])
            areasFound = 0
            if c["prop"] == self.vtypes.VehicleProperty.HVAC_TEMPERATURE_DISPLAY_UNITS:
                # This property doesn't have sensible min/max
                continue

            for a in c["areaConfigs"]:
                # Make sure this doesn't override one of the other areas found.
                asserts.assertEqual(0, areasFound & a["areaId"])
                areasFound |= a["areaId"]

                # Do some basic checking the min and max aren't mixed up.
                checks = [
                    ("minInt32Value", "maxInt32Value"),
                    ("minInt64Value", "maxInt64Value"),
                    ("minFloatValue", "maxFloatValue")
                ]
                for minName, maxName in checks:
                    asserts.assertFalse(
                        a[minName] > a[maxName],
                        "Prop 0x%x Area 0x%X %s > %s: %d > %d" %
                            (c["prop"], a["areaId"],
                             minName, maxName, a[minName], a[maxName]))

                # Get a value and make sure it's within the bounds.
                propVal = self.readVhalProperty(c["prop"], a["areaId"])
                # Some values may not be available, which is not an error.
                if propVal is None:
                    continue
                val = propVal["value"]
                valTypes = {
                    "int32Values": ("minInt32Value", "maxInt32Value"),
                    "int64Values": ("minInt64Value", "maxInt64Value"),
                    "floatValues": ("minFloatValue", "maxFloatValue"),
                }
                for valType, valBoundNames in valTypes.items():
                    for v in val[valType]:
                        # Make sure value isn't less than the minimum.
                        asserts.assertFalse(
                            v < a[valBoundNames[0]],
                            "Prop 0x%x Area 0x%X %s < min: %s < %s" %
                                (c["prop"], a["areaId"],
                                 valType, v, a[valBoundNames[0]]))
                        # Make sure value isn't greater than the maximum.
                        asserts.assertFalse(
                            v > a[valBoundNames[1]],
                            "Prop 0x%x Area 0x%X %s > max: %s > %s" %
                                (c["prop"], a["areaId"],
                                 valType, v, a[valBoundNames[1]]))

    def getValueIfPropSupported(self, propertyId):
        """Returns tuple of boolean (indicating value supported or not) and the value itself"""
        if (propertyId in self.propToConfig):
            propValue = self.readVhalProperty(propertyId)
            asserts.assertNotEqual(None, propValue, "expected value, prop: 0x%x" % propertyId)
            asserts.assertEqual(propertyId, propValue['prop'])
            return True, self.extractValue(propValue)
        else:
            return False, None

    def testInfoVinMakeModel(self):
        """Verifies INFO_VIN, INFO_MAKE, INFO_MODEL properties"""
        stringProperties = set([
            self.vtypes.VehicleProperty.INFO_VIN,
            self.vtypes.VehicleProperty.INFO_MAKE,
            self.vtypes.VehicleProperty.INFO_MODEL])
        for prop in stringProperties:
            supported, val = self.getValueIfPropSupported(prop)
            if supported:
                asserts.assertEqual(str, type(val), "prop: 0x%x" % prop)
                asserts.assertLess(0, (len(val)), "prop: 0x%x" % prop)

    def testGlobalFloatProperties(self):
        """Verifies that values of global float properties are in the correct range"""
        floatProperties = {
            self.vtypes.VehicleProperty.ENV_OUTSIDE_TEMPERATURE: (-50, 100),  # celsius
            self.vtypes.VehicleProperty.ENGINE_RPM : (0, 30000),  # RPMs
            self.vtypes.VehicleProperty.ENGINE_OIL_TEMP : (-50, 150),  # celsius
            self.vtypes.VehicleProperty.ENGINE_COOLANT_TEMP : (-50, 150),  #
            self.vtypes.VehicleProperty.PERF_VEHICLE_SPEED : (0, 150),  # m/s, 150 m/s = 330 mph
            self.vtypes.VehicleProperty.PERF_ODOMETER : (0, 1000000),  # km
            self.vtypes.VehicleProperty.INFO_FUEL_CAPACITY : (0, 1000000),  # milliliter
            self.vtypes.VehicleProperty.INFO_MODEL_YEAR : (1901, 2101),  # year
        }

        for prop, validRange in floatProperties.iteritems():
            supported, val = self.getValueIfPropSupported(prop)
            if supported:
                asserts.assertEqual(float, type(val))
                self.assertValueInRangeForProp(val, validRange[0], validRange[1], prop)

    def testGlobalBoolProperties(self):
        """Verifies that values of global boolean properties are in the correct range"""
        booleanProperties = set([
            self.vtypes.VehicleProperty.PARKING_BRAKE_ON,
            self.vtypes.VehicleProperty.FUEL_LEVEL_LOW,
            self.vtypes.VehicleProperty.NIGHT_MODE,
        ])
        for prop in booleanProperties:
            self.verifyEnumPropIfSupported(prop, [0, 1])

    def testGlobalEnumProperties(self):
        """Verifies that values of global enum properties are in the correct range"""
        enumProperties = {
            self.vtypes.VehicleProperty.ENGINE_OIL_LEVEL : self.vtypes.VehicleOilLevel,
            self.vtypes.VehicleProperty.GEAR_SELECTION : self.vtypes.VehicleGear,
            self.vtypes.VehicleProperty.CURRENT_GEAR : self.vtypes.VehicleGear,
            self.vtypes.VehicleProperty.TURN_SIGNAL_STATE : self.vtypes.VehicleTurnSignal,
            self.vtypes.VehicleProperty.IGNITION_STATE : self.vtypes.VehicleIgnitionState,
        }
        for prop, enum in enumProperties.iteritems():
            self.verifyEnumPropIfSupported(prop, vars(enum).values())

    def testDebugDump(self):
        """Verifies that call to IVehicle#debugDump is not failing"""
        dumpStr = self.vehicle.debugDump()
        asserts.assertNotEqual(None, dumpStr)

    def extractValue(self, propValue):
        """Extracts value depending on data type of the property"""
        if propValue == None:
            return None

        # Extract data type
        dataType = propValue['prop'] & self.vtypes.VehiclePropertyType.MASK
        val = propValue['value']
        if self.vtypes.VehiclePropertyType.STRING == dataType:
            asserts.assertNotEqual(None, val['stringValue'])
            return val['stringValue']
        elif self.vtypes.VehiclePropertyType.INT32 == dataType or \
                self.vtypes.VehiclePropertyType.BOOLEAN == dataType:
            asserts.assertEqual(1, len(val["int32Values"]))
            return val["int32Values"][0]
        elif self.vtypes.VehiclePropertyType.INT64 == dataType:
            asserts.assertEqual(1, len(val["int64Values"]))
            return val["int64Values"][0]
        elif self.vtypes.VehiclePropertyType.FLOAT == dataType:
            asserts.assertEqual(1, len(val["floatValues"]))
            return val["floatValues"][0]
        elif self.vtypes.VehiclePropertyType.INT32_VEC == dataType:
            asserts.assertLess(0, len(val["int32Values"]))
            return val["int32Values"]
        elif self.vtypes.VehiclePropertyType.FLOAT_VEC == dataType:
            asserts.assertLess(0, len(val["floatValues"]))
            return val["floatValues"]
        elif self.vtypes.VehiclePropertyType.BYTES == dataType:
            asserts.assertLess(0, len(val["bytes"]))
            return val["bytes"]
        else:
            return val

    def verifyEnumPropIfSupported(self, propertyId, validValues):
        """Verifies that if given property supported it is one of the value in validValues set"""
        supported, val = self.getValueIfPropSupported(propertyId)
        if supported:
            asserts.assertEqual(int, type(val))
            self.assertIntValueInRangeForProp(val, validValues, propertyId)

    def assertLessOrEqual(self, first, second, msg=None):
        """Asserts that first <= second"""
        if second < first:
            fullMsg = "%s is not less or equal to %s" % (first, second)
            if msg:
                fullMsg = "%s %s" % (fullMsg, msg)
            fail(fullMsg)

    def assertIntValueInRangeForProp(self, value, validValues, prop):
        """Asserts that given value is in the validValues range"""
        asserts.assertTrue(value in validValues,
                "Invalid value %d for property: 0x%x, expected one of: %s" % (value, prop, validValues))

    def assertValueInRangeForProp(self, value, rangeBegin, rangeEnd, prop):
        """Asserts that given value is in the range [rangeBegin, rangeEnd]"""
        msg = "Value %s is out of range [%s, %s] for property 0x%x" % (value, rangeBegin, rangeEnd, prop)
        self.assertLessOrEqual(rangeBegin, value, msg)
        self.assertLessOrEqual(value, rangeEnd,  msg)

    def getPropConfig(self, propertyId):
        return self.propToConfig[propertyId]

    def isPropSupported(self, propertyId):
        return self.getPropConfig(propertyId) is not None

    def testEngineOilTemp(self):
        """tests engine oil temperature.

        This also tests an HIDL async callback.
        """
        self.onPropertyEventCalled = 0
        self.onPropertySetCalled = 0
        self.onPropertySetErrorCalled = 0

        def onPropertyEvent(vehiclePropValues):
            logging.info("onPropertyEvent received: %s", vehiclePropValues)
            self.onPropertyEventCalled += 1

        def onPropertySet(vehiclePropValue):
            logging.info("onPropertySet notification received: %s", vehiclePropValue)
            self.onPropertySetCalled += 1

        def onPropertySetError(erroCode, propId, areaId):
            logging.info("onPropertySetError, error: %d, prop: 0x%x, area: 0x%x",
                         erroCode, prop, area)
            self.onPropertySetErrorCalled += 1

        config = self.getPropConfig(self.vtypes.VehicleProperty.ENGINE_OIL_TEMP)
        if (config is None):
            logging.info("ENGINE_OIL_TEMP property is not supported")
            return  # Property not supported, we are done here.

        propValue = self.readVhalProperty(self.vtypes.VehicleProperty.ENGINE_OIL_TEMP)
        asserts.assertEqual(1, len(propValue['value']['floatValues']))
        oilTemp = propValue['value']['floatValues'][0]
        logging.info("Current oil temperature: %f C", oilTemp)
        asserts.assertLess(oilTemp, 200)    # Check it is in reasinable range
        asserts.assertLess(-50, oilTemp)

        if (config["changeMode"] == self.vtypes.VehiclePropertyChangeMode.CONTINUOUS):
            logging.info("ENGINE_OIL_TEMP is continuous property, subscribing...")
            callback = self.vehicle.GetHidlCallbackInterface("IVehicleCallback",
                onPropertyEvent=onPropertyEvent,
                onPropertySet=onPropertySet,
                onPropertySetError=onPropertySetError)

            subscribeOptions = {
                "propId" : self.vtypes.VehicleProperty.ENGINE_OIL_TEMP,
                "sampleRate" : 10.0,  # Hz
                "flags" : self.vtypes.SubscribeFlags.EVENTS_FROM_CAR,
            }
            pbSubscribeOptions = self.vtypes.Py2Pb("SubscribeOptions", subscribeOptions)

            self.vehicle.subscribe(callback, [pbSubscribeOptions])
            for _ in range(5):
                if (self.onPropertyEventCalled > 0 or
                    self.onPropertySetCalled > 0 or
                    self.onPropertySetErrorCalled > 0):
                    return
                time.sleep(1)
            asserts.fail("Callback not called in 5 seconds.")

    def getDiagnosticSupportInfo(self):
        """Check which of the OBD2 diagnostic properties are supported."""
        properties = [self.vtypes.VehicleProperty.OBD2_LIVE_FRAME,
            self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME,
            self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_INFO,
            self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_CLEAR]
        return {x:self.isPropSupported(x) for x in properties}

    class CheckRead(object):
        """An object whose job it is to read a Vehicle HAL property and run
           routine validation checks on the result."""

        def __init__(self, test, propertyId, areaId=0):
            """Creates a CheckRead instance.

            Args:
                test: the containing testcase object.
                propertyId: the numeric identifier of the vehicle property.
            """
            self.test = test
            self.propertyId = propertyId
            self.areaId = 0

        def validateGet(self, status, value):
            """Validate the result of IVehicle.get.

            Args:
                status: the StatusCode returned from Vehicle HAL.
                value: the VehiclePropValue returned from Vehicle HAL.

            Returns: a VehiclePropValue instance, or None on failure."""
            asserts.assertEqual(self.test.vtypes.StatusCode.OK, status)
            asserts.assertNotEqual(value, None)
            asserts.assertEqual(self.propertyId, value['prop'])
            return value

        def prepareRequest(self, propValue):
            """Setup this request with any property-specific data.

            Args:
                propValue: a dictionary in the format of a VehiclePropValue.

            Returns: a dictionary in the format of a VehclePropValue."""
            return propValue

        def __call__(self):
            asserts.assertTrue(self.test.isPropSupported(self.propertyId), "error")
            request = {
                'prop' : self.propertyId,
                'timestamp' : 0,
                'areaId' : self.areaId,
                'status' : self.test.vtypes.VehiclePropertyStatus.AVAILABLE,
                'value' : {
                    'int32Values' : [],
                    'floatValues' : [],
                    'int64Values' : [],
                    'bytes' : [],
                    'stringValue' : ""
                }
            }
            request = self.prepareRequest(request)
            requestPropValue = self.test.vtypes.Py2Pb("VehiclePropValue",
                request)
            status, responsePropValue = self.test.vehicle.get(requestPropValue)
            return self.validateGet(status, responsePropValue)

    class CheckWrite(object):
        """An object whose job it is to write a Vehicle HAL property and run
           routine validation checks on the result."""

        def __init__(self, test, propertyId, areaId=0):
            """Creates a CheckWrite instance.

            Args:
                test: the containing testcase object.
                propertyId: the numeric identifier of the vehicle property.
                areaId: the numeric identifier of the vehicle area.
            """
            self.test = test
            self.propertyId = propertyId
            self.areaId = 0

        def validateSet(self, status):
            """Validate the result of IVehicle.set.
            Reading back the written-to property to ensure a consistent
            value is fair game for this method.

            Args:
                status: the StatusCode returned from Vehicle HAL.

            Returns: None."""
            asserts.assertEqual(self.test.vtypes.StatusCode.OK, status)

        def prepareRequest(self, propValue):
            """Setup this request with any property-specific data.

            Args:
                propValue: a dictionary in the format of a VehiclePropValue.

            Returns: a dictionary in the format of a VehclePropValue."""
            return propValue

        def __call__(self):
            asserts.assertTrue(self.test.isPropSupported(self.propertyId), "error")
            request = {
                'prop' : self.propertyId,
                'timestamp' : 0,
                'areaId' : self.areaId,
                'status' : self.test.vtypes.VehiclePropertyStatus.AVAILABLE,
                'value' : {
                    'int32Values' : [],
                    'floatValues' : [],
                    'int64Values' : [],
                    'bytes' : [],
                    'stringValue' : ""
                }
            }
            request = self.prepareRequest(request)
            requestPropValue = self.test.vtypes.Py2Pb("VehiclePropValue",
                request)
            status = self.test.vehicle.set(requestPropValue)
            return self.validateSet(status)

    def testReadObd2LiveFrame(self):
        """Test that one can correctly read the OBD2 live frame."""
        supportInfo = self.getDiagnosticSupportInfo()
        if supportInfo[self.vtypes.VehicleProperty.OBD2_LIVE_FRAME]:
            checkRead = self.CheckRead(self,
                self.vtypes.VehicleProperty.OBD2_LIVE_FRAME)
            checkRead()
        else:
            # live frame not supported by this HAL implementation. done
            logging.info("OBD2_LIVE_FRAME not supported.")

    def testReadObd2FreezeFrameInfo(self):
        """Test that one can read the list of OBD2 freeze timestamps."""
        supportInfo = self.getDiagnosticSupportInfo()
        if supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_INFO]:
            checkRead = self.CheckRead(self,
                self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_INFO)
            checkRead()
        else:
            # freeze frame info not supported by this HAL implementation. done
            logging.info("OBD2_FREEZE_FRAME_INFO not supported.")

    def testReadValidObd2FreezeFrame(self):
        """Test that one can read the OBD2 freeze frame data."""
        class FreezeFrameCheckRead(self.CheckRead):
            def __init__(self, test, timestamp):
                self.test = test
                self.propertyId = \
                    self.test.vtypes.VehicleProperty.OBD2_FREEZE_FRAME
                self.timestamp = timestamp
                self.areaId = 0

            def prepareRequest(self, propValue):
                propValue['value']['int64Values'] = [self.timestamp]
                return propValue

            def validateGet(self, status, value):
                # None is acceptable, as a newer fault could have overwritten
                # the one we're trying to read
                if value is not None:
                    asserts.assertEqual(self.test.vtypes.StatusCode.OK, status)
                    asserts.assertEqual(self.propertyId, value['prop'])
                    asserts.assertEqual(self.timestamp, value['timestamp'])

        supportInfo = self.getDiagnosticSupportInfo()
        if supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_INFO] \
            and supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME]:
            infoCheckRead = self.CheckRead(self,
                self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_INFO)
            frameInfos = infoCheckRead()
            timestamps = frameInfos["value"]["int64Values"]
            for timestamp in timestamps:
                freezeCheckRead = FreezeFrameCheckRead(self, timestamp)
                freezeCheckRead()
        else:
            # freeze frame not supported by this HAL implementation. done
            logging.info("OBD2_FREEZE_FRAME and _INFO not supported.")

    def testReadInvalidObd2FreezeFrame(self):
        """Test that trying to read freeze frame at invalid timestamps
            behaves correctly (i.e. returns an error code)."""
        class FreezeFrameCheckRead(self.CheckRead):
            def __init__(self, test, timestamp):
                self.test = test
                self.propertyId = self.test.vtypes.VehicleProperty.OBD2_FREEZE_FRAME
                self.timestamp = timestamp
                self.areaId = 0

            def prepareRequest(self, propValue):
                propValue['value']['int64Values'] = [self.timestamp]
                return propValue

            def validateGet(self, status, value):
                asserts.assertEqual(
                    self.test.vtypes.StatusCode.INVALID_ARG, status)

        supportInfo = self.getDiagnosticSupportInfo()
        invalidTimestamps = [0,482005800]
        if supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME]:
            for timestamp in invalidTimestamps:
                freezeCheckRead = FreezeFrameCheckRead(self, timestamp)
                freezeCheckRead()
        else:
            # freeze frame not supported by this HAL implementation. done
            logging.info("OBD2_FREEZE_FRAME not supported.")

    def testClearValidObd2FreezeFrame(self):
        """Test that deleting a diagnostic freeze frame works.
        Given the timing behavor of OBD2_FREEZE_FRAME, the only sensible
        definition of works here is that, after deleting a frame, trying to read
        at its timestamp, will not be successful."""
        class FreezeFrameClearCheckWrite(self.CheckWrite):
            def __init__(self, test, timestamp):
                self.test = test
                self.propertyId = self.test.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_CLEAR
                self.timestamp = timestamp
                self.areaId = 0

            def prepareRequest(self, propValue):
                propValue['value']['int64Values'] = [self.timestamp]
                return propValue

            def validateSet(self, status):
                asserts.assertTrue(status in [
                    self.test.vtypes.StatusCode.OK,
                    self.test.vtypes.StatusCode.INVALID_ARG], "error")

        class FreezeFrameCheckRead(self.CheckRead):
            def __init__(self, test, timestamp):
                self.test = test
                self.propertyId = \
                    self.test.vtypes.VehicleProperty.OBD2_FREEZE_FRAME
                self.timestamp = timestamp
                self.areaId = 0

            def prepareRequest(self, propValue):
                propValue['value']['int64Values'] = [self.timestamp]
                return propValue

            def validateGet(self, status, value):
                asserts.assertEqual(
                    self.test.vtypes.StatusCode.INVALID_ARG, status)

        supportInfo = self.getDiagnosticSupportInfo()
        if supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_INFO] \
            and supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME] \
            and supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_CLEAR]:
            infoCheckRead = self.CheckRead(self,
                self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_INFO)
            frameInfos = infoCheckRead()
            timestamps = frameInfos["value"]["int64Values"]
            for timestamp in timestamps:
                checkWrite = FreezeFrameClearCheckWrite(self, timestamp)
                checkWrite()
                checkRead = FreezeFrameCheckRead(self, timestamp)
                checkRead()
        else:
            # freeze frame not supported by this HAL implementation. done
            logging.info("OBD2_FREEZE_FRAME, _CLEAR and _INFO not supported.")

    def testClearInvalidObd2FreezeFrame(self):
        """Test that deleting an invalid freeze frame behaves correctly."""
        class FreezeFrameClearCheckWrite(self.CheckWrite):
            def __init__(self, test, timestamp):
                self.test = test
                self.propertyId = \
                    self.test.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_CLEAR
                self.timestamp = timestamp
                self.areaId = 0

            def prepareRequest(self, propValue):
                propValue['value']['int64Values'] = [self.timestamp]
                return propValue

            def validateSet(self, status):
                asserts.assertEqual(self.test.vtypes.StatusCode.INVALID_ARG,
                    status, "PropId: 0x%s, Timestamp: %d" % (self.propertyId, self.timestamp))

        supportInfo = self.getDiagnosticSupportInfo()
        if supportInfo[self.vtypes.VehicleProperty.OBD2_FREEZE_FRAME_CLEAR]:
            invalidTimestamps = [0,482005800]
            for timestamp in invalidTimestamps:
                checkWrite = FreezeFrameClearCheckWrite(self, timestamp)
                checkWrite()
        else:
            # freeze frame not supported by this HAL implementation. done
            logging.info("OBD2_FREEZE_FRAME_CLEAR not supported.")

if __name__ == "__main__":
    test_runner.main()
