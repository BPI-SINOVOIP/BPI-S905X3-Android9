/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "dns_responder_client"
#include "dns_responder_client.h"

#include <android-base/stringprintf.h>
#include <utils/Log.h>

// TODO: make this dynamic and stop depending on implementation details.
#define TEST_OEM_NETWORK "oem29"
#define TEST_NETID 30

// TODO: move this somewhere shared.
static const char* ANDROID_DNS_MODE = "ANDROID_DNS_MODE";

using android::base::StringPrintf;

void DnsResponderClient::SetupMappings(unsigned num_hosts, const std::vector<std::string>& domains,
        std::vector<Mapping>* mappings) {
    mappings->resize(num_hosts * domains.size());
    auto mappings_it = mappings->begin();
    for (unsigned i = 0 ; i < num_hosts ; ++i) {
        for (const auto& domain : domains) {
            mappings_it->host = StringPrintf("host%u", i);
            mappings_it->entry = StringPrintf("%s.%s.", mappings_it->host.c_str(),
                    domain.c_str());
            mappings_it->ip4 = StringPrintf("192.0.2.%u", i%253 + 1);
            mappings_it->ip6 = StringPrintf("2001:db8::%x", i%65534 + 1);
            ++mappings_it;
        }
    }
}

bool DnsResponderClient::SetResolversForNetwork(const std::vector<std::string>& servers,
        const std::vector<std::string>& domains, const std::vector<int>& params) {
    const auto rv = mNetdSrv->setResolverConfiguration(TEST_NETID, servers, domains, params,
            "", {}, {});
    return rv.isOk();
}

bool DnsResponderClient::SetResolversWithTls(const std::vector<std::string>& servers,
        const std::vector<std::string>& domains, const std::vector<int>& params,
        const std::vector<std::string>& tlsServers,
        const std::string& name, const std::vector<std::string>& fingerprints) {
    const auto rv = mNetdSrv->setResolverConfiguration(TEST_NETID, servers, domains, params,
            name, tlsServers, fingerprints);
    if (!rv.isOk()) ALOGI("SetResolversWithTls() -> %s", rv.toString8().c_str());
    return rv.isOk();
}

void DnsResponderClient::SetupDNSServers(unsigned num_servers, const std::vector<Mapping>& mappings,
        std::vector<std::unique_ptr<test::DNSResponder>>* dns,
        std::vector<std::string>* servers) {
    const char* listen_srv = "53";
    dns->resize(num_servers);
    servers->resize(num_servers);
    for (unsigned i = 0 ; i < num_servers ; ++i) {
        auto& server = (*servers)[i];
        auto& d = (*dns)[i];
        server = StringPrintf("127.0.0.%u", i + 100);
        d = std::make_unique<test::DNSResponder>(server, listen_srv, 250,
                ns_rcode::ns_r_servfail, 1.0);
        for (const auto& mapping : mappings) {
            d->addMapping(mapping.entry.c_str(), ns_type::ns_t_a, mapping.ip4.c_str());
            d->addMapping(mapping.entry.c_str(), ns_type::ns_t_aaaa, mapping.ip6.c_str());
        }
        d->startServer();
    }
}

void DnsResponderClient::ShutdownDNSServers(std::vector<std::unique_ptr<test::DNSResponder>>* dns) {
    for (const auto& d : *dns) {
        d->stopServer();
    }
    dns->clear();
}

int DnsResponderClient::SetupOemNetwork() {
    mNetdSrv->networkDestroy(TEST_NETID);
    auto ret = mNetdSrv->networkCreatePhysical(TEST_NETID, "");
    if (!ret.isOk()) {
        fprintf(stderr, "Creating physical network %d failed, %s\n", TEST_NETID,
                ret.toString8().string());
        return -1;
    }
    int oemNetId = TEST_NETID;
    setNetworkForProcess(oemNetId);
    if ((unsigned) oemNetId != getNetworkForProcess()) {
        return -1;
    }
    return oemNetId;
}

void DnsResponderClient::TearDownOemNetwork(int oemNetId) {
    if (oemNetId != -1) {
        mNetdSrv->networkDestroy(oemNetId);
    }
}

void DnsResponderClient::SetUp() {
    // binder setup
    auto binder = android::defaultServiceManager()->getService(android::String16("netd"));
    mNetdSrv = android::interface_cast<android::net::INetd>(binder);

    // Ensure resolutions go via proxy.
    setenv(ANDROID_DNS_MODE, "", 1);
    mOemNetId = SetupOemNetwork();
}

void DnsResponderClient::TearDown() {
    TearDownOemNetwork(mOemNetId);
}
