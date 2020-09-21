#
# Copyright (C) 2014 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

#
# To sync with upstream:
#

#  # Update.
#  git remote add toybox https://github.com/landley/toybox.git
#  git fetch toybox
#  git merge toybox/master

#  # Regenerate generated files.
#  make

#  # Make any necessary Android.mk changes and rebuild.
#  mm -j32

#  # Run tests.
#  ./run-tests-on-android.sh
#  # Run a single test.
#  ./run-tests-on-android.sh wc

#  # Upload changes.
#  git commit -a --amend
#  git push aosp HEAD:refs/for/master  # Push to gerrit for review.
#  git push aosp HEAD:master  # Push directly, avoiding gerrit.


#
# To add a toy:
#

#  Edit .config to enable the toy you want to add.
#  make clean && make  # Regenerate the generated files.
#  # Edit LOCAL_SRC_FILES below to add the toy.
#  # If you just want to use it as "toybox x" rather than "x", you can stop now.
#  # If you want this toy to have a symbolic link in /system/bin, add the toy to ALL_TOOLS.

common_SRC_FILES := \
    lib/args.c \
    lib/dirtree.c \
    lib/getmountlist.c \
    lib/help.c \
    lib/interestingtimes.c \
    lib/lib.c \
    lib/linestack.c \
    lib/llist.c \
    lib/net.c \
    lib/portability.c \
    lib/xwrap.c \
    main.c \
    toys/android/getenforce.c \
    toys/android/load_policy.c \
    toys/android/log.c \
    toys/android/restorecon.c \
    toys/android/runcon.c \
    toys/android/sendevent.c \
    toys/android/setenforce.c \
    toys/android/setprop.c \
    toys/android/start.c \
    toys/lsb/dmesg.c \
    toys/lsb/hostname.c \
    toys/lsb/killall.c \
    toys/lsb/md5sum.c \
    toys/lsb/mknod.c \
    toys/lsb/mktemp.c \
    toys/lsb/mount.c \
    toys/lsb/pidof.c \
    toys/lsb/seq.c \
    toys/lsb/sync.c \
    toys/lsb/umount.c \
    toys/net/ifconfig.c \
    toys/net/microcom.c \
    toys/net/netcat.c \
    toys/net/netstat.c \
    toys/net/rfkill.c \
    toys/net/tunctl.c \
    toys/other/acpi.c \
    toys/other/base64.c \
    toys/other/blkid.c \
    toys/other/blockdev.c \
    toys/other/chcon.c \
    toys/other/chroot.c \
    toys/other/chrt.c \
    toys/other/clear.c \
    toys/other/dos2unix.c \
    toys/other/fallocate.c \
    toys/other/flock.c \
    toys/other/free.c \
    toys/other/freeramdisk.c \
    toys/other/fsfreeze.c \
    toys/other/help.c \
    toys/other/hwclock.c \
    toys/other/inotifyd.c \
    toys/other/insmod.c \
    toys/other/ionice.c \
    toys/other/losetup.c \
    toys/other/lsattr.c \
    toys/other/lsmod.c \
    toys/other/lspci.c \
    toys/other/lsusb.c \
    toys/other/makedevs.c \
    toys/other/mkswap.c \
    toys/other/modinfo.c \
    toys/other/mountpoint.c \
    toys/other/nbd_client.c \
    toys/other/partprobe.c \
    toys/other/pivot_root.c \
    toys/other/pmap.c \
    toys/other/printenv.c \
    toys/other/pwdx.c \
    toys/other/readlink.c \
    toys/other/realpath.c \
    toys/other/rev.c \
    toys/other/rmmod.c \
    toys/other/setfattr.c \
    toys/other/setsid.c \
    toys/other/stat.c \
    toys/other/swapoff.c \
    toys/other/swapon.c \
    toys/other/sysctl.c \
    toys/other/tac.c \
    toys/other/taskset.c \
    toys/other/timeout.c \
    toys/other/truncate.c \
    toys/other/uptime.c \
    toys/other/usleep.c \
    toys/other/vconfig.c \
    toys/other/vmstat.c \
    toys/other/which.c \
    toys/other/xxd.c \
    toys/other/yes.c \
    toys/pending/dd.c \
    toys/pending/diff.c \
    toys/pending/expr.c \
    toys/pending/fmt.c \
    toys/pending/getfattr.c \
    toys/pending/gzip.c \
    toys/pending/lsof.c \
    toys/pending/modprobe.c \
    toys/pending/more.c \
    toys/pending/stty.c \
    toys/pending/tar.c \
    toys/pending/tr.c \
    toys/pending/traceroute.c \
    toys/posix/basename.c \
    toys/posix/cal.c \
    toys/posix/cat.c \
    toys/posix/chgrp.c \
    toys/posix/chmod.c \
    toys/posix/cksum.c \
    toys/posix/cmp.c \
    toys/posix/comm.c \
    toys/posix/cp.c \
    toys/posix/cpio.c \
    toys/posix/cut.c \
    toys/posix/date.c \
    toys/posix/df.c \
    toys/posix/dirname.c \
    toys/posix/du.c \
    toys/posix/echo.c \
    toys/posix/env.c \
    toys/posix/expand.c \
    toys/posix/false.c \
    toys/posix/file.c \
    toys/posix/find.c \
    toys/posix/grep.c \
    toys/posix/head.c \
    toys/posix/id.c \
    toys/posix/kill.c \
    toys/posix/ln.c \
    toys/posix/ls.c \
    toys/posix/mkdir.c \
    toys/posix/mkfifo.c \
    toys/posix/nice.c \
    toys/posix/nl.c \
    toys/posix/nohup.c \
    toys/posix/od.c \
    toys/posix/paste.c \
    toys/posix/patch.c \
    toys/posix/printf.c \
    toys/posix/ps.c \
    toys/posix/pwd.c \
    toys/posix/renice.c \
    toys/posix/rm.c \
    toys/posix/rmdir.c \
    toys/posix/sed.c \
    toys/posix/sleep.c \
    toys/posix/sort.c \
    toys/posix/split.c \
    toys/posix/strings.c \
    toys/posix/tail.c \
    toys/posix/tee.c \
    toys/posix/time.c \
    toys/posix/touch.c \
    toys/posix/true.c \
    toys/posix/tty.c \
    toys/posix/ulimit.c \
    toys/posix/uname.c \
    toys/posix/uniq.c \
    toys/posix/uudecode.c \
    toys/posix/uuencode.c \
    toys/posix/wc.c \
    toys/posix/xargs.c \

common_CFLAGS := \
    -std=gnu11 \
    -Os \
    -Wall -Werror \
    -Wno-char-subscripts \
    -Wno-gnu-variable-sized-type-not-at-end \
    -Wno-missing-field-initializers \
    -Wno-sign-compare \
    -Wno-string-plus-int \
    -Wno-uninitialized \
    -Wno-unused-parameter \
    -funsigned-char \
    -ffunction-sections -fdata-sections \
    -fno-asynchronous-unwind-tables \

toybox_libraries := liblog libselinux libcutils libcrypto libz

common_CFLAGS += -DTOYBOX_VENDOR=\"-android\"

# not usable on Android?: freeramdisk fsfreeze install makedevs nbd-client
#                         partprobe pivot_root pwdx rev rfkill vconfig
# currently prefer BSD system/core/toolbox: dd
# currently prefer BSD external/netcat: nc netcat
# currently prefer external/efs2progs: blkid chattr lsattr

ALL_TOOLS := \
    acpi \
    base64 \
    basename \
    blockdev \
    cal \
    cat \
    chcon \
    chgrp \
    chmod \
    chown \
    chroot \
    chrt \
    cksum \
    clear \
    comm \
    cmp \
    cp \
    cpio \
    cut \
    date \
    df \
    diff \
    dirname \
    dmesg \
    dos2unix \
    du \
    echo \
    env \
    expand \
    expr \
    fallocate \
    false \
    file \
    find \
    flock \
    fmt \
    free \
    getenforce \
    groups \
    gunzip \
    gzip \
    head \
    hostname \
    hwclock \
    id \
    ifconfig \
    inotifyd \
    insmod \
    ionice \
    iorenice \
    kill \
    killall \
    load_policy \
    ln \
    log \
    logname \
    losetup \
    ls \
    lsmod \
    lsof \
    lspci \
    lsusb \
    md5sum \
    mkdir \
    mkfifo \
    mknod \
    mkswap \
    mktemp \
    microcom \
    modinfo \
    modprobe \
    more \
    mount \
    mountpoint \
    mv \
    netstat \
    nice \
    nl \
    nohup \
    od \
    paste \
    patch \
    pgrep \
    pidof \
    pkill \
    pmap \
    printenv \
    printf \
    ps \
    pwd \
    readlink \
    realpath \
    renice \
    restorecon \
    rm \
    rmdir \
    rmmod \
    runcon \
    sed \
    sendevent \
    seq \
    setenforce \
    setprop \
    setsid \
    sha1sum \
    sha224sum \
    sha256sum \
    sha384sum \
    sha512sum \
    sleep \
    sort \
    split \
    start \
    stat \
    stop \
    strings \
    stty \
    swapoff \
    swapon \
    sync \
    sysctl \
    tac \
    tail \
    tar \
    taskset \
    tee \
    time \
    timeout \
    top \
    touch \
    tr \
    true \
    truncate \
    tty \
    ulimit \
    umount \
    uname \
    uniq \
    unix2dos \
    uptime \
    usleep \
    uudecode \
    uuencode \
    vmstat \
    wc \
    which \
    whoami \
    xargs \
    xxd \
    yes \
    zcat \

############################################
# toybox for /system
############################################

include $(CLEAR_VARS)
LOCAL_MODULE := toybox
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS := $(common_CFLAGS)
LOCAL_SHARED_LIBRARIES := $(toybox_libraries)
# This doesn't actually prevent us from dragging in libc++ at runtime
# because libnetd_client.so is C++.
LOCAL_CXX_STL := none
LOCAL_POST_INSTALL_CMD := $(hide) $(foreach t,$(ALL_TOOLS),ln -sf toybox $(TARGET_OUT)/bin/$(t);)
include $(BUILD_EXECUTABLE)

############################################
# toybox for /vendor
############################################

include $(CLEAR_VARS)
LOCAL_MODULE := toybox_vendor
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS := $(common_CFLAGS)
LOCAL_STATIC_LIBRARIES := libcutils libcrypto libz
LOCAL_SHARED_LIBRARIES := libselinux liblog
LOCAL_MODULE_TAGS := optional
LOCAL_POST_INSTALL_CMD := $(hide) $(foreach t,$(ALL_TOOLS),ln -sf ${LOCAL_MODULE} $(TARGET_OUT_VENDOR_EXECUTABLES)/$(t);)
include $(BUILD_EXECUTABLE)

############################################
# static version to be installed in recovery
############################################

include $(CLEAR_VARS)
LOCAL_MODULE := toybox_static
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS := $(common_CFLAGS)
LOCAL_STATIC_LIBRARIES := $(toybox_libraries)
# libc++_static is needed by static liblog
LOCAL_CXX_STL := libc++_static
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_POST_INSTALL_CMD := $(hide) $(foreach t,$(ALL_TOOLS),ln -sf ${LOCAL_MODULE} $(LOCAL_MODULE_PATH)/$(t);)
include $(BUILD_EXECUTABLE)
