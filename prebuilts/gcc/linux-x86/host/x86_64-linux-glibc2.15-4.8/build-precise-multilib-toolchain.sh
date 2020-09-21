#!/bin/bash
#
# Copyright (C) 2010 The Android Open Source Project
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

# This script is used to rebuild the Linux 32-bit cross-toolchain
# that allows you to generate 32-bit binaries that target Ubuntu 8.04
# (a.k.a. Hardy Heron) instead of the host system (which usually is 10.04,
# a.k.a. Lucid Lynx)
#
# Use --help for complete usage information.
#
# WARNING: At this time, the generated toolchain binaries will *not* run
#          with GLibc 2.15, only the machine code it generates.
#

PROGNAME="`basename \"$0\"`"
PATCHES_DIR="$( cd "$( dirname "$0" )" && pwd )/toolchain-patches"

###########################################################################
###########################################################################
#####
#####  C O N F I G U R A T I O N
#####
###########################################################################
###########################################################################

panic ()
{
    echo "ERROR: $@"
    exit 1
}

fail_panic ()
{
    if [ $? != 0 ] ; then
        echo "ERROR: $@"
        exit 1
    fi
}


# We only support running this script on Linux
OS=$(uname -s)
if [ "$OS" != Linux ]; then
    panic "This script can only run on Linux machines!"
fi

UBUNTU_ARCHS="i386 amd64"

# Used to set the host bitness of the generted toolchain binaries
# First call with the build machine's bitness, and later with 32
# if --32-bits option is used.
# $1: 32 or 64
set_host_bits ()
{
    HOST_BITS=$1
    GMP_ABI=$1
    case $1 in
        32)
            HOST_ARCH=i686
            GCC_TARGET=i686-linux
            GMP_TARGET=i386-linux
            ;;
        64)
            HOST_ARCH=x86_64
            GCC_TARGET=x86_64-linux
            GMP_TARGET=x86_64-linux
            ;;
        *)
            panic "Invalid host bitness (32 or 64 expected): $1"
    esac
}

# Determine build machine bitness
BUILD_ARCH=$(uname -m)
case "$BUILD_ARCH" in
    x86_64|amd64)
        BUILD_BITS=64
        BUILD_ARCH=x86_64
        BUILD_GCC_TARGET=x86_64-linux
        set_host_bits 64
        ;;
    i?86)
        BUILD_BITS=32
        BUILD_ARCH=i686
        BUILD_GCC_TARGET=i686-linux
        set_host_bits 32
        ;;
    *)
        panic "Unknown build CPU architecture: $BUILD_ARCH"
esac

# Versions of various toolchain components, do not touch unless you know
# what you're doing!

BINUTILS_VERSION=2.23
GMP_VERSION=5.0.5
MPFR_VERSION=3.1.1
MPC_VERSION=1.0.1
GCC_VERSION=4.8
CLOOG_VERSION=0.18.0
ISL_VERSION=0.11.1

GLIBC_VERSION=2.15

GIT_CMD=git
GIT_DATE=
GIT_BRANCH=master
GIT_REFERENCE=
GIT_BASE=
GIT_BASE_DEFAULT=https://android.googlesource.com/toolchain

# Location where we're going to install the toolchain during the build
# This will depend on the phase of the build.
install_dir () { echo "$WORK_DIR/$PHASE/$TOOLCHAIN_NAME"; }

# Given an input string that looks like <major>.<minor>.<patch>
# Return <major>.<minor> only.
major_minor_only () {
   local MAJOR=$(echo -n "$1" | cut -f1 -d.)
   local MINOR=$(echo -n "$1" | cut -f2 -d.)
   echo "$MAJOR.$MINOR"
}

# Location where we're going to install the final binaries
# If empty, TOOLCHAIN_ARCHIVE will be generated
PREFIX_DIR=

# Location of the final sysroot. This must be a sub-directory of INSTALL_DIR
# to ensure that the toolchain binaries are properly relocatable (i.e. can
# be used when moved to another directory).
sysroot_dir () { echo "$(install_dir)/sysroot"; }

# Try to parallelize the build for faster performance.
JOBS=$(cat /proc/cpuinfo | grep -c processor)

# The base URL of the Ubuntu mirror we're going to use.
UBUNTU_MIRROR=http://mirrors.us.kernel.org

# Ubuntu release name we want packages from. Can be a name or a number
# (i.e. "precise" or "12.04")
UBUNTU_RELEASE=precise


# The list of packages we need to download from the Ubuntu servers and
# extract into the original sysroot
#
# Call add_ubuntu_package <package-name> to add a package to the list,
# which will be processed later.
#
UBUNTU_PACKAGES=

add_ubuntu_package ()
{
    UBUNTU_PACKAGES="$UBUNTU_PACKAGES $@"
}

# The package files containing kernel headers for Hardy and the C
# library headers and binaries
add_ubuntu_package \
    linux-libc-dev \
    libc6 \
    libc6-dev \
    libcap2 \
    libcap-dev \
    libattr1 \
    libattr1-dev \
    libacl1 \
    libacl1-dev \

# The X11 headers and binaries (for the emulator)
add_ubuntu_package \
    libx11-6 \
    libx11-dev \
    libxau6 \
    libxcb1-dev \
    libxdmcp6 \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    x11proto-core-dev \
    x11proto-fixes-dev \
    x11proto-xext-dev \
    x11proto-input-dev \
    x11proto-kb-dev

# The OpenGL-related headers and libraries (for GLES emulation)
add_ubuntu_package \
    mesa-common-dev \
    libgl1-mesa-dev \
    libgl1-mesa-glx \
    libxxf86vm1 \
    libxext6 \
    libxdamage1 \
    libxfixes3 \
    libdrm2

# Audio libraries (required by the emulator)
add_ubuntu_package \
    libasound2 \
    libasound2-dev \
    libesd0-dev \
    libaudiofile-dev \
    libpulse0 \
    libpulse-dev

# ZLib and others.
add_ubuntu_package \
    zlib1g \
    zlib1g-dev \
    libncurses5 \
    libncurses5-dev \
    libtinfo5 \
    libtinfo-dev



###########################################################################
###########################################################################
#####
#####  E N D   O F   C O N F I G U R A T I O N
#####
###########################################################################
###########################################################################

# Parse all options
OPTION_HELP=no
VERBOSE=0
FORCE=no
ONLY_SYSROOT=no
ONLY_TOOLCHAIN_DIR=
BOOTSTRAP=
PARAMETERS=
FORCE_32=
LIST_TASKS=

for opt do
  optarg=$(expr "x$opt" : 'x[^=]*=\(.*\)')
  case $opt in
  --help|-h|-\?) OPTION_HELP=yes
  ;;
  --verbose) VERBOSE=$(( $VERBOSE + 1 ))
  ;;
  --force) FORCE="yes"
  ;;
  --32-bits) FORCE_32=true
  ;;
  --ubuntu-mirror=*) UBUNTU_MIRROR=$optarg
  ;;
  --ubuntu-release=*) UBUNTU_RELEASE=$optarg
  ;;
  --prefix=*) PREFIX_DIR=$optarg
  ;;
  --work-dir=*) WORK_DIR=$optarg
  ;;
  --gcc-version=*) GCC_VERSION=$optarg
  ;;
  --binutils-version=*) BINUTILS_VERSION=$optarg
  ;;
  --gmp-version=*) GMP_VERSION=$optarg
  ;;
  --mpfr-version=*) MPFR_VERSION=$optarg
  ;;
  --mpc-version=*) MPC_VERSION=$optarg
  ;;
  --isl-version=*) ISL_VERSION=$optarg
  ;;
  --cloog-version=*) CLOOG_VERSION=$oparg
  ;;
  --git=*) GIT_CMD=$optarg
  ;;
  --git-date=*) GIT_DATE=$optarg
  ;;
  --git-branch=*) GIT_BRANCH=$optarg
  ;;
  --git-base=*) GIT_BASE=$optarg
  ;;
  --git-reference=*) GIT_REFERENCE=$optarg
  ;;
  --out-dir=*) OPTION_OUT_DIR=$optarg
  ;;
  --cc=*) OPTION_CC=$optarg
  ;;
  --jobs=*) JOBS=$optarg
  ;;
  -j*) JOBS=${opt#-j}
  ;;
  --only-sysroot) ONLY_SYSROOT=yes
  ;;
  --bootstrap) BOOTSTRAP=yes
  ;;
  --list-tasks) LIST_TASKS=yes
  ;;
  -*)
    echo "unknown option '$opt', use --help"
    exit 1
    ;;
  *)
    if [ -z "$PARAMETERS" ]; then
        PARAMETERS=$opt
    else
        PARAMETERS="$PARAMETERS $opt"
    fi
  esac
done

if [ "$OPTION_HELP" = "yes" ]; then
    cat << EOF

Usage: $PROGNAME [options] [<path-to-toolchain-sources>]

This script is used to rebuild a custom Linux host toolchain that targets
GLibc $GLIBC_VERSION or higher. The machine code generated by this toolchain
will run properly on Ubuntu $UBUNTU_RELEASE or higher.

If you're running on a 32-bit system, it will generate a 32-bit toolchain.
If you're running on a 64-bit system, it will generate a 64-bit toolchain
unless you use the --32-bits option.

You can provide the path to a local copy of the toolchain sources repository
as a first parameter. If you don't, the sources will be downloaded and
extracted automatically into your work directory.

Note that this script will download various binary packages from Ubuntu
servers in order to prepare a compatible "sysroot".

By default, it generates a package archive ($TOOLCHAIN_ARCHIVE) but you can,
as an alternative, ask for direct installation with --prefix=<path>

Use the bootstrap option to re-generate the toolchain with itself. This is
useful if you want to ensure that the generated compiler binaries will work
properly on Ubuntu 8.04 or higher. By default, they will only run on systems
that match your build system's C library ABI, or higher.

Options: [defaults in brackets after descriptions]
EOF
    echo "Standard options:"
    echo "  --help                        Print this message"
    echo "  --force                       Force-rebuild everything"
    echo "  --prefix=PATH                 Installation path [$PREFIX_DIR]"
    echo "  --ubuntu-mirror=URL           Ubuntu mirror URL [$UBUNTU_MIRROR]"
    echo "  --ubuntu-release=NAME         Ubuntu release name [$UBUNTU_RELEASE]"
    echo "  --work-dir=PATH               Temporary work directory [/tmp/gcc.<random>]"
    echo "  --only-sysroot                Only download and build sysroot packages"
    echo "  --verbose                     Verbose output. Can be used twice."
    echo "  --binutils-version=VERSION    Binutils version number [$BINUTILS_VERSION]"
    echo "  --gcc-version=VERSION         GCC version number [$GCC_VERSION]"
    echo "  --gmp-version=VERSION         GMP version number [$GMP_VERSION]"
    echo "  --mpfr-version=VERSION        MPFR version numner [$MPFR_VERSION]"
    echo "  --mpc-version=VERSION         MPC version number [$MPC_VERSION]"
    echo "  --isl-version=VERSION         ISL version number [$ISL_VERSION]"
    echo "  --cloog-version=VERSION       Cloog version number [$CLOOG_VERSION]"
    echo "  --jobs=COUNT                  Run COUNT build jobs in parallel [$JOBS]"
    echo "  -j<COUNT>                     Same as --jobs=COUNT."
    echo "  --git=<cmd>                   Use this version of the git tool [$GIT_CMD]"
    echo "  --git-date=<date>             Specify specific git date when download sources [none]"
    echo "  --git-branch=<name>           Specify which branch to use when downloading the sources [$GIT_BRANCH]"
    echo "  --git-reference=<path>        Use a git reference repository"
    echo "  --git-base=<url>              Use this git repository base [$GIT_BASE]"
    echo "  --bootstrap                   Bootstrap toolchain (i.e. compile it with itself)"
    echo "  --32-bits                     Generate 32-bit toolchain on 64-bit build system."
    echo ""
    exit 1
fi

if [ "$FORCE_32" ]; then
    if [ "$BUILD_BITS" = 64 ]; then
        set_host_bits 32
    else
       echo "Warning: --32-bits option ignored on 32-bit build machine."
    fi
fi

# Determine root working directory for our script
if [ -z "$WORK_DIR" ]; then
    WORK_DIR=$(mktemp -d /tmp/$USER-gcc-$HOST_BITS-XXXXXX)
    WORK_DIR_CLEANUP=true
else
    mkdir -p "$WORK_DIR"
    fail_panic "Could not create directory: $WORK_DIR"
    WORK_DIR_CLEANUP=false
fi

if [ -z "$PARAMETERS" ] ; then
    if [ -n "$GIT_REFERENCE" ] ; then
        if [ ! -d "$GIT_REFERENCE" -o ! -d "$GIT_REFERENCE/build" ]; then
            echo "ERROR: Invalid reference repository directory path: $GIT_REFERENCE"
            exit 1
        fi
        if [ -n "$GIT_BASE" ]; then
            echo "Using git clone reference: $GIT_REFERENCE"
        else
            # If we have a reference without a base, use it as a download base instead.
            GIT_BASE=$GIT_REFERENCE
            GIT_REFERENCE=
            echo "Using git clone base: $GIT_BASE"
        fi
    elif [ -z "$GIT_BASE" ]; then
        GIT_BASE=$GIT_BASE_DEFAULT
        echo "Auto-config: --git-base=$GIT_BASE"
    fi

    # Location where we will download the toolchain sources
    TOOLCHAIN_SRC_DIR=$WORK_DIR/toolchain-src
else
    set_parameters () {
        TOOLCHAIN_SRC_DIR="$1"
        if [ ! -d "$TOOLCHAIN_SRC_DIR" ]; then
            echo "ERROR: Not a directory: $1"
            exit 1
        fi
        if [ ! -d "$TOOLCHAIN_SRC_DIR/build" ]; then
            echo "ERROR: Missing directory: $1/build"
            exit 1
        fi
    }

    set_parameters $PARAMETERS
fi

# Location of original sysroot. This is where we're going to extract all
# binary Ubuntu packages.
ORG_SYSROOT_DIR=$WORK_DIR/sysroot

# Name of the final generated toolchain
TOOLCHAIN_NAME=$GCC_TARGET-glibc$GLIBC_VERSION-$(major_minor_only $GCC_VERSION)

# Name of the final toolchain binary tarball that this script will create
TOOLCHAIN_ARCHIVE=/tmp/$TOOLCHAIN_NAME.tar.bz2

# A file that will contain details about all the sources used to generate
# the final toolchain. This includes both SHA-1 for toolchain git repositories
# and SHA-1 hashes for downloaded Ubuntu packages.
SOURCES_LIST=$WORK_DIR/SOURCES

# Determine Make flags
MAKE_FLAGS="-j$JOBS"

# Create the work directory
mkdir -p "$WORK_DIR"
mkdir -p "$TOOLCHAIN_SRC_DIR"

# Location where we download packages from the Ubuntu servers
DOWNLOAD_DIR=$WORK_DIR/download

# Empty the SOURCES file
rm -f "$SOURCES_LIST" && touch "$SOURCES_LIST"


if [ "$VERBOSE" -ge 1 ] ; then
    run () {
        echo "## COMMAND: $@"
        $@
    }
    log () {
        echo "$@"
    }
    if [ "$VERBOSE" -ge 2 ] ; then
        log2 () {
            echo "$@"
        }
    else
        log2 () {
            return
        }
    fi
else
    run () {
        "$@" >>$TMPLOG 2>&1
    }
    log () {
        return
    }
    log2 () {
        return
    }
fi

# Sanitize a path list, we want to remove empty sub-dirs and
# leading/trailing columns.
sanitize_path_list ()
{
  local RESULT
  RESULT=$(printf "%s\n" "$*" | tr ':' '\n' | awk '$1 != "" && $1 != "." { print $0; }' | tr '\n' ':')
  printf "%s" ${RESULT%:}
}

PATH=$(sanitize_path_list $PATH)
LD_LIBRARY_PATH=$(sanitize_path_list $LD_LIBRARY_PATH)

BUILD_DIR=$WORK_DIR/build
mkdir -p $BUILD_DIR

TMPLOG=$BUILD_DIR/build.log
rm -rf $TMPLOG && touch $TMPLOG

build_dir_for () { echo "$BUILD_DIR/$PHASE/$1"; }

TIMESTAMPS_DIR=$BUILD_DIR/timestamps
mkdir -p $TIMESTAMPS_DIR

stamp_check () {
    [ -f "$TIMESTAMPS_DIR/$1" ]
}

stamp_clear () {
    rm -f "$TIMESTAMPS_DIR/$1"
}

stamp_set () {
    touch "$TIMESTAMPS_DIR/$1"
}

if [ "$FORCE" = "yes" ] ; then
    echo "Cleaning up timestamps (forcing the build)."
    rm -f $TIMESTAMPS_DIR/*
fi

if [ "$VERBOSE" = 0 ] ; then
    echo "To follow build, run: tail -F $TMPLOG"
fi

# returns 0 iff the string in $2 matches the pattern in $1
# $1: pattern
# $2: string
pattern_match ()
{
    echo "$2" | grep -q -E -e "$1"
}

# Find if a given shell program is available.
# We need to take care of the fact that the 'which <foo>' command
# may return either an empty string (Linux) or something like
# "no <foo> in ..." (Darwin). Also, we need to redirect stderr
# to /dev/null for Cygwin
#
# $1: variable name
# $2: program name
#
# Result: set $1 to the full path of the corresponding command
#         or to the empty/undefined string if not available
#
find_program ()
{
    local PROG
    PROG=`which $2 2>/dev/null`
    if [ -n "$PROG" ] ; then
        if pattern_match '^no ' "$PROG"; then
            PROG=
        fi
    fi
    eval $1="$PROG"
}

# Copy a directory, create target location if needed
#
# $1: source directory
# $2: target directory location
#
copy_directory ()
{
    local SRCDIR="$1"
    local DSTDIR="$2"
    if [ ! -d "$SRCDIR" ] ; then
        panic "Can't copy from non-directory: $SRCDIR"
    fi
    log2 "Directory copy: $SRCDIR -> $DSTDIR"
    mkdir -p "$DSTDIR" && (cd "$SRCDIR" && tar cf - *) | (tar xf - -C "$DSTDIR")
    fail_panic "Cannot copy to directory: $DSTDIR"
}

find_program CMD_WGET wget
find_program CMD_CURL curl
find_program CMD_SCP  scp

# Download a file with either 'curl', 'wget' or 'scp'
#
# $1: source URL (e.g. http://foo.com, ssh://blah, /some/path)
# $2: target file
download_file ()
{
    # Is this HTTP, HTTPS or FTP ?
    if pattern_match "^(http|https|ftp):.*" "$1"; then
        if [ -n "$CMD_WGET" ] ; then
            run $CMD_WGET -O $2 $1
        elif [ -n "$CMD_CURL" ] ; then
            run $CMD_CURL -o $2 $1
        else
            echo "Please install wget or curl on this machine"
            exit 1
        fi
        return
    fi

    # Is this SSH ?
    # Accept both ssh://<path> or <machine>:<path>
    #
    if pattern_match "^(ssh|[^:]+):.*" "$1"; then
        if [ -n "$CMD_SCP" ] ; then
            scp_src=`echo $1 | sed -e s%ssh://%%g`
            run $CMD_SCP $scp_src $2
        else
            echo "Please install scp on this machine"
            exit 1
        fi
        return
    fi

    # Is this a file copy ?
    # Accept both file://<path> or /<path>
    #
    if pattern_match "^(file://|/).*" "$1"; then
        cp_src=`echo $1 | sed -e s%^file://%%g`
        run cp -f $cp_src $2
        return
    fi

    # Unknown schema
    echo "ERROR: Unsupported source URI: $1"
    exit 1
}

# A variant of 'download_file' used to specify the target directory
# $1: source URL
# $2: target directory
download_file_to ()
{
    local URL="$1"
    local DIR="$2"
    local DST="$DIR/`basename $URL`"
    mkdir -p $DIR
    download_file "$URL" "$DST"
}

# Pack a given archive
#
# $1: archive file path (including extension)
# $2: source directory for archive content
# $3+: list of files (including patterns), all if empty
pack_archive ()
{
    local ARCHIVE="$1"
    local SRCDIR="$2"
    local SRCFILES
    local TARFLAGS ZIPFLAGS
    shift; shift;
    if [ -z "$1" ] ; then
        SRCFILES="*"
    else
        SRCFILES="$@"
    fi
    if [ "`basename $ARCHIVE`" = "$ARCHIVE" ] ; then
        ARCHIVE="`pwd`/$ARCHIVE"
    fi
    mkdir -p `dirname $ARCHIVE`
    if [ "$VERBOSE" -ge 2 ] ; then
        TARFLAGS="vcf"
        ZIPFLAGS="-9r"
    else
        TARFLAGS="cf"
        ZIPFLAGS="-9qr"
    fi
    case "$ARCHIVE" in
        *.zip)
            (cd $SRCDIR && run zip $ZIPFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *.tar)
            (cd $SRCDIR && run tar $TARFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *.tar.gz)
            (cd $SRCDIR && run tar z$TARFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *.tar.bz2)
            (cd $SRCDIR && run tar j$TARFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *)
            panic "Unsupported archive format: $ARCHIVE"
            ;;
    esac
}

no_trailing_slash ()
{
    echo ${1##/}
}

# Load the Ubuntu packages file. This is a long text file that will list
# each package for a given release.
#
# $1: Ubuntu mirror base URL (e.g. http://mirrors.us.kernel.org/)
# $2: Release name
#
get_ubuntu_packages_list ()
{
    local RELEASE=$2
    local BASE="`no_trailing_slash \"$1\"`"
    local SRCFILE DSTFILE
    for UA in $UBUNTU_ARCHS; do
        SRCFILE="$BASE/ubuntu/dists/$RELEASE/main/binary-$UA/Packages.bz2"
        DSTFILE="$DOWNLOAD_DIR/Packages-$UA.bz2"
        log "Trying to load $SRCFILE"
        download_file "$SRCFILE" "$DSTFILE"
        fail_panic "Could not download $SRCFILE"
        (cd $DOWNLOAD_DIR && bunzip2 -cf Packages-$UA.bz2 > Packages-$UA)
        fail_panic "Could not uncompress $DSTFILE to Packages-$UA"
    done

    # Write a small awk script used to extract filenames for a given package
    cat > $DOWNLOAD_DIR/extract-filename.awk <<EOF
BEGIN {
    # escape special characters in package name
    gsub("\\\\.","\\\\.",PKG)
    gsub("\\\\+","\\\\+",PKG)
    FILE = ""
    PACKAGE = ""
}

\$1 == "Package:" {
    if (\$2 == PKG) {
        PACKAGE = \$2
    } else {
        PACKAGE = ""
    }
}

\$1 == "Filename:" && PACKAGE == PKG {
    FILE = \$2
}

END {
    print FILE
}
EOF
}

# Convert an unversioned package name into a .deb package URL
#
# $1: Package name without version information (e.g. libc6-dev)
# $2: Ubuntu mirror base URL
# $3: Ubuntu arch ("i386" or "amd64")
#
get_ubuntu_package_deb_url ()
{
    # The following is an awk command to parse the Packages file and extract
    # the filename of a given package.
    local BASE="`no_trailing_slash \"$1\"`"
    local FILE=`awk -f "$DOWNLOAD_DIR/extract-filename.awk" -v PKG=$1 $DOWNLOAD_DIR/Packages-$3`
    if [ -z "$FILE" ]; then
        log "Could not find filename for package $1"
        exit 1
    fi
    echo "$2/ubuntu/$FILE"
}

# Does the host compiler generate 32-bit machine code?
# If not, add the -m32 flag to the compiler name to ensure this.
#
compute_host_flags ()
{
    HOST_CC=${CC:-gcc}
    HOST_CXX=${CXX-g++}
    if [ -n "$USE_CCACHE" ]; then
        echo -n "Checking for ccache..."
        find_program CMD_CCACHE ccache
        if [ -n "$CMD_CCACHE" ] ; then
            echo "$HOST_CC" | tr ' ' '\n' | grep -q -e "ccache"
            if [ $? = 0 ] ; then
                echo "yes (ignored)"
            else
                echo "yes"
                HOST_CC="ccache $HOST_CC"
                HOST_CXX="ccache $HOST_CXX"
            fi
        else
            echo "no"
        fi
    fi
    echo -n "Checking compiler bitness... "
    cat > "$BUILD_DIR"/conftest.c << EOF
#include <stdio.h>
int main(void) {
    printf("%d\n",sizeof(void*)*8);
    return 0;
}
EOF
    $HOST_CC -o "$BUILD_DIR"/conftest "$BUILD_DIR"/conftest.c > "$BUILD_DIR"/conftest.log 2>&1
    if [ $? != 0 ] ; then
        echo "Could not compile test program!!"
        echo "Error log is:"
        cat "$BUILD_DIR"/conftest.log
        rm "$BUID_DIR"/conftest.log
        panic "Need a working build toolchain!"
    fi
    HOST_CC_BITS=$("$BUILD_DIR"/conftest)
    echo -n "$HOST_CC_BITS"
    case $HOST_CC_BITS in
        32) # Nothing to do
            ;;
        64) # Do we need to force 32-bits
            if [ "$FORCE_32" ]; then
                echo " (forcing generation of 32-bit binaries)"
                HOST_CC=$HOST_CC" -m32"
                HOST_CXX=$HOST_CXX" -m32"
            fi
            ;;
        *)
            panic "Unknown bitness (32 or 64 expected) !!"
    esac
    echo ""
    echo "Using build C compiler: $HOST_CC"
    echo "Using build C++ compiler: $HOST_CXX"
    echo "GCC target name: $GCC_TARGET"
    echo "GMP target name: $GMP_TARGET"
    echo "GMP ABI: $GMP_ABI"
    export CC="$HOST_CC"
    export CXX="$HOST_CXX"
}

compute_host_flags

# Return the value of a given named variable
# $1: variable name
#
# example:
#    FOO=BAR
#    BAR=ZOO
#    echo `var_value $FOO`
#    will print 'ZOO'
#
var_value ()
{
    eval echo \$$1
}

var_list_append ()
{
    local VARNAME=$1
    local VARVAL=`var_value $VARNAME`
    shift
    if [ -z "$VARVAL" ] ; then
        eval $VARNAME=\"$@\"
    else
        eval $VARNAME=\"$VARVAL $@\"
    fi
}

var_list_prepend ()
{
    local VARNAME=$1
    local VARVAL=`var_value $VARNAME`
    shift
    if [ -z "$VARVAL" ] ; then
        eval $VARNAME=\"$@\"
    else
        eval $VARNAME=\"$@ $VARVAL\"
    fi
}

_list_first ()
{
    echo $1
}

_list_rest ()
{
    shift
    echo "$@"
}

_list_reverse ()
{
    local I1 I2 I3 I4 I5 I6 I7 I8 I9 REST RET
    I1=$1; I2=$2; I3=$3; I4=$I4; I5=$I5; I6=$I6; I7=$I7; I8=$I8; I9=$I9
    shift 9
    RET=$I9${I8:+" "}$I8${I7:+" "}$I7${I6:+" "}$I6${I5:+" "}$I5${I4:+" "}$I4${I3:+" "}$I3${I2:+" "}$I2${I1:+" "}$I1
    REST="$*"
    if [ "$REST" ]; then
        RET=$(_list_reverse $REST)$RET
    fi
    echo "$RET"
}

var_list_pop_first ()
{
    local VARNAME=$1
    local VARVAL=`var_value $VARNAME`
    local FIRST=`_list_first $VARVAL`
    eval $VARNAME=\"`_list_rest $VARVAL`\"
    echo "$FIRST"
}

_list_first ()
{
    echo $1
}

_list_rest ()
{
    shift
    echo "$@"
}

var_list_first ()
{
    local VAL=`var_value $1`
    _list_first $VAL
}

var_list_rest ()
{
    local VAL=`var_value $1`
    _list_rest $VAL
}

ALL_TASKS=

# Define a new task for this build script
# $1: Task name (e.g. build_stuff)
# $2: Task description
# $3: Optional: command name (will be cmd_$1 by default)
#
task_define ()
{
    local TASK="$1"
    local DESCR="$2"
    local COMMAND="${3:-cmd_$1}"

    var_list_append ALL_TASKS $TASK
    task_set $TASK name "$TASK"
    task_set $TASK descr "$DESCR"
    task_set $TASK cmd "$COMMAND"
    task_set $TASK deps ""
}

# Variant of task define for dual tasks
# This really defines two tasks named '<task>_1' and '<task>_2"
# $1: Task base name
# $2: Task description
# $3: Optional: command name (will be cmd_$1 by default)
task2_define ()
{
    local TASK="$1"
    local DESCR="$2"
    local COMMAND="${3:-cmd_$1}"

    task_define "${TASK}_1" "$DESCR 1/2" "phase_1 $COMMAND"
    task_define "${TASK}_2" "$DESCR 2/2" "phase_2 $COMMAND"
}

task_set ()
{
    local TASK="$1"
    local FIELD="$2"
    shift; shift;
    eval TASK_${TASK}__${FIELD}=\"$@\"
}

task_get ()
{
    var_value TASK_$1__$2
}

# return the list of dependencies for a given task
task_get_deps ()
{
    task_get $1 deps
}

task_get_cmd ()
{
    task_get $1 cmd
}

task_get_descr ()
{
    task_get $1 descr
}

# $1: task name
# $2+: other tasks this task depends on.
task_depends ()
{
    local TASK="$1"
    shift;
    var_list_append TASK_${TASK}__deps $@
}

# $1: dual task name
# $2+: other non-dual tasks this dual task depends on
task2_depends1 ()
{
    local TASK="$1"
    shift
    var_list_append TASK_${TASK}_1__deps $@
    var_list_append TASK_${TASK}_2__deps $@
}

# $1: dual task name
# $2+: other dual tasks this dual task depends on
task2_depends2 ()
{
    local TASK="$1"
    local DEP
    shift
    for DEP; do
        var_list_append TASK_${TASK}_1__deps ${DEP}_1
        var_list_append TASK_${TASK}_2__deps ${DEP}_2
    done
}

task_dump ()
{
    local TASK
    for TASK in $ALL_TASKS; do
        local DEPS="`task_get_deps $TASK`"
        local CMD="`task_get_cmd $TASK`"
        local DESCR="`task_get_descr $TASK`"
        echo "TASK $TASK: $DESCR: $CMD"
        echo ">  $DEPS"
    done
}

task_visit ()
{
    task_set $TASK visit 1
}

task_unvisit ()
{
    task_set $TASK visit 0
}

task_is_visited ()
{
    [ `task_get $TASK visit` = 1 ]
}

task_queue_reset ()
{
    TASK_QUEUE=
}

task_queue_push ()
{
    var_list_append TASK_QUEUE $1
}

task_queue_pop ()
{
    local FIRST=`var_list_first TASK_QUEUE`
    TASK_QUEUE=`var_list_rest TASK_QUEUE`
}

do_all_tasks ()
{
    local TASK
    local TASK_LIST=
    task_queue_reset
    # Clear visit flags
    for TASK in $ALL_TASKS; do
        task_unvisit $TASK
    done
    task_queue_push $1
    while [ -n "$TASK_QUEUE" ] ; do
        TASK=`task_queue_pop`
        if task_is_visited $TASK; then
            continue
        fi
        # Prepend the task to the list if its timestamp is not set
        if stamp_check $TASK; then
            var_list_prepend TASK_LIST $TASK
        fi
        # Add all dependencies to the work-queue
        local SUBTASK
        for SUBTASK in `task_get_deps $TASK`; do
            task_queue_push $SUBTASK
        done
        task_visit $TASK
    done

    # Now, TASK_LIST contains the
}


# Return the first item of a space-separated list
list_first () {
    set -- "$@"
    echo "$1"
}

# Append an item to a given list
list_append () {
    local ITEM=$1
    shift;
    echo $@${@:+" "}$1
}

# Return the second-to-last items of a space-separated list
list_rest () {
    set -- "$@"
    shift
    echo "$@"
}

# Reverse a space-separated list
list_reverse ()
{
    set -- "$@"
    local I1 I2 I3 I4 I5 I6 I7 I8 I9 REST RET
    I1=$1; I2=$2; I3=$3; I4=$4; I5=$5; I6=$6; I7=$7; I8=$8; I9=$9
    shift; shift; shift; shift; shift; shift; shift; shift; shift;
    RET=$I9${I9:+" "}$I8${I8:+" "}$I7${I7:+" "}$I6${I6:+" "}$I5${I5:+" "}$I4${I4:+" "}$I3${I3:+" "}$I2${I2:+" "}$I1
    REST="$*"
    if [ -n "$REST" ]; then
        RET=$(list_reverse $REST)" "$RET
    fi
    echo "$RET"
}

# Used to build the list of tasks with a tree post-order traversal, i.e.
# the list starts at the leaves and finishes with the top level task,
# so that if task(A) depends on task(B), then A will always appear _after_
# B in the result.
#
# $1: space-separated list of tasks to visit
# Out: list of all tasks in post-order
#
task_build_postorder_list ()
{
    local TASK
    local STACK="$1"
    local RET=""
    for TASK in $ALL_TASKS; do
        stamp_clear $TASK.visit
    done
    while true; do
        # Peek at stack
        TASK=$(list_first $STACK)
        #echo >&2 "STACK: ($TASK) '$STACK'"
        if [ -z "$TASK" ]; then
            break
        fi
        HAS_DEPS=
        for DEP in $(task_get_deps $TASK); do
            #echo >&2 "CHECK: '$DEP'"
            if ! stamp_check $DEP.visit; then
                STACK=$DEP" "$STACK
                #echo >&2 "PUSH: '$DEP' => '$STACK'"
                HAS_DEPS=1
            fi
        done

        if [ -z "$HAS_DEPS" ]; then
            #echo >&2 "ADD: $TASK -> '$RET'"
            STACK=$(list_rest $STACK)
            if ! stamp_check $TASK.visit; then
                RET=$RET${RET:+" "}$TASK
                stamp_set $TASK.visit
            fi
        fi
    done
    for TASK in $ALL_TASKS; do
        stamp_clear $TASK.visit
    done
    echo "$RET"
}

run_task ()
{
    # Build the list of tasks, in reverse order (from leafs to last)
    local TASKS=$(task_build_postorder_list $1)
    # Do all tasks
    local TASK DEP DESCR

    # Dump list of tasks:
#     echo "ALL TASKS:"
#     for TASK in $TASKS; do
#         echo "  $TASK"
#     done

    # Clean timestamps of any tasks whose any of its dependents needs
    # to be re-done.
    #
    for TASK in $TASKS; do
       for DEP in $(task_get_deps $TASK); do
            if ! stamp_check $DEP; then
                #echo "Redo: $TASK due to $DEP"
                stamp_clear $TASK
                break
            fi
       done
    done

    for TASK in $TASKS; do
        DESCR=$(task_get_descr $TASK)
        if stamp_check $TASK; then
            echo "Skipping: $DESCR"
            continue
        fi
        echo "Running: $DESCR"
        if [ "$VERBOSE" -ge 1 ] ; then
            (eval $(task_get_cmd $TASK))
        else
            (eval $(task_get_cmd $TASK)) >> $TMPLOG 2>&1
        fi
        if [ $? != 0 ] ; then
            echo "ERROR: Cannot $DESCR"
            exit 1
        fi

        stamp_set $TASK
    done
}

# This function is used to clone a source repository either from a given
# git base or a git reference.
# $1: project/subdir name
# $2: path to SOURCES file
toolchain_clone ()
{
    local GITFLAGS
    GITFLAGS=
    if [ "$GIT_REFERENCE" ]; then
        GITFLAGS="$GITFLAGS --shared --reference $GIT_REFERENCE/$1"
    fi
    echo "cleaning up toolchain/$1"
    rm -rf $1
    fail_panic "Could not clean $(pwd)/$1"
    echo "downloading sources for toolchain/$1"
    if [ -d "$GIT_BASE/$1" ]; then
        log "cloning $GIT_BASE/$1"
        run $GIT_CMD clone $GITFLAGS $GIT_BASE/$1 $1
    else
        log "cloning $GITPREFIX/$1.git"
        run $GIT_CMD clone $GITFLAGS $GIT_BASE/$1.git $1
    fi
    fail_panic "Could not clone $GIT_BASE/$1.git ?"
    cd $1
    if [ "$GIT_BRANCH" != "master" ] ; then
        log "checking out $GIT_BRANCH branch of $1.git"
        run $GIT_CMD checkout -b $GIT_BRANCH origin/$GIT_BRANCH
        fail_panic "Could not checkout $1 ?"
    fi
    # If --git-date is used, or we have a default
    if [ -n "$GIT_DATE" ] ; then
        REVISION=`git rev-list -n 1 --until="$GIT_DATE" HEAD`
        echo "Using sources for date '$GIT_DATE': toolchain/$1 revision $REVISION"
        run $GIT_CMD checkout $REVISION
        fail_panic "Could not checkout $1 ?"
    fi
    (printf "%-32s " "toolchain/$1.git: " && git log -1 --format=oneline) >> $2
    cd ..
}

task_define download_toolchain_sources "Download toolchain sources from $GIT_BASE "
cmd_download_toolchain_sources ()
{
    local SUBDIRS="binutils build gcc gdb gold gmp mpfr mpc isl cloog"
    (mkdir -p $TOOLCHAIN_SRC_DIR && cd $TOOLCHAIN_SRC_DIR &&
    # Create a temporary SOURCES file for the toolchain sources only
    # It's content will be copied to the final SOURCES file later.
    SOURCES_LIST=$TOOLCHAIN_SRC_DIR/SOURCES
    rm -f $SOURCES_LIST && touch $SOURCES_LIST
    for SUB in $SUBDIRS; do
        toolchain_clone $SUB $SOURCES_LIST
    done
    )
}

task_define download_ubuntu_packages_list "Download Ubuntu packages list"
cmd_download_ubuntu_packages_list ()
{
    mkdir -p $DOWNLOAD_DIR
    get_ubuntu_packages_list "$UBUNTU_MIRROR" "$UBUNTU_RELEASE"
    fail_panic "Unable to download packages list, try --ubuntu-mirror=<url> to use another archive mirror"
}

task_define download_packages "Download Ubuntu packages"
task_depends download_packages download_ubuntu_packages_list
cmd_download_packages ()
{
    local PACKAGE PKGURL

    rm -f $DOWNLOAD_DIR/SOURCES && touch $DOWNLOAD_DIR/SOURCES
    for PACKAGE in $UBUNTU_PACKAGES; do
        echo "Downloading $PACKAGE"
	    for UA in $UBUNTU_ARCHS; do
            PKGURL=`get_ubuntu_package_deb_url $PACKAGE $UBUNTU_MIRROR $UA`
            echo "URL: $PKGURL"
            download_file_to $PKGURL $DOWNLOAD_DIR
            fail_panic "Could not download $PKGURL"
        done
    done
    sha1sum $DOWNLOAD_DIR/*.deb | while read LINE; do
        PACKAGE=$(basename $(echo $LINE | awk '{ print $2;}'))
        SHA1=$(echo $LINE | awk '{ print $1; }')
        printf "%-64s %s\n" $PACKAGE $SHA1 >> $DOWNLOAD_DIR/SOURCES
    done
}

task_define build_sysroot "Build sysroot"
task_depends build_sysroot download_packages

cmd_build_sysroot ()
{
    local PACKAGE PKGURL SRC_PKG
    mkdir -p $SRC_PKG $ORG_SYSROOT_DIR
    for PACKAGE in $UBUNTU_PACKAGES; do
	    for UA in $UBUNTU_ARCHS; do
            PKGURL=`get_ubuntu_package_deb_url $PACKAGE $UBUNTU_MIRROR $UA`
            SRC_PKG=$DOWNLOAD_DIR/`basename $PKGURL`
            echo "Extracting $SRC_PKG"
            dpkg -x $SRC_PKG $ORG_SYSROOT_DIR/$UA
	    done
    done
}

# Now, we need to patch libc.so which is actually a linker script
# referencing /lib* and /usr/lib*. Do the same for libpthread.so
patch_library ()
{
    echo "Patching $1"
    sed -i -e "s: /usr/lib[^ ]*/: :g;s: /lib[^ ]*/: :g" $1
}

# Used to setup phase 1 the run a command
phase_1 ()
{
    PHASE=1
    $@
}

# Used to setup phase 2 then run a command
phase_2 ()
{
    PHASE=1
    BINPREFIX=$(install_dir)/bin/${GCC_TARGET}-
    CC=${BINPREFIX}gcc
    CXX=${BINPREFIX}g++
    LD=${BINPREFIX}ld
    AR=${BINPREFIX}ar
    AS=${BINPREFIX}as
    RANLIB=${BINPREFIX}ranlib
    STRIP=${BINPREFIX}strip
    CC_FOR_TARGET=${BINPREFIX}gcc
    export CC CXX LD AR AS RANLIB STRIP CC_FOR_TARGET
    PHASE=2
    $@
}

# Return the list of all symbolic links in a given directory, excluding
# any links in its sub-directories.
# $1: Sub-directory path.
find_symlinks_in () {
    (cd $1 && find . -maxdepth 1 -type l) | sed -e 's|^\./||g'
}

task2_define copy_sysroot "Fix and copy sysroot"
task2_depends1 copy_sysroot build_sysroot
cmd_copy_sysroot ()
{
    local SL

    # Copy the content of $ORG_SYSROOT_DIR/.../lib to $(sysroot_dir)/usr/lib32
    copy_directory $ORG_SYSROOT_DIR/i386/lib $(sysroot_dir)/usr/lib32
    copy_directory $ORG_SYSROOT_DIR/i386/usr/lib $(sysroot_dir)/usr/lib32
    copy_directory $ORG_SYSROOT_DIR/i386/usr/include $(sysroot_dir)/usr/include

    copy_directory $ORG_SYSROOT_DIR/amd64/lib $(sysroot_dir)/usr/lib
    copy_directory $ORG_SYSROOT_DIR/amd64/usr/lib $(sysroot_dir)/usr/lib
    copy_directory $ORG_SYSROOT_DIR/amd64/usr/include $(sysroot_dir)/usr/include

    # Ubuntu precise release has .so files in
    # /usr/lib/x86_64-linux-gnu and /usr/lib32/i386-linux-gnu.
    for LIB in $(sysroot_dir)/usr/lib/x86_64-linux-gnu \
               $(sysroot_dir)/usr/lib32/i386-linux-gnu; do
        mv $LIB/* `dirname $LIB` && rmdir $LIB
        fail_panic "Cannot move files in $LIB"
    done

    for LIB in lib lib32; do
        # We need to fix the symlink like librt.so -> /lib*/librt.so.1
        # in $(sysroot_dir)/usr/$LIB, they should point to librt.so.1 instead now.
        SYMLINKS=$(find_symlinks_in $(sysroot_dir)/usr/$LIB)
        cd $(sysroot_dir)/usr/$LIB
        for SL in $SYMLINKS; do
            # convert /$LIB/libfoo.so.<n> into 'libfoo.so.<n>' for the target
            local DST=$(readlink $SL 2>/dev/null)
            local DST2=`basename $DST`
            if [ "$DST2" != "$DST" ]; then
                echo "Fixing symlink $SL --> $DST"
                rm $SL && ln -s $DST2 $SL
            fi
        done
        patch_library libc.so
        patch_library libpthread.so
    done
}

task_define patch_toolchain_sources "Patch toolchain sources."
task_depends patch_toolchain_sources download_toolchain_sources
cmd_patch_toolchain_sources ()
{
    log "PATCHES_DIR = $PATCHES_DIR"
    if [ ! -d "$PATCHES_DIR" ]; then
        log "$PATCHES_DIR doesn't exist"
        return 0
    fi

    local PATCHES=`(cd $PATCHES_DIR && find . -name "*.patch" | sort ) 2> /dev/null`
    if [ -z "$PATCHES" ] ; then
        log "No patches files in $PATCHES_DIR"
        return 0
    fi

    PATCHES=`echo $PATCHES | sed -e s%^\./%%g`
    for PATCH in $PATCHES; do
        PATCHDIR=`dirname $PATCH`
        PATCHNAME=`basename $PATCH`
        log "Applying $PATCHNAME into $TOOLCHAIN_SRC_DIR/$PATCHDIR"
        (cd $TOOLCHAIN_SRC_DIR/$PATCHDIR && patch -p1 < $PATCHES_DIR/$PATCH)
        fail_panic "Patch failure!! Please check your patches directory!"
    done

    log "Done patching."
}

task_define prepare_toolchain_sources "Prepare toolchain sources."
if [ -n "$GIT_BASE" -o -n "$GIT_REFERENCE" ]; then
    task_depends prepare_toolchain_sources patch_toolchain_sources
fi
cmd_prepare_toolchain_sources ()
{
    return
}

task2_define configure_binutils "Configure binutils-$BINUTILS_VERSION"
task2_depends1 configure_binutils prepare_toolchain_sources
task2_depends2 configure_binutils copy_sysroot
cmd_configure_binutils ()
{
    OUT_DIR=$(build_dir_for binutils)
    mkdir -p $OUT_DIR && cd $OUT_DIR &&
    run $TOOLCHAIN_SRC_DIR/binutils/binutils-$BINUTILS_VERSION/configure \
        --prefix=$(install_dir) \
        --with-sysroot=$(sysroot_dir) \
        --target=$GCC_TARGET \
        --enable-gold=default \
        --with-host-libstdcxx='-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic -lm' \
        --with-gold-ldflags='-static-libgcc -static-libstdc++' \
        --with-bugurl=http://source.android.com/source/report-bugs.html
}

task2_define build_binutils "Build binutils-$BINUTILS_VERSION"
task2_depends2 build_binutils configure_binutils
cmd_build_binutils ()
{
    cd $(build_dir_for binutils) &&
    make $MAKE_FLAGS
}

task2_define install_binutils "Install binutils-$BINUTILS_VERSION"
task2_depends2 install_binutils build_binutils
cmd_install_binutils ()
{
    cd $(build_dir_for binutils) &&
    make install
}

task2_define extract_gmp "Extract sources for gmp-$GMP_VERSION"
task2_depends1 extract_gmp prepare_toolchain_sources
cmd_extract_gmp ()
{
    OUT_DIR=$(build_dir_for gmp)
    GMP_TARBALL=$TOOLCHAIN_SRC_DIR/gmp/gmp-$GMP_VERSION.tar.bz2
    if [ ! -f "$GMP_TARBALL" ]; then
        GMP_TARBALL=$TOOLCHAIN_SRC_DIR/tarballs/gmp-$GMP_VERSION.tar.bz2
        if [ ! -f "$GMP_TARBALL" ]; then
            panic "Can't find gmp-$GMP_VERSION sources!!"
        fi
    fi
    mkdir -p $OUT_DIR && cd $OUT_DIR &&
    tar xjf "$GMP_TARBALL"
}

task2_define configure_gmp "Configure gmp-$GMP_VERSION"
task2_depends2 configure_gmp extract_gmp install_binutils
cmd_configure_gmp ()
{
    export ABI=$GMP_ABI &&
    cd $(build_dir_for gmp) && mkdir -p build && cd build &&
    ../gmp-$GMP_VERSION/configure \
        --prefix=$(install_dir) \
        --host=$GMP_TARGET \
        --with-sysroot=$(install_dir) \
        --disable-shared
}

task2_define build_gmp "Build gmp-$GMP_VERSION"
task2_depends2 build_gmp configure_gmp
cmd_build_gmp ()
{
    export ABI=$GMP_ABI &&
    cd $(build_dir_for gmp)/build &&
    make $MAKE_FLAGS
}

task2_define install_gmp "Install gmp-$GMP_VERSION"
task2_depends2 install_gmp build_gmp
cmd_install_gmp ()
{
    cd $(build_dir_for gmp)/build &&
    make install
}

# Third, build mpfr
task2_define extract_mpfr "Extract sources from mpfr-$MPFR_VERSION"
task2_depends1 extract_mpfr prepare_toolchain_sources
cmd_extract_mpfr ()
{
    OUT_DIR=$(build_dir_for mpfr)
    MPFR_TARBALL=$TOOLCHAIN_SRC_DIR/mpfr/mpfr-$MPFR_VERSION.tar.bz2
    if [ ! -f "$MPFR_TARBALL" ]; then
        MPFR_TARBALL=$TOOLCHAIN_SRC_DIR/tarballs/mpfr-$MPFR_VERSION.tar.bz2
        if [ ! -f "$MPFR_TARBALL" ]; then
            panic "Can't find mpfr-$MPFR_VERSION sources!!"
        fi
    fi
    mkdir -p $OUT_DIR && cd $OUT_DIR &&
    tar xjf "$MPFR_TARBALL"
}

task2_define configure_mpfr "Configure mpfr-$MPFR_VERSION"
task2_depends2 configure_mpfr extract_mpfr install_gmp
cmd_configure_mpfr ()
{
    cd $(build_dir_for mpfr) && mkdir -p build && cd build &&
    run ../mpfr-$MPFR_VERSION/configure \
        --prefix=$(install_dir) \
        --host=$GMP_TARGET \
        --with-gmp=$(install_dir) \
        --with-sysroot=$(sysroot_dir) \
        --disable-shared
}

task2_define build_mpfr "Build mpfr-$MPFR_VERSION"
task2_depends2 build_mpfr configure_mpfr
cmd_build_mpfr ()
{
    cd $(build_dir_for mpfr)/build &&
    run make $MAKE_FLAGS
}

task2_define install_mpfr "Install mpfr-$MPFR_VERSION"
task2_depends2 install_mpfr build_mpfr
cmd_install_mpfr ()
{
    cd $(build_dir_for mpfr)/build &&
    run make install
}

task2_define extract_mpc "Extract sources for mpc-$MPC_VERSION"
task2_depends1 extract_mpc prepare_toolchain_sources
cmd_extract_mpc ()
{
    OUT_DIR=$(build_dir_for mpc)
    MPC_TARBALL=$TOOLCHAIN_SRC_DIR/mpc/mpc-$MPC_VERSION.tar.gz
    if [ ! -f "$MPC_TARBALL" ]; then
        MPC_TARBALL=$TOOLCHAIN_SRC_DIR/tarballs/mpc-$MPC_VERSION.tar.gz
        if [ ! -f "$MPC_TARBALL" ]; then
            panic "Can't find mpc-$MPC_VERSION sources!!"
        fi
    fi
    mkdir -p $OUT_DIR && cd $OUT_DIR &&
    tar xzf "$MPC_TARBALL"
}

task2_define configure_mpc "Configure mpc-$MPC_VERSION"
task2_depends2 configure_mpc extract_mpc install_mpfr
cmd_configure_mpc ()
{
    cd $(build_dir_for mpc) && mkdir -p build && cd build &&
    run ../mpc-$MPC_VERSION/configure \
        --prefix=$(install_dir) \
        --host=$GMP_TARGET \
        --with-gmp=$(install_dir) \
        --with-mpfr=$(install_dir) \
        --disable-shared
}

task2_define build_mpc "Build mpc-$MPC_VERSION"
task2_depends2 build_mpc configure_mpc
cmd_build_mpc ()
{
    cd $(build_dir_for mpc)/build &&
    run make $MAKE_FLAGS
}

task2_define install_mpc "Install mpc-$MPC_VERSION"
task2_depends2 install_mpc build_mpc
cmd_install_mpc ()
{
    cd $(build_dir_for mpc)/build &&
    run make install
}

task2_define extract_isl "Extract sources for isl-$ISL_VERSION"
task2_depends2 extract_isl prepare_toolchain_sources
cmd_extract_isl ()
{
    OUT_DIR=$(build_dir_for isl)
    ISL_TARBALL=$TOOLCHAIN_SRC_DIR/isl/isl-$ISL_VERSION.tar.bz2
    if [ ! -f "$ISL_TARBALL" ]; then
        panic "Can't find isl-$ISL_VERSION sources!!"
    fi
    mkdir -p $OUT_DIR && cd $OUT_DIR &&
    tar xf "$ISL_TARBALL"
}

task2_define configure_isl "Configuring isl-$ISL_VERSION"
task2_depends2 configure_isl extract_isl install_gmp
cmd_configure_isl ()
{
    cd $(build_dir_for isl) && mkdir -p build && cd build &&
    run ../isl-$ISL_VERSION/configure \
        --prefix=$(install_dir) \
        --host=$GMP_TARGET \
        --with-gmp-prefix=$(install_dir) \
        --with-sysroot=$(sysroot_dir) \
        --disable-shared
}

task2_define build_isl "Building isl-$ISL_VERSION"
task2_depends2 build_isl configure_isl
cmd_build_isl ()
{
    cd $(build_dir_for isl)/build &&
    run make $MAKE_FLAGS
}

task2_define install_isl "Installing isl-$ISL_VERSION"
task2_depends2 install_isl build_isl
cmd_install_isl ()
{
    cd $(build_dir_for isl)/build &&
    make install
}

task2_define configure_cloog "Configure Cloog-$CLOOG_VERSION"
task2_depends2 configure_cloog prepare_toolchain_sources install_gmp install_isl
cmd_configure_cloog () {
    mkdir -p $(build_dir_for cloog)/build && cd $(build_dir_for cloog)/build &&
    run $TOOLCHAIN_SRC_DIR/cloog/cloog-$CLOOG_VERSION/configure \
        --prefix=$(install_dir) \
        --host=$GMP_TARGET \
        --with-gmp-prefix=$(install_dir) \
        --with-sysroot=$(sysroot_dir) \
        --disable-shared
}

task2_define build_cloog "Building Cloog-$CLOOG_VERSION"
task2_depends2 build_cloog configure_cloog
cmd_build_cloog ()
{
    cd $(build_dir_for cloog)/build &&
    run make $MAKE_FLAGS
}

task2_define install_cloog "Installing Cloog-$CLOOG_VERSION"
task2_depends2 install_cloog build_cloog
cmd_install_cloog ()
{
    cd $(build_dir_for cloog)/build &&
    run make install
}

# Fourth, the compiler itself
task2_define configure_gcc "Configure gcc-$GCC_VERSION"
task2_depends1 configure_gcc prepare_toolchain_sources
task2_depends2 configure_gcc install_binutils install_gmp install_mpfr install_mpc install_cloog
cmd_configure_gcc ()
{
    local EXTRA_CONFIGURE_FLAGS=
    if [ "$GCC_VERSION" != "4.6" ]; then
        EXTRA_CONFIGURE_FLAGS="--with-cloog=$(install_dir)"
    fi
    OUT_DIR=$(build_dir_for gcc)
    mkdir -p $OUT_DIR && cd $OUT_DIR &&
    export CC=$HOST_CC &&
    export CC_FOR_TARGET="$HOST_CC" &&
    run $TOOLCHAIN_SRC_DIR/gcc/gcc-$GCC_VERSION/configure \
        --enable-multiarch \
        --with-arch-32=i686 \
        --with-abi=m64 \
        --prefix=$(install_dir) \
        --with-sysroot=$(sysroot_dir) \
        --disable-nls \
        --with-gmp=$(install_dir) \
        --with-mpfr=$(install_dir) \
        --with-mpc=$(install_dir) \
        --target=$GCC_TARGET \
        --with-arch=x86-64 \
        --with-multilib-list=m32,m64 \
        --disable-plugin \
        --disable-docs \
        --disable-bootstrap \
        --disable-libgomp \
        --disable-libmudflap \
        --disable-libquadmath \
        --enable-target-optspace \
        --enable-gold=default \
        --enable-languages=c,c++ \
        $EXTRA_CONFIGURE_FLAGS
}

task2_define build_gcc "Build gcc-$GCC_VERSION"
task2_depends2 build_gcc configure_gcc
cmd_build_gcc ()
{
    cd $(build_dir_for gcc) &&
    make $MAKE_FLAGS
}

task2_define install_gcc "Install gcc-$GCC_VERSION"
task2_depends2 install_gcc build_gcc
cmd_install_gcc ()
{
    cd $(build_dir_for gcc) &&
    make install
}

task2_define cleanup_toolchain "Cleanup toolchain"
task2_depends2 cleanup_toolchain install_gcc
cmd_cleanup_toolchain ()
{
    # Remove un-needed directories and files
    rm -rf $(install_dir)/share
    rm -rf $(install_dir)/man
    rm -rf $(install_dir)/info
    rm -rf $(install_dir)/libexec/*/*/install-tools
    #rm -rf $(install_dir)/$GCC_TARGET/bin
    find $(install_dir) -name "*.la" -exec rm -f {} \;

    (strip $(install_dir)/bin/*)
    (strip $(install_dir)/libexec/gcc/$GCC_TARGET/*/*)
    true
}

task2_define package_toolchain "Package final toolchain"
task2_depends2 package_toolchain cleanup_toolchain
cmd_package_toolchain ()
{
    # Copy this script to the install directory
    cp -f $0 $(install_dir)
    fail_panic "Could not copy build script to install directory"

    if [ -d "$PATCHES_DIR" ]; then
        # Copy patches to the install directory
        cp -rf "$PATCHES_DIR" $(install_dir)
        fail_panic "Could not copy patch directory to install directory"
    fi

    # Copy the SOURCES file as well
    cp $DOWNLOAD_DIR/SOURCES $(install_dir)/PACKAGE_SOURCES &&
    cp $TOOLCHAIN_SRC_DIR/SOURCES $(install_dir)/TOOLCHAIN_SOURCES
    fail_panic "Could not copy SOURCES files to install directory"

    # Package everything
    pack_archive $TOOLCHAIN_ARCHIVE "`dirname $(install_dir)`" "`basename $(install_dir)`"
}

task2_define install_toolchain "Install final toolchain"
task2_depends2 install_toolchain cleanup_toolchain
cmd_install_toolchain ()
{
    copy_directory "$(install_dir)" "$PREFIX_DIR/$TOOLCHAIN_NAME"
    cp -f $0 "$PREFIX_DIR/$TOOLCHAIN_NAME/"
}

# Make sure that the second toolchain depends on the first one
task_depends configure_binutils_2 install_gcc_1

if [ "$ONLY_SYSROOT" = "yes" ]; then
    MAIN_TASK=copy_sysroot
    COMPLETION_TEXT="Done, see sysroot files in $(sysroot_dir)"
elif [ -n "$PREFIX_DIR" ]; then
    if [ -z "$BOOTSTRAP" ]; then
        MAIN_TASK=install_toolchain_1
    else
        MAIN_TASK=install_toolchain_2
    fi
    COMPLETION_TEXT="Done, see $PREFIX_DIR/$TOOLCHAIN_NAME"
else
    if [ -z "$BOOTSTRAP" ]; then
        MAIN_TASK=package_toolchain_1
    else
        MAIN_TASK=package_toolchain_2
    fi
    COMPLETION_TEXT="Done, see $TOOLCHAIN_ARCHIVE"
fi

if [ "$LIST_TASKS" ]; then
    task_dump
else
    run_task $MAIN_TASK
    echo "$COMPLETION_TEXT"
fi
