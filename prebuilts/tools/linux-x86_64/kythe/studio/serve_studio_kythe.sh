#!/bin/bash -e
# Takes kythe indexer output and processes it into servable format
# And brings up a webserver (port 8080) to inspect the data.
#
# This is not used for normal production runs as processing on a single
# machine is very slow (our index output is currently around 20GB).
#
# Instead the processing is done with a MapReduce job.

# Path to the kyth binaries.
KYTHE_ROOT="$(readlink -f prebuilts/tools/linux-x86_64/kythe)"

# Get the output path for the kythe artifacts.
OUT="$1"
OUT_ENTRIES="${OUT}/entries"
OUT_GRAPHSTORE="${OUT}/graphstore"
OUT_SERVING="${OUT}/serving"
if [ -z "${OUT}" ]; then
  echo Usage: $0 \<out_dir\>
  echo  e.g. $0 $HOME/studio_kythe
  echo
  echo $0 must be launched from the root of the studio branch.
  exit 1
fi

# if the graphstore has not been created, create it from the
# entries created using "build_studio_kythe.sh"
if [ ! -d "${OUT_GRAPHSTORE}" ]; then

  # delete all empty files as the write_entries tool is not happy with
  # empty files
  find "${OUT_ENTRIES}" -empty -type f -delete
  ENTRIES=$(find "${OUT_ENTRIES}" -name *.entries \
    -exec realpath {} \;)

  for ENTRY in ${ENTRIES}; do
    "${KYTHE_ROOT}/tools/write_entries" \
      --graphstore "leveldb:${OUT_GRAPHSTORE}" < "${ENTRY}"
  done;
fi

# If no serving table exists yet, create it from the graphstore.
if [ ! -d "${OUT_SERVING}" ]; then
  "${KYTHE_ROOT}/tools/write_tables" \
    --graphstore "${OUT_GRAPHSTORE}" \
    --out "${OUT_SERVING}"
fi

# Start the kythe webserver for the serving table.
"${KYTHE_ROOT}/tools/http_server" \
  --public_resources "${KYTHE_ROOT}/web/ui" \
  --listen localhost:8080 \
  --serving_table "${OUT_SERVING}"