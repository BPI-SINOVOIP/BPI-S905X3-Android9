
Android WearComplicationProvidersTestSuite Sample
===================================

Complication Test Suite is a set of complication providers that provide dummy data and it can be
used to test how different types of complications render on a watch face.

Introduction
------------

Steps for trying out the sample:
* Compile and install the wearable app onto your Wear device or emulator (for Wear scenario).

* This sample does not have a main Activity (just Services that provide the complication data).
Therefore, you may see an error next to the 'Run' button. To fix, click on the
"Wearable" dropdown next to the 'Run' button and select 'Edit Configurations'. Under the
'Launch Options', change the 'Launch' field from 'Default APK' to 'Nothing' and save.

This sample provides dummy data for testing the complications UI in your watch face. After
selecting a type from your watch face configuration Activity, you can tap on the complications to
see more options.

The Wear app demonstrates the use of [ComplicationData][1], [ComplicationManager][2],
[ComplicationProviderService][3], [ComplicationText][4], and [ProviderUpdateRequester][5].

[1]: https://developer.android.com/reference/android/support/wearable/complications/ComplicationData.html
[2]: https://developer.android.com/reference/android/support/wearable/complications/ComplicationManager.html
[3]: https://developer.android.com/reference/android/support/wearable/complications/ComplicationProviderService.html
[4]: https://developer.android.com/reference/android/support/wearable/complications/ComplicationText.html
[5]: https://developer.android.com/reference/android/support/wearable/complications/ProviderUpdateRequester.html

Pre-requisites
--------------

- Android SDK 27
- Android Build Tools v27.0.2
- Android Support Repository

Screenshots
-------------

<img src="screenshots/wear-1.png" height="400" alt="Screenshot"/> <img src="screenshots/wear-2.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-WearComplicationProvidersTestSuite

Patches are encouraged, and may be submitted by forking this project and
submitting a pull request through GitHub. Please see CONTRIBUTING.md for more details.

License
-------

Copyright 2017 The Android Open Source Project, Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements.  See the NOTICE file distributed with this work for
additional information regarding copyright ownership.  The ASF licenses this
file to you under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy of
the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations under
the License.
