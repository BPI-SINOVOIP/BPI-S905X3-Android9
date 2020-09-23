
Android BasicGestureDetect Sample
===================================

This sample detects gestures on a view and logs them. In order to try this
sample out, try dragging or tapping the text.

Introduction
------------

In this sample, the gestures are detected using a custom gesture listener that extends
[SimpleOnGestureListener][1] and writes the detected [MotionEvent][2] into the log.

In this example, the steps followed to set up the gesture detector are:
1. Create the GestureListener that includes all your callbacks.
2. Create the GestureDetector ([SimpleOnGestureListener][1]) that will take the listener as an argument.
3. For the view where the gestures will occur, create an [onTouchListener][3]
that sends all motion events to the gesture detector.

[1]: http://developer.android.com/reference/android/view/GestureDetector.SimpleOnGestureListener.html
[2]: http://developer.android.com/reference/android/view/MotionEvent.html
[3]: http://developer.android.com/reference/android/view/View.OnTouchListener.html

Pre-requisites
--------------

- Android SDK 24
- Android Build Tools v24.0.2
- Android Support Repository

Screenshots
-------------

<img src="screenshots/1-main.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-BasicGestureDetect

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
