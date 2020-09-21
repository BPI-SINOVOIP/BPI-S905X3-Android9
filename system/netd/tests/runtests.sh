#!/usr/bin/env bash

readonly PROJECT_TOP="system/netd"

# TODO:
#   - add Android.bp test targets
#   - switch away from runtest.py
readonly ALL_TESTS="
    server/netd_unit_test.cpp
    tests/netd_integration_test.cpp
"

REPO_TOP=""
DEBUG=""

function logToStdErr() {
    echo "$1" >&2
}

function testAndSetRepoTop() {
    if [[ -n "$1" && -d "$1/.repo" ]]; then
        REPO_TOP="$1"
        return 0
    fi
    return 1
}

function gotoRepoTop() {
    if testAndSetRepoTop "$ANDROID_BUILD_TOP"; then
        return
    fi

    while ! testAndSetRepoTop "$PWD"; do
        if [[ "$PWD" == "/" ]]; then
            break
        fi
        cd ..
    done
}

function runOneTest() {
    local testName="$1"
    local cmd="$REPO_TOP/development/testrunner/runtest.py -x $PROJECT_TOP/$testName"
    echo "###"
    echo "# $testName"
    echo "#"
    echo "# $cmd"
    echo "###"
    echo ""
    $DEBUG $cmd
    local rval=$?
    echo ""

    # NOTE: currently runtest.py returns 0 even for failed tests.
    return $rval
}

function main() {
    gotoRepoTop
    if ! testAndSetRepoTop "$REPO_TOP"; then
        logToStdErr "Could not find useful top of repo directory"
        return 1
    fi
    logToStdErr "using REPO_TOP=$REPO_TOP"

    if [[ -n "$1" ]]; then
        case "$1" in
            "-n")
                DEBUG=echo
                shift
                ;;
        esac
    fi

    # Allow us to do things like "runtests.sh integration", etc.
    readonly TEST_REGEX="$1"

    failures=0
    for testName in $ALL_TESTS; do
        if [[ -z "$TEST_REGEX" || "$testName" =~ "$TEST_REGEX" ]]; then
            runOneTest "$testName"
            let failures+=$?
        else
            logToStdErr "Skipping $testName"
        fi
    done

    echo "Number of tests failing: $failures"
    return $failures
}


main "$@"
exit $?
