#!/system/bin/sh
# Usage: spin_n_threads.sh <num_threads> [<nice>]
#        spin_n_threads.sh kill

TGID_FILE=/data/local/tmp/spin_n_threads_tgid.txt

spin_loop() {
  while :
  do
    NUM=$(expr 1 + 1)
  done
}

clean_up() {
  trap - SIGINT SIGTERM SIGKILL
  kill -- -$$
}

NUM_THREADS=1
if [ ! -z ${1} ]; then
  if [ ${1} == "kill" ]; then
    TGID=$(cat ${TGID_FILE})
    kill -- -${TGID}
    exit 0
  fi

  if [ ${1} -gt 1 ]; then
    NUM_THREADS=${1}
  else
    exit 0
  fi
fi

if [ ! -z ${2} ]; then
 renice -n ${2} -p $$
fi

# Register cleanup on trap
trap clean_up SIGINT SIGTERM SIGKILL
for i in $(seq 1 $NUM_THREADS ); do
  spin_loop &
done

echo $$ > ${TGID_FILE}
