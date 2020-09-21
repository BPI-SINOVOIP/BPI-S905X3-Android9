# SampleDvbTuner

SampleDvbTuner is the reference DVB Tuner. Partners should copy these files to
their own directory and modify as needed.

## Prerequisites

*   A DVB Tuner
    *   A Nexus player with a USB Tuner attached will work.
*   system privileged app
    *   The DVB_DEVICE permission requires the app to be a privileged system app

## First install

#### Root

```bash
adb root
adb remount
```

### modify privapp-permissions-atv.xml

Edit system/etc/permissions/privapp-permissions-atv.xml

```xml
<privapp-permissions package="com.android.tv.tuner.sample.dvb">
    <permission name="android.permission.DVB_DEVICE"/>
</privapp-permissions>
```

### Push to system/priv-app

```bash
adb shell mkdir /system/priv-app/SampleDvbTuner
adb push <path to apk>  /system/priv-app/SampleDvbTuner/SampleDvbTuner.apk
```
