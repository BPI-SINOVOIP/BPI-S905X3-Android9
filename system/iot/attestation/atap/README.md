# Android Things Attestation Provisioning Protocol: Version 1

This directory contains an implementation of the Android Things
attestation provisioning protocol intended for use with the Android
Things fastboot commands "oem at-get-ca-request" and
"oem set-ca-response".

## Introduction

The `libatap` library provides reference code for the provisioning
protocol that enables an Android Things Factory Applicance (ATFA) to
provision a device with attestation product keys and a Cast
authentication key. This library is intended to be used in TEEs on
devices running Android Things. It has a simple abstraction for system
dependencies (see `atap_sysdeps.h`) as well as operations that the
platform is expected to implement (see `atap_ops.h`). The main entry
points are `atap_get_ca_request()` and `atap_set_ca_response()`
(see `libatap.h`).

The version will only be bumped when protocol message formats change.

## Files and Directories

* `libatap/`
    + An implementation of the provisioning protocol. This code is
      designed to be highly portable so it can be used in as many
      contexts as possible. This code requires a C99-compliant C
      compiler. System dependencies expected to be provided by the
      platform is defined in `atap_sysdeps.h`. If the platform
      provides the standard C runtime `atap_sysdeps_posix.c` can be used.
* `Android.mk`
    + Build instructions for building libatap (a static library for use
      on the device), host-side libraries (for unit tests), and unit
      tests.
* `test/`
    + Unit tests for `libatap`

## Audience and portability notes

This code is intended to be used in TEEs in devices running
Android Things. The suggested approach is to copy the appropriate
header and C files mentioned in the previous section into the TEE an
integrate as appropriate.

This code can also be used in more general purpose bootloaders, but
porting to a TEE is recommended since a TEE will typically have all
the required crypto callbacks, and is the manager of the provisioned
keys. If ported to a bootloader, the keys will still have to be passed
from the bootloader to the TEE.

The `libatap/` codebase may change over time so integration should be
as non-invasive as possible. The intention is to keep the API of the
library stable however it will be broken if necessary. As for
portability, the library is intended to be highly portable, work on
both little- and big-endian architectures and 32- and 64-bit. It's
also intended to work in non-standard environments without the
standard C library and runtime.

If the `ATAP_ENABLE_DEBUG` preprocessor symbol is set, the code will
include useful debug information and run-time checks. Production
builds should not use this.
