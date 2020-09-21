#!/bin/bash
# Force stop on first error.
set -e
if [ $# -ne 2 -a $# -ne 3 ]; then
    echo "$0 <error prone version> <error prone javac version> [checkerframework version]" >&2
    exit 1;
fi
if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Missing environment variables. Did you run build/envsetup.sh and lunch?" >&2
    exit 1
fi
EP_VERSION="$1"
JAVAC_VERSION="$2"
# checkerframework
CF_VERSION="$3"
JAR_REPO="https://oss.sonatype.org/service/local/repositories/releases/content/com/google/errorprone"
EP_JAR_URL="${JAR_REPO}/error_prone_core/${EP_VERSION}/error_prone_core-${EP_VERSION}-with-dependencies.jar"
EP_ANNO_JAR_URL="${JAR_REPO}/error_prone_annotations/${EP_VERSION}/error_prone_annotations-${EP_VERSION}.jar"
JAVAC_JAR_URL="${JAR_REPO}/javac/${JAVAC_VERSION}/javac-${JAVAC_VERSION}.jar"
JAVAC_SOURCES_JAR_URL="${JAR_REPO}/javac/${JAVAC_VERSION}/javac-${JAVAC_VERSION}-sources.jar"
CF_DATAFLOW_JAR_URL="http://repo1.maven.org/maven2/org/checkerframework/dataflow/${CF_VERSION}/dataflow-${CF_VERSION}.jar"
CF_DATAFLOW_SOURCES_JAR_URL="http://repo1.maven.org/maven2/org/checkerframework/dataflow/${CF_VERSION}/dataflow-${CF_VERSION}-sources.jar"
CF_JAVACUTIL_JAR_URL="http://repo1.maven.org/maven2/org/checkerframework/javacutil/${CF_VERSION}/javacutil-${CF_VERSION}.jar"
CF_JAVACUTIL_SOURCES_JAR_URL="http://repo1.maven.org/maven2/org/checkerframework/javacutil/${CF_VERSION}/javacutil-${CF_VERSION}-sources.jar"
TOOLS_DIR=$(dirname $0)

function update_jar {
    typeset VERSION="$1" JAR_URL="$2" DIR="$3" JAR_FILE="$4"
    typeset JAR_URL_PREFIX=$(dirname $(dirname ${JAR_URL}))

    # Update the version and binary JAR URL.
    perl -pi -e "s|version: .*|version: \"${VERSION}\"|; s|\"${JAR_URL_PREFIX}.*\"|\"${JAR_URL}\"|" "$DIR/METADATA"

    # Update the last upgrade date
    perl -pi -e "s|last_upgrade_date.*|last_upgrade_date { year: $(date +%Y) month: $(date +%-m) day: $(date +%-d)}|" "$DIR/METADATA"

    # Get the JAR.
    wget ${JAR_URL} -O ${DIR}/$(basename ${JAR_URL})
    wget ${JAR_URL}.sha1 -O ${DIR}/$(basename ${JAR_URL}).sha1
    wget ${JAR_URL}.asc -O ${DIR}/$(basename ${JAR_URL}).asc
}

rm -f error_prone/*.jar*
rm -f javac/*.jar*

update_jar "${EP_VERSION}" "${EP_JAR_URL}" "${TOOLS_DIR}/error_prone"
update_jar "${EP_VERSION}" "${EP_ANNO_JAR_URL}" "${TOOLS_DIR}/error_prone"
update_jar "${JAVAC_VERSION}" "${JAVAC_SOURCES_JAR_URL}" "${TOOLS_DIR}/javac"
update_jar "${JAVAC_VERSION}" "${JAVAC_JAR_URL}" "${TOOLS_DIR}/javac"

# Update the versions for soong
perl -pi -e "\
    s|\"(external/error_prone/javac/javac).*\"|\"\\1-${JAVAC_VERSION}.jar\"|;\
    s|\"(external/error_prone/error_prone/error_prone_core).*\"|\"\\1-${EP_VERSION}-with-dependencies.jar\"|;\
    s|\"(external/error_prone/error_prone/error_prone_annotations).*\"|\"\\1-${EP_VERSION}.jar\"|;\
" "$TOOLS_DIR/soong/error_prone.go"

if [ "${CF_VERSION}" != '' ]; then
  rm -f checkerframework/*.jar*
  update_jar "${CF_VERSION}" "${CF_DATAFLOW_JAR_URL}" "${TOOLS_DIR}/checkerframework"
  update_jar "${CF_VERSION}" "${CF_DATAFLOW_SOURCES_JAR_URL}" "${TOOLS_DIR}/checkerframework"
  update_jar "${CF_VERSION}" "${CF_JAVACUTIL_JAR_URL}" "${TOOLS_DIR}/checkerframework"
  update_jar "${CF_VERSION}" "${CF_JAVACUTIL_SOURCES_JAR_URL}" "${TOOLS_DIR}/checkerframework"
  perl -pi -e "\
    s|\"(external/error_prone/checkerframework/dataflow).*\"|\"\\1-${CF_VERSION}.jar\"|;\
    s|\"(external/error_prone/checkerframework/javacutil).*\"|\"\\1-${CF_VERSION}.jar\"|;\
  " "$TOOLS_DIR/soong/error_prone.go"
fi
