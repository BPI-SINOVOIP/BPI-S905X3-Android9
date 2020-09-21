# Citadel Test Harness #

This project houses the code needed to run tests from a host connected to the
citdadel chip.

## Requirements ##

  * Software
   * bazel
   * build-essential
   * gcc-arm-none-eabi
  * Hardware
   * FPGA test setup with ultradebug or Citadel test board

## Quickstart

The command to run the tests from a host machine is:
> bazel run runtests

On Android run:
> mmma -j`nproc` external/nos
Make sure verity is disabled and the system partion is remounted then run:
> adb sync
> adb citadel_integration_tests

