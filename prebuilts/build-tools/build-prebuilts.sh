#!/bin/bash -ex

if [ -z "${OUT_DIR}" ]; then
    echo Must set OUT_DIR
    exit 1
fi

TOP=$(pwd)

UNAME="$(uname)"
case "$UNAME" in
Linux)
    OS='linux'
    ;;
Darwin)
    OS='darwin'
    ;;
*)
    exit 1
    ;;
esac

build_soong=1
if [ -d ${TOP}/toolchain/go ]; then
    build_go=1
fi

if [ -n ${build_soong} ]; then
    # ckati and makeparallel (Soong)
    SOONG_OUT=${OUT_DIR}/soong
    SOONG_HOST_OUT=${OUT_DIR}/soong/host/${OS}-x86
    rm -rf ${SOONG_OUT}
    mkdir -p ${SOONG_OUT}
    cat > ${SOONG_OUT}/soong.variables << EOF
{
    "Allow_missing_dependencies": true,
    "HostArch":"x86_64",
    "HostSecondaryArch":"x86"
}
EOF
    SOONG_BINARIES=(
        acp
        aidl
        bison
        bpfmt
        ckati
        ckati_stamp_dump
        makeparallel
        ninja
        soong_zip
        zip2zip
        zipalign
        ziptime
    )
    SOONG_ASAN_BINARIES=(
        acp
        aidl
        ckati
        makeparallel
        ninja
        zipalign
        ziptime
    )
    SOONG_JAVA_LIBRARIES=(
        desugar
        dx
        turbine
    )
    SOONG_JAVA_WRAPPERS=(
        dx
    )

    binaries=$(for i in "${SOONG_BINARIES[@]}"; do echo ${SOONG_HOST_OUT}/bin/${i}; done)
    asan_binaries=$(for i in "${SOONG_ASAN_BINARIES[@]}"; do echo ${SOONG_HOST_OUT}/bin/${i}; done)
    jars=$(for i in "${SOONG_JAVA_LIBRARIES[@]}"; do echo ${SOONG_HOST_OUT}/framework/${i}.jar; done)
    wrappers=$(for i in "${SOONG_JAVA_WRAPPERS[@]}"; do echo ${SOONG_HOST_OUT}/bin/${i}; done)

    # Build everything
    build/soong/soong_ui.bash --make-mode --skip-make \
        ${binaries} \
        ${wrappers} \
        ${jars} \
        ${SOONG_HOST_OUT}/nativetest64/ninja_test/ninja_test \
        ${SOONG_HOST_OUT}/nativetest64/ckati_test/find_test \
        soong_docs

    # Run ninja tests
    ${SOONG_HOST_OUT}/nativetest64/ninja_test/ninja_test

    # Run ckati tests
    ${SOONG_HOST_OUT}/nativetest64/ckati_test/find_test

    # Copy arch-specific binaries
    mkdir -p ${SOONG_OUT}/dist/bin
    cp ${binaries} ${SOONG_OUT}/dist/bin/
    cp -R ${SOONG_HOST_OUT}/lib* ${SOONG_OUT}/dist/

    # Copy jars and wrappers
    mkdir -p ${SOONG_OUT}/dist-common/bin ${SOONG_OUT}/dist-common/framework
    cp ${wrappers} ${SOONG_OUT}/dist-common/bin
    cp ${jars} ${SOONG_OUT}/dist-common/framework

    cp -r external/bison/data ${SOONG_OUT}/dist-common/bison

    if [[ $OS == "linux" ]]; then
        # Build ASAN versions
        export ASAN_OPTIONS=detect_leaks=0
        cat > ${SOONG_OUT}/soong.variables << EOF
{
    "Allow_missing_dependencies": true,
    "HostArch":"x86_64",
    "HostSecondaryArch":"x86",
    "SanitizeHost": ["address"]
}
EOF

        # Clean up non-ASAN installed versions
        rm -rf ${SOONG_HOST_OUT}

        # Build everything with ASAN
        build/soong/soong_ui.bash --make-mode --skip-make \
            ${asan_binaries} \
            ${SOONG_HOST_OUT}/nativetest64/ninja_test/ninja_test \
            ${SOONG_HOST_OUT}/nativetest64/ckati_test/find_test

        # Run ninja tests
        ${SOONG_HOST_OUT}/nativetest64/ninja_test/ninja_test

        # Run ckati tests
        ${SOONG_HOST_OUT}/nativetest64/ckati_test/find_test

        # Copy arch-specific binaries
        mkdir -p ${SOONG_OUT}/dist/asan/bin
        cp ${asan_binaries} ${SOONG_OUT}/dist/asan/bin/
        cp -R ${SOONG_HOST_OUT}/lib* ${SOONG_OUT}/dist/asan/
    fi

    # Package arch-specific prebuilts
    (
        cd ${SOONG_OUT}/dist
        zip -qryX build-prebuilts.zip *
    )

    # Package common prebuilts
    (
        cd ${SOONG_OUT}/dist-common
        zip -qryX build-common-prebuilts.zip *
    )
fi

# Go
if [ -n ${build_go} ]; then
    GO_OUT=${OUT_DIR}/obj/go
    rm -rf ${GO_OUT}
    mkdir -p ${GO_OUT}
    cp -a ${TOP}/toolchain/go/* ${GO_OUT}/
    (
        cd ${GO_OUT}/src
        export GOROOT_BOOTSTRAP=${TOP}/prebuilts/go/${OS}-x86
        export GOROOT_FINAL=./prebuilts/go/${OS}-x86
        export GO_TEST_TIMEOUT_SCALE=100
        export GOCACHE=off
        ./make.bash
        rm -rf ../pkg/bootstrap
        GOROOT=$(pwd)/.. ../bin/go install -race std
    )
    (
        cd ${GO_OUT}
        zip -qryX go.zip *
    )
fi

if [ -n "${DIST_DIR}" ]; then
    mkdir -p ${DIST_DIR} || true

    if [ -n ${build_soong} ]; then
        cp ${SOONG_OUT}/dist/build-prebuilts.zip ${DIST_DIR}/
        cp ${SOONG_OUT}/dist-common/build-common-prebuilts.zip ${DIST_DIR}/
        cp ${SOONG_OUT}/docs/soong_build.html ${DIST_DIR}/
    fi
    if [ -n ${build_go} ]; then
        cp ${GO_OUT}/go.zip ${DIST_DIR}/
    fi
fi
