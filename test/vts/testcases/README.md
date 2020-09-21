In principle, VTS test cases are co-located with their respective projects.

* HIDL HAL Tests: `test/vts-testcase/hal/<hal name>/<version>` and `hardware/interfaces/<hal name>/<version>/vts`

* Kernel Tests: `test/vts-testcase/kernel`

 * LTP: `external/ltp`

 * Linux Kselftest: `external/linux-kselftest`

* VNDK (Vendor Native Development Kit) Tests: `test/vts-testcase/vndk`

* Performance Tests: `test/vts-testcase/performance`

* Fuzz Tests: `test/vts-testcase/fuzz`

* Security Tests: `test/vts-testcase/security`

The files under this directory (`test/vts/testcases`) are only for:
VTS codelab, template, tests for legacy components, and tests that are
under development and do not have any respective projects.
