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
# Aware power settings values for interactive (high power) and
# non-interactive (low power) modes
######################################################

POWER_DW_24_INTERACTIVE = 1
POWER_DW_5_INTERACTIVE = 1
POWER_DISC_BEACON_INTERVAL_INTERACTIVE = 0
POWER_NUM_SS_IN_DISC_INTERACTIVE = 0
POWER_ENABLE_DW_EARLY_TERM_INTERACTIVE = 0

POWER_DW_24_NON_INTERACTIVE = 4
POWER_DW_5_NON_INTERACTIVE = 0
POWER_DISC_BEACON_INTERVAL_NON_INTERACTIVE = 0
POWER_NUM_SS_IN_DISC_NON_INTERACTIVE = 0
POWER_ENABLE_DW_EARLY_TERM_NON_INTERACTIVE = 0

######################################################
# Broadcast events
######################################################
BROADCAST_WIFI_AWARE_AVAILABLE = "WifiAwareAvailable"
BROADCAST_WIFI_AWARE_NOT_AVAILABLE = "WifiAwareNotAvailable"

######################################################
# ConfigRequest keys
######################################################

CONFIG_KEY_5G_BAND = "Support5gBand"
CONFIG_KEY_MASTER_PREF = "MasterPreference"
CONFIG_KEY_CLUSTER_LOW = "ClusterLow"
CONFIG_KEY_CLUSTER_HIGH = "ClusterHigh"
CONFIG_KEY_ENABLE_IDEN_CB = "EnableIdentityChangeCallback"

######################################################
# Publish & Subscribe Config keys
######################################################

DISCOVERY_KEY_SERVICE_NAME = "ServiceName"
DISCOVERY_KEY_SSI = "ServiceSpecificInfo"
DISCOVERY_KEY_MATCH_FILTER = "MatchFilter"
DISCOVERY_KEY_MATCH_FILTER_LIST = "MatchFilterList"
DISCOVERY_KEY_DISCOVERY_TYPE = "DiscoveryType"
DISCOVERY_KEY_TTL = "TtlSec"
DISCOVERY_KEY_TERM_CB_ENABLED = "TerminateNotificationEnabled"
DISCOVERY_KEY_RANGING_ENABLED = "RangingEnabled"
DISCOVERY_KEY_MIN_DISTANCE_MM = "MinDistanceMm"
DISCOVERY_KEY_MAX_DISTANCE_MM = "MaxDistanceMm"

PUBLISH_TYPE_UNSOLICITED = 0
PUBLISH_TYPE_SOLICITED = 1

SUBSCRIBE_TYPE_PASSIVE = 0
SUBSCRIBE_TYPE_ACTIVE = 1

######################################################
# WifiAwareAttachCallback events
######################################################
EVENT_CB_ON_ATTACHED = "WifiAwareOnAttached"
EVENT_CB_ON_ATTACH_FAILED = "WifiAwareOnAttachFailed"

######################################################
# WifiAwareIdentityChangedListener events
######################################################
EVENT_CB_ON_IDENTITY_CHANGED = "WifiAwareOnIdentityChanged"

# WifiAwareAttachCallback & WifiAwareIdentityChangedListener events keys
EVENT_CB_KEY_REASON = "reason"
EVENT_CB_KEY_MAC = "mac"
EVENT_CB_KEY_LATENCY_MS = "latencyMs"
EVENT_CB_KEY_TIMESTAMP_MS = "timestampMs"

######################################################
# WifiAwareDiscoverySessionCallback events
######################################################
SESSION_CB_ON_PUBLISH_STARTED = "WifiAwareSessionOnPublishStarted"
SESSION_CB_ON_SUBSCRIBE_STARTED = "WifiAwareSessionOnSubscribeStarted"
SESSION_CB_ON_SESSION_CONFIG_UPDATED = "WifiAwareSessionOnSessionConfigUpdated"
SESSION_CB_ON_SESSION_CONFIG_FAILED = "WifiAwareSessionOnSessionConfigFailed"
SESSION_CB_ON_SESSION_TERMINATED = "WifiAwareSessionOnSessionTerminated"
SESSION_CB_ON_SERVICE_DISCOVERED = "WifiAwareSessionOnServiceDiscovered"
SESSION_CB_ON_MESSAGE_SENT = "WifiAwareSessionOnMessageSent"
SESSION_CB_ON_MESSAGE_SEND_FAILED = "WifiAwareSessionOnMessageSendFailed"
SESSION_CB_ON_MESSAGE_RECEIVED = "WifiAwareSessionOnMessageReceived"

# WifiAwareDiscoverySessionCallback events keys
SESSION_CB_KEY_CB_ID = "callbackId"
SESSION_CB_KEY_SESSION_ID = "discoverySessionId"
SESSION_CB_KEY_REASON = "reason"
SESSION_CB_KEY_PEER_ID = "peerId"
SESSION_CB_KEY_SERVICE_SPECIFIC_INFO = "serviceSpecificInfo"
SESSION_CB_KEY_MATCH_FILTER = "matchFilter"
SESSION_CB_KEY_MATCH_FILTER_LIST = "matchFilterList"
SESSION_CB_KEY_MESSAGE = "message"
SESSION_CB_KEY_MESSAGE_ID = "messageId"
SESSION_CB_KEY_MESSAGE_AS_STRING = "messageAsString"
SESSION_CB_KEY_LATENCY_MS = "latencyMs"
SESSION_CB_KEY_TIMESTAMP_MS = "timestampMs"
SESSION_CB_KEY_DISTANCE_MM = "distanceMm"

######################################################
# WifiAwareRangingListener events (RttManager.RttListener)
######################################################
RTT_LISTENER_CB_ON_SUCCESS = "WifiAwareRangingListenerOnSuccess"
RTT_LISTENER_CB_ON_FAILURE = "WifiAwareRangingListenerOnFailure"
RTT_LISTENER_CB_ON_ABORT = "WifiAwareRangingListenerOnAborted"

# WifiAwareRangingListener events (RttManager.RttListener) keys
RTT_LISTENER_CB_KEY_CB_ID = "callbackId"
RTT_LISTENER_CB_KEY_SESSION_ID = "sessionId"
RTT_LISTENER_CB_KEY_RESULTS = "Results"
RTT_LISTENER_CB_KEY_REASON = "reason"
RTT_LISTENER_CB_KEY_DESCRIPTION = "description"

######################################################
# Capabilities keys
######################################################

CAP_MAX_CONCURRENT_AWARE_CLUSTERS = "maxConcurrentAwareClusters"
CAP_MAX_PUBLISHES = "maxPublishes"
CAP_MAX_SUBSCRIBES = "maxSubscribes"
CAP_MAX_SERVICE_NAME_LEN = "maxServiceNameLen"
CAP_MAX_MATCH_FILTER_LEN = "maxMatchFilterLen"
CAP_MAX_TOTAL_MATCH_FILTER_LEN = "maxTotalMatchFilterLen"
CAP_MAX_SERVICE_SPECIFIC_INFO_LEN = "maxServiceSpecificInfoLen"
CAP_MAX_EXTENDED_SERVICE_SPECIFIC_INFO_LEN = "maxExtendedServiceSpecificInfoLen"
CAP_MAX_NDI_INTERFACES = "maxNdiInterfaces"
CAP_MAX_NDP_SESSIONS = "maxNdpSessions"
CAP_MAX_APP_INFO_LEN = "maxAppInfoLen"
CAP_MAX_QUEUED_TRANSMIT_MESSAGES = "maxQueuedTransmitMessages"
CAP_MAX_SUBSCRIBE_INTERFACE_ADDRESSES = "maxSubscribeInterfaceAddresses"
CAP_SUPPORTED_CIPHER_SUITES = "supportedCipherSuites"

######################################################

# Aware NDI (NAN data-interface) name prefix
AWARE_NDI_PREFIX = "aware_data"

# Aware discovery channels
AWARE_DISCOVERY_CHANNEL_24_BAND = 6
AWARE_DISCOVERY_CHANNEL_5_BAND = 149

# Aware Data-Path Constants
DATA_PATH_INITIATOR = 0
DATA_PATH_RESPONDER = 1

# Maximum send retry
MAX_TX_RETRIES = 5

# Callback keys (for 'adb shell cmd wifiaware native_cb get_cb_count')
CB_EV_CLUSTER = "0"
CB_EV_DISABLED = "1"
CB_EV_PUBLISH_TERMINATED = "2"
CB_EV_SUBSCRIBE_TERMINATED = "3"
CB_EV_MATCH = "4"
CB_EV_MATCH_EXPIRED = "5"
CB_EV_FOLLOWUP_RECEIVED = "6"
CB_EV_TRANSMIT_FOLLOWUP = "7"
CB_EV_DATA_PATH_REQUEST = "8"
CB_EV_DATA_PATH_CONFIRM = "9"
CB_EV_DATA_PATH_TERMINATED = "10"
