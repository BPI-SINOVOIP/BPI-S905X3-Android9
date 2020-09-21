#!/bin/bash

curl -H "Content-Type: application/json" -X POST -d '{"devices": [{"serial": "123", "product": "myfish", "serial": "myserial", "status": 1}], "hostname": "hc1"}' https://vtslab-schedule-prod.appspot.com/_ah/api/host_info/v1/set

curl -H "Content-Type: application/json" -X POST -d '{"manifest_branch": "master", "build_id": "a2131", "build_target": "sailfish", "build_type": "userdebug"}' https://vtslab-schedule-prod.appspot.com/_ah/api/build_info/v1/set

curl -H "Content-Type: application/json" -X POST -d '{"device": [{"index": 1}], "host_name": "hc1"}' https://vtslab-schedule-prod.appspot.com/_ah/api/host_info/v1/set

curl -H "Content-Type: application/json" -X POST -d '{"hostname": "vtslab-mtv43-2"}' https://vtslab-schedule-prod.appspot.com/_ah/api/job_queue/v1/get
