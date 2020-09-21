# Contributing

DroidDriver issues are [tracked on GitHub](https://github.com/appium/droiddriver/issues)

The [`master` branch](https://github.com/appium/droiddriver/tree/master) on GitHub tracks [the AOSP master branch](https://android.googlesource.com/platform/external/droiddriver). [All releases](releasing_to_jcenter.md) are made from the master branch.

Code changes should be [submitted to AOSP](contributing_aosp.md) and then they'll be synced to GitHub once they've passed code reivew on Gerrit.

#### Build

`./gradlew build`

#### Import into Android Studio

- Clone from git
- Launch Android Studio and select `Open an existing Android Studio project`
- Navigate to `droiddriver/build.gradle` and press Choose
- Android Studio will now import the project successfully
