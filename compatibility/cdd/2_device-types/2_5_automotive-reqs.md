## 2.5\. Automotive Requirements

**Android Automotive implementation** refers to a vehicle head unit running
Android as an operating system for part or all of the system and/or
infotainment functionality.

Android device implementations are classified as an Automotive if they declare
the feature `android.hardware.type.automotive` or meet all the following
criteria.

*   Are embedded as part of, or pluggable to, an automotive vehicle.
*   Are using a screen in the driver's seat row as the primary display.

The additional requirements in the rest of this section are specific to Android
Automotive device implementations.

### 2.5.1\. Hardware

Automotive device implementations:

*   [[7.1](#7_1_display-and-graphics).1.1/A-0-1] MUST have a screen at least 6
inches in physical diagonal size.
*   [[7.1](#7_1_display-and-graphics).1.1/A-0-2] MUST have a screen size layout
of at least 750 dp x 480 dp.

*   [[7.2](#7_2_input-devices).3/A-0-1] MUST provide the Home function and MAY
provide Back and Recent functions.
*   [[7.2](#7_2_input-devices).3/A-0-2] MUST send both the normal and long press
event of the Back function ([`KEYCODE_BACK`](
http://developer.android.com/reference/android/view/KeyEvent.html#KEYCODE_BACK))
to the foreground application.

*   [[7.3](#7_3_sensors).1/A-SR] Are STRONGLY RECOMMENDED to include a 3-axis
accelerometer.

If Automotive device implementations include a 3-axis accelerometer, they:

*   [[7.3](#7_3_sensors).1/A-1-1] MUST be able to report events up to a
frequency of at least 100 Hz.
*   [[7.3](#7_3_sensors).1/A-1-2] MUST comply with the Android
[car sensor coordinate system](
http://source.android.com/devices/sensors/sensor-types.html#auto_axes).

If Automotive device implementations include a GPS/GNSS receiver and report
the capability to applications through the `android.hardware.location.gps`
feature flag:

*   [[7.3](#7_3_sensors).3/A-1-1] GNSS technology generation MUST be the year
"2017" or newer.

If Automotive device implementations include a gyroscope, they:

*   [[7.3](#7_3_sensors).4/A-1-1] MUST be able to report events up to a
frequency of at least 100 Hz.

Automotive device implementations:

*    [[7.3](#7_3_sensors).11/A] SHOULD provide current gear as
`SENSOR_TYPE_GEAR`.

Automotive device implementations:

*    [[7.3](#7_3_sensors).11.2/A-0-1] MUST support day/night mode defined as
`SENSOR_TYPE_NIGHT`.
*    [[7.3](#7_3_sensors).11.2/A-0-2] The value of the `SENSOR_TYPE_NIGHT` flag
MUST be consistent with dashboard day/night mode and SHOULD be based on ambient
light sensor input.
*    The underlying ambient light sensor MAY be the same as
[Photometer](#7_3_7_photometer).

*    [[7.3](#7_3_sensors).11.3/A-0-1] MUST support driving status defined as
     `SENSOR_TYPE_DRIVING_STATUS`, with a default value of
     `DRIVE_STATUS_UNRESTRICTED` when the vehicle is fully stopped and parked.
     It is the responsibility of device manufacturers to configure
     `SENSOR_TYPE_DRIVING_STATUS` in compliance with all laws and regulations
     that apply to markets where the product is shipping.

*    [[7.3](#7_3_sensors).11.4/A-0-1] MUST provide vehicle speed defined as
`SENSOR_TYPE_CAR_SPEED`.

*    [[7.4](#7_4_data-connectivity).3/A-0-1] MUST support Bluetooth and SHOULD
support Bluetooth LE.
*    [[7.4](#7_4_data-connectivity).3/A-0-2] Android Automotive implementations
MUST support the following Bluetooth profiles:
     * Phone calling over Hands-Free Profile (HFP).
     * Media playback over Audio Distribution Profile (A2DP).
     * Media playback control over Remote Control Profile (AVRCP).
     * Contact sharing using the Phone Book Access Profile (PBAP).
*    [[7.4](#7_4_data-connectivity).3/A] SHOULD support Message Access Profile
(MAP).

*   [[7.4](#7_4_data-connectivity).5/A] SHOULD include support for cellular
network based data connectivity.

*   [[7.6](#7_6_memory-and-storage).1/A-0-1] MUST have at least 4GB of
non-volatile storage available for application private data
(a.k.a. "/data" partition).

If Automotive device implementations are 32-bit:

*   [[7.6](#7_6_memory-and-storage).1/A-1-1] The memory available to the kernel
and userspace MUST be at least 512MB if any of the following densities are used:
     *    280dpi or lower on small/normal screens
     *    ldpi or lower on extra large screens
     *    mdpi or lower on large screens

*   [[7.6](#7_6_memory-and-storage).1/A-1-2] The memory available to the kernel
and userspace MUST be at least 608MB if any of the following densities are used:
     *   xhdpi or higher on small/normal screens
     *   hdpi or higher on large screens
     *   mdpi or higher on extra large screens

*   [[7.6](#7_6_memory-and-storage).1/A-1-3] The memory available to the kernel
and userspace MUST be at least 896MB if any of the following densities are used:
     *   400dpi or higher on small/normal screens
     *   xhdpi or higher on large screens
     *   tvdpi or higher on extra large screens

*   [[7.6](#7_6_memory-and-storage).1/A-1-4] The memory available to the kernel
and userspace MUST be at least 1344MB if any of the following densities are
used:
     *   560dpi or higher on small/normal screens
     *   400dpi or higher on large screens
     *   xhdpi or higher on extra large screens

If Automotive device implementations are 64-bit:

*   [[7.6](#7_6_memory-and-storage).1/A-2-1] The memory available to the kernel
and userspace MUST be at least 816MB if any of the following densities are used:
     *   280dpi or lower on small/normal screens
     *   ldpi or lower on extra large screens
     *   mdpi or lower on large screens

*   [[7.6](#7_6_memory-and-storage).1/A-2-2] The memory available to the kernel
and userspace MUST be at least 944MB if any of the following densities are used:
     *   xhdpi or higher on small/normal screens
     *   hdpi or higher on large screens
     *   mdpi or higher on extra large screens

*   [[7.6](#7_6_memory-and-storage).1/A-2-3] The memory available to the kernel
and userspace MUST be at least 1280MB if any of the following densities are used:
     *  400dpi or higher on small/normal screens
     *  xhdpi or higher on large screens
     *  tvdpi or higher on extra large screens

*   [[7.6](#7_6_memory-and-storage).1/A-2-4] The memory available to the kernel
and userspace MUST be at least 1824MB if any of the following densities are used:
     *   560dpi or higher on small/normal screens
     *   400dpi or higher on large screens
     *   xhdpi or higher on extra large screens

Note that the "memory available to the kernel and userspace" above refers to the
memory space provided in addition to any memory already dedicated to hardware
components such as radio, video, and so on that are not under the kernel’s
control on device implementations.

Automotive device implementations:

*   [[7.7](#7_7_usb).1/A] SHOULD include a USB port supporting peripheral mode.

Automotive device implementations:

*   [[7.8](#7_8_audio).1/A-0-1] MUST include a microphone.

Automotive device implementations:

*   [[7.8](#7_8_audio).2/A-0-1] MUST have an audio output and declare
    `android.hardware.audio.output`.

### 2.5.2\. Multimedia

Automotive device implementations MUST support the following audio encoding:

*    [[5.1](#5_1_media-codecs)/A-0-1] MPEG-4 AAC Profile (AAC LC)
*    [[5.1](#5_1_media-codecs)/A-0-2] MPEG-4 HE AAC Profile (AAC+)
*    [[5.1](#5_1_media-codecs)/A-0-3] AAC ELD (enhanced low delay AAC)

Automotive device implementations MUST support the following video encoding:

*    [[5.2](#5_2_video-encoding)/A-0-1] H.264 AVC
*    [[5.2](#5_2_video-encoding)/A-0-2] VP8

Automotive device implementations MUST support the following video decoding:

*    [[5.3](#5_3_video-decoding)/A-0-1] H.264 AVC
*    [[5.3](#5_3_video-decoding)/A-0-2] MPEG-4 SP
*    [[5.3](#5_3_video-decoding)/A-0-3] VP8
*    [[5.3](#5_3_video-decoding)/A-0-4] VP9

Automotive device implementations are STRONGLY RECOMMENDED to support the
following video decoding:

*    [[5.3](#5_3_video-decoding)/A-SR] H.265 HEVC


### 2.5.3\. Software

Automotive device implementations:

*   [[3](#3_0_intro)/A-0-1] MUST declare the feature
`android.hardware.type.automotive`.
*   [[3](#3_0_intro)/A-0-2] MUST support uiMode = [UI_MODE_TYPE_CAR](
http://developer.android.com/reference/android/content/res/Configuration.html#UI_MODE_TYPE_CAR).
*   [[3](#3_0_intro)/A-0-3] Android Automotive implementations MUST support all
public APIs in the `android.car.*` namespace.

*   [[3.4](#3_4_web-compatibility).1/A-0-1] MUST provide a complete
implementation of the `android.webkit.Webview` API.

*   [[3.8](#3_8_user-interface-compatibility).3/A-0-1] MUST display
notifications that use the [`Notification.CarExtender`](
https://developer.android.com/reference/android/app/Notification.CarExtender.html)
API when requested by third-party applications.

*   [[3.8](#3_8_user-interface-compatibility).4/A-0-1] MUST implement an
assistant on the device to handle the [Assist action](
http://developer.android.com/reference/android/content/Intent.html#ACTION_ASSIST).

*   [[3.14](#3_14_media_ui)/A-0-1] MUST include a UI framework to support
third-party apps using the media APIs as described in section 3.14.

### 2.2.4\. Performance and Power

Automotive device implementations:

*   [[8.3](#8_3_power-saving-modes)/A-0-1] All Apps exempted from App Standby
and Doze power-saving modes MUST be made visible to the end user.
*   [[8.3](#8_3_power-saving-modes)/A-0-2] The triggering, maintenance, wakeup
algorithms and the use of global system settings of App Standby and Doze
power-saving modes MUST not deviate from the Android Open Source Project.


*   [[8.4](#8_4_power-consumption-accounting)/A-0-1] MUST provide a
per-component power profile that defines the [current consumption value](
http://source.android.com/devices/tech/power/values.html)
for each hardware component and the approximate battery drain caused by the
components over time as documented in the Android Open Source Project site.
*   [[8.4](#8_4_power-consumption-accounting)/A-0-2] MUST report all power
consumption values in milliampere hours (mAh).
*   [[8.4](#8_4_power-consumption-accounting)/A-0-3] MUST report CPU power
consumption per each process's UID. The Android Open Source Project meets the
requirement through the `uid_cputime` kernel module implementation.
*   [[8.4](#8_4_power-consumption-accounting)/A] SHOULD be attributed to the
hardware component itself if unable to attribute hardware component power usage
to an application.
*   [[8.4](#8_4_power-consumption-accounting)/A-0-4] MUST make this power usage
available via the [`adb shell dumpsys batterystats`](
http://source.android.com/devices/tech/power/batterystats.html)
shell command to the app developer.

### 2.2.5\. Security Model


If Automotive device implementations include multiple users, they:

*   [[9.5](#9_5_multi-user-support)/A-1-1] MUST include a guest account that
allows all functions provided by the vehicle system without requiring a user to
log in.

Automotive device implementations:

*   [[9.14](#9_14_automotive-system-isolation)/A-0-1] MUST gatekeep messages
from Android framework vehicle subsystems, e.g., whitelisting permitted message
types and message sources.
*   [[9.14](#9_14_automotive-system-isolation)/A-0-2] MUST watchdog against
denial of service attacks from the Android framework or third-party apps. This
guards against malicious software flooding the vehicle network with traffic,
which may lead to malfunctioning vehicle subsystems.
