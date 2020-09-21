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
    header-abi-linker
    header-abi-dumper
    header-abi-diff
    merge-abi-diff
)

binaries=$(for i in "${SOONG_BINARIES[@]}"; do echo ${SOONG_HOST_OUT}/bin/${i}; done)

# Build everything
build/soong/soong_ui.bash --make-mode --skip-make \
    ${binaries}

# Copy arch-specific binaries
mkdir -p ${SOONG_OUT}/dist/bin
cp ${binaries} ${SOONG_OUT}/dist/bin/
cp -R ${SOONG_HOST_OUT}/lib* ${SOONG_OUT}/dist/

# Package arch-specific prebuilts
(
    cd ${SOONG_OUT}/dist
    zip -qryX build-prebuilts.zip *
)

if [ -n "${DIST_DIR}" ]; then
    mkdir -p ${DIST_DIR} || true
    cp ${SOONG_OUT}/dist/build-prebuilts.zip ${DIST_DIR}/
fi
