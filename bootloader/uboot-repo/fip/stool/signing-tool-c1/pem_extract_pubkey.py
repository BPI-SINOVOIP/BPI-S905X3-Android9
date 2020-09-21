#!/usr/bin/env python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Extract the public key from a .pem RSA private key file (PKCS#1),
  and compute the public key coefficients used by the signature verification
  code.

  Example:
    ./util/pem_extract_pubkey board/zinger/zinger_dev_key.pem

  Note: to generate a suitable private key :
  RSA 2048-bit with public exponent F4 (65537)
  you can use the following OpenSSL command :
  openssl genrsa -F4 -out private.pem 2048
"""

import array
import base64
import struct
import sys

VERSION = '0.0.2'

"""
RSA Private Key file (PKCS#1) encoding :

It starts and ends with the tags:
-----BEGIN RSA PRIVATE KEY-----
BASE64 ENCODED DATA
-----END RSA PRIVATE KEY-----

The base64 encoded data is using an ASN.1 / DER structure :

RSAPrivateKey ::= SEQUENCE {
  version           Version,
  modulus           INTEGER,  -- n
  publicExponent    INTEGER,  -- e
  privateExponent   INTEGER,  -- d
  prime1            INTEGER,  -- p
  prime2            INTEGER,  -- q
  exponent1         INTEGER,  -- d mod (p-1)
  exponent2         INTEGER,  -- d mod (q-1)
  coefficient       INTEGER,  -- (inverse of q) mod p
  otherPrimeInfos   OtherPrimeInfos OPTIONAL
}


RSA Public Key file (PKCS#1) encoding :

-----BEGIN RSA PUBLIC KEY-----
BASE64 ENCODED DATA
-----END RSA PUBLIC KEY-----

The base64 encoded data is using an ASN.1 / DER structure :

RSAPublicKey ::= SEQUENCE {
  modulus           INTEGER,  -- n
  publicExponent    INTEGER   -- e
}
"""

PEM_HEADER='-----BEGIN RSA PRIVATE KEY-----'
PEM_FOOTER='-----END RSA PRIVATE KEY-----'

PUB_HEADER='-----BEGIN RSA PUBLIC KEY-----'
PUB_FOOTER='-----END RSA PUBLIC KEY-----'

PEM_HEADER_GENPKEY='-----BEGIN PRIVATE KEY-----'
PEM_FOOTER_GENPKEY='-----END PRIVATE KEY-----'

# supported RSA key sizes
RSA_KEY_SIZES=[1024, 2048, 4096, 8192]

class PEMError(Exception):
  """Exception class for pem_extract_pubkey utility."""

# "Constructed" bit in DER tag
DER_C=0x20
# DER Sequence tag (always constructed)
DER_SEQUENCE=DER_C|0x10
# DER Integer tag
DER_INTEGER=0x02

class DER:
  """DER encoded binary data storage and parser."""
  def __init__(self, data):
    # DER encoded binary data
    self._data = data
    # Initialize index in the data stream
    self._idx = 0

  def get_byte(self):
    octet = ord(self._data[self._idx])
    self._idx += 1
    return octet

  def get_len(self):
    octet = self.get_byte()
    if octet == 0x80:
      raise PEMError('length indefinite form not supported')
    if octet & 0x80: # Length long form
      bytecnt = octet & ~0x80
      total = 0
      for i in range(bytecnt):
        total = (total << 8) | self.get_byte()
      return total
    else: # Length short form
      return octet

  def get_tag(self):
    tag = self.get_byte()
    length = self.get_len()
    data = self._data[self._idx:self._idx + length]
    self._idx += length
    return {"tag" : tag, "length" : length, "data" : data}

def pem_get_mod(filename):
  """Extract the modulus from a PEM private key file.

  the PEM file is DER encoded according the structure quoted above.

  Args:
    filename : Full path to the .pem private key file.

  Raises:
    PEMError: If unable to parse .pem file or invalid file format.
  """
  # Read all the content of the .pem file
  content = file(filename).readlines()
  # Check the PEM RSA Private/Public key tags
  pubkey = False
  if (content[0].strip() != PEM_HEADER) and (content[0].strip() != PEM_HEADER_GENPKEY) and \
    (content[0].strip() != PUB_HEADER):
    raise PEMError('invalid PEM key header')
  if (content[0].strip() == PUB_HEADER):
    pubkey = True
  if (content[-1].strip() != PEM_FOOTER) and (content[-1].strip() != PEM_FOOTER_GENPKEY) and \
    (content[-1].strip() != PUB_FOOTER):
    raise PEMError('invalid PEM key footer')
  # Decode the DER binary stream from the base64 data
  b64 = "".join([l.strip() for l in content[1:-1]])
  der = DER(base64.b64decode(b64))

  # Skip outter sequence in case openssl genpkey PEM private key
  if (content[0].strip() == PEM_HEADER_GENPKEY):
    for i in range(0, 26):
      der.get_byte()

  # Parse the DER and fail at the first error
  # The private key should be a (constructed) sequence
  seq = der.get_tag()
  if seq["tag"] != DER_SEQUENCE:
    raise PEMError('expecting an ASN.1 sequence')
  seq = DER(seq["data"])

  if not pubkey:
    # 1st field is Version
    ver = seq.get_tag()
    if ver["tag"] != DER_INTEGER:
      raise PEMError('version field should be an integer')

  # 2nd field is Modulus
  mod = seq.get_tag()
  if mod["tag"] != DER_INTEGER:
    raise PEMError('modulus field should be an integer')
  # 2048 bits + mandatory ASN.1 sign (0) => 257 Bytes
  modSize = (mod["length"] - 1) * 8
  if modSize not in RSA_KEY_SIZES or mod["data"][0] != '\x00':
    raise PEMError('Invalid key length : %d bits' % (modSize))

  # 3rd field is Public Exponent
  exp = seq.get_tag()
  if exp["tag"] != DER_INTEGER:
    raise PEMError('exponent field should be an integer')
  #if exp["length"] != 3 or exp["data"] != "\x01\x00\x01":
  #  raise PEMError('the public exponent must be F4 (65537)')

  return (mod["data"], exp["data"])

def modinv(a, m):
  """ The multiplicitive inverse of a in the integers modulo m.

  Return b when a * b == 1 mod m
  """
  # Extended GCD
  lastrem, rem = abs(a), abs(m)
  x, lastx, y, lasty = 0, 1, 1, 0
  while rem:
    lastrem, (quotient, rem) = rem, divmod(lastrem, rem)
    x, lastx = lastx - quotient*x, x
    y, lasty = lasty - quotient*y, y
  #
  if lastrem != 1:
    raise ValueError
  x = lastx * (-1 if a < 0 else 1)
  return x % m

def to_words(n, count):
  h = '%x' % n
  s = ('0'*(len(h) % 2) + h).zfill(count*8).decode('hex')
  return array.array("I", s[::-1])

def compute_mod_parameters(modulus, exp):
  ''' Prepare/pre-compute coefficients for the RSA public key signature
    verification code.
  '''
  # create an array of uint32_t to store the modulus but skip the sign byte
  w = array.array("I",modulus[1:])
  wordCount = (len(modulus) - 1) / 4
  # all integers in DER encoded .pem file are big endian.
  w.reverse()
  w.byteswap()
  # convert the big-endian modulus to a big integer for the computations
  N = 0
  for i in range(len(modulus)):
    N = (N << 8) | ord(modulus[i])
  # -1 / N[0] mod 2^32
  B = 0x100000000L
  n0inv = B - modinv(w[0], B)

  # -1 / N127~0 mod 2^128
  B128 = (1L << 128)
  w128 = w[3] * (1L << 96) + w[2] * (1L << 64) + w[1] * (1L << 32) + w[0]
  n0inv128 = to_words(B128 - modinv(w128, B128), 4)

  # R = 2^(modulo size); RR = (R * R) % N
  modSize = (len(modulus) - 1) * 8
  if modSize not in RSA_KEY_SIZES:
    raise PEMError('Invalid key length : %d bits' % (modSize))
  RR = pow(2, modSize*2, N)
  rr_words = to_words(RR, wordCount)

  return {'exp':int(exp.encode('hex'), 16), 'len':wordCount, 'mod':w, 'rr':rr_words, 'n0inv':n0inv, 'n0inv128':n0inv128}

def print_header(params):
  print "{\n\t.e = %s," % (params['exp'])
  print "\t.len = %d," % (params['len'])
  print "\t.n = {%s}," % (",".join(["0x%08x" % (i) for i in params['mod']]))
  print "\t.rr = {%s}," % (",".join(["0x%08x" % (i) for i in params['rr']]))
  print "\t.n0inv = 0x%08x\n};" % (params['n0inv'])
  print "\t.n0inv128 = {%s}\n};" % (",".join(["0x%08x" % (i) for i in params['n0inv128']]))

def dump_blob(n0inv128Enable, params):
  pad_words = 128 - params['len']
  padding = '\x00\x00\x00\x00' * pad_words

  mod_bin = params['mod'].tostring() + padding
  e_bin = struct.pack('<I', params['exp'])
  rr_bin = params['rr'].tostring() + padding
  n0inv_bin = array.array("I",[params['n0inv']]).tostring()
  len_bin = struct.pack('<I', params['len'])
  blob = mod_bin + e_bin + rr_bin + n0inv_bin + len_bin

  if n0inv128Enable:
    n0inv128_bin = params['n0inv128'].tostring()
    blob += n0inv128_bin
  return blob

def extract_pubkey(pemfile, headerMode=True, n0inv128Enable=False):
  # Read the modulus in the .pem file
  (mod, exponent) = pem_get_mod(pemfile)
  # Pre-compute the parameters used by the verification code
  p = compute_mod_parameters(mod, exponent)

  if headerMode:
    # Generate a C header file with the parameters
    print_header(p)
  else:
    # Generate the packed structure as a binary blob
    return dump_blob(n0inv128Enable, p)

if __name__ == '__main__':
  try:
    if len(sys.argv) < 2:
      raise PEMError('Invalid arguments. Usage: ./pem_extract_pubkey priv.pem')
    extract_pubkey(sys.argv[1])
  except KeyboardInterrupt:
    sys.exit(0)
  except PEMError as e:
    sys.stderr.write("Error: %s\n" % (e.message))
    sys.exit(1)
