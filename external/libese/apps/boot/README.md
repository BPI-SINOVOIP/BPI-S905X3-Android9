# Verified Boot Storage Applet for AVB 2.0

  - Status: Draft as of April 6, 2017

## Introduction

The application and support libraries in this directory provide
a mechanism for a device's bootloader, using [AVB](https://android.googlesource.com/platform/external/avb/),
to store sensitive information.  For a bootloader, sensitive information
includes whether the device is unlocked or locked, whether it is unlockable,
and what the minimum version of the OS/kernel is allowed to be booted.  It
may also store other sensitive flags.

The verified boot storage applet provides a mechanism to store this
data in a way that enforceѕ the expected policies even if the higher level
operating system is compromised or operates in an unexpected fashion.


## Design Overview

The Verified Boot Storage Applet, VBSA, provides three purpose-built
interfaces:

  - Lock storage and policy enforcement
  - Rollback index storage
  - Applet state

Each will be detailed below.

### Locks

There are four supported lock types:

  - `LOCK_CARRIER`
  - `LOCK_DEVICE`
  - `LOCK_BOOT`
  - `LOCK_OWNER`

Each lock has a single byte of "lock" storage.  If that byte is 0x0, then it is
unlocked, or cleared.  If it is non-zero, then the lock is locked.  Any
non-zero value is valid and may be used by the bootloader if any additional
internal flagging is necessary.

In addition, a lock may have associated metadata which must be supplied during
lock or unlock, or both.

See `ese_boot_lock_*` in include/ese/app/boot.h for the specific interfaces.


#### LOCK\_CARRIER

The Carrier Lock implements a lock which can only be set when the device is not
in production and can only be unlocked if provided a cryptographic signature.

This lock is available for use to implement "sim locking" or "phone locking"
such that the carrier can determine if the device is allowed to boot an
unsigned or unknown system image.

To provision this lock, device-specific data must be provided in an exact
format.  An example of this can be found in
`'ese-boot-tool.cpp':collect_device_data()`.  This data is then provided to
the applet using `ese_boot_lock_xset()`.

##### Metadata format for locking/provisioning

The following system attributes must be collected in the given order:

  - ro.product.brand
  - ro.product.device
  - ro.build.product
  - ro.serialno
  - [Modem ID: MEID or IMEI]
  - ro.product.manufacturer
  - ro.product.model

The data is serialized as follows:

    \[length||string\]\[...\]

The length is a `uint8_t` and the value is appended as a stream of
`uint8_t` values.

This data is then prefixed with the desired non-zero lock value before
being submitted to the applet.  If successful, the applet will have
stored a SHA256 hash of the device data

Note, `LOCK_CARRIER` can only be locked (non-zero lock value) when the
applet is not in 'production' mode.

##### Clearing/unlocking

If `LOCK_CARRIER` is set to a non-zero value and the applet is in
production mode, then clearing the lock value requires authorization.

Authorization comes in the form of a `RSA_SHA256-PKCS#1` signature over
the provisioned device data SHA256 hash and a supplied montonically
increasing "nonce".

The nonce value must be higher than the last seen nonce value and the
signature must validate using public key internally stored in the
applet (`CarrierLock.java:PK_MOD`).

To perform a clear, `ese_boot_lock_xset()` must be called with lock
data that begins with 0x0, to clear the lock, and then contains data of
the following format:

```
    unlockToken = VERSION || NONCE || SIGNATURE

    SIGNATURE = RSA_Sign(SHA256(deviceData))
```

  - The version is a little endian `uint64_t` (8 bytes).
  - The nonce is a little endian `uint64_t` (8 bytes).
  - The signature is a RSA 2048-bit with SHA-256 PKCS#1 v1.5 (256 bytes).

On unlock, the device data hash is cleared.

##### Testing

It is possible to test the key with a valid signature but a fake
internal nonce and fake internal device data using
`ese_boot_carrier_lock_test()`.  When using this interface, it
expects the same unlock token as in the prior but prefixes with the
fake data:

```
    testVector = LAST_NONCE || DEVICE_DATA || unlockToken
```

  - The last nonce is the value the nonce is compared against (8 bytes).
  - Device data is a replacement for the internally stored SHA-256
    hash of the deviec data. (32 bytes).

#### LOCK\_DEVICE

The device lock is one of the setting used by the bootloader to
determine if the boot lock can be changed.  It may only be set by the
operating system and is meant to protect the device from being reflashed
by someone that cannot unlock or access the OS.  This may also be used
by an enterprise administrator to control lock policy for managed
devices.

As `LOCK_DEVICE` has not metadata, it can be set and retrieved using
`ese_boot_lock_set()` and `ese_boot_lock_get()`.

#### LOCK\_BOOT

The boot lock is used by the bootloader to control whether it should
only boot verified system software or not.  When the lock value
is cleared (0x0), the bootloader will boot anything.  When the lock
value is non-zero, it should only boot software that is signed by a key
stored in the bootloader except if `LOCK_OWNER` is set.  Discussion of
`LOCK_OWNER` will follow.

`LOCK_BOOT` can only be toggled when in the bootloader/fastboot and if
both `LOCK_CARRIER` and `LOCK_DEVICE` are cleared/unlocked.

As with `LOCK_DEVICE`, `LOCK_BOOT` has no metadata so it does not need the
extended accessors.

#### LOCK\_OWNER

The owner lock is used by the bootloader to support an alternative
OS signing key provided by the device owner.  `LOCK_OWNER` can only be
toggled if `LOCK_BOOT` is cleared. `LOCK_OWNER` does not require
any metadata to unlock, but to lock, it requires a blob of up to 2048
bytes be provided.  This is just secure storage for use by the
bootloader.  `LOCK_OWNER` may be toggled in the bootloader or the
operating system.  This allows an unlocked device (`LOCK_BOOT=0x0`) to
install an owner key using fastboot or using software on the operating
system itself.

Before `LOCK_OWNER`'s key should be honored by the bootloader, `LOCK_BOOT`
should be set (in the bootloader) to enforce use of the key and to keep
malicious software in the operating system from changing it.

(Note, that the owner key should not be treated as equivalent to the
bootloader's internally stored key in that the device user should have a
means of knowing if an owner key is in use, but the requirement for the
device to be unlocked implies both physical access the `LOCK_DEVICE`
being cleared.)


### Rollback storage

Verifying an operating system kernel and image is useful both for system
reliability and for ensuring that the software has not been modified by
a malicious party.  However, if the system software is updated,
malicious software may attempt to "roll" a device back to an older
version in order to take advantage of mistakes in the older, verified
code.

Rollback index values, or versions, may be stored securely by the bootloader
to prevent these problems.  The Verified Boot Storage applet provides
eight 64-bit slots for storing a value.  They may be read at any time,
but they may only be written to when the device is in the bootloader (or
fastboot).

Rollback storage is written to using
`ese_boot_rollback_index_write()` and read using
`ese_boot_rollback_index_read()`.

### Applet state

The applet supports two operational states:

  - production=true
  - production=false

On initial installation, production is false.  When the applet is not
in production mode, it does not enforce a number of security boundaries,
such as requiring bootloader or hlos mode for lock toggling or
CarrierLock verification.  This allows the factory tools to run in any
mode and properly configure a default lock state.

To transition to "production", a call to `ese_boot_set_production(true)`
is necessary.

To check the state and collect debugging information, the call
`ese_boot_get_state()` will return the current bootloader value,
the production state, any errors codes from lock initialization, and the
contents of lock storage.

#### Example applet provisioning

After the applet is installed, a flow as follows would prepare the
applet for use:

  - `ese_boot_session_init();`
  - `ese_boot_session_open();`
  - `ese_boot_session_lock_xset(LOCK_OWNER, {0, ...});`
  - `ese_boot_session_lock_set(LOCK_BOOT, 0x1);`
  - `ese_boot_session_lock_set(LOCK_DEVICE, 0x1);`
  - [collect device data]
  - `ese_boot_session_lock_set(LOCK_CARRIER, {1, deviceData});`
  - `ese_boot_session_set_production(true);`
  - `ese_boot_session_close();`

### Additional details

#### Bootloader mode

Bootloader mode detection depends on hardware support to signal the
applet that the application processor has been reset.  Additionally,
there is a mechanism for the bootloader to indicate that is loading the
main OS where it flips the value.  This signal provides the assurances
around when storage is mutable or not during the time a device is
powered on.

#### Error results

EseAppResult is an enum that all `ese_boot_*` functions return.  The
enum only covers the lower 16-bits.  The upper 16-bits are reserved for
passing applet and secure element OS status for debugging and analysis.
When the lower 16-bits are `ESE_APP_RESULT_ERROR_APPLET`, then the
upper bytes will be the applet code. That code can then be
cross-referenced in the applet by function and code.  If the lower
bytes are `ESE_APP_RESULT_ERROR_OS`, then the status code are the
ISO7816 code from an uncaught exception or OS-level error.

##### Cooldown

`ESE_APP_RESULT_ERROR_COOLDOWN` indicates that the secure element needs to
stay powered on for a period of time -- either at the end of use or before the
requested command can be serviced.  As the behavior is implementation specific,
the only effective option is to keep the secure element powered for the number of
seconds specified by the response `uint32_t`.

For chips that support it, like the one this applet is being tested on, the
cooldown time can be requested with a special APDU to `ese_transceive()`:

```
    FFE10000
```

In response, a 6 byte response will contain a `uint32_t` and a successful status
code (`90 00`).  The unsigned little-endian integer indicates how many seconds
the chip needs to stay powered and unused to cooldown.  If this happens before
the locks or rollback storage can be read, the bootloader will need to
determine a safe delay or recovery path until boot can proceed securely.

## Examples

There are many ways to integrate this library and the associated applet.
Below are some concrete examples to guide standard approach.

### Configuration in factory

- Install configure the secure element and install the applets  
(outside of the scope of this document).
- Boot to an environment to run the ese-boot-tool.
- Leave the inBootloader() signal asserted (recommended but not required).
- Configure the desired lock states:
  - `# ese-boot-tool lock set carrier 1 modem-imei-string`
  - `# ese-boot-tool lock set device 1`
  - `# ese-boot-tool lock set boot 1`
  - `# ese-boot-tool lock set owner 0`
- To move from factory mode to production mode call:
  - `# ese-boot-tool production set true`

### Configuration during repair

- Boot to an environment to run the ese-boot-tool.
- Leave inBootloader() signal asserted or implement the steps below in  
  the bootloader.
- Transition out of production mode:
  - `# ese-boot-tool production set false`
- If a `LOCK_CARRIER` problem is being repaired, it is possible to reset the  
  internal nonce counter and all lock state with the command below.  A full  
  lock reset is not expected in most cases.
  - `# ese-boot-tool lock reset`
- Reconfigure the lock states:
  - `# ese-boot-tool lock set carrier 1 modem-imei-string`
  - `# ese-boot-tool lock set device 1`
  - `# ese-boot-tool lock set boot 1`
  - `# ese-boot-tool lock set owner 0`
    (To clear data from the owner lock, set owner 1 must be called with  
     4096 00s.)
- Then move back to production mode:
  - `# ese-boot-tool production set true`

### Use during boot

Do not load any non-repair or non-factory OS without clearing the inBootloader
signal as the applet may be transitioned out of production mode and/or the
rollback state may be changed.

#### Checking rollback values

- Read and write rollback values as per libavb using the API
  - `ese_boot_rollback_index_write()`
  - `ese_boot_rollback_index_read()`
- Prior to leaving the bootloader, clear the inBootloader signal.

As rollback index values can only be written when inBootloader signal is set,
it is critical to clear it when leaving the bootloader.

#### Checking locks

The pseudo-code and comments below should outline the basic algorithm, but it
does not include integration into libavb or include use of rollback index
value checking:

```
// Read LOCK_BOOT
ese_boot_lock_get(session, kEseBootLockIdBoot, &lockBoot);

if (lockBoot != 0x0) {  // Boot is LOCKED.
    // Read the LOCK_OWNER
    ese_boot_lock_xget(session, kEseBootLockIdOwner, &lockOwner);
    if (lockOwner != 0x0) {  // Owner is LOCKED
      // Get the lock owner value with metadata.
      // This is done as a second stage to avoid wasted copying when it
      // is not locked.
      uint8_t ownerData[kEseBootOwnerKeyMax + 1];
      ese_boot_lock_xget(session, kEseBootLockIdOwner, ownerData
                         sizeof(ownerData), &ownerDataUsed);
      // lockOwner == ownerData[0]
      // Parse the stored metadata into a key as per your bootloader
      // design.
      SomeBootKey key;
      parseDeviceOwnerKeyForBooting(ownerData + 1, ownerDataUsed, &key);
      // Boot using the supplied owner key
      // (E.g., as part of avb_validate_vbmeta_public_key())
      setDeviceOwnerKeyForBooting(&key);
      continueBootFlow();
} else {  // Boot is UNLOCKED (0x0)
    // Perform the boot flow.
    setBootIsUnverified();
    continueBootFlow();
}
```

### In fastboot

- `LOCK_BOOT` may be toggled by a fastboot command.  If the conditions of  
  unlock are not allowed by applet policy, it will fail.
- `LOCK_OWNER`may be toggled and set a boot key from a fastboot command  
  or from an unlocked OS image.
- If the verified boot design dictates that rollback indices are clear on  
  lock/unlock, this can be done by calling
  - `ese_boot_rollback_index_write()` on each slot with the value of 0.

Note, `LOCK_DEVICE` and `LOCK_CARRIER` should not need to be used by fastboot.

For debugging and support, it may be desirable to connect the
`ese_boot_get_state()` to allow fastboot to return the current value of
production, inbootloader, and the lock metadata.

