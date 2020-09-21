/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// const values shared by both kernel program and userspace bpfloader

#define BPF_CGROUP_INGRESS_PROG_NAME "cgroup_ingress_prog"
#define BPF_CGROUP_EGRESS_PROG_NAME "cgroup_egress_prog"
#define XT_BPF_INGRESS_PROG_NAME "xt_ingress_prog"
#define XT_BPF_EGRESS_PROG_NAME "xt_egress_prog"

#define COOKIE_TAG_MAP 0xbfceaaffffffffff
#define UID_COUNTERSET_MAP 0xbfdceeafffffffff
#define APP_UID_STATS_MAP 0xbfa1daafffffffff
#define UID_STATS_MAP 0xbfdaafffffffffff
#define TAG_STATS_MAP 0xbfaaafffffffffff
#define IFACE_STATS_MAP 0xbf1faceaafffffff
#define DOZABLE_UID_MAP 0Xbfd0ab1e1dafffff
#define STANDBY_UID_MAP 0Xbfadb1daffffffff
#define POWERSAVE_UID_MAP 0Xbf0eae1dafffffff
// These are also defined in NetdConstants.h, but we want to minimize the number of headers
// included by the BPF kernel program.
// TODO: refactor the the following constant into a seperate file so
// NetdConstants.h can also include it from there.
#define MIN_SYSTEM_UID 0
#define MAX_SYSTEM_UID 9999
#define UID_MAP_ENABLED UINT32_MAX
