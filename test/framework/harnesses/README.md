To build a new CTS TF prebuilt binary

`$ lunch aosp_arm64`
`$ make cts-tradefed -j

To release to TEST
`$ cp out/host/linux-x86/testcases/cts-tradefed/cts-tradefed.jar test/framework/harnesses/cts-tradefed/tradefed-cts-prebuilt-staging.jar`

To release to PROD
`$ cp test/framework/harnesses/cts-tradefed/tradefed-cts-prebuilt-staging.jar test/framework/harnesses/cts-tradefed/tradefed-cts-prebuilt.jar`

To test a test suite which uses that prebuilt binary

`$ make vts -j32`

