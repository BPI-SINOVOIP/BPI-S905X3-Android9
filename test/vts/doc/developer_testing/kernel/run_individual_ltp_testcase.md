# How to run individual LTP test case
## 1. Compile VTS and LTP source code
`$ make vts -j12`

## 2. Start VTS-TradeFed
`$ vts-tradefed`

## 3. Run individual LTP test from VTS-TradeFed console
`> run vts-kernel -m VtsKernelLtp -t <testname1,testname2,...>`

Test names in filter can be any of the following formats:
`<testsuite.testname>`, `<testsuite.testname_bitness>`

It is recommended to include test suite, i.e., the first two formats above.

Example:

`> run vts-kernel -m VtsKernelLtp -t syscalls.accept01,mm.mmapstress05_64bit`

Test cases specified in this filter will be run regardless of
whether they are disabled in configuration, unless a specified test case name
is not listed in the definitions or a required executable was not built.