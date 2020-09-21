#ifndef DNS_RESPONDER_CLIENT_H
#define DNS_RESPONDER_CLIENT_H

#include <cutils/sockets.h>

#include <private/android_filesystem_config.h>
#include <utils/StrongPointer.h>

#include "android/net/INetd.h"
#include "binder/IServiceManager.h"
#include "NetdClient.h"
#include "dns_responder.h"
#include "resolv_params.h"

class DnsResponderClient {
public:
    struct Mapping {
        std::string host;
        std::string entry;
        std::string ip4;
        std::string ip6;
    };

    virtual ~DnsResponderClient() = default;

    static void SetupMappings(unsigned num_hosts, const std::vector<std::string>& domains,
            std::vector<Mapping>* mappings);

    bool SetResolversForNetwork(const std::vector<std::string>& servers,
            const std::vector<std::string>& domains, const std::vector<int>& params);

    bool SetResolversForNetwork(const std::vector<std::string>& servers,
            const std::vector<std::string>& searchDomains,
            const std::string& params);

    bool SetResolversWithTls(const std::vector<std::string>& servers,
            const std::vector<std::string>& searchDomains,
            const std::vector<int>& params,
            const std::string& name,
            const std::vector<std::string>& fingerprints) {
        // Pass servers as both network-assigned and TLS servers.  Tests can
        // determine on which server and by which protocol queries arrived.
        return SetResolversWithTls(servers, searchDomains, params,
                                   servers, name, fingerprints);
    }

    bool SetResolversWithTls(const std::vector<std::string>& servers,
            const std::vector<std::string>& searchDomains,
            const std::vector<int>& params,
            const std::vector<std::string>& tlsServers,
            const std::string& name,
            const std::vector<std::string>& fingerprints);

    static void SetupDNSServers(unsigned num_servers, const std::vector<Mapping>& mappings,
            std::vector<std::unique_ptr<test::DNSResponder>>* dns,
            std::vector<std::string>* servers);

    static void ShutdownDNSServers(std::vector<std::unique_ptr<test::DNSResponder>>* dns);

    int SetupOemNetwork();

    void TearDownOemNetwork(int oemNetId);

    virtual void SetUp();

    virtual void TearDown();

public:
    android::sp<android::net::INetd> mNetdSrv = nullptr;
    int mOemNetId = -1;
};

#endif  // DNS_RESPONDER_CLIENT_H
