#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

######################################################
# NsdManager.RegistrationListener events
######################################################
REG_LISTENER_EVENT = "NsdRegistrationListener"

# event type - using REG_LISTENER_CALLBACK
REG_LISTENER_EVENT_ON_REG_FAILED = "OnRegistrationFailed"
REG_LISTENER_EVENT_ON_SERVICE_REGISTERED = "OnServiceRegistered"
REG_LISTENER_EVENT_ON_SERVICE_UNREG = "OnServiceUnregistered"
REG_LISTENER_EVENT_ON_UNREG_FAILED = "OnUnregistrationFailed"

# event data keys
REG_LISTENER_DATA_ID = "id"
REG_LISTENER_CALLBACK = "callback"
REG_LISTENER_ERROR_CODE = "error_code"

######################################################
# NsdManager.DiscoveryListener events
######################################################
DISCOVERY_LISTENER_EVENT = "NsdDiscoveryListener"

# event type - using DISCOVERY_LISTENER_DATA_CALLBACK
DISCOVERY_LISTENER_EVENT_ON_DISCOVERY_STARTED = "OnDiscoveryStarted"
DISCOVERY_LISTENER_EVENT_ON_DISCOVERY_STOPPED = "OnDiscoveryStopped"
DISCOVERY_LISTENER_EVENT_ON_SERVICE_FOUND = "OnServiceFound"
DISCOVERY_LISTENER_EVENT_ON_SERVICE_LOST = "OnServiceLost"
DISCOVERY_LISTENER_EVENT_ON_START_DISCOVERY_FAILED = "OnStartDiscoveryFailed"
DISCOVERY_LISTENER_EVENT_ON_STOP_DISCOVERY_FAILED = "OnStopDiscoveryFailed"

# event data keys
DISCOVERY_LISTENER_DATA_ID = "id"
DISCOVERY_LISTENER_DATA_CALLBACK = "callback"
DISCOVERY_LISTENER_DATA_SERVICE_TYPE = "service_type"
DISCOVERY_LISTENER_DATA_ERROR_CODE = "error_code"

######################################################
# NsdManager.ResolveListener events
######################################################
RESOLVE_LISTENER_EVENT = "NsdResolveListener"

# event type using RESOLVE_LISTENER_DATA_CALLBACK
RESOLVE_LISTENER_EVENT_ON_RESOLVE_FAIL = "OnResolveFail"
RESOLVE_LISTENER_EVENT_ON_SERVICE_RESOLVED = "OnServiceResolved"

# event data keys
RESOLVE_LISTENER_DATA_ID = "id"
RESOLVE_LISTENER_DATA_CALLBACK = "callback"
RESOLVE_LISTENER_DATA_ERROR_CODE = "error_code"

######################################################
# NsdServiceInfo elements
######################################################
NSD_SERVICE_INFO_HOST = "serviceInfoHost"
NSD_SERVICE_INFO_PORT = "serviceInfoPort"
NSD_SERVICE_INFO_SERVICE_NAME = "serviceInfoServiceName"
NSD_SERVICE_INFO_SERVICE_TYPE = "serviceInfoServiceType"