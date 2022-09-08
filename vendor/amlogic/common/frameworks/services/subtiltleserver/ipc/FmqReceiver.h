#pragma once

#include <utils/Log.h>
#include <utils/Thread.h>
#include <mutex>
#include <thread>
#include<vector>

#include "DataSource.h"

#include "IpcDataTypes.h"
#include "ringbuffer.h"
#include "FmqReader.h"

static inline uint32_t __min(uint32_t a, uint32_t b) {
    return (((a) < (b)) ? (a) : (b));
}
class FmqReceiver {
public:
    FmqReceiver() = delete;
    FmqReceiver(std::unique_ptr<FmqReader> reader);
    ~FmqReceiver();

    bool registClient(std::shared_ptr<DataListener> client) {
        std::lock_guard<std::mutex> guard(mLock);
        mClients.push_back(client);
        ALOGD("registClient: %p size=%d", client.get(), mClients.size());
        return true;
    }

    bool unregistClient(std::shared_ptr<DataListener> client) {
        // obviously, BUG here! impl later, support multi-client.
        // TODO: revise the whole mClient, if we want to support multi subtitle
        std::lock_guard<std::mutex> guard(mLock);

        std::vector<std::shared_ptr<DataListener>> &vecs = mClients;
        if (vecs.size() > 0) {
            for (auto it = vecs.cbegin(); it != vecs.cend(); it++) {
                if ((*it).get() == client.get()) {
                    vecs.erase(it);
                    break;
                }
            }

            //GetInstance()->mClients.pop_back();
            ALOGD("unregistClient: %p size=%d", client.get(), mClients.size());
        }
        return true;
    }

    void dump(int fd, const char *prefix);

private:
    bool readLoop();
    bool mStop;
    std::unique_ptr<FmqReader> mReader;
    std::thread mReaderThread;


    std::mutex mLock;
    std::vector<std::shared_ptr<DataListener>> mClients; //TODO: for clients
};
