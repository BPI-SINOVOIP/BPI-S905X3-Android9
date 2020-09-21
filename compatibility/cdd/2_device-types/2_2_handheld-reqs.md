## 2.2\. Handheld Requirements

An **Android Handheld device** refers to an Android device implementation that is
typically used by holding it in the hand, such as an mp3 player, phone, or
tablet.

Android device implementations are classified as a Handheld if they meet all the
following criteria:

*   Have a power source that provides mobility, such as a battery.
*   Have a physical diagonal screen size in the range of 2.5 to 8 inches.

The additional requirements in the rest of this section are specific to Android
Handheld device implementations.


<div class="note">
<b>Note:</b> Requirements that do not apply to Android Tablet devices are marked with an *.
</div>

### 2.2.1\. Hardware

Handheld device implementations:

*   [[7.1](#7_1_display-and-graphics).1.1/H-0-1] MUST have a screen at least
2.5 inches in physical diagonal size.
*   [[7.1](#7_1_display-and-graphics).1.3/H-SR] Are STRONGLY RECOMMENDED to
provide users an affordance to change the display size.(Screen Density)
*   [[7.1](#7_1_display-and-graphics).5/H-0-1] MUST include support for legacy
application compatibility mode as implemented by the upstream Android open
source code. That is, device implementations MUST NOT alter the triggers or
thresholds at which compatibility mode is activated, and MUST NOT alter the
behavior of the compatibility mode itself.
*   [[7.2](#7_2_input-devices).1/H-0-1] MUST include support for third-party
Input Method Editor (IME) applications.
*   [[7.2](#7_2_input-devices).3/H-0-1] MUST provide the Home, Recents, and Back
functions.
*   [[7.2](#7_2_input-devices).3/H-0-2] MUST send both the normal and long press
event of the Back function ([`KEYCODE_BACK`](
http://developer.android.com/reference/android/view/KeyEvent.html#KEYCODE_BACK))
to the foreground application.
*   [[7.2](#7_2_input-devices).4/H-0-1] MUST support touchscreen input.
*  [[7.3](#7_3_sensors).1/H-SR] Are STRONGLY RECOMMENDED to include a 3-axis
accelerometer.

If Handheld device implementations include a 3-axis accelerometer, they:

*  [[7.3](#7_3_sensors).1/H-1-1] MUST be able to report events up to a frequency
of at least 100 Hz.

If Handheld device implementations include a gyroscope, they:

*  [[7.3](#7_3_sensors).4/H-1-1] MUST be able to report events up to a frequency
of at least 100 Hz.

Handheld device implementations that can make a voice call and indicate
any value other than `PHONE_TYPE_NONE` in `getPhoneType`:

*  [[7.3](#7_3_sensors).8/H] SHOULD include a proximity sensor.

Handheld device implementations:

*  [[7.3](#7_3_sensors).12/H-SR] Are RECOMMENDED to support pose sensor with 6
degrees of freedom.
*  [[7.4](#7_4_data-connectivity).3/H]SHOULD include support for Bluetooth and
Bluetooth LE.

If Handheld device implementations include a metered connection, they:

*  [[7.4](#7_4_data-connectivity).7/H-1-1] MUST provide the data saver mode.

Handheld device implementations:

*  [[7.6](#7_6_memory-and-storage).1/H-0-1] MUST have at least 4GB of
non-volatile storage available for application private data
(a.k.a. "/data" partition).
*  [[7.6](#7_6_memory-and-storage).1/H-0-2] MUST return “true” for
`ActivityManager.isLowRamDevice()` when there is less than 1GB of memory
available to the kernel and userspace.


If Handheld device implementations are 32-bit:

*   [[7.6](#7_6_memory-and-storage).1/H-1-1] The memory available to the kernel
and userspace MUST be at least 512MB if any of the following densities are used:
     *    280dpi or lower on small/normal screens<sup>*</sup>
     *    ldpi or lower on extra large screens
     *    mdpi or lower on large screens

*   [[7.6](#7_6_memory-and-storage).1/H-2-1] The memory available to the kernel
and userspace MUST be at least 608MB if any of the following densities are used:
     *   xhdpi or higher on small/normal screens<sup>*</sup>
     *   hdpi or higher on large screens
     *   mdpi or higher on extra large screens

*   [[7.6](#7_6_memory-and-storage).1/H-3-1] The memory available to the kernel
and userspace MUST be at least 896MB if any of the following densities are used:
     *   400dpi or higher on small/normal screens<sup>*</sup>
     *   xhdpi or higher on large screens
     *   tvdpi or higher on extra large screens

*    [[7.6](#7_6_memory-and-storage).1/H-4-1] The memory available to the kernel
and userspace MUST be at least 1344MB if any of the following densities are used:
     *   560dpi or higher on small/normal screens<sup>*</sup>
     *   400dpi or higher on large screens
     *   xhdpi or higher on extra large screens

If Handheld device implementations are 64-bit:

*    [[7.6](#7_6_memory-and-storage).1/H-5-1] The memory available to the kernel
and userspace MUST be at least 816MB if any of the following densities are used:
     *   280dpi or lower on small/normal screens<sup>*</sup>
     *   ldpi or lower on extra large screens
     *   mdpi or lower on large screens

*    [[7.6](#7_6_memory-and-storage).1/H-6-1] The memory available to the kernel
and userspace MUST be at least 944MB if any of the following densities are used:
     *   xhdpi or higher on small/normal screens<sup>*</sup>
     *   hdpi or higher on large screens
     *   mdpi or higher on extra large screens

*    [[7.6](#7_6_memory-and-storage).1/H-7-1] The memory available to the kernel
and userspace MUST be at least 1280MB if any of the following densities are used:
     *  400dpi or higher on small/normal screens<sup>*</sup>
     *  xhdpi or higher on large screens
     *  tvdpi or higher on extra large screens

*    [[7.6](#7_6_memory-and-storage).1/H-8-1] The memory available to the kernel
and userspace MUST be at least 1824MB if any of the following densities are used:
     *   560dpi or higher on small/normal screens<sup>*</sup>
     *   400dpi or higher on large screens
     *   xhdpi or higher on extra large screens

Note that the "memory available to the kernel and userspace" above refers to the
memory space provided in addition to any memory already dedicated to hardware
components such as radio, video, and so on that are not under the kernel’s
control on device implementations.


Handheld device implementations:

*   [[7.6](#7_6_memory-and-storage).2/H-0-1] MUST NOT provide an application
shared storage smaller than 1 GiB.
*   [[7.7](#7_7_usb).1/H] SHOULD include a USB port supporting peripheral mode.

If handheld device implementations include a USB port supporting peripheral
mode, they:

*   [[7.7](#7_7_usb).1/H-1-1] MUST implement the Android Open Accessory (AOA)
API.

Handheld device implementations:

*   [[7.8](#7_8_audio).1/H-0-1] MUST include a microphone.
*   [[7.8](#7_8_audio).2/H-0-1] MUST have an audio output and declare
`android.hardware.audio.output`.

If Handheld device implementations include support for the VR mode, they:

*   [[7.9](#7_9_virtual-reality).1/H-1-1] MUST declare the
`android.software.vr.mode` feature.


If device implementations declare `android.software.vr.mode` feature, they:

*   [[7.9](#7_9_virtual-reality).1/H-2-1] MUST include an application
implementing `android.service.vr.VrListenerService`
that can be enabled by VR applications via
`android.app.Activity#setVrModeEnabled`.

If Handheld device implementations are capable of meeting all the requirements
to declare the `android.hardware.vr.high_performance` feature flag, they:

*   [[7.9](#7_9_virtual-reality).2/-1-1] MUST declare the
`android.hardware.vr.high_performance` feature flag.

### 2.2.2\. Multimedia

Handheld device implementations MUST support the following audio encoding:

*    [[5.1](#5_1_media-codecs).1/H-0-1] AMR-NB
*    [[5.1](#5_1_media-codecs).1/H-0-2] AMR-WB
*    [[5.1](#5_1_media-codecs).1/H-0-3] MPEG-4 AAC Profile (AAC LC)
*    [[5.1](#5_1_media-codecs).1/H-0-4] MPEG-4 HE AAC Profile (AAC+)
*    [[5.1](#5_1_media-codecs).1/H-0-5] AAC ELD (enhanced low delay AAC)

Handheld device implementations MUST support the following audio decoding:

*    [[5.1](#5_1_media-codecs).2/H-0-1] AMR-NB
*    [[5.1](#5_1_media-codecs).2/H-0-2] AMR-WB

Handheld device implementations MUST support the following video encoding and
make it available to third-party applications:

*    [[5.2](#5_2_video-encoding)/H-0-1] H.264 AVC
*    [[5.2](#5_2_video-encoding)/H-0-2] VP8

Handheld device implementations MUST support the following video decoding:

*    [[5.3](#5_3_video-decoding)/H-0-1] H.264 AVC
*    [[5.3](#5_3_video-decoding)/H-0-2] H.265 HEVC
*    [[5.3](#5_3_video-decoding)/H-0-3] MPEG-4 SP
*    [[5.3](#5_3_video-decoding)/H-0-4] VP8
*    [[5.3](#5_3_video-decoding)/H-0-5] VP9

### 2.2.3\. Software

Handheld device implementations:

*   [[3.4](#3_4_web-compatibility).1/H-0-1] MUST provide a complete
implementation of the `android.webkit.Webview` API.
*   [[3.4](#3_4_web-compatibility).2/H-0-1] MUST include a standalone Browser
application for general user web browsing.
*   [[3.8](#3_8_user-interface-compatibility).1/H-SR] Are STRONGLY RECOMMENDED
to implement a default launcher that supports in-app pinning of shortcuts and
widgets.
*   [[3.8](#3_8_user-interface-compatibility).1/H-SR] Are STRONGLY RECOMMENDED
to implement a default launcher that provides quick access to the additional
shortcuts provided by third-party apps through the [ShortcutManager](
https://developer.android.com/reference/android/content/pm/ShortcutManager.html)
API.
*   [[3.8](#3_8_user-interface-compatibility).1/H-SR] Are STRONGLY RECOMMENDED
to include a default launcher app that shows badges for the app icons.
*   [[3.8](#3_8_user-interface-compatibility).2/H-SR] Are STRONGLY RECOMMENDED
to support third-party app widgets.
*   [[3.8](#3_8_user-interface-compatibility).3/H-0-1] MUST allow third-party
apps to notify users of notable events through the [`Notification`](
https://developer.android.com/reference/android/app/Notification.html) and
[`NotificationManager`](
https://developer.android.com/reference/android/app/NotificationManager.html)
API classes.
*   [[3.8](#3_8_user-interface-compatibility).3/H-0-2] MUST support rich
notifications.
*   [[3.8](#3_8_user-interface-compatibility).3/H-0-3] MUST support heads-up
notifications.
*   [[3.8](#3_8_user-interface-compatibility).3/H-0-4] MUST include a
notification shade, providing the user the ability to directly control (e.g.
reply, snooze, dismiss, block) the notifications through user affordance such as
action buttons or the control panel as implemented in the AOSP.
*   [[3.8](#3_8_user-interface-compatibility).4/H-SR] Are STRONGLY RECOMMENDED
to implement an assistant on the device to handle the [Assist action](
http://developer.android.com/reference/android/content/Intent.html#ACTION_ASSIST).

If Android Handheld device implementations support a lock screen, they:

*   [[3.8](#3_8_user-interface-compatibility).10/H-1-1] MUST display the Lock
screen Notifications including the Media Notification Template.

If Handheld device implementations support a secure lock screen, they:

*   [[3.9](#3_9_device-administration)/H-1-1] MUST implement the full range of
[device administration](
http://developer.android.com/guide/topics/admin/device-admin.html)
policies defined in the Android SDK documentation.

Handheld device implementations:

*  [[3.10](#3_10_accessibility)/H-0-1] MUST support third-party accessibility
services.
*  [[3.10](#3_10_accessibility)/H-SR] Are STRONGLY RECOMMENDED to preload
accessibility services on the device comparable with or exceeding functionality
of the Switch Access and TalkBack (for languages supported by the preloaded
Text-to-speech engine) accessibility services as provided in the [talkback open
source project](https://github.com/google/talkback).
*   [[3.11](#3_11_text-to-speech)/H-0-1] MUST support installation of
third-party TTS engines.
*   [[3.11](#3_11_text-to-speech)/H-SR] Are STRONGLY RECOMMENDED to include a
TTS engine supporting the languages available on the device.
*   [[3.13](#3_13_quick_settings)/H-SR] Are STRONGLY RECOMMENDED to include a
Quick Settings UI component.

If Android handheld device implementations declare `FEATURE_BLUETOOTH` or
`FEATURE_WIFI` support, they:

*   [[3.15](#3_15_instant_apps)/H-1-1] MUST support the companion device pairing
feature.

### 2.2.4\. Performance and Power

*   [[8.1](#8_1_user-experience-consistency)/H-0-1] **Consistent frame latency**.
Inconsistent frame latency or a delay to render frames MUST NOT happen more
often than 5 frames in a second, and SHOULD be below 1 frames in a second.
*   [[8.1](#8_1_user-experience-consistency)/H-0-2] **User interface latency**.
Device implementations MUST ensure low latency user experience by scrolling a
list of 10K list entries as defined by the Android Compatibility Test Suite
(CTS) in less than 36 secs.
*   [[8.1](#8_1_user-experience-consistency)/H-0-3] **Task switching**. When
multiple applications have been launched, re-launching an already-running
application after it has been launched MUST take less than 1 second.

Handheld device implementations:

*   [[8.2](#8_2_file-io-access-performance)/H-0-1] MUST ensure a sequential
write performance of at least 5 MB/s.
*   [[8.2](#8_2_file-io-access-performance)/H-0-2] MUST ensure a random write
performance of at least 0.5 MB/s.
*   [[8.2](#8_2_file-io-access-performance)/H-0-3] MUST ensure a sequential read
performance of at least 15 MB/s.
*   [[8.2](#8_2_file-io-access-performance)/H-0-4] MUST ensure a random read
performance of at least 3.5 MB/s.
*   [[8.3](#8_3_power-saving-modes)/H-0-1] All Apps exempted from App Standby
and Doze power-saving modes MUST be made visible to the end user.
*   [[8.3](#8_3_power-saving-modes)/H-0-2] The triggering, maintenance, wakeup
algorithms and the use of global system settings of App Standby and Doze
power-saving modes MUST not deviate from the Android Open Source Project.

Handheld device implementations:

*    [[8.4](#8_4_power-consumption-accounting)/H-0-1] MUST provide a
per-component power profile that defines the [current consumption value](
http://source.android.com/devices/tech/power/values.html)
for each hardware component and the approximate battery drain caused by the
components over time as documented in the Android Open Source Project site.
*    [[8.4](#8_4_power-consumption-accounting)/H-0-2] MUST report all power
consumption values in milliampere hours (mAh).
*    [[8.4](#8_4_power-consumption-accounting)/H-0-3] MUST report CPU power
consumption per each process's UID. The Android Open Source Project meets the
requirement through the `uid_cputime` kernel module implementation.
*   [[8.4](#8_4_power-consumption-accounting)/H-0-4] MUST make this power usage
available via the [`adb shell dumpsys batterystats`](
http://source.android.com/devices/tech/power/batterystats.html)
shell command to the app developer.
*   [[8.4](#8_4_power-consumption-accounting)/H] SHOULD be attributed to the
hardware component itself if unable to attribute hardware component power usage
to an application.

If Handheld device implementations include a screen or video output, they:

*   [[8.4](#8_4_power-consumption-accounting)/H-1-1] MUST honor the
[`android.intent.action.POWER_USAGE_SUMMARY`](
http://developer.android.com/reference/android/content/Intent.html#ACTION_POWER_USAGE_SUMMARY)
intent and display a settings menu that shows this power usage.

### 2.2.5\. Security Model

Handheld device implementations:

*   [[9.1](#9_1_permissions)/H-0-1] MUST allow third-party apps to access the
usage statistics via the `android.permission.PACKAGE_USAGE_STATS` permission and
provide a user-accessible mechanism to grant or revoke access to such apps in
response to the [`android.settings.ACTION_USAGE_ACCESS_SETTINGS`](
https://developer.android.com/reference/android/provider/Settings.html#ACTION&lowbar;USAGE&lowbar;ACCESS&lowbar;SETTINGS)
intent.

