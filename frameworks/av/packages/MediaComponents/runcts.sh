#!/bin/bash
# Copyright 2018 The Android Open Source Project
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

# Usage '. runcts.sh'

function _runtest_cts_mediacomponent_usage() {
  echo 'runtest-cts-MediaComponents [option]: Build, flash device,'
  echo '              and run subset of CtsMediaTestCases that MediaComponents covers.'
  echo '          *Warning* This bypasses CTS setup (e.g. download media contents from server)'
  echo '          For running CTS in official way, use atest or cts-tradefed '
  echo '     -h|--help: This help'
  echo '     --skip: Skip build and flash. Just rerun-tests'
  echo '     --min: Only rebuild tests and updatable library.'
  echo '     --test: Only rebuild tests'
  echo '     -s [device_id]: Specify a device name to run test against.'
  echo '                     You can define ${ADBHOST} instead.'
  echo '     -r [count]: Repeat tests for given count. It will stop when fails.'
  echo '     --ignore: Keep repeating tests even when it fails.'
  echo '     -t [test]: Only run the specific test. Can be either a class or a method.'
}

function runtest-cts-MediaComponents() {
  # Edit here if you want to support other tests.
  # List up libs and apks in the media_api needed for tests, and place test target at the last.
  local TEST_PACKAGE_DIR=("frameworks/av/packages/MediaComponents/test")
  local TEST_PACKAGE=("android.media.cts")
  local BUILD_TARGETS=("MediaComponents" "CtsMediaTestCases")
  # Don't include MediaComponents -- if we simply install it, system server
  # wouldn't use the installed one.
  local INSTALL_TARGETS=("CtsMediaTestCases")
  local TEST_RUNNER="android.support.test.runner.AndroidJUnitRunner"
  local DEPENDENCIES=("mockito-target-minus-junit4" "android-support-test" "compatibility-device-util")
  local DEFAULT_TEST_TARGET=""
  DEFAULT_TEST_TARGET+="android.media.cts.MediaBrowser2Test"
  DEFAULT_TEST_TARGET+=",android.media.cts.MediaController2Test"
  DEFAULT_TEST_TARGET+=",android.media.cts.MediaMetadata2Test"
  DEFAULT_TEST_TARGET+=",android.media.cts.MediaSession2Test"
  DEFAULT_TEST_TARGET+=",android.media.cts.MediaSession2_PermissionTest"
  DEFAULT_TEST_TARGET+=",android.media.cts.MediaSessionManager_MediaSession2Test"
  DEFAULT_TEST_TARGET+=",android.media.cts.SessionToken2Test"
  if [[ -z "${ANDROID_BUILD_TOP}" ]]; then
    echo "Needs to lunch a target first"
    return
  fi

  local old_path=${OLDPWD}
  while true; do
    local OPTION_SKIP="false"
    local OPTION_MIN="false"
    local OPTION_TEST="false"
    local OPTION_REPEAT_COUNT="1"
    local OPTION_IGNORE="false"
    local OPTION_TEST_TARGET="${DEFAULT_TEST_TARGET}"
    local adbhost_local
    while (( "$#" )); do
      case "${1}" in
        -h|--help)
          _runtest_cts_mediacomponent_usage
          return
          ;;
        --skip)
          OPTION_SKIP="true"
          ;;
        --min)
          OPTION_MIN="true"
          ;;
        --test)
          OPTION_TEST="true"
          ;;
        -s)
          shift
          adbhost_local=${1}
          ;;
        -r)
          shift
          OPTION_REPEAT_COUNT="${1}"
          ;;
        --ignore)
          OPTION_IGNORE="true"
          ;;
        -t)
          shift
          OPTION_TEST_TARGET="${1}"
      esac
      shift
    done

    # Build adb command.
    local adb
    if [[ -z "${adbhost_local}" ]]; then
      adbhost_local=${ADBHOST}
    fi
    if [[ -z "${adbhost_local}" ]]; then
      local device_count=$(adb devices | sed '/^[[:space:]]*$/d' | wc -l)
      if [[ "${device_count}" != "2" ]]; then
        echo "Too many devices. Specify a device." && break
      fi
      adb="adb"
    else
      adb="adb -s ${adbhost_local}"
    fi

    local target_dir="${ANDROID_BUILD_TOP}/${TEST_PACKAGE_DIR}"
    #local TEST_PACKAGE=$(sed -n 's/^.*\bpackage\b="\([a-z0-9\.]*\)".*$/\1/p' ${target_dir}/AndroidManifest.xml)

    if [[ "${OPTION_SKIP}" != "true" ]]; then
      # Build dependencies if needed.
      local dependency
      local build_dependency=""
      for dependency in ${DEPENDENCIES[@]}; do
        if [[ "${dependency}" == "out/"* ]]; then
          if [[ ! -f ${ANDROID_BUILD_TOP}/${dependency} ]]; then
            build_dependency="true"
            break
          fi
        else
          if [[ "$(find ${OUT} -name ${dependency}_intermediates | wc -l)" == "0" ]]; then
            build_dependency="true"
            break
          fi
        fi
      done
      if [[ "${build_dependency}" == "true" ]]; then
        echo "Building dependencies. Will only print stderr."
        m ${DEPENDENCIES[@]} -j > /dev/null
      fi

      # Build test apk and required apk.
      local build_targets
      if [[ "${OPTION_TEST}" == "true" ]]; then
        build_targets="${INSTALL_TARGETS[@]}"
      elif [[ "${OPTION_MIN}" == "true" ]]; then
        build_targets="${BUILD_TARGETS[@]}"
      else
        build_targets="${BUILD_TARGETS[@]} droid"
      fi
      m ${build_targets} -j || break

      if [[ "${OPTION_TEST}" != "true" ]]; then
        # Flash only when needed
        local device_build_type="$(${adb} shell getprop ro.build.type)"
        if [[ "${device_build_type}" == "user" ]]; then
          # User build. Cannot adb sync
          ${adb} reboot bootloader
          fastboot flashall
        else
          ${adb} root
          local device_verity_mode="$(${adb} shell getprop ro.boot.veritymode)"
          if [[ "${device_verity_mode}" != "disabled" ]]; then
            ${adb} disable-verity
            ${adb} reboot
            ${adb} wait-for-device || break
            ${adb} root
          fi
          ${adb} remount
          ${adb} shell stop
          ${adb} shell setprop log.tag.MediaSessionService DEBUG
          ${adb} sync
          ${adb} shell start
        fi
        ${adb} wait-for-device || break
        # Ensure package manager is loaded.
        # TODO(jaewan): Find better way to wait
        sleep 15
      fi

      # Install apks
      local install_failed="false"
      for target in ${INSTALL_TARGETS[@]}; do
        local apk_path=$(find ${OUT}/system ${OUT}/data -name ${target}.apk)
        local apk_num=$(find ${OUT}/system ${OUT}/data -name ${target}.apk | wc -l)
        if [[ "${apk_num}" != "1" ]]; then
          echo "Cannot locate a ${target}.apk. Found ${apk_num} apks" && break
        fi
        echo "Installing ${target}.apk. path=${apk_path}"
        ${adb} install -r ${apk_path}
        if [[ "${?}" != "0" ]]; then
          install_failed="true"
          break
        fi
      done
      if [[ "${install_failed}" == "true" ]]; then
        echo "Failed to install. Test wouldn't run."
        break
      fi
    fi

    local test_target=""
    if [[ -n "${OPTION_TEST_TARGET}" ]]; then
      test_target="-e class ${OPTION_TEST_TARGET}"
    fi

    local i
    local tmpfile=$(tempfile)
    for ((i=1; i <= ${OPTION_REPEAT_COUNT}; i++)); do
      echo "Run test ${i}/${OPTION_REPEAT_COUNT}"
      ${adb} shell am instrument ${test_target} -w ${TEST_PACKAGE}/${TEST_RUNNER} >& ${tmpfile}
      cat ${tmpfile}
      if [[ "${OPTION_IGNORE}" != "true" ]]; then
        if [[ -n "$(grep ${tmpfile} -e 'FAILURE\|crashed')" ]]; then
          # am instrument doesn't return error code so need to grep result message instead
          break
        fi
      fi
    done
    rm ${tmpfile}
    break
  done
}

echo "Following functions are added to your environment:"
_runtest_cts_mediacomponent_usage
