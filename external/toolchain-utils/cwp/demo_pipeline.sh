#!/bin/bash

# These should be on the local filesystem. We'll be hitting it hard.
DATA_DIR=/usr/local/google/home/${USER}/
SYMBOL_CACHE=${DATA_DIR}cache/
REPORT_DIR=${DATA_DIR}reports/
SAMPLE_DIR=${DATA_DIR}samples/
RECORD_FILE=/tmp/profiles.rio
COLUMN_FILE=/tmp/profiles.cio
mkdir -p ${SYMBOL_CACHE}
mkdir -p ${REPORT_DIR}
mkdir -p ${SAMPLE_DIR}

# Directory that has the scripts app_engine_pull.py and symbolizer.py
INTERPRETER_DIR=/google/src/files/p2/head/depot2/gcctools/chromeos/v14/cwp/interpreter/
V14_DIR=$(dirname $(dirname ${INTERPRETER_DIR}))

PYTHONPATH=$PYTHONPATH:$V14_DIR

# Profile util binary
PROFILE_UTIL_BINARY=/home/mrdmnd/${USER}-profiledb/google3/blaze-bin/perftools/gwp/chromeos/profile_util

# mr-convert binary
MR_CONVERT_BINARY=/home/build/static/projects/dremel/mr-convert

CNS_LOC=/cns/ag-d/home/${USER}/profiledb/

# Protofile location
PROTO_LOC=${CNS_LOC}cwp_profile_db_entry.proto

echo "0. Cleaning up old data."
rm /tmp/profiles.*
rm ${REPORT_DIR}*
rm ${SAMPLE_DIR}*


echo "Starting CWP Pipeline..."
echo "1. Pulling samples to local filesystem from server."
echo "   For demo purposes, UN=${USER}@google.com, PW=xjpbmshkzefutlrm"
python ${INTERPRETER_DIR}app_engine_pull.py --output_dir=${SAMPLE_DIR}
echo "2. Symbolizing samples to perf reports. Hold on..."

python ${INTERPRETER_DIR}symbolizer.py --in=${SAMPLE_DIR} --out=${REPORT_DIR} --cache=${SYMBOL_CACHE}
echo "3. Loading reports into RecordIO format..."
# Will need to make append_dir more clever / incremental
${PROFILE_UTIL_BINARY} --record=${RECORD_FILE} --append_dir=${REPORT_DIR}
echo "Done."
echo "4. Converting records to columnio."
${MR_CONVERT_BINARY} --mapreduce_input_map=recordio:${RECORD_FILE} --mapreduce_output_map=${COLUMN_FILE}@1 --columnio_mroutput_message_type=CwpProfileDbEntry --columnio_mroutput_protofiles=${PROTO_LOC}
echo "5. Uploading columnio to colossus."
fileutil cp -f /tmp/profiles.cio-* ${CNS_LOC}
echo "6. Let's try some dremel queries..."
echo "   dremel> define table t /cns/ag-d/home/${USER}/profiledb/profiles.cio-*"
echo "   Like, say, dremel> select sum(frames.count) as count, left(frames.function_name, 80) as name from t group by name order by count desc limit 25;"

