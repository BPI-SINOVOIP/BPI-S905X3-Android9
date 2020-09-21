#/bin/bash
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

SCRIPT_NAME=$(basename $0)
usage()
{
  echo "Usage: $SCRIPT_NAME <task_name> [options]"
  echo ""
  echo "Options:"
  echo "  -p PASSWORD         password for hosts."
  echo "  -H PATH             path to a .py file contains hosts information"
  echo "  -i PATH             path to a .py file contains ip address information of certified machines"
  exit 1
}

FABRIC_EXISTS=$(pip show fabric)
if [ -z "$FABRIC_EXISTS" ]; then
  sudo pip install fabric
fi

TASK=$1
if [[ ${TASK:0:1} == "-" ]]; then
  usage
fi

shift
PASSWORD=""
HOSTS_PATH="hosts.py"
IPADDRESSES_PATH="ip_addresses.py"

while getopts ":p:H:i:" opt; do
  case $opt in
    p)
      PASSWORD=$OPTARG
      ;;
    H)
      HOSTS_PATH=$OPTARG
      ;;
    i)
      IPADDRESSES_PATH=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG"
      usage
      ;;
    :)
      echo "Option -$OPTARG requires an argument."
      usage
      ;;
  esac
done

if [ "$TASK" == "SetupIptables" ]; then
  fab SetPassword:$PASSWORD GetHosts:$HOSTS_PATH $TASK:$IPADDRESSES_PATH
else
  fab SetPassword:$PASSWORD GetHosts:$HOSTS_PATH $TASK
fi
