#!/bin/sh
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Rebuild the mingw64 cross-toolchain from scratch
#
# See --help for usage example.

PROGNAME=$(basename $0)
PROGDIR=$(dirname $0)
PROGDIR=$(cd $PROGDIR && pwd)
ANDROID_BUILD_TOP=$(realpath $PROGDIR/../..)
TOOLCHAIN_DIR=$(realpath $ANDROID_BUILD_TOP/toolchain)

HELP=
VERBOSE=2

# This will be reset later.
LOG_FILE=/dev/null

panic ()
{
    1>&2 echo "Error: $@"
    exit 1
}

fail_panic ()
{
    if [ $? != 0 ]; then
        panic "$@"
    fi
}

var_value ()
{
    eval echo \"$1\"
}

var_append ()
{
    local _varname=$1
    local _varval=$(var_value $_varname)
    shift
    if [ -z "$_varval" ]; then
        eval $_varname=\"$*\"
    else
        eval $_varname=\$$_varname\" $*\"
    fi
}

run ()
{
    if [ "$VERBOSE" -gt 0 ]; then
        echo "COMMAND: >>>> $@" >> $LOG_FILE
    fi
    if [ "$VERBOSE" -gt 1 ]; then
        echo "COMMAND: >>>> $@"
    fi
    if [ "$VERBOSE" -gt 1 ]; then
        "$@"
    else
       "$@" > /dev/null 2>&1
    fi
}

log ()
{
    if [ "$LOG_FILE" ]; then
        echo "$@" >> $LOG_FILE
    fi
    if [ "$VERBOSE" -gt 0 ]; then
        echo "$@"
    fi
}

NUM_CORES=$(grep -c -e '^processor' /proc/cpuinfo)
JOBS=$(( $NUM_CORES * 2 ))

GMP_VERSION=5.0.5
MPFR_VERSION=3.1.1
MPC_VERSION=1.0.1
BINUTILS_VERSION=2.25
GCC_VERSION=4.8.3
MINGW_W64_VERSION=v5.0.0

TARGET_ARCH=x86_64
TARGET_MULTILIBS=true  # not empty to enable multilib
CLEANUP=

OPT_GCC_VERSION=

for opt; do
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case $opt in
        -h|-?|--help) HELP=true;;
        --verbose) VERBOSE=$(( $VERBOSE + 1 ));;
        --quiet) VERBOSE=$(( $VERBOSE - 1 ));;
        -j*|--jobs=*) JOBS=$optarg;;
        --target-arch=*) TARGET_ARCH=$optarg;;
        --no-multilib) TARGET_MULTILIBS="";;
        --gcc-version=*) OPT_GCC_VERSION=$optarg;;
        --cleanup) CLEANUP=true;;
        -*) panic "Unknown option '$opt', see --help for list of valid ones.";;
        *) panic "This script doesn't take any parameter, see --help for details.";;
    esac
done


if [ "$HELP" ]; then
    cat <<EOF
Usage: $PROGNAME [options]

This program is used to rebuild a mingw64 cross-toolchain from scratch.

All toolchain binaries can generate both Win32 and Win64 executables. Use -m32
or -m64 at compile/link time to select a specific target.

Valid options:
  -h|-?|--help                 Print this message."
  --verbose                    Increase verbosity."
  --quiet                      Decrease verbosity."
  --jobs=<num>                 Run <num> build tasks in parallel [$JOBS]."
  -j<num>                      Same as --jobs=<num>."
  --no-multilib                Disable multilib toolchain build."
  --target-arch=<arch>         Select default target architecture [$TARGET_ARCH]."
  --gcc-version=<version>      Select alternative GCC version [$GCC_VERSION]"
EOF
    exit 0
fi

if [ "$OPT_GCC_VERSION" ]; then
    GCC_VERSION=$OPT_GCC_VERSION
fi

GCC_SRC_DIR=$TOOLCHAIN_DIR/gcc/gcc-$GCC_VERSION
if [ ! -d "$GCC_SRC_DIR" ]; then
    panic "Missing GCC source directory: $GCC_SRC_DIR"
fi

GCC_MAJOR_MINOR=$(echo "$GCC_VERSION" | cut -d. -f1-2)

# Top level out directory.
OUT_DIR=$ANDROID_BUILD_TOP/out

# Name of the directory inside the package.
PACKAGE_DIR=x86_64-w64-mingw32-$GCC_MAJOR_MINOR

# Directory to isolate the install package from any similarly named directories.
OUTER_INSTALL_DIR=$OUT_DIR/install

# Actual install path.
INSTALL_DIR=$OUTER_INSTALL_DIR/$PACKAGE_DIR

# Install directory for build dependencies that are not in the final package
# (gmp and whatnot).
SUPPORT_DIR=$INSTALL_DIR

# For the final artifacts. Will be archived on the build server.
if [ -z "$DIST_DIR" ]; then
  DIST_DIR=$OUT_DIR/dist
fi

BUILD_TAG64=x86_64-linux-gnu
BUILD_TAG32=i686-linux-gnu

# We don't want debug executables
BUILD_CFLAGS="-O2 -fomit-frame-pointer -s"
BUILD_LDFLAGS=""

rm -rf $OUT_DIR
mkdir -p $OUT_DIR
mkdir -p $INSTALL_DIR
mkdir -p $DIST_DIR
mkdir -p $SUPPORT_DIR

LOG_FILE=$OUT_DIR/build.log
rm -f $LOG_FILE && touch $LOG_FILE
if [ "$VERBOSE" -eq 1 ]; then
    echo  "To follow build, use in another terminal: tail -F $LOG_FILE"
fi

case $TARGET_ARCH in
    x86_64) TARGET_BITS=64;;
    i686) TARGET_BITS=32;;
    *) panic "Invalid --target parameter. Valid values are: x86_64 i686";;
esac
TARGET_TAG=$TARGET_ARCH-w64-mingw32
log "Target arch: $TARGET_TAG"
log "Target bits: $TARGET_BITS"

HOST_ARCH=x86_64
HOST_BITS=64

HOST_TAG=$HOST_ARCH-linux-gnu
log "Host arch: $HOST_TAG"

# Copy this script
cp $0 $INSTALL_DIR/ &&
echo "This file has been automatically generated on $(date) with the following command:" > $INSTALL_DIR/README &&
echo "$PROGNAME $@" >> $INSTALL_DIR/README &&
echo "" >> $INSTALL_DIR/README &&
echo "The MD5 hashes for the original sources packages are:" >> $INSTALL_DIR/README
fail_panic "Could not copy script to installation directory."

PREFIX_FOR_TARGET=$INSTALL_DIR/$TARGET_TAG
WITH_WIDL=$INSTALL_DIR/bin
MINGW_W64_SRC=$TOOLCHAIN_DIR/mingw/mingw-w64-$MINGW_W64_VERSION

setup_host_build_env ()
{
    local BINPREFIX=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.11-4.8/bin/x86_64-linux-

    CC=${BINPREFIX}gcc
    CXX=${BINPREFIX}g++
    LD=${BINPREFIX}ld
    AS=${BINPREFIX}as
    AR=${BINPREFIX}ar
    RANLIB=${BINPREFIX}ranlib
    STRIP=${BINPREFIX}strip
    export CC CXX LD AS AR RANLIB STRIP

    export CFLAGS="$BUILD_CFLAGS"
    export CXXFLAGS="$BUILD_CFLAGS"
    export LDFLAGS="$BUILD_LDFLAGS"
}

setup_mingw_build_env ()
{
    local BINPREFIX=$INSTALL_DIR/bin/x86_64-w64-mingw32-

    CC=${BINPREFIX}gcc
    CXX=${BINPREFIX}g++
    LD=${BINPREFIX}ld
    AS=${BINPREFIX}as
    AR=${BINPREFIX}ar
    RANLIB=${BINPREFIX}ranlib
    RC=${BINPREFIX}windres
    STRIP=${BINPREFIX}strip
    export CC CXX LD AS AR RANLIB RC STRIP

}

setup_install_env ()
{
    export PATH=$INSTALL_DIR/bin:$PATH
}

build_binutils_package ()
{
    local PKGNAME=$1
    shift

    (
        mkdir -p $OUT_DIR/$PKGNAME &&
        cd $OUT_DIR/$PKGNAME &&
        setup_host_build_env &&
        log "$PKGNAME: Configuring" &&
        run $TOOLCHAIN_DIR/binutils/$PKGNAME/configure "$@"
        fail_panic "Can't configure $PKGNAME !!"

        log "$PKGNAME: Building" &&
        run make -j$JOBS MAKEINFO=true
        fail_panic "Can't build $PKGNAME !!"

        log "$PKGNAME: Installing" &&
        run make install MAKEINFO=true
        fail_panic "Can't install $PKGNAME"
    ) || exit 1
}

# The GCC build in Android is insane and stores gmp and friends as tarballs and
# extracts them as a part of the build step (in the meta-configure of all
# places). No one understands how any of that mess works, so just deal with
# extracting them here...
EXTRACTED_PACKAGES=$OUT_DIR/packages
mkdir -p $EXTRACTED_PACKAGES
fail_panic "Could not create directory for packages."

log "gmp-$GMP_VERSION: Extracting" &&
tar xf $TOOLCHAIN_DIR/gmp/gmp-$GMP_VERSION.tar.bz2 -C $EXTRACTED_PACKAGES
log "mpfr-$MPFR_VERSION: Extracting" &&
tar xf $TOOLCHAIN_DIR/mpfr/mpfr-$MPFR_VERSION.tar.bz2 -C $EXTRACTED_PACKAGES
log "mpc-$MPC_VERSION: Extracting" &&
tar xf $TOOLCHAIN_DIR/mpc/mpc-$MPC_VERSION.tar.gz -C $EXTRACTED_PACKAGES

build_host_package ()
{
    local PKGNAME=$1
    shift

    (
        mkdir -p $OUT_DIR/$PKGNAME &&
        cd $OUT_DIR/$PKGNAME &&
        setup_host_build_env &&
        log "$PKGNAME: Configuring" &&
        run $EXTRACTED_PACKAGES/$PKGNAME/configure "$@"
        fail_panic "Can't configure $PKGNAME !!"

        log "$PKGNAME: Building" &&
        run make -j$JOBS
        fail_panic "Can't build $PKGNAME !!"

        log "$PKGNAME: Installing" &&
        run make install
        fail_panic "Can't install $PKGNAME"
    ) || exit 1
}

export ABI=$HOST_BITS
SUPPORT_INSTALL=
BASE_HOST_OPTIONS="--prefix=$SUPPORT_DIR --disable-shared"
build_host_package gmp-$GMP_VERSION $BASE_HOST_OPTIONS
var_append BASE_HOST_OPTIONS "--with-gmp=$SUPPORT_DIR"

build_host_package mpfr-$MPFR_VERSION $BASE_HOST_OPTIONS
var_append BASE_HOST_OPTIONS "--with-mpfr=$SUPPORT_DIR"

build_host_package mpc-$MPC_VERSION $BASE_HOST_OPTIONS
var_append BASE_HOST_OPTIONS "--with-mpc=$SUPPORT_DIR"

BINUTILS_CONFIGURE_OPTIONS=$BASE_HOST_OPTIONS
var_append BINUTILS_CONFIGURE_OPTIONS "--target=$TARGET_TAG --disable-nls"
if [ "$TARGET_MULTILIBS" ]; then
    var_append BINUTILS_CONFIGURE_OPTIONS "--enable-targets=x86_64-w64-mingw32,i686-w64-mingw32"
fi

var_append BINUTILS_CONFIGURE_OPTIONS "--with-sysroot=$INSTALL_DIR"
var_append BINUTILS_CONFIGURE_OPTIONS "--enable-lto --enable-plugin --enable-gold"

build_binutils_package binutils-$BINUTILS_VERSION $BINUTILS_CONFIGURE_OPTIONS

build_mingw_tools ()
{
    local PKGNAME=$1

    (
        mkdir -p $OUT_DIR/$PKGNAME &&
        cd $OUT_DIR/$PKGNAME &&
        log "$PKGNAME: Configuring" &&
        run $MINGW_W64_SRC/mingw-w64-tools/widl/configure --prefix=$INSTALL_DIR --target=$TARGET_TAG --includedir=$OUT_DIR/install/x86_64-w64-mingw32-4.8/x86_64-w64-mingw32/
        fail_panic "Can't configure mingw-64-tools"
        log "$PKGNAME: Installing" &&
        run make install -j$JOBS
    ) || exit 1
}

build_mingw_pthreads_lib ()
{
    local PKGNAME=$1
    local CONFIGURE_EXTRA_ARGS=$2

    (
        mkdir -p $OUT_DIR/$PKGNAME &&
        cd $OUT_DIR/$PKGNAME &&
        setup_mingw_build_env &&
        log "$PKGNAME (32-bit): Configuring" &&
        run $MINGW_W64_SRC/mingw-w64-libraries/winpthreads/configure --prefix=$PREFIX_FOR_TARGET --target=$TARGET_TAG --host=$TARGET_TAG $CONFIGURE_EXTRA_ARGS &&
        fail_panic "Can't configure $PKGNAME"
    ) || exit 1

    # run it once so fakelib/libgcc.a is created and make subsequently fails
    # while looking for libpthread.a.  Copy libgcc.a to libpthread.a and
    # retry.
    cd $OUT_DIR/$PKGNAME && run make install -j$JOBS

    (
        cd $OUT_DIR/$PKGNAME
        cp fakelib/libgcc.a fakelib/libpthread.a &&
        log "$PKGNAME: Installing" &&
        run make install -j$JOBS
    ) || exit 1
}

build_mingw_pthreads ()
{
    local PKGNAME=$1

    (
        CFLAGS="$CFLAGS -m32"
        CXXFLAGS="$CXXFLAGS -m32"
        LDFLAGS="-m32"
        RCFLAGS="-F pe-i386"
        export CFLAGS CXXFLAGS LDFLAGS RCFLAGS
        build_mingw_pthreads_lib $PKGNAME-32 "--build=$BUILD_TAG32 --libdir=$PREFIX_FOR_TARGET/lib32"
        (run cp $PREFIX_FOR_TARGET/bin/libwinpthread-1.dll $PREFIX_FOR_TARGET/lib32) || exit 1
    )

    build_mingw_pthreads_lib $PKGNAME-64 "--build=$BUILD_TAG64"
}

# Install the right mingw64 headers into the sysroot
build_mingw_headers ()
{
    local PKGNAME=$1

    (
        mkdir -p $OUT_DIR/$PKGNAME &&
        cd $OUT_DIR/$PKGNAME &&
        log "$PKGNAME: Configuring" &&
        run $MINGW_W64_SRC/mingw-w64-headers/configure --prefix=$PREFIX_FOR_TARGET --host=$TARGET_TAG \
            --build=$HOST_TAG --enable-sdk=all --enable-secure-api
        fail_panic "Can't configure mingw-64-headers"

        run make
        log "$PKGNAME: Installing" &&
        run make install -j$JOBS &&
        run cd $INSTALL_DIR &&
        run ln -s $TARGET_TAG mingw &&
        run cd $INSTALL_DIR/mingw &&
        run ln -s lib lib$TARGET_BITS
        fail_panic "Can't install mingw-64-headers"
    ) || exit 1
}

# Slightly different from build_host_package because we need to call
# 'make all-gcc' and 'make install-gcc' as a special case.
#
build_core_gcc ()
{
    local PKGNAME=$1
    shift

    (
        mkdir -p $OUT_DIR/$PKGNAME &&
        cd $OUT_DIR/$PKGNAME &&
        setup_host_build_env &&
        log "core-$PKGNAME: Configuring" &&
        run $TOOLCHAIN_DIR/gcc/$PKGNAME/configure "$@"
        fail_panic "Can't configure $PKGNAME !!"

        log "core-$PKGNAME: Building" &&
        run make -j$JOBS all-gcc
        fail_panic "Can't build $PKGNAME !!"

        log "core-$PKGNAME: Installing" &&
        run make -j$JOBS install-gcc
        fail_panic "Can't install $PKGNAME"
    ) || exit 1
}


# Build and install the C runtime files needed by the toolchain
build_mingw_crt ()
{
    local PKGNAME=$1
    shift

    (
        mkdir -p $OUT_DIR/$PKGNAME &&
        cd $OUT_DIR/$PKGNAME &&
        export PATH=$INSTALL_DIR/bin:$PATH
        log "$PKGNAME: Configuring" &&
        run $MINGW_W64_SRC/mingw-w64-crt/configure "$@"
        fail_panic "Can't configure $PKGNAME !!"

        log "$PKGNAME: Building" &&
        run make -j$JOBS
        fail_panic "Can't build $PKGNAME !!"

        log "$PKGNAME: Installing" &&
        run make -j$JOBS install
        fail_panic "Can't install $PKGNAME"
    ) || exit 1
}


build_libgcc ()
{
    local PKGNAME=$1
    shift

    (
        # No configure step here! We're resuming work that was started
        # in build_core_gcc.
        cd $OUT_DIR/$PKGNAME &&
        setup_host_build_env &&
        log "libgcc-$PKGNAME: Building" &&
        run make -j$JOBS
        fail_panic "Can't build libgcc-$PKGNAME !!"

        log "libgcc-$PKGNAME: Installing" &&
        run make -j$JOBS install
        fail_panic "Can't install libgcc-$PKGNAME"
    ) || exit 1
}

GCC_CONFIGURE_OPTIONS=$BASE_HOST_OPTIONS
var_append GCC_CONFIGURE_OPTIONS "--target=$TARGET_TAG"
if [ "$TARGET_MULTILIBS" ]; then
    var_append GCC_CONFIGURE_OPTIONS "--enable-targets=all"
fi
var_append GCC_CONFIGURE_OPTIONS "--enable-languages=c,c++"
var_append GCC_CONFIGURE_OPTIONS "--with-sysroot=$INSTALL_DIR"
var_append GCC_CONFIGURE_OPTIONS "--enable-threads=posix"

build_mingw_tools mingw-w64-tools
build_mingw_headers mingw-w64-headers

build_core_gcc gcc-$GCC_VERSION $GCC_CONFIGURE_OPTIONS

CRT_CONFIGURE_OPTIONS="--host=$TARGET_TAG --with-sysroot=$INSTALL_DIR --prefix=$PREFIX_FOR_TARGET"
if [ "$TARGET_MULTILIBS" ]; then
    var_append CRT_CONFIGURE_OPTIONS "--enable-lib32"
fi

build_mingw_crt mingw-w64-crt $CRT_CONFIGURE_OPTIONS

# Build winpthreads
build_mingw_pthreads mingw-w64-pthreads

build_libgcc gcc-$GCC_VERSION

# Let's generate the licenses/ directory
LICENSE_DIRS="$SRC_DIR"
var_append LICENSE_DIRS "$TOOLCHAIN_DIR/binutils/binutils-$BINUTILS_VERSION"
var_append LICENSE_DIRS "$GCC_SRC_DIR"
var_append LICENSE_DIRS "$EXTRACTED_PACKAGES"

echo > $INSTALL_DIR/NOTICE
for LICENSE in $(find $LICENSE_DIRS -name "COPYING*"); do
    cat $SRC_DIR/$LICENSE >> $INSTALL_DIR/NOTICE
done

touch $INSTALL_DIR/MODULE_LICENSE_GPL

# The build server generates a repo.prop file that contains the current SHAs of
# each project.
REPO_PROP_PATH=$INSTALL_DIR/repo.prop
if [ -f $DIST_DIR/repo.prop ]; then
    cp $DIST_DIR/repo.prop $REPO_PROP_PATH
else
    # Generate our own if we're building locally.
    # The pushd/popd is to ensure that we're at least somewhere within our
    # source tree. There aren't any assumptions made about our CWD.
    pushd $ANDROID_BUILD_TOP
    repo forall \
        -c 'echo $REPO_PROJECT $(git rev-parse HEAD)' > $REPO_PROP_PATH
    popd
fi

PACKAGE_NAME=$DIST_DIR/$TARGET_TAG-linux-x86_64.tar.bz2
log "Packaging $TARGET_TAG toolchain to $PACKAGE_NAME"
run tar cjf $PACKAGE_NAME -C $OUTER_INSTALL_DIR $PACKAGE_DIR/
fail_panic "Could not package $TARGET_TAG toolchain!"
log "Done. See $DIST_DIR:"
ls -l $PACKAGE_NAME

exit 0
