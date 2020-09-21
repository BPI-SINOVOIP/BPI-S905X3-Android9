# 11\. Updatable Software

*    [C-0-1] Device implementations MUST include a mechanism to replace the
entirety of the system software. The mechanism need not perform “live”
upgrades—that is, a device restart MAY be required.

Any method can be used, provided that it can replace the entirety of the
software preinstalled on the device. For instance, any of the following
approaches will satisfy this requirement:

*   “Over-the-air (OTA)” downloads with offline update via reboot.
*   “Tethered” updates over USB from a host PC.
*   “Offline” updates via a reboot and update from a file on removable storage.

*    [C-0-2] The update mechanism used MUST support updates without wiping user
data. That is, the update mechanism MUST preserve application private data and
application shared data. Note that the upstream Android software includes an
update mechanism that satisfies this requirement.

If the device implementations includes support for an unmetered data
connection such as 802.11 or Bluetooth PAN (Personal Area Network) profile,
then, they:

*    [C-1-1] MUST support OTA downloads with offline update via reboot.

For device implementations that are launching with Android 6.0 and
later, the update mechanism SHOULD support verifying that the system image is
binary identical to expected result following an OTA. The block-based OTA
implementation in the upstream Android Open Source Project, added since Android
5.1, satisfies this requirement.

Also, device implementations SHOULD support [A/B system updates](https://source.android.com/devices/tech/ota/ab_updates.html).
The AOSP implements this feature using the boot control HAL.

If an error is found in a device implementation after it has been released but
within its reasonable product lifetime that is determined in consultation with
the Android Compatibility Team to affect the compatibility of third-party
applications, then:

*    [C-2-1]  The device implementer MUST correct the error via a software
update available that can be applied per the mechanism just described.

Android includes features that allow the Device Owner app (if present) to
control the installation of system updates. If the system update subsystem
for devices report android.software.device_admin then, they:

*    [C-3-1]  MUST implement the behavior described in the [SystemUpdatePolicy](http://developer.android.com/reference/android/app/admin/SystemUpdatePolicy.html)
 class.
