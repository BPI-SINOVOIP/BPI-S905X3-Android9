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
"""AES GCM functions.

Class to organize GCM-related functions
"""

import os
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.ciphers import algorithms
from cryptography.hazmat.primitives.ciphers import Cipher
from cryptography.hazmat.primitives.ciphers import modes


class AESGCM(object):
  """Contains static methods for AES GCM operations.

  Attributes:
    None
  """

  @staticmethod
  def encrypt(plaintext, key, associated_data=''):
    """Encrypts provided plaintext using AES-GCM.

    Encrypts plaintext with a provided key and optional associated data.  Uses
    a 96 bit IV.

    Args:
      plaintext: The plaintext to be encrypted
      key: The AES-GCM key
      associated_data: Associated data (optional)

    Returns:
      iv: The IV
      ciphertext: The ciphertext
      tag: The GCM TAG

    Raises:
      None
    """

    iv = os.urandom(12)

    encryptor = Cipher(
        algorithms.AES(key), modes.GCM(iv),
        backend=default_backend()).encryptor()

    encryptor.authenticate_additional_data(associated_data)

    ciphertext = encryptor.update(plaintext) + encryptor.finalize()

    return (iv, ciphertext, encryptor.tag)

  @staticmethod
  def decrypt(ciphertext, key, iv, tag, associated_data=''):
    """Decrypts provided plaintext using AES-GCM.

    Decrypts ciphertext with a provided key, iv, tag, and optional associated
    data.

    Args:
      ciphertext: The ciphertext
      key: An AES-128 key
      iv: The IV
      tag: The GCM Tag
      associated_data: Associated data (optional)

    Returns:
      The plaintext

    Raises:
      cryptography.exceptions.InvalidTag
    """

    decryptor = Cipher(
        algorithms.AES(key), modes.GCM(iv, tag),
        backend=default_backend()).decryptor()

    decryptor.authenticate_additional_data(associated_data)

    return decryptor.update(ciphertext) + decryptor.finalize()
