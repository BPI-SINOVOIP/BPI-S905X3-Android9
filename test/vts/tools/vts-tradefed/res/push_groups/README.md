# VTS File Push Groups

As part of the test setup procedure, a VTS test can push a set of files to
a target device. A list of files can be defined as a push group (i.e., a `.push`
file in this directory).

A list of the predefined, basic push groups is as follows:

- VtsAgent.push: VTS agent files.
- VtsDriverShell.push: VTS shell driver files.
- VtsDriverHal.push: VTS HAL driver files.
- VtsProfilerHal.push: VTS HAL profiler files.
- VtsSpec.push: VTS specification files for all HIDL HALs.

Based on those, the following push groups are defined where each group is for a
particular test type:

- HalHidlTargetTest.push: For target-side HIDL HAL test.
- HalHidlTargetProfilingTest.push: For target-side, HIDL HAL profiling test.
- HalHidlHostTest.push: For host-driven HIDL HAL test.
- HalHidlHostProfilingTest.push: For host-driven, HIDL HAL profiling test.
- HostDrivenTest.push: For host-driven test (both HIDL HAL and other system-level components).

The other push groups are fuzzing and record-and-replay tests:

- LLVMFuzzerTest.push
- IfaceFuzzerTest.push
- RecordReplayTest.push
