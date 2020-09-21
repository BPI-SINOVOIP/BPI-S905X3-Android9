#!/bin/bash

function run() {
    local FAILED_TESTS=()

    # Tests directly relevant to HIDL infrustructure but aren't
    # located in system/tools/hidl
    local RELATED_RUNTIME_TESTS=(\
        libhidl_test \
    )

    local RUN_TIME_TESTS=(\
        libhidl-gen-utils_test \
    )
    RUN_TIME_TESTS+=(${RELATED_RUNTIME_TESTS[@]})

    local SCRIPT_TESTS=(\
        hidl_test\
        hidl_test_java\
        fmq_test\
    )

    $ANDROID_BUILD_TOP/build/soong/soong_ui.bash --make-mode -j \
        ${RUN_TIME_TESTS[*]} ${SCRIPT_TESTS[*]} || return

    adb sync || return

    local BITNESS=("nativetest" "nativetest64")

    for test in ${RUN_TIME_TESTS[@]}; do
        for bits in ${BITNESS[@]}; do
            echo $bits $test
            adb shell /data/$bits/$test/$test ||
                FAILED_TESTS+=("$bits:$test")
        done
    done

    for test in ${SCRIPT_TESTS[@]}; do
        echo $test
        adb shell /data/nativetest64/$test ||
            FAILED_TESTS+=("$test")
    done

    echo
    echo ===== ALL DEVICE TESTS SUMMARY =====
    echo
    if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
        for failed in ${FAILED_TESTS[@]}; do
            echo "FAILED TEST: $failed"
        done
    else
        echo "SUCCESS"
    fi
}

run