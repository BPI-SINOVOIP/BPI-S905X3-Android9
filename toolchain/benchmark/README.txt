This is a Android Toolchain benchmark suite.
===========================================
Where to find this suite:
	This suite locates at toolchain/benchmark under Android repository tree.

To use this suite:
	1. Configure the basic envrionment in env_setting file.

	2. Run ./apply_patches.py, which will:
		1) Create non-existing benchmarks in the Android tree. Apply
		patch to some benchmarks to make them workable to the suite.

		2) Apply patch autotest.diff to android_root/external/autotest,
		which includes all the test scripts for benchmarks. Copy
		testcases to related autotest directory.

	   If you have applied the patch partially or hope to discard all the
	   patches, just run ./discard_patches.py

	3. Build and run benchmark on the device using ./run.py. You can either
	use test configuration file (-t test_config), or set all the variables
	manually.

	4. The raw results locate at bench_result_* in bench suite home
	directory.

	5. The JSON format result will be generated for crosperf report.

Utility tools:
	1. Autotest is a test framework located in android exteranl/autotest
		Before first time running it, please run
		utils/build_externals.py first to ensure all the environments
		and tools needed are installed.

	2. Crosperf is a report generating tool in ChromeOS toolchain
	utilities, which has a mirror at external/toolchain-utils in Android
	tree.
