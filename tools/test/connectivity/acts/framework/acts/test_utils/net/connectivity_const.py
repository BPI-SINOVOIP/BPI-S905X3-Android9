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

import enum

######################################################
# ConnectivityManager.NetworkCallback events
######################################################
EVENT_NETWORK_CALLBACK = "NetworkCallback"

# event types
NETWORK_CB_PRE_CHECK = "PreCheck"
NETWORK_CB_AVAILABLE = "Available"
NETWORK_CB_LOSING = "Losing"
NETWORK_CB_LOST = "Lost"
NETWORK_CB_UNAVAILABLE = "Unavailable"
NETWORK_CB_CAPABILITIES_CHANGED = "CapabilitiesChanged"
NETWORK_CB_SUSPENDED = "Suspended"
NETWORK_CB_RESUMED = "Resumed"
NETWORK_CB_LINK_PROPERTIES_CHANGED = "LinkPropertiesChanged"
NETWORK_CB_INVALID = "Invalid"

# event data keys
NETWORK_CB_KEY_ID = "id"
NETWORK_CB_KEY_EVENT = "networkCallbackEvent"
NETWORK_CB_KEY_MAX_MS_TO_LIVE = "maxMsToLive"
NETWORK_CB_KEY_RSSI = "rssi"
NETWORK_CB_KEY_INTERFACE_NAME = "interfaceName"
NETWORK_CB_KEY_CREATE_TS = "creation_timestamp"
NETWORK_CB_KEY_CURRENT_TS = "current_timestamp"

# Constants for VPN connection status
VPN_STATE_DISCONNECTED = 0
VPN_STATE_INITIALIZING = 1
VPN_STATE_CONNECTING = 2
VPN_STATE_CONNECTED = 3
VPN_STATE_TIMEOUT = 4
VPN_STATE_FAILED = 5
# TODO gmoturu: determine the exact timeout value
# This is a random value as of now
VPN_TIMEOUT = 15

# Connectiivty Manager constants
TYPE_MOBILE = 0
TYPE_WIFI = 1

# Multipath preference constants
MULTIPATH_PREFERENCE_NONE = 0
MULTIPATH_PREFERENCE_HANDOVER = 1 << 0
MULTIPATH_PREFERENCE_RELIABILITY = 1 << 1
MULTIPATH_PREFERENCE_PERFORMANCE = 1 << 2

# IpSec constants
SOCK_STREAM = 1
SOCK_DGRAM = 2
AF_INET = 2
AF_INET6 = 10
DIRECTION_IN = 0
DIRECTION_OUT = 1
MODE_TRANSPORT = 0
MODE_TUNNEL = 1
CRYPT_NULL = "ecb(cipher_null)"
CRYPT_AES_CBC = "cbc(aes)"
AUTH_HMAC_MD5 = "hmac(md5)"
AUTH_HMAC_SHA1 = "hmac(sha1)"
AUTH_HMAC_SHA256 = "hmac(sha256)"
AUTH_HMAC_SHA384 = "hmac(sha384)"
AUTH_HMAC_SHA512 = "hmac(sha512)"
AUTH_CRYPT_AES_GCM = "rfc4106(gcm(aes))"

# Constants for VpnProfile
class VpnProfile(object):
    """ This class contains all the possible
        parameters required for VPN connection
    """
    NAME = "name"
    TYPE = "type"
    SERVER = "server"
    USER = "username"
    PWD = "password"
    DNS = "dnsServers"
    SEARCH_DOMAINS = "searchDomains"
    ROUTES = "routes"
    MPPE = "mppe"
    L2TP_SECRET = "l2tpSecret"
    IPSEC_ID = "ipsecIdentifier"
    IPSEC_SECRET = "ipsecSecret"
    IPSEC_USER_CERT = "ipsecUserCert"
    IPSEC_CA_CERT = "ipsecCaCert"
    IPSEC_SERVER_CERT = "ipsecServerCert"

# Enums for VPN profile types
class VpnProfileType(enum.Enum):
    """ Integer constant for each type of VPN
    """
    PPTP = 0
    L2TP_IPSEC_PSK = 1
    L2TP_IPSEC_RSA = 2
    IPSEC_XAUTH_PSK = 3
    IPSEC_XAUTH_RSA = 4
    IPSEC_HYBRID_RSA = 5

# Constants for config file
class VpnReqParams(object):
    """ Config file parameters required for
        VPN connection
    """
    wifi_network = "wifi_network"
    vpn_server_addresses = "vpn_server_addresses"
    vpn_verify_address = "vpn_verify_address"
    vpn_username = "vpn_username"
    vpn_password = "vpn_password"
    psk_secret = "psk_secret"
    client_pkcs_file_name = "client_pkcs_file_name"
    cert_path_vpnserver = "cert_path_vpnserver"
    cert_password = "cert_password"
    pptp_mppe = "pptp_mppe"
    ipsec_server_type = "ipsec_server_type"
