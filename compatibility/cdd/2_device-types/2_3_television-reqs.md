## 2.3\. Television Requirements

An **Android Television device** refers to an Android device implementation that
is an entertainment interface for consuming digital media, movies, games, apps,
and/or live TV for users sitting about ten feet away (a “lean back” or “10-foot
user interface”).

Android device implementations are classified as a Television if they meet all
the following criteria:

*   Have provided a mechanism to remotely control the rendered user interface on
    the display that might sit ten feet away from the user.
*   Have an embedded screen display with the diagonal length larger than 24
    inches OR include a video output port, such as VGA, HDMI, DisplayPort or a
    wireless port for display.

The additional requirements in the rest of this section are specific to Android
Television device implementations.

### 2.3.1\. Hardware

Television device implementations:

*   [[7.2](#7_2_input-devices).2/T-0-1] MUST support [D-pad](
https://developer.android.com/reference/android/content/res/Configuration.html#NAVIGATION_DPAD).
*   [[7.2](#7_2_input-devices).3/T-0-1] MUST provide the Home and Back
functions.
*   [[7.2](#7_2_input-devices).3/T-0-2] MUST send both the normal and long press
event of the Back function ([`KEYCODE_BACK`](
http://developer.android.com/reference/android/view/KeyEvent.html#KEYCODE_BACK))
to the foreground application.
*   [[7.2](#7_2_input-devices).6.1/T-0-1] MUST include support for game
controllers and declare the `android.hardware.gamepad` feature flag.
*   [[7.2](#7_2_input-devices).7/T] SHOULD provide a remote control from which
users can access [non-touch navigation](#7_2_2_non-touch_navigation) and
[core navigation keys](#7_2_3_navigation_keys) inputs.


If Television device implementations include a gyroscope, they:

*   [[7.3](#7_3_sensors).4/T-1-1] MUST be able to report events up to a
frequency of at least 100 Hz.


Television device implementations:

*   [[7.4](#7_4_data-connectivity).3/T-0-1] MUST support Bluetooth and
Bluetooth LE.
*   [[7.6](#7_6_memory-and-storage).1/T-0-1] MUST have at least 4GB of
non-volatile storage available for application private data
(a.k.a. "/data" partition).

If TV device implementations are 32-bit:

*   [[7.6](#7_6_memory-and-storage).1/T-1-1] The memory available to the kernel
and userspace MUST be at least 896MB if any of the following densities are used:

    *   400dpi or higher on small/normal screens
    *   xhdpi or higher on large screens
    *   tvdpi or higher on extra large screens

If TV device implementations are 64-bit:

*   [[7.6](#7_6_memory-and-storage).1/T-2-1] The memory available to the kernel
and userspace MUST be at least 1280MB if any of the following densities are
used:

    *   400dpi or higher on small/normal screens
    *   xhdpi or higher on large screens
    *   tvdpi or higher on extra large screens

Note that the "memory available to the kernel and userspace" above refers to the
memory space provided in addition to any memory already
dedicated to hardware components such as radio, video, and so on that are not
under the kernel’s control on device implementations.

Television device implementations:

*   [[7.8](#7_8_audio).1/T] SHOULD include a microphone.
*   [[7.8](#7_8_audio).2/T-0-1] MUST have an audio output and declare
    `android.hardware.audio.output`.

### 2.3.2\. Multimedia

Television device implementations MUST support the following audio encoding formats:

*    [[5.1](#5_1_media-codecs)/T-0-1] MPEG-4 AAC Profile (AAC LC)
*    [[5.1](#5_1_media-codecs)/T-0-2] MPEG-4 HE AAC Profile (AAC+)
*    [[5.1](#5_1_media-codecs)/T-0-3] AAC ELD (enhanced low delay AAC)


Television device implementations MUST support the following video encoding formats:

*    [[5.2](#5_2_video-encoding)/T-0-1] H.264 
*    [[5.2](#5_2_video-encoding)/T-0-2] VP8

Television device implementations:

*   [[5.2](#5_2_video-encoding).2/T-SR] Are STRONGLY RECOMMENDED to support
H.264 encoding of 720p and 1080p resolution videos at 30 frames per second.

Television device implementations MUST support the following video decoding formats:

*    [[5.3.3](#5_3_video-decoding)/T-0-1] MPEG-4 SP
*    [[5.3.4](#5_3_video-decoding)/T-0-2] H.264 AVC
*    [[5.3.5](#5_3_video-decoding)/T-0-3] H.265 HEVC
*    [[5.3.6](#5_3_video-decoding)/T-0-4] VP8
*    [[5.3.7](#5_3_video-decoding)/T-0-5] VP9

Television device implementations are STRONGLY RECOMMENDED to support the
following video decoding formats:

*    [[5.3.1](#5_3_video-decoding)/T-SR] MPEG-2

Television device implementations MUST support H.264 decoding, as detailed in Section 5.3.4, 
at standard video frame rates and resolutions up to and including:

*   [[5.3.4](#5_3_video-decoding).4/T-1-1] HD 1080p at 60 frames per second with Basline Profile
*   [[5.3.4](#5_3_video-decoding).4/T-1-2] HD 1080p at 60 frames per second with Main Profile 
*   [[5.3.4](#5_3_video-decoding).4/T-1-3] HD 1080p at 60 frames per second with High Profile Level 4.2

Television device implementations  with H.265 hardware decoders MUST support H.265 decoding, 
as detailed in Section 5.3.5, at standard video frame rates and resolutions up to and including:

*   [[5.3.5](#5_3_video-decoding).4/T-1-1] HD 1080p at 60 frames per second with Main Profile Level 4.1

If Television device implementations with H.265 hardware decoders support 
H.265 decoding and the UHD decoding profile, they:

*   [[5.3.5](#5_3_video-decoding).5/T-2-1] MUST support UHD 3480p at 60 frames per second with Main10 Level 5
Main Tier profile.

Television device implementations MUST support VP8 decoding, as detailed in Section 5.3.6, 
at standard video frame rates and resolutions up to and including:

*   [[5.3.6](#5_3_video-decoding).4/T-1-1] HD 1080p at 60 frames per second decoding profile

Television device implementations  with VP9 hardware decoders MUST support VP9 decoding, as detailed in Section 5.3.7, 
at standard video frame rates and resolutions up to and including:

*   [[5.3.7](#5_3_video-decoding).4/T-1-1] HD 1080p at 60 frames per second with profile 0 (8 bit colour depth) 

If Television device implementations with VP9 hardware decoders support VP9 decoding and the UHD decoding
profile, they:

*   [[5.3.7](#5_3_video-decoding).5/T-2-1] MUST support UHD 3480p at 60 frames per second with profile 0 (8 bit colour depth).
*   [[5.3.7](#5_3_video-decoding).5/T-2-1] Are STRONGLY RECOMMENDED to support UHD 3480p at 60 frames per second with profile 2 (10 bit colour depth).

Television device implementations:

*    [[5.8](#5_8_secure-media)/T-SR] Are STRONGLY RECOMMENDED to support
simultaneous decoding of secure streams. At minimum, simultaneous decoding of
two steams is STRONGLY RECOMMENDED.

If device implementations are Android Television devices and support 4K
resolution, they:

*    [[5.8](#5_8_secure-media)/T-1-1] MUST support HDCP 2.2 for all wired
external displays.

If Television device implementations don't support 4K resolution, they:

*    [[5.8](#5_8_secure-media)/T-2-1] MUST support HDCP 1.4 for all wired
external displays.

Television device implementations:

*   [[5.5](#5_5_audio-playback).3/T-0-1] MUST include support for system Master
Volume and digital audio output volume attenuation on supported outputs,
except for compressed audio passthrough output (where no audio decoding is done
on the device).


### 2.3.3\. Software

Television device implementations:

*    [[3](#3_0_intro)/T-0-1] MUST declare the features
[`android.software.leanback`](
http://developer.android.com/reference/android/content/pm/PackageManager.html#FEATURE_LEANBACK)
and `android.hardware.type.television`.
*   [[3.4](#3_4_web-compatibility).1/T-0-1] MUST provide a complete
implementation of the `android.webkit.Webview` API.

If Android Television device implementations support a lock screen,they:

*   [[3.8](#3_8_user-interface-compatibility).10/T-1-1] MUST display the Lock
screen Notifications including the Media Notification Template.

Television device implementations:

*   [[3.8](#3_8_user-interface-compatibility).14/T-SR] Are STRONGLY RECOMMENDED
to support picture-in-picture (PIP) mode multi-window.
*   [[3.10](#3_10_accessibility)/T-0-1] MUST support third-party accessibility
services.
*   [[3.10](#3_10_accessibility)/T-SR] Are STRONGLY RECOMMENDED to
    preload accessibility services on the device comparable with or exceeding
    functionality of the Switch Access and TalkBack (for languages supported by
    the preloaded Text-to-speech engine) accessibility services as provided in
    the [talkback open source project](https://github.com/google/talkback).


If Television device implementations report the feature
`android.hardware.audio.output`, they:

*   [[3.11](#3_11_text-to-speech)/T-SR] Are STRONGLY RECOMMENDED to include a
TTS engine supporting the languages available on the device.
*   [[3.11](#3_11_text-to-speech)/T-1-1] MUST support installation of
third-party TTS engines.


Television device implementations:

*    [[3.12](#3_12_tv-input-framework)/T-0-1] MUST support TV Input Framework.


### 2.2.4\. Performance and Power

*   [[8.1](#8_1_user-experience-consistency)/T-0-1] **Consistent frame latency**.
   Inconsistent frame latency or a delay to render frames MUST NOT happen more
   often than 5 frames in a second, and SHOULD be below 1 frames in a second.
*   [[8.2](#8_2_file-io-access-performance)/T-0-1] MUST ensure a sequential
   write performance of at least 5MB/s.
*   [[8.2](#8_2_file-io-access-performance)/T-0-2] MUST ensure a random write
   performance of at least 0.5MB/s.
*   [[8.2](#8_2_file-io-access-performance)/T-0-3] MUST ensure a sequential
   read performance of at least 15MB/s.
*   [[8.2](#8_2_file-io-access-performance)/T-0-4] MUST ensure a random read
   performance of at least 3.5MB/s.


*   [[8.3](#8_3_power-saving-modes)/T-0-1] All apps exempted from App Standby
and Doze power-saving modes MUST be made visible to the end user.
*   [[8.3](#8_3_power-saving-modes)/T-0-2] The triggering, maintenance, wakeup
algorithms and use of global system settings of App Standby and Doze
power-saving modes MUST not deviate from the Android Open Source Project.

Television device implementations:

*    [[8.4](#8_4_power-consumption-accounting)/T-0-1] MUST provide a
per-component power profile that defines the [current consumption value](
http://source.android.com/devices/tech/power/values.html)
for each hardware component and the approximate battery drain caused by the
components over time as documented in the Android Open Source Project site.
*    [[8.4](#8_4_power-consumption-accounting)/T-0-2] MUST report all power
consumption values in milliampere hours (mAh).
*    [[8.4](#8_4_power-consumption-accounting)/T-0-3] MUST report CPU power
consumption per each process's UID. The Android Open Source Project meets the
requirement through the `uid_cputime` kernel module implementation.
*    [[8.4](#8_4_power-consumption-accounting)/T] SHOULD be attributed to the
hardware component itself if unable to attribute hardware component power usage
to an application.
*   [[8.4](#8_4_power-consumption-accounting)/T-0-4] MUST make this power usage
available via the [`adb shell dumpsys batterystats`](
http://source.android.com/devices/tech/power/batterystats.html)
shell command to the app developer.
