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

/*
 * This h file together with bpf_kern.c is used for compiling the eBPF kernel
 * program. To generate the bpf_kern.o file manually, use the clang prebuilt in
 * this android tree to compile the files with --target=bpf options. For
 * example, in system/netd/ directory, execute the following command:
 * $: ANDROID_BASE_DIRECTORY/prebuilts/clang/host/linux-x86/clang-4691093/bin/clang  \
 *    -I ANDROID_BASE_DIRECTORY/bionic/libc/kernel/uapi/ \
 *    -I ANDROID_BASE_DIRECTORY/system/netd/bpfloader/ \
 *    -I ANDROID_BASE_DIRECTORY/bionic/libc/kernel/android/uapi/ \
 *    -I ANDROID_BASE_DIRECTORY/bionic/libc/include \
 *    -I ANDROID_BASE_DIRECTORY/system/netd/libbpf/include  \
 *    --target=bpf -O2 -c bpfloader/bpf_kern.c -o bpfloader/bpf_kern.o
 */

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <stdint.h>
#include "bpf/bpf_shared.h"

#define ELF_SEC(NAME) __attribute__((section(NAME), used))

struct uid_tag {
    uint32_t uid;
    uint32_t tag;
};

struct stats_key {
    uint32_t uid;
    uint32_t tag;
    uint32_t counterSet;
    uint32_t ifaceIndex;
};

struct stats_value {
    uint64_t rxPackets;
    uint64_t rxBytes;
    uint64_t txPackets;
    uint64_t txBytes;
};

/* helper functions called from eBPF programs written in C */
static void* (*find_map_entry)(uint64_t map, void* key) = (void*)BPF_FUNC_map_lookup_elem;
static int (*write_to_map_entry)(uint64_t map, void* key, void* value,
                                 uint64_t flags) = (void*)BPF_FUNC_map_update_elem;
static int (*delete_map_entry)(uint64_t map, void* key) = (void*)BPF_FUNC_map_delete_elem;
static uint64_t (*get_socket_cookie)(struct __sk_buff* skb) = (void*)BPF_FUNC_get_socket_cookie;
static uint32_t (*get_socket_uid)(struct __sk_buff* skb) = (void*)BPF_FUNC_get_socket_uid;
static int (*bpf_skb_load_bytes)(struct __sk_buff* skb, int off, void* to,
                                 int len) = (void*)BPF_FUNC_skb_load_bytes;
#define BPF_PASS 1
#define BPF_DROP 0
#define BPF_EGRESS 0
#define BPF_INGRESS 1

#define IP_PROTO_OFF offsetof(struct iphdr, protocol)
#define IPV6_PROTO_OFF offsetof(struct ipv6hdr, nexthdr)
#define IPPROTO_IHL_OFF 0
#define TCP_FLAG_OFF 13
#define RST_OFFSET 2

static __always_inline inline void bpf_update_stats(struct __sk_buff* skb, uint64_t map,
                                                    int direction, void *key) {
    struct stats_value* value;
    value = find_map_entry(map, key);
    if (!value) {
        struct stats_value newValue = {};
        write_to_map_entry(map, key, &newValue, BPF_NOEXIST);
        value = find_map_entry(map, key);
    }
    if (value) {
        if (direction == BPF_EGRESS) {
            __sync_fetch_and_add(&value->txPackets, 1);
            __sync_fetch_and_add(&value->txBytes, skb->len);
        } else if (direction == BPF_INGRESS) {
            __sync_fetch_and_add(&value->rxPackets, 1);
            __sync_fetch_and_add(&value->rxBytes, skb->len);
        }
    }
}

static inline int bpf_owner_match(struct __sk_buff* skb, uint32_t uid) {
    int offset = -1;
    int ret = 0;
    if (skb->protocol == ETH_P_IP) {
        offset = IP_PROTO_OFF;
        uint8_t proto, ihl;
        uint16_t flag;
        ret = bpf_skb_load_bytes(skb, offset, &proto, 1);
        if (!ret) {
            if (proto == IPPROTO_ESP) {
                return 1;
            } else if (proto == IPPROTO_TCP) {
                ret = bpf_skb_load_bytes(skb, IPPROTO_IHL_OFF, &ihl, 1);
                ihl = ihl & 0x0F;
                ret = bpf_skb_load_bytes(skb, ihl * 4 + TCP_FLAG_OFF, &flag, 1);
                if (ret == 0 && (flag >> RST_OFFSET & 1)) {
                    return BPF_PASS;
                }
            }
        }
    } else if (skb->protocol == ETH_P_IPV6) {
        offset = IPV6_PROTO_OFF;
        uint8_t proto;
        ret = bpf_skb_load_bytes(skb, offset, &proto, 1);
        if (!ret) {
            if (proto == IPPROTO_ESP) {
                return BPF_PASS;
            } else if (proto == IPPROTO_TCP) {
                uint16_t flag;
                ret = bpf_skb_load_bytes(skb, sizeof(struct ipv6hdr) + TCP_FLAG_OFF, &flag, 1);
                if (ret == 0 && (flag >> RST_OFFSET & 1)) {
                    return BPF_PASS;
                }
            }
        }
    }

    if ((uid <= MAX_SYSTEM_UID) && (uid >= MIN_SYSTEM_UID)) return BPF_PASS;

    // In each of these maps, the entry with key UID_MAP_ENABLED tells us whether that
    // map is enabled or not.
    // TODO: replace this with a map of size one that contains a config structure defined in
    // bpf_shared.h that can be written by userspace and read here.
    uint32_t mapSettingKey = UID_MAP_ENABLED;
    uint8_t* ownerMatch;
    uint8_t* mapEnabled = find_map_entry(DOZABLE_UID_MAP, &mapSettingKey);
    if (mapEnabled && *mapEnabled) {
        ownerMatch = find_map_entry(DOZABLE_UID_MAP, &uid);
        if (ownerMatch) return *ownerMatch;
        return BPF_DROP;
    }
    mapEnabled = find_map_entry(STANDBY_UID_MAP, &mapSettingKey);
    if (mapEnabled && *mapEnabled) {
        ownerMatch = find_map_entry(STANDBY_UID_MAP, &uid);
        if (ownerMatch) return *ownerMatch;
    }
    mapEnabled = find_map_entry(POWERSAVE_UID_MAP, &mapSettingKey);
    if (mapEnabled && *mapEnabled) {
        ownerMatch = find_map_entry(POWERSAVE_UID_MAP, &uid);
        if (ownerMatch) return *ownerMatch;
        return BPF_DROP;
    }
    return BPF_PASS;
}

static __always_inline inline int bpf_traffic_account(struct __sk_buff* skb, int direction) {
    uint32_t sock_uid = get_socket_uid(skb);
    int match = bpf_owner_match(skb, sock_uid);
    if ((direction == BPF_EGRESS) && (match == BPF_DROP)) {
        // If an outbound packet is going to be dropped, we do not count that
        // traffic.
        return match;
    }

    uint64_t cookie = get_socket_cookie(skb);
    struct uid_tag* utag = find_map_entry(COOKIE_TAG_MAP, &cookie);
    uint32_t uid, tag;
    if (utag) {
        uid = utag->uid;
        tag = utag->tag;
    } else {
        uid = sock_uid;
        tag = 0;
    }

    struct stats_key key = {.uid = uid, .tag = tag, .counterSet = 0, .ifaceIndex = skb->ifindex};

    uint8_t* counterSet = find_map_entry(UID_COUNTERSET_MAP, &uid);
    if (counterSet) key.counterSet = (uint32_t)*counterSet;

    if (tag) {
        bpf_update_stats(skb, TAG_STATS_MAP, direction, &key);
    }

    key.tag = 0;
    bpf_update_stats(skb, UID_STATS_MAP, direction, &key);
    bpf_update_stats(skb, APP_UID_STATS_MAP, direction, &uid);
    return match;
}
