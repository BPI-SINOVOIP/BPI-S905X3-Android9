#
# Copyright 2017 The Android Open Source Project
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
"""Methods to call native OpenSSL APIs to support P256 with compression.

Since there are no Python crypto libraries that support P256 EC keys with
with X9.62 compression, this module uses the native OpenSSL APIs to support
key generation and deriving a shared secret using generated P256 EC keys.
"""

from ctypes import byref
from ctypes import c_int
from ctypes import c_ubyte
from ctypes import c_uint
from ctypes import cdll
from ctypes import create_string_buffer
from ctypes import POINTER
from ctypes.util import find_library

_ECDH_KEY_LEN = 33


def _ec_helper_native():
  """Loads the ec_helper_native library.

  Returns:
    The ctypes ec_helper_native library.
  """
  cdll.LoadLibrary(find_library('crypto'))
  cdll.LoadLibrary(find_library('ssl'))
  return cdll.LoadLibrary('./ec_helper_native.so')


def generate_p256_key():
  """Geneates a new p256 key pair.

  Raises:
    RuntimeError: Generating a new key fails.

  Returns:
    A tuple containing the der-encoded private key and the X9.62 compressed
    public key.
  """
  ec_helper = _ec_helper_native()
  native_generate_p256_key = ec_helper.generate_p256_key
  native_generate_p256_key.argtypes = [
      POINTER(POINTER(c_ubyte)),
      POINTER(c_uint),
      POINTER(c_ubyte)
  ]
  native_generate_p256_key.restype = c_int
  pub_key = (c_ubyte * _ECDH_KEY_LEN).from_buffer(bytearray(_ECDH_KEY_LEN))
  pub_key_ptr = POINTER(c_ubyte)(pub_key)
  priv_key = POINTER(c_ubyte)()
  priv_key_len = c_uint(0)
  res = native_generate_p256_key(
      byref(priv_key), byref(priv_key_len), pub_key_ptr)
  if res != 0:
    raise RuntimeError('Failed to generate EC key')
  private_key = bytes(bytearray(priv_key[:priv_key_len.value]))
  public_key = bytes(bytearray(pub_key[0:_ECDH_KEY_LEN]))
  return [private_key, public_key]


def compute_p256_shared_secret(private_key, device_public_key):
  """Computes a shared secret between the script and the device.

  Args:
    private_key: the script's private key.
    device_public_key: the device's public key.

  Raises:
    RuntimeError: Computing the shared secret fails.

  Returns:
    The shared secret.
  """
  ec_helper = _ec_helper_native()
  shared_secret_compute = ec_helper.shared_secret_compute
  shared_secret_compute.argtypes = [
      POINTER(c_ubyte), c_uint,
      POINTER(c_ubyte),
      POINTER(c_ubyte)
  ]
  shared_secret_compute.restype = c_int
  shared_secret = (c_ubyte * 32).from_buffer(bytearray(32))
  shared_secret_ptr = POINTER(c_ubyte)(shared_secret)
  device_public_key_ptr = POINTER(c_ubyte)(
      create_string_buffer(device_public_key))
  private_key_ptr = POINTER(c_ubyte)(create_string_buffer(private_key))
  res = shared_secret_compute(private_key_ptr,
                              len(private_key), device_public_key_ptr,
                              shared_secret_ptr)
  if res != 0:
    raise RuntimeError('Failed to compute P256 shared secret')
  return bytes(bytearray(shared_secret[0:32]))
