## 8.4\. Power Consumption Accounting

A more accurate accounting and reporting of the power consumption provides the
app developer both the incentives and the tools to optimize the power usage
pattern of the application.


Device implementations:

*   [SR] STRONGLY RECOMMENDED to provide a per-component power profile
that defines the [current consumption value](
http://source.android.com/devices/tech/power/values.html)
for each hardware component and the approximate battery drain caused by the
components over time as documented in the Android Open Source Project site.
*   [SR] STRONGLY RECOMMENDED to report all power consumption values in milliampere
hours (mAh).
*   [SR] STRONGLY RECOMMENDED to report CPU power consumption per each process's UID.
The Android Open Source Project meets the requirement through the
`uid_cputime` kernel module implementation.
*   [SR] STRONGLY RECOMMENDED to make this power usage available via the
[`adb shell dumpsys batterystats`](
http://source.android.com/devices/tech/power/batterystats.html)
shell command to the app developer.
*   SHOULD be attributed to the hardware component itself if unable to
attribute hardware component power usage to an application.



