#!/bin/bash -e

# Bazel build target for running kythe extractor as an extra_action
# to create kythe index files as a side effect of running the build.
EAL=//prebuilts/tools/linux-x86_64/kythe/extractors:extract_kindex

# Path to the kyth binaries.
KYTHE_ROOT="$(readlink -f prebuilts/tools/linux-x86_64/kythe)"

# Get the output path for the kythe artifacts.
OUT="$1"
if [ -z "${OUT}" ]; then
  echo Usage: $0 \<out_dir\> [gcs_bucket]
  echo  e.g. $0 $HOME/studio_kythe
  echo
  echo $0 must be launched from the root of the studio branch.
  exit 1
fi

if [ -z "${JDK_18_x64}" ]; then
  echo $0 requires the JDK_18_x64 env variable to be defined.
  exit 1
fi

OUT_ENTRIES="${OUT}/entries"
mkdir -p "${OUT_ENTRIES}"

TARGETS="$(cat tools/base/bazel/targets)"

# ensure no stale .kindex file exist
tools/base/bazel/bazel clean

# Build all targets and run the kythe extractor via extra_actions.
# ignore failures in the build as Kythe will do statistical analysis
# on build and produce indexes for builds with 95% coverage and up.
tools/base/bazel/bazel build --keep_going \
  --experimental_action_listener=${EAL} -- ${TARGETS} || true

# Find all generated kythe index files.
KINDEXES=$(find bazel-out/local-fastbuild/extra_actions/ \
  -name *.kindex -exec realpath {} \;)

# For each kythe index file run the java index to generate kythe
# entries.
# Ignore failures from analysis (|| true) as Kythe will do statistical analysis
# on the entries and produce indexes for builds with 95% coverage and up.
cd "${OUT_ENTRIES}"
for KINDEX in ${KINDEXES}; do
  ENTRIES="$(basename "${KINDEX}").entries"
  if [ ! -f "${ENTRIES}" ]; then
    "${JDK_18_x64}/bin/java" -jar \
    "${KYTHE_ROOT}/indexers/java_indexer.jar" \
      "${KINDEX}" > "${ENTRIES}" || true
  fi
done;

GSBUCKET="$2"
# allow buildbot to specify gsutil script to use.
GSUTIL="${GSUTIL:-gsutil}"

if [ -n "${GSBUCKET}" ]; then
  TIMESTAMP=$(date +'%s')
  "${GSUTIL}" -m cp "${OUT_ENTRIES}/*" "${GSBUCKET}/${TIMESTAMP}/"
  LATEST_FILE="$(mktemp)"
  echo "str_var <name:'kythe_index_version' value:'${TIMESTAMP}'>">"${LATEST_FILE}"
  "${GSUTIL}" cp "${LATEST_FILE}" "${GSBUCKET}/latest.txt"
  rm "${LATEST_FILE}"
fi