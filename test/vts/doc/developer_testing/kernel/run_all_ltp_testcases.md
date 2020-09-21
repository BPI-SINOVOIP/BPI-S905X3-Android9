# How to run all the enabled LTP tests
## 1. Compile VTS and LTP source code
`$ make vts -j12`

## 2. Start VTS-TradeFed
`$ vts-tradefed`

## 3. Run kernel LTP test from VTS-TradeFed console
`> run vts-kernel`

This will take from 30 minutes to 2 hours to run.
The test results can be found under `out/host/linux-x86/vts/android-vts/results/`,
while the device logcat and host logs can be found under `out/host/linux-x86/vts/android-vts/logs/`.
