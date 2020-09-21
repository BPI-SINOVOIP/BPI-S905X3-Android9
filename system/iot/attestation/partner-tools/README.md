# Key Provisioning Test Suite

This directory contains a test script to verify that a device
works with the Android Things key provisioning protocol. Usage:

./provision-test.py -a [p256|x25519] -s FASTBOOT_SERIAL_NUMBER
                    -o [ISSUE|ISSUE_ENC]

## Dependencies

Install openssl, python cryptography, pycurve25519. Build ec_helper_native.so
in this directory ($ make ec_helper_native). Build and install fastboot from
AOSP master.

## How to get key sets

provision-test.py looks for key set payloads unencryped.keyset and
encrypted.keyset and under the keysets/ directory. Provided here are
files that contain test keys that do not verify to the real Android
Things Root CA. unencrypted.keyset is simply a raw CA Response
Message. encrypted.keyset encrypts unencrypted.keyset with a global key
of 16 zero bytes.
