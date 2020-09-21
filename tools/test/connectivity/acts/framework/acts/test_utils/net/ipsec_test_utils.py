#
#   Copyright 2018 - The Android Open Source Project
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

import binascii
import os
import random
import re
import threading
import time

from acts.test_utils.net import connectivity_const as cconst
from acts import asserts

PKTS = 5

def make_key(len_bits):
    asserts.assert_true(
        len_bits % 8 == 0, "Unexpected key length. Should be a multiple "
        "of 8, got %s" % len_bits)
    return binascii.hexlify(os.urandom(int(len_bits/8))).decode()

def allocate_spis(ad, ip_a, ip_b, in_spi = None, out_spi = None):
    """ Allocate in and out SPIs for android device

    Args:
      1. ad : android device object
      2. ip_a : local IP address for In SPI
      3. ip_b : remote IP address for Out SPI
      4. in_spi : Generate In SPI with this value
      5. out_spi : Generate Out SPI with this value

    Returns:
      List of In and Out SPI
    """
    in_spi_key = ad.droid.ipSecAllocateSecurityParameterIndex(ip_a, in_spi)
    in_spi = ad.droid.ipSecGetSecurityParameterIndex(in_spi_key)
    ad.log.info("In SPI: %s" % hex(in_spi))

    out_spi_key = ad.droid.ipSecAllocateSecurityParameterIndex(ip_b, out_spi)
    out_spi = ad.droid.ipSecGetSecurityParameterIndex(out_spi_key)
    ad.log.info("Out SPI: %s" % hex(out_spi))

    asserts.assert_true(in_spi and out_spi, "Failed to allocate SPIs")
    return [in_spi_key, out_spi_key]

def release_spis(ad, spis):
    """ Destroy SPIs

    Args:
      1. ad : android device object
      2. spis : list of SPI keys to destroy
    """
    for spi_key in spis:
        ad.droid.ipSecReleaseSecurityParameterIndex(spi_key)
        spi = ad.droid.ipSecGetSecurityParameterIndex(spi_key)
        asserts.assert_true(not spi, "Failed to release SPI")

def create_transport_mode_transforms(ad,
                                     spis,
                                     ip_a,
                                     ip_b,
                                     crypt_algo,
                                     crypt_key,
                                     auth_algo,
                                     auth_key,
                                     trunc_bit,
                                     udp_encap_sock=None):
    """ Create transport mode transforms on the device

    Args:
      1. ad : android device object
      2. spis : spi keys of the SPIs created
      3. ip_a : local IP addr
      4. ip_b : remote IP addr
      5. crypt_key : encryption key
      6. auth_key : authentication key
      7. udp_encap_sock : set udp encapsulation for ESP packets

    Returns:
      List of In and Out Transforms
    """
    in_transform = ad.droid.ipSecCreateTransportModeTransform(
        crypt_algo, crypt_key, auth_algo, auth_key, trunc_bit, spis[0],
        ip_b, udp_encap_sock)
    ad.log.info("In Transform: %s" % in_transform)
    out_transform = ad.droid.ipSecCreateTransportModeTransform(
        crypt_algo, crypt_key, auth_algo, auth_key, trunc_bit, spis[1],
        ip_a, udp_encap_sock)
    ad.log.info("Out Transform: %s" % out_transform)
    asserts.assert_true(in_transform and out_transform,
                        "Failed to create transforms")
    return [in_transform, out_transform]

def destroy_transport_mode_transforms(ad, transforms):
    """ Destroy transforms on the device

    Args:
      1. ad : android device object
      2. transforms : list to transform keys to destroy
    """
    for transform in transforms:
        ad.droid.ipSecDestroyTransportModeTransform(transform)
        status = ad.droid.ipSecGetTransformStatus(transform)
        ad.log.info("Transform status: %s" % status)
        asserts.assert_true(not status, "Failed to destroy transform")

def apply_transport_mode_transforms_file_descriptors(ad, fd, transforms):
    """ Apply transpot mode transform to FileDescriptor object

    Args:
      1. ad - android device object
      2. fd - FileDescriptor key
      3. transforms - list of in and out transforms
    """
    in_transform = ad.droid.ipSecApplyTransportModeTransformFileDescriptor(
        fd, cconst.DIRECTION_IN, transforms[0])
    out_transform = ad.droid.ipSecApplyTransportModeTransformFileDescriptor(
        fd, cconst.DIRECTION_OUT, transforms[1])
    asserts.assert_true(in_transform and out_transform,
                        "Failed to apply transform")
    ip_xfrm_state = ad.adb.shell("ip -s xfrm state")
    ad.log.info("XFRM STATE:\n%s\n" % ip_xfrm_state)
    ip_xfrm_policy = ad.adb.shell("ip -s xfrm policy")
    ad.log.info("XFRM POLICY:\n%s\n" % ip_xfrm_policy)

def remove_transport_mode_transforms_file_descriptors(ad, fd):
    """ Remove transport mode transform from FileDescriptor object

    Args:
      1. ad - android device object
      2. socket - FileDescriptor key
    """
    status = ad.droid.ipSecRemoveTransportModeTransformsFileDescriptor(fd)
    asserts.assert_true(status, "Failed to remove transform")

def apply_transport_mode_transforms_datagram_socket(ad, socket, transforms):
    """ Apply transport mode transform to DatagramSocket object

    Args:
      1. ad - android device object
      2. socket - DatagramSocket object key
      3. transforms - list of in and out transforms
    """
    in_tfrm_status = ad.droid.ipSecApplyTransportModeTransformDatagramSocket(
        socket, cconst.DIRECTION_IN, transforms[0])
    out_tfrm_status = ad.droid.ipSecApplyTransportModeTransformDatagramSocket(
        socket, cconst.DIRECTION_OUT, transforms[1])
    asserts.assert_true(in_tfrm_status and out_tfrm_status,
                        "Failed to apply transform")

    ip_xfrm_state = ad.adb.shell("ip -s xfrm state")
    ad.log.info("XFRM STATE:\n%s\n" % ip_xfrm_state)

def remove_transport_mode_transforms_datagram_socket(ad, socket):
    """ Remove transport mode transform from DatagramSocket object

    Args:
      1. ad - android device object
      2. socket - DatagramSocket object key
    """
    status = ad.droid.ipSecRemoveTransportModeTransformsDatagramSocket(socket)
    asserts.assert_true(status, "Failed to remove transform")

def apply_transport_mode_transforms_socket(ad, socket, transforms):
    """ Apply transport mode transform to Socket object

    Args:
      1. ad - android device object
      2. socket - Socket object key
      3. transforms - list of in and out transforms
    """
    in_tfrm_status = ad.droid.ipSecApplyTransportModeTransformSocket(
        socket, cconst.DIRECTION_IN, transforms[0])
    out_tfrm_status = ad.droid.ipSecApplyTransportModeTransformSocket(
        socket, cconst.DIRECTION_OUT, transforms[1])
    asserts.assert_true(in_tfrm_status and out_tfrm_status,
                        "Failed to apply transform")

    ip_xfrm_state = ad.adb.shell("ip -s xfrm state")
    ad.log.info("XFRM STATE:\n%s\n" % ip_xfrm_state)

def remove_transport_mode_transforms_socket(ad, socket):
    """ Remove transport mode transform from Socket object

    Args:
      1. ad - android device object
      2. socket - Socket object key
    """
    status = ad.droid.ipSecRemoveTransportModeTransformsSocket(socket)
    asserts.assert_true(status, "Failed to remove transform")

def verify_esp_packets(ads):
    """ Verify that encrypted ESP packets are sent

    Args:
      1. ads - Verify ESP packets on all devices
    """
    for ad in ads:
        ip_xfrm_state = ad.adb.shell("ip -s xfrm state")
        ad.log.info("XFRM STATE on %s:\n%s\n" % (ad.serial, ip_xfrm_state))
        pattern = re.findall(r'\d+\(packets\)', ip_xfrm_state)
        esp_pkts = False
        for _ in pattern:
            if int(_.split('(')[0]) >= PKTS:
                esp_pkts = True
                break
        asserts.assert_true(esp_pkts, "Could not find ESP pkts")

def generate_random_crypt_auth_combo():
    """ Generate every possible combination of crypt and auth keys,
        auth algo, trunc bits supported by IpSecManager
    """
    crypt_key_length = [128, 192, 256]
    auth_method_key = { cconst.AUTH_HMAC_MD5 : 128,
                        cconst.AUTH_HMAC_SHA1 : 160,
                        cconst.AUTH_HMAC_SHA256 : 256,
                        cconst.AUTH_HMAC_SHA384 : 384,
                        cconst.AUTH_HMAC_SHA512 : 512 }
    auth_method_trunc = { cconst.AUTH_HMAC_MD5 : list(range(96, 136, 8)),
                          cconst.AUTH_HMAC_SHA1 : list(range(96, 168, 8)),
                          cconst.AUTH_HMAC_SHA256 : list(range(96, 264, 8)),
                          cconst.AUTH_HMAC_SHA384 : list(range(192, 392, 8)),
                          cconst.AUTH_HMAC_SHA512 : list(range(256, 520, 8)) }
    return_list = []
    for c in crypt_key_length:
        for k in auth_method_key.keys():
            auth_key = auth_method_key[k]
            lst = auth_method_trunc[k]
            for t in lst:
                combo = []
                combo.append(c)
                combo.append(k)
                combo.append(auth_key)
                combo.append(t)
                return_list.append(combo)

    return return_list
