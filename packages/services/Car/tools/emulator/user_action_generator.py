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

import time

import vhal_consts_2_0 as c

# A list of user actions as a subset of VHAL properties to simulate
userActions = [
    c.VEHICLEPROPERTY_HVAC_POWER_ON,
    c.VEHICLEPROPERTY_HVAC_TEMPERATURE_SET,
    c.VEHICLEPROPERTY_HVAC_FAN_SPEED,
    c.VEHICLEPROPERTY_HVAC_FAN_DIRECTION,
    c.VEHICLEPROPERTY_HVAC_AUTO_ON,
    c.VEHICLEPROPERTY_HVAC_RECIRC_ON,
    c.VEHICLEPROPERTY_HVAC_AC_ON,
    c.VEHICLEPROPERTY_HVAC_DEFROSTER
]

propDesc = {
    c.VEHICLEPROPERTY_HVAC_POWER_ON: 'HVAC_POWER_ON',
    c.VEHICLEPROPERTY_HVAC_TEMPERATURE_SET: 'HVAC_TEMPERATURE_SET',
    c.VEHICLEPROPERTY_HVAC_FAN_SPEED: 'HVAC_FAN_SPEED',
    c.VEHICLEPROPERTY_HVAC_FAN_DIRECTION: 'HVAC_FAN_DIRECTION',
    c.VEHICLEPROPERTY_HVAC_AUTO_ON: 'HVAC_AUTO_ON',
    c.VEHICLEPROPERTY_HVAC_RECIRC_ON: 'HVAC_AUTO_RECIRC_ON',
    c.VEHICLEPROPERTY_HVAC_AC_ON: 'HVAC_AC_ON',
    c.VEHICLEPROPERTY_HVAC_DEFROSTER: 'HVAC_DEFROSTER'
}

SHORT_SLEEP_TIME_SEC = 2
LONG_SLEEP_TIME_SEC = 30

MIN_TEMPERATURE_C = 22
MAX_TEMPERATURE_C = 28

MIN_FAN_SPEED = 1
MAX_FAN_SPEED = 5

class ActionPropConfig(object):
    """
        A configuration class that parse the vhal property config message and hold the
        configuration values
    """
    def __init__(self, vhalConfig):
        self.supportedAreas = self._getSupportedAreas(vhalConfig)
        self.areaConfigs = self._getAreaConfigs(vhalConfig, self.supportedAreas,
                                                vhalConfig.value_type)

    def _getSupportedAreas(self, vhalConfig):
        supportedAreas = []
        for config in vhalConfig.area_configs:
            supportedAreas.append(config.area_id)

        return supportedAreas

    def _getAreaConfigs(self, vhalConfig, supportedAreas, valueType):
        areaConfigs = {}
        configs = vhalConfig.area_configs
        if not configs:
            return None

        if len(configs) == 1:
            for area in supportedAreas:
                areaConfigs[area] = AreaConfig(configs[0], valueType)
        else:
            for config in configs:
                areaConfigs[config.area_id] = AreaConfig(config, valueType)

        return areaConfigs

class AreaConfig(object):
    """
        A configuration class is representing an area config of a vhal property.
    """
    def __init__(self, vhalAreaConfig, valueType):
        """
            The class is initialized by parsing the vhal area config object
        """
        if valueType == c.VEHICLEPROPERTYTYPE_INT32:
            self.min = vhalAreaConfig.min_int32_value
            self.max = vhalAreaConfig.max_int32_value
        elif valueType == c.VEHICLEPROPERTYTYPE_INT64:
            self.min = vhalAreaConfig.min_int64_value
            self.max = vhalAreaConfig.max_int64_value
        elif valueType == c.VEHICLEPROPERTYTYPE_FLOAT:
            self.min = vhalAreaConfig.min_float_value
            self.max = vhalAreaConfig.max_float_value
        else:
            self.min = None
            self.max = None

class UserActionGenerator(object):
    """
        A class generate user action related vhal properties in a deterministic algorithm based on
        pre-fetched vhal configuration
    """
    def __init__(self, vhal):
        self.configs = self._getConfig(vhal)


    def _getConfig(self, vhal):
        """
            Get vhal configuration for properties that need to be simulated
        """
        vhalConfig = {}
        for actionProp in userActions:
            vhal.getConfig(actionProp)
            vhalConfig[actionProp] = ActionPropConfig(vhal.rxMsg().config[0])
        return vhalConfig

    def _adjustContinuousProperty(self, prop, begin, end, listener):
        """
            The method generate continuous property value from "begin" value to "end" value
            (exclusive).
        """
        config = self.configs[prop]
        for area in config.supportedAreas:
            areaConfig = config.areaConfigs[area]
            if begin < end:
                begin = areaConfig.min if begin < areaConfig.min else begin
                end = areaConfig.max if end > areaConfig.max else end
            else:
                begin = areaConfig.max if begin > areaConfig.max else begin
                end = areaConfig.min if end < areaConfig.min else end

            for value in self._range(begin, end):
                listener.handle(prop, area, value, propDesc[prop])
                time.sleep(0.2)

    def _setProperty(self, prop, value, listener):
        """
            This method generates single property value (e.g. boolean or integer value)
        """
        config = self.configs[prop]
        for area in config.supportedAreas:
            listener.handle(prop, area, value, propDesc[prop])
            time.sleep(1)

    def _range(self, begin, end):
        if begin < end:
            i = begin
            while i < end:
                yield i
                i += 1
        else:
            i = begin
            while i > end:
                yield i
                i += -1

    def generate(self, listener):
        """
            Main method that simulate user in-car actions such as HVAC
        """

        # Turn on HVAC only for front seats
        listener.handle(c.VEHICLEPROPERTY_HVAC_POWER_ON, c.VEHICLEAREASEAT_ROW_1_LEFT,
                        1, 'HVAC_POWER_ON')
        listener.handle(c.VEHICLEPROPERTY_HVAC_POWER_ON, c.VEHICLEAREASEAT_ROW_1_RIGHT,
                        1, 'HVAC_POWER_ON')
        time.sleep(SHORT_SLEEP_TIME_SEC)

        while True:

            self._setProperty(c.VEHICLEPROPERTY_HVAC_AC_ON, 1, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)

            self._setProperty(c.VEHICLEPROPERTY_HVAC_FAN_DIRECTION,
                              c.VEHICLEHVACFANDIRECTION_FACE, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)

            self._adjustContinuousProperty(c.VEHICLEPROPERTY_HVAC_TEMPERATURE_SET,
                                           MAX_TEMPERATURE_C, MIN_TEMPERATURE_C, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)

            self._adjustContinuousProperty(c.VEHICLEPROPERTY_HVAC_FAN_SPEED,
                                           MIN_FAN_SPEED, MAX_FAN_SPEED, listener)
            time.sleep(LONG_SLEEP_TIME_SEC)

            self._setProperty(c.VEHICLEPROPERTY_HVAC_AUTO_ON, 1, listener)
            time.sleep(LONG_SLEEP_TIME_SEC)

            self._setProperty(c.VEHICLEPROPERTY_HVAC_AUTO_ON, 0, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)

            self._setProperty(c.VEHICLEPROPERTY_HVAC_FAN_DIRECTION,
                              c.VEHICLEHVACFANDIRECTION_FACE, listener)
            time.sleep(LONG_SLEEP_TIME_SEC)

            self._setProperty(c.VEHICLEPROPERTY_HVAC_DEFROSTER, 1, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)

            self._setProperty(c.VEHICLEPROPERTY_HVAC_FAN_DIRECTION,
                              c.VEHICLEHVACFANDIRECTION_DEFROST, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)

            self._adjustContinuousProperty(c.VEHICLEPROPERTY_HVAC_TEMPERATURE_SET,
                                           MIN_TEMPERATURE_C, MAX_TEMPERATURE_C, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)

            self._adjustContinuousProperty(c.VEHICLEPROPERTY_HVAC_FAN_SPEED,
                                           MAX_FAN_SPEED, MIN_FAN_SPEED, listener)
            time.sleep(LONG_SLEEP_TIME_SEC)

            self._setProperty(c.VEHICLEPROPERTY_HVAC_AC_ON, 0, listener)
            time.sleep(SHORT_SLEEP_TIME_SEC)
