## 10.1\. Compatibility Test Suite

Device implementations:

*    [C-0-1] MUST pass the [Android Compatibility Test Suite (CTS)](http://source.android.com/compatibility/index.html)
available from the Android Open Source Project, using the final shipping
software on the device.

*    [C-0-2] MUST ensure compatibility in cases of ambiguity in CTS and for any
reimplementations of parts of the reference source code.

The CTS is designed to be run on an actual device. Like any software, the CTS
may itself contain bugs. The CTS will be versioned independently of this
Compatibility Definition, and multiple revisions of the CTS may be released for
Android ANDROID_VERSION.

Device implementations:

*    [C-0-3] MUST pass the latest CTS version available at the time the device
software is completed.

*    SHOULD use the reference implementation in the Android Open Source tree
as much as possible.