The AudioLoopbackTest is used to test round-trip audio performance over headset jack and USB.
It is based on the Loopback.apk run with Loopback dongle: https://source.android.com/devices/audio/loopback

* How to run the test?
  1. plug in the Loopback dongle to your device.
  2. `make vts`
  3. `vts-tradefed run vts -m AudioLoopbackTest`

* What metrics this test report?
  1. Average round-trip latency.
  2. Standard deviation of round-trip latency.

* Which version of Loopback.apk is used in the test?
  The Loopback.apk used in this test is Loopback-v17 copied from https://tradefed.teams.x20web.corp.google.com/testdata/media/apps/

* Where can I get the source code for the Loopback.apk?
  Internal developer: https://googleplex-android.git.corp.google.com/platform/vendor/google_toolbox/+/master/user/rago/studio/LoopbackApp/
  Open source: https://github.com/gkasten/drrickorang