
Android BasicNetworking Sample
===================================

This sample demonstrates how to check network connectivity with Android APIs.

Introduction
------------

It utilizes the [`ConnectivityManager`][1] to determine if you have
a network connection, and if so, what type of connection it is.

A [`NetworkInfo`][2] object is retrieved from the ConnectivityManager, which contains information
on the active connection, and then the connection type is printed to an on-screen console.

Multiple types of connectivity can be displayed and could be used to take different measures
in actual production code.

[1]: https://developer.android.com/reference/android/net/ConnectivityManager.html
[2]: https://developer.android.com/reference/android/net/NetworkInfo.html

Pre-requisites
--------------

- Android SDK 27
- Android Build Tools v27.0.2
- Android Support Repository

Screenshots
-------------

<img src="screenshots/start.png" height="400" alt="Screenshot"/> <img src="screenshots/tested.png" height="400" alt="Screenshot"/> 

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-BasicNetworking

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
