
Android BasicMultitouch Sample
===================================

Sample demonstrates the use of [MotionEvent][1] properties to keep track of
individual touches across multiple touch events.

[1]: http://developer.android.com/reference/android/view/MotionEvent.html

Introduction
------------

This is an example of keeping track of individual touches across multiple
[MotionEvent][1]s.

This sample uses a custom View (`TouchDisplayView`) that responds to
touch events and draws a colored circle for each touch point. The view holds
data related to a touch pointer, including its current position, pressure,
and its past touch history.

The View draws graphics based on data associated with each touch event to a
canvas. A large circle indicates the current position of a touch, while smaller
trailing circles represent previous positions for that touch.
The size of the large circle is scaled depending on the pressure of the user's
touch.

[1]: http://developer.android.com/reference/android/view/MotionEvent.html

Pre-requisites
--------------

- Android SDK 24
- Android Build Tools v24.0.2
- Android Support Repository

Screenshots
-------------

<img src="screenshots/intro.png" height="400" alt="Screenshot"/> <img src="screenshots/touches.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-BasicMultitouch

Patches are encouraged, and may be submitted by forking this project and
submitting a pull request through GitHub. Please see CONTRIBUTING.md for more details.

License
-------

Copyright 2016 The Android Open Source Project, Inc.

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
