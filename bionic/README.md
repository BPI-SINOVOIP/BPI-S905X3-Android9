Using bionic
============

See the [additional documentation](docs/).

Working on bionic
=================

What are the big pieces of bionic?
----------------------------------

#### libc/ --- libc.so, libc.a

The C library. Stuff like `fopen(3)` and `kill(2)`.

#### libm/ --- libm.so, libm.a

The math library. Traditionally Unix systems kept stuff like `sin(3)` and
`cos(3)` in a separate library to save space in the days before shared
libraries.

#### libdl/ --- libdl.so

The dynamic linker interface library. This is actually just a bunch of stubs
that the dynamic linker replaces with pointers to its own implementation at
runtime. This is where stuff like `dlopen(3)` lives.

#### libstdc++/ --- libstdc++.so

The C++ ABI support functions. The C++ compiler doesn't know how to implement
thread-safe static initialization and the like, so it just calls functions that
are supplied by the system. Stuff like `__cxa_guard_acquire` and
`__cxa_pure_virtual` live here.

#### linker/ --- /system/bin/linker and /system/bin/linker64

The dynamic linker. When you run a dynamically-linked executable, its ELF file
has a `DT_INTERP` entry that says "use the following program to start me".  On
Android, that's either `linker` or `linker64` (depending on whether it's a
32-bit or 64-bit executable). It's responsible for loading the ELF executable
into memory and resolving references to symbols (so that when your code tries to
jump to `fopen(3)`, say, it lands in the right place).

#### tests/ --- unit tests

The `tests/` directory contains unit tests. Roughly arranged as one file per
publicly-exported header file.

#### benchmarks/ --- benchmarks

The `benchmarks/` directory contains benchmarks, with its own [documentation](benchmarks/README.md).


What's in libc/?
----------------

<pre>
libc/
  arch-arm/
  arch-arm64/
  arch-common/
  arch-mips/
  arch-mips64/
  arch-x86/
  arch-x86_64/
    # Each architecture has its own subdirectory for stuff that isn't shared
    # because it's architecture-specific. There will be a .mk file in here that
    # drags in all the architecture-specific files.
    bionic/
      # Every architecture needs a handful of machine-specific assembler files.
      # They live here.
    include/
      machine/
        # The majority of header files are actually in libc/include/, but many
        # of them pull in a <machine/something.h> for things like limits,
        # endianness, and how floating point numbers are represented. Those
        # headers live here.
    string/
      # Most architectures have a handful of optional assembler files
      # implementing optimized versions of various routines. The <string.h>
      # functions are particular favorites.
    syscalls/
      # The syscalls directories contain script-generated assembler files.
      # See 'Adding system calls' later.

  include/
    # The public header files on everyone's include path. These are a mixture of
    # files written by us and files taken from BSD.

  kernel/
    # The kernel uapi header files. These are scrubbed copies of the originals
    # in external/kernel-headers/. These files must not be edited directly. The
    # generate_uapi_headers.sh script should be used to go from a kernel tree to
    # external/kernel-headers/ --- this takes care of the architecture-specific
    # details. The update_all.py script should be used to regenerate bionic's
    # scrubbed headers from external/kernel-headers/.

  private/
    # These are private header files meant for use within bionic itself.

  dns/
    # Contains the DNS resolver (originates from NetBSD code).

  upstream-freebsd/
  upstream-netbsd/
  upstream-openbsd/
    # These directories contain unmolested upstream source. Any time we can
    # just use a BSD implementation of something unmodified, we should.
    # The structure under these directories mimics the upstream tree,
    # but there's also...
    android/
      include/
        # This is where we keep the hacks necessary to build BSD source
        # in our world. The *-compat.h files are automatically included
        # using -include, but we also provide equivalents for missing
        # header/source files needed by the BSD implementation.

  bionic/
    # This is the biggest mess. The C++ files are files we own, typically
    # because the Linux kernel interface is sufficiently different that we
    # can't use any of the BSD implementations. The C files are usually
    # legacy mess that needs to be sorted out, either by replacing it with
    # current upstream source in one of the upstream directories or by
    # switching the file to C++ and cleaning it up.

  malloc_debug/
    # The code that implements the functionality to enable debugging of
    # native allocation problems.

  stdio/
    # These are legacy files of dubious provenance. We're working to clean
    # this mess up, and this directory should disappear.

  tools/
    # Various tools used to maintain bionic.

  tzcode/
    # A modified superset of the IANA tzcode. Most of the modifications relate
    # to Android's use of a single file (with corresponding index) to contain
    # time zone data.
  zoneinfo/
    # Android-format time zone data.
    # See 'Updating tzdata' later.
</pre>


Adding libc wrappers for system calls
-------------------------------------

The first question you should ask is "should I add a libc wrapper for
this system call?". The answer is usually "no".

The answer is "yes" if the system call is part of the POSIX standard.

The answer is probably "yes" if the system call has a wrapper in at
least one other C library.

The answer may be "yes" if the system call has three/four distinct
users in different projects, and there isn't a more specific library
that would make more sense as the place to add the wrapper.

In all other cases, you should use
[syscall(3)](http://man7.org/linux/man-pages/man2/syscall.2.html) instead.

Adding a system call usually involves:

  1. Add entries to SYSCALLS.TXT.
     See SYSCALLS.TXT itself for documentation on the format.
  2. Run the gensyscalls.py script.
  3. Add constants (and perhaps types) to the appropriate header file.
     Note that you should check to see whether the constants are already in
     kernel uapi header files, in which case you just need to make sure that
     the appropriate POSIX header file in libc/include/ includes the
     relevant file or files.
  4. Add function declarations to the appropriate header file. Don't forget
     to include the appropriate `__INTRODUCED_IN()`.
  5. Add the function name to the correct section in libc/libc.map.txt and
     run `./libc/tools/genversion-scripts.py`.
  6. Add at least basic tests. Even a test that deliberately supplies
     an invalid argument helps check that we're generating the right symbol
     and have the right declaration in the header file, and that you correctly
     updated the maps in step 5. (You can use strace(1) to confirm that the
     correct system call is being made.)


Updating kernel header files
----------------------------

As mentioned above, this is currently a two-step process:

  1. Use generate_uapi_headers.sh to go from a Linux source tree to appropriate
     contents for external/kernel-headers/.
  2. Run update_all.py to scrub those headers and import them into bionic.

Note that if you're actually just trying to expose device-specific headers to
build your device drivers, you shouldn't modify bionic. Instead use
`TARGET_DEVICE_KERNEL_HEADERS` and friends described in [config.mk](https://android.googlesource.com/platform/build/+/master/core/config.mk#186).


Updating tzdata
---------------

This is fully automated (and these days handled by the libcore team, because
they own icu, and that needs to be updated in sync with bionic):

  1. Run update-tzdata.py in external/icu/tools/.


Verifying changes
-----------------

If you make a change that is likely to have a wide effect on the tree (such as a
libc header change), you should run `make checkbuild`. A regular `make` will
_not_ build the entire tree; just the minimum number of projects that are
required for the device. Tests, additional developer tools, and various other
modules will not be built. Note that `make checkbuild` will not be complete
either, as `make tests` covers a few additional modules, but generally speaking
`make checkbuild` is enough.


Running the tests
-----------------

The tests are all built from the tests/ directory.

### Device tests

    $ mma # In $ANDROID_ROOT/bionic.
    $ adb root && adb remount && adb sync
    $ adb shell /data/nativetest/bionic-unit-tests/bionic-unit-tests
    $ adb shell \
        /data/nativetest/bionic-unit-tests-static/bionic-unit-tests-static
    # Only for 64-bit targets
    $ adb shell /data/nativetest64/bionic-unit-tests/bionic-unit-tests
    $ adb shell \
        /data/nativetest64/bionic-unit-tests-static/bionic-unit-tests-static

Note that we use our own custom gtest runner that offers a superset of the
options documented at
<https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md#running-test-programs-advanced-options>,
in particular for test isolation and parallelism (both on by default).

### Device tests via CTS

Most of the unit tests are executed by CTS. By default, CTS runs as
a non-root user, so the unit tests must also pass when not run as root.
Some tests cannot do any useful work unless run as root. In this case,
the test should check `getuid() == 0` and do nothing otherwise (typically
we log in this case to prevent accidents!). Obviously, if the test can be
rewritten to not require root, that's an even better solution.

Currently, the list of bionic CTS tests is generated at build time by
running a host version of the test executable and dumping the list of
all tests. In order for this to continue to work, all architectures must
have the same number of tests, and the host version of the executable
must also have the same number of tests.

Running the gtests directly is orders of magnitude faster than using CTS,
but in cases where you really have to run CTS:

    $ make cts # In $ANDROID_ROOT.
    $ adb unroot # Because real CTS doesn't run as root.
    # This will sync any *test* changes, but not *code* changes:
    $ cts-tradefed \
        run singleCommand cts --skip-preconditions -m CtsBionicTestCases

### Host tests

The host tests require that you have `lunch`ed either an x86 or x86_64 target.
Note that due to ABI limitations (specifically, the size of pthread_mutex_t),
32-bit bionic requires PIDs less than 65536. To enforce this, set /proc/sys/kernel/pid_max
to 65536.

    $ ./tests/run-on-host.sh 32
    $ ./tests/run-on-host.sh 64   # For x86_64-bit *targets* only.

You can supply gtest flags as extra arguments to this script.

### Against glibc

As a way to check that our tests do in fact test the correct behavior (and not
just the behavior we think is correct), it is possible to run the tests against
the host's glibc.

    $ ./tests/run-on-host.sh glibc


Gathering test coverage
-----------------------

For either host or target coverage, you must first:

 * `$ export NATIVE_COVERAGE=true`
     * Note that the build system is ignorant to this flag being toggled, i.e. if
       you change this flag, you will have to manually rebuild bionic.
 * Set `bionic_coverage=true` in `libc/Android.mk` and `libm/Android.mk`.

### Coverage from device tests

    $ mma
    $ adb sync
    $ adb shell \
        GCOV_PREFIX=/data/local/tmp/gcov \
        GCOV_PREFIX_STRIP=`echo $ANDROID_BUILD_TOP | grep -o / | wc -l` \
        /data/nativetest/bionic-unit-tests/bionic-unit-tests
    $ acov

`acov` will pull all coverage information from the device, push it to the right
directories, run `lcov`, and open the coverage report in your browser.

### Coverage from host tests

First, build and run the host tests as usual (see above).

    $ croot
    $ lcov -c -d $ANDROID_PRODUCT_OUT -o coverage.info
    $ genhtml -o covreport coverage.info # or lcov --list coverage.info

The coverage report is now available at `covreport/index.html`.


Attaching GDB to the tests
--------------------------

Bionic's test runner will run each test in its own process by default to prevent
tests failures from impacting other tests. This also has the added benefit of
running them in parallel, so they are much faster.

However, this also makes it difficult to run the tests under GDB. To prevent
each test from being forked, run the tests with the flag `--no-isolate`.


32-bit ABI bugs
---------------

See [32-bit ABI bugs](docs/32-bit-abi.md).
