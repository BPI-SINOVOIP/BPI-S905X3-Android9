# VTS Test Plans

A VTS test plan consists of a set of VTS tests. Typically the tests within a
plan are for the same target component or testing the same or similar aspect
(e.g., functionality, performance, robustness, or power). There are three kinds
of plans in VTS:

## 1. Official Plans

Official plans contain only verified tests, and are for all Android developers
and partners.

 * __vts__: For all default VTS tests.
 * __vts-firmware__: For all default VTS System Firmware tests.
 * __vts-fuzz__: For all default VTS fuzz tests.
 * __vts-hal__: For all default VTS HAL (hardware abstraction layer) module tests.
 * __vts-hal-profiling__: For all default VTS HAL performance profiling tests.
 * __vts-hal-replay__: For all default VTS HAL replay tests.
 * __vts-kernel__: For all default VTS kernel tests.
 * __vts-library__: For all default VTS library tests.
 * __vts-performance__: For all default VTS performance tests

## 2. Staging Plans

Serving plans contain experimental tests, and are for Android
partners to use as part of their continuous integration infrastructure. The
name of a serving plan always starts with 'vts-staging-'.

## 3. Other Plans

The following plans are also available for development purposes.

 * __vts-camera-its__: For camera ITS (Image Test Suite) tests ported to VTS.
 * __vts-codelab__: For VTS codelab.
 * __vts-codelab-multi-device__: For VTS codelab of multi-device testing.
 * __vts-gce__: For VTS tests which can be run on Google Compute Engine (GCE)
 * __vts-hal-auto__: For VTS automotive vehicle HAL test.
 * __vts-hal-tv__: For VTS tv HAL test.
 * __vts-host__: For VTS host-driven tests.
 * __vts-performance-systrace__: For VTS performance tests with systrace
   enabled.
 * __vts-presubmit__: For VTS pre-submit time tests.
 * __vts-security__: For VTS security tests.
 * __vts-system__: For VTS system tests.
 * __vts-vndk__: for VTS vndk tests.
