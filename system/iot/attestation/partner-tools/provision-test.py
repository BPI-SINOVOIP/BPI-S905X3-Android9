#!/usr/bin/python

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
"""Test that implements the Android Things Attestation Provisioning protocol.

Enables testing of the device side of the Android Things Attestation
Provisioning (ATAP) Protocol without access to a CA or Android Things Factory
Appliance (ATFA).
"""

import argparse
from collections import namedtuple
import os
import struct

from aesgcm import AESGCM
import cryptography.exceptions
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
import curve25519
import ec_helper

_ATAPSessionParameters = namedtuple('_AtapSessionParameters', [
    'algorithm', 'operation', 'private_key', 'public_key'
])

_MESSAGE_VERSION = 1
_OPERATIONS = {'ISSUE': 2, 'ISSUE_ENC': 3}
_ALGORITHMS = {'p256': 1, 'x25519': 2}
_ECDH_KEY_LEN = 33

_session_params = _ATAPSessionParameters(0, 0, bytes(), bytes())


def _write_operation_start(algorithm, operation):
  """Writes a fresh Operation Start message to tmp/operation_start.bin.

  Generates an ECDHE key specified by <algorithm> and writes an Operation
  Start message for executing <operation> on the device.

  Args:
    algorithm: Integer specifying the curve to use for the session key.
        1: P256, 2: X25519
    operation: Specifies the operation. 1: Certify, 2: Issue, 3: Issue Encrypted

  Raises:
    ValueError: algorithm or operation is is invalid.
  """

  global _session_params

  if algorithm > 2 or algorithm < 1:
    raise ValueError('Invalid algorithm value.')

  if operation > 3 or operation < 1:
    raise ValueError('Invalid operation value.')

  # Generate new key for each provisioning session
  if algorithm == _ALGORITHMS['x25519']:
    private_key = curve25519.genkey()
    # Make 33 bytes to match P256
    public_key = curve25519.public(private_key) + '\0'
  elif algorithm == _ALGORITHMS['p256']:
    [private_key, public_key] = ec_helper.generate_p256_key()

  _session_params = _ATAPSessionParameters(algorithm, operation, private_key,
                                           public_key)

  # "Operation Start" Header
  # +2 for algo and operation bytes
  header = (_MESSAGE_VERSION, 0, 0, 0, _ECDH_KEY_LEN + 2)
  operation_start = bytearray(struct.pack('<4B I', *header))

  # "Operation Start" Message
  op_start = (algorithm, operation, public_key)
  operation_start.extend(struct.pack('<2B 33s', *op_start))

  with open('tmp/operation_start.bin', 'wb') as f:
    f.write(operation_start)


def _get_ca_response(ca_request):
  """Writes a CA Response message to tmp/ca_response.bin.

  Parses the CA Request message at ca_request. Computes the session key from
  the ca_request, decrypts the inner request, verifies the SOM key signature,
  and issues or certifies attestation keys as applicable. The CA Response
  message containing test keys is written to ca_response.bin.

  Args:
    ca_request: The CA Request message from the device.

  Raises:
    ValueError: ca_request is malformed.

  CA Request message format for reference, sizes in bytes

  cleartext header                            8
  cleartext device ephemeral public key       33
  cleartext GCM IV                            12
  encrypted header                            8
  encrypted SOM key certificate chain         variable
  encrypted SOM key authentication signature  variable
  encrypted product ID SHA256 hash            32
  encrypted RSA public key                    variable
  encrypted ECDSA public key                  variable
  encrypted edDSA public key                  variable
  cleartext GCM tag                           16
  """

  var_len = 4
  header_len = 8
  pub_key_len = _ECDH_KEY_LEN
  gcm_iv_len = 12
  prod_id_hash_len = 32
  gcm_tag_len = 16

  min_message_length = (
      header_len + pub_key_len + gcm_iv_len + header_len + var_len + var_len +
      prod_id_hash_len + var_len + var_len + var_len + gcm_tag_len)

  if len(ca_request) < min_message_length:
    raise ValueError('Malformed message: Length invalid')

  # Unpack Request header
  end = header_len
  ca_req_start = ca_request[:end]
  (device_message_version, res1, res2, res3,
   device_message_len) = struct.unpack('<4B I', ca_req_start)

  if device_message_version != _MESSAGE_VERSION:
    raise ValueError('Malformed message: Incorrect message version')

  if res1 or res2 or res3:
    raise ValueError('Malformed message: Reserved values set')

  if device_message_len > len(ca_request) - header_len:
    raise ValueError('Malformed message: Incorrect device message length')

  # Extract AT device ephemeral public key
  start = header_len
  end = start + pub_key_len
  device_pub_key = bytes(ca_request[start:end])

  # Generate shared_key
  salt = _session_params.public_key + device_pub_key
  shared_key = _get_shared_key(_session_params.algorithm, device_pub_key, salt)

  # Decrypt AES-128-GCM message using the shared_key
  # Extract the GCM IV
  start = header_len + pub_key_len
  end = start + gcm_iv_len
  gcm_iv = bytes(ca_request[start:end])

  # Extract the encrypted message
  start = header_len + pub_key_len + gcm_iv_len
  enc_message_len = _get_var_len(ca_request, start)

  if enc_message_len > len(ca_request) - gcm_tag_len - start - var_len:
    raise ValueError('Encrypted message size %d too large' % enc_message_len)

  start += var_len
  end = start + enc_message_len
  enc_message = bytes(ca_request[start:end])

  # Extract the GCM Tag
  gcm_tag = bytes(ca_request[-gcm_tag_len:])

  # Decrypt message
  try:
    data = AESGCM.decrypt(enc_message, shared_key, gcm_iv, gcm_tag)
  except cryptography.exceptions.InvalidTag:
    raise ValueError('Malformed message: GCM decrypt failed')

  # Unpack Inner header
  end = header_len
  ca_req_inner_header = data[:end]
  (device_message_version, res1, res2, res3, inner_message_len) = struct.unpack(
      '<4B I', ca_req_inner_header)

  if device_message_version != _MESSAGE_VERSION:
    raise ValueError('Malformed message: Incorrect inner message version')

  if res1 or res2 or res3:
    raise ValueError('Malformed message: Reserved values set')

  remaining_bytes = len(ca_request) - header_len - pub_key_len
  remaining_bytes = remaining_bytes - gcm_iv_len - gcm_tag_len
  if inner_message_len > remaining_bytes:
    raise ValueError('Malformed message: Incorrect device inner message length')

  # SOM key certificate chain
  som_chain_start = header_len
  som_chain_len = _get_var_len(data, som_chain_start)
  if som_chain_len > 0:
    raise ValueError(
        'SOM authentication not yet supported, set cert chain length to zero')

  # SOM key authentication signature
  som_key_start = som_chain_start + var_len + som_chain_len
  som_len = _get_var_len(data, som_key_start)
  if som_len > 0:
    raise ValueError(
        'SOM authentication not yet supported, set signature length to zero')

  # Product ID SHA-256 hash
  prod_id_start = som_key_start + var_len + som_len
  prod_id_end = prod_id_start + prod_id_hash_len
  prod_id_hash = data[prod_id_start:prod_id_end]
  print 'product_id hash:' + prod_id_hash.encode('hex')

  # RSA public key to certify
  rsa_start = prod_id_start + prod_id_hash_len
  rsa_len = _get_var_len(data, rsa_start)
  if rsa_len > 0:
    raise ValueError(
        'Certify operation not supported, set RSA public key length to zero')

  # ECDSA public key to certify
  ecdsa_start = rsa_start + var_len + rsa_len
  ecdsa_len = _get_var_len(data, ecdsa_start)
  if ecdsa_len > 0:
    raise ValueError(
        'Certify operation not supported, set ECDSA public key length to zero')

  # edDSA public key to certify
  eddsa_start = prod_id_start + var_len + prod_id_hash_len
  eddsa_len = _get_var_len(data, eddsa_start)
  if eddsa_len > 0:
    raise ValueError(
        'Certify operation not supported, set edDSA public key length to zero')

  # ATFA treats ISSUE and ISSUE_ENCRYPTED operations the same
  if _session_params.operation == _OPERATIONS['ISSUE']:
    with open('keysets/unencrypted.keyset', 'rb') as infile:
      inner_ca_response = bytes(infile.read())
  elif _session_params.operation == _OPERATIONS['ISSUE_ENC']:
    with open('keysets/encrypted.keyset', 'rb') as infile:
      inner_ca_response = bytes(infile.read())

  (gcm_iv, encrypted_keyset, gcm_tag) = AESGCM.encrypt(inner_ca_response,
                                                       shared_key)

  # "CA Response" Header
  # +2 for algo and operation bytes
  header = (_MESSAGE_VERSION, 0, 0, 0, 12 + 4 + len(encrypted_keyset) + 16)
  ca_response = bytearray(struct.pack('<4B I', *header))

  struct_fmt = '12s I %ds 16s' % len(inner_ca_response)
  message = (gcm_iv, len(encrypted_keyset), encrypted_keyset, gcm_tag)
  ca_response.extend(struct.pack(struct_fmt, *message))

  with open('tmp/ca_response.bin', 'wb') as f:
    f.write(ca_response)


def _get_shared_key(algorithm,
                    device_pub_key,
                    hkdf_salt,
                    hkdf_info='KEY',
                    hkdf_hash_len=16):
  """Generates the shared key based on ECDH and HKDF.

  Uses a particular ECDH algorithm and HKDF-SHA256 to create a shared key

  Args:
    algorithm: p256 or curve25519
    device_pub_key: ephemeral public key from the AT device
    hkdf_salt: salt to use in the HKDF operation
    hkdf_info: info value to use in the HKDF operation
    hkdf_hash_len: length of the outputted hash value for use as a shared key

  Raises:
    RuntimeError: Computing the shared secret fails.

  Returns:
    The shared key.
  """

  if algorithm == _ALGORITHMS['p256']:
    ecdhe_shared_secret = ec_helper.compute_p256_shared_secret(
        _session_params.private_key, device_pub_key)

  elif algorithm == _ALGORITHMS['x25519']:
    device_pub_key = device_pub_key[:-1]
    ecdhe_shared_secret = curve25519.shared(_session_params.private_key,
                                            device_pub_key)

  hkdf = HKDF(
      algorithm=hashes.SHA256(),
      length=hkdf_hash_len,
      salt=hkdf_salt,
      info=hkdf_info,
      backend=default_backend())
  shared_key = hkdf.derive(ecdhe_shared_secret)

  return shared_key


def _get_var_len(data, index):
  """Reads the 4 byte little endian unsigned integer at data[index].

  Args:
    data: Start of bytearray
    index: Offset that indicates where the integer begins

  Returns:
    Little endian unsigned integer at data[index]
  """
  return struct.unpack('<I', data[index:index + 4])[0]


def main():
  parser = argparse.ArgumentParser(
      description='Test for Android Things key provisioning.')
  parser.add_argument(
      '-a',
      '--algorithm',
      type=str,
      choices=['p256', 'x25519'],
      required=True,
      dest='algorithm',
      help='Algorithm for deriving the ECDHE shared secret')
  parser.add_argument(
      '-s',
      '--serial',
      type=str,
      required=True,
      dest='serial',
      help='Fastboot serial device',
      metavar='FASTBOOT_SERIAL_NUMBER')
  parser.add_argument(
      '-o',
      '--operation',
      type=str,
      default='ISSUE',
      choices=['ISSUE', 'ISSUE_ENC'],
      dest='operation',
      help='Operation for provisioning the device')

  results = parser.parse_args()
  fastboot_device = results.serial
  algorithm = _ALGORITHMS[results.algorithm]
  operation = _OPERATIONS[results.operation]
  _write_operation_start(algorithm, operation)
  print 'Wrote Operation Start message to tmp/operation_start.bin'
  os.system('fastboot -s %s stage tmp/operation_start.bin' % fastboot_device)
  os.system('fastboot -s %s oem at-get-ca-request' % fastboot_device)
  os.system('fastboot -s %s get_staged tmp/ca_request.bin' % fastboot_device)
  with open('tmp/ca_request.bin', 'rb') as f:
    ca_request = bytearray(f.read())
    _get_ca_response(ca_request)
  print 'Wrote CA Response message to tmp/ca_response.bin'
  os.system('fastboot -s %s stage tmp/ca_response.bin' % fastboot_device)
  os.system('fastboot -s %s oem at-set-ca-response' % fastboot_device)
  os.system('fastboot -s %s getvar at-attest-uuid' % fastboot_device)


if __name__ == '__main__':
  main()
