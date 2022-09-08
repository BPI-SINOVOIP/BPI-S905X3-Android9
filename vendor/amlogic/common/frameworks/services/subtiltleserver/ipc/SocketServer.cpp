// currently, we use android thread
// TODO: impl with std lib, make it portable

#define LOG_NDEBUG 0

#define LOG_TAG "SubSocketServer"
#include <fcntl.h>
#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>


#include "DataSource.h"
#include "Segment.h"

#include "SocketServer.h"

/**
    payload is:
        startFlag   : 4bytes
        sessionID   : 4Bytes (TBD)
        magic       : 4bytes (for double confirm)
        payload Type: defined in PayloadType_t
        payload size: 4Bytes (indicate the size of payload, size is total_send_bytes - 4*5)
                      exclude this and above 4 items.
        payload data: TO BE PARSED data
*/

/**
    payload is:
        startFlag   : 4bytes
        sessionID   : 4Bytes (TBD)
        magic       : 4bytes (for double confirm)
        payload Type: defined in PayloadType_t
        payload size: 4Bytes (indicate the size of payload, size is total_send_bytes - 4*5)
                      exclude this and above 4 items.
        payload data: TO BE PARSED data
*/

static inline void dump(const char *buf, int size) {
    char str[64] = {0};

    for (int i=0; i<size; i++) {
        char chars[6] = {0};
        sprintf(chars, "%02x ", buf[i]);
        strcat(str, chars);
        if (i%8 == 7) {
            ALOGD("%s", str);
            str[0] = str[1] = 0;
        }
    }
    ALOGD("%s", str);
}


static std::mutex _g_inst_mutex;
SubSocketServer::SubSocketServer() : mExitRequested(false) {
    ALOGD("%s ?", __func__);
}

SubSocketServer::~SubSocketServer() {
    ALOGD("%s", __func__);
    mExitRequested = true;
    mThread.join();
    mDispatchThread.join();
}

SubSocketServer* SubSocketServer::mInstance = nullptr;

SubSocketServer* SubSocketServer::GetInstance() {
    ALOGD("%s", __func__);

    std::unique_lock<std::mutex> autolock(_g_inst_mutex);
    if (mInstance == nullptr) {
        mInstance = new SubSocketServer();
    }

    return mInstance;
}

int SubSocketServer::serve() {
    ALOGD("%s", __func__);

    mThread = std::thread(&SubSocketServer::__threadLoop, this);
    return 0;
}

void SubSocketServer::__threadLoop() {
    while (!mExitRequested && threadLoop());
}

bool SubSocketServer::threadLoop() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int flag = 1;

    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) return true;

    if ((setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) < 0) {
        ALOGE("setsockopt failed.\n");
        close(sockFd);
        return true;
    }

    if (bind(sockFd, (struct sockaddr *)&addr,sizeof(addr)) == -1) {
        ALOGE("bind fail. error=%d, err:%s\n", errno, strerror(errno));
        close(sockFd);
        return true;
    }

    if (listen(sockFd, QUEUE_SIZE) == -1) {
        ALOGE("listen fail.error=%d, err:%s\n", errno, strerror(errno));
        close(sockFd);
        return true;
    }

    ALOGV("[startServerThread] listen success.\n");
    while (!mExitRequested) {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        int connFd = accept(sockFd, (struct sockaddr*)&client_addr, &length);

        if (connFd < 0) {
            ALOGE("client connect fail.error=%d, err:%s\n", errno, strerror(errno));
            close(sockFd);
            return true;
        }

        ALOGV("new client accepted connFd=%d.\n", connFd);

        // TODO: in a new thread?
        if (mClientThreads.size() >= QUEUE_SIZE) {
            mClientThreads.pop_front();
        }
        std::shared_ptr<std::thread> pthread = std::shared_ptr<std::thread>(
                new std::thread(&SubSocketServer::clientConnected, this, connFd));
        pthread->detach();
        mClientThreads.push_back(pthread);
    }

    ALOGV("closed.\n");
    close(sockFd);
    return true;
}

// TODO: rename
// TODO: check how to exit
int SubSocketServer::clientConnected(int sockfd) {
    char recvBuf[1024] = {0};
    bool pendingByData = false;
    bool childConnected = false;
    IpcPackageHeader curHeader;

    rbuf_handle_t bufferHandle = ringbuffer_create(512*1024, "socket_buffer");

    ALOGV("start process chient message connFd=%d.\n", sockfd);
    while (!mExitRequested) {

        if (mClients.size() <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (childConnected) {
                // no need now! client source already exited.
                ringbuffer_free(bufferHandle);
                close(sockfd);
                return 0;
            } else {
                // no consumer attached, avoid lost message, wait here
                continue;
            }
        }
        childConnected = true;
        int retLen = recv(sockfd, recvBuf, sizeof(recvBuf), 0);

        if (errno != 0 && errno != EACCES) {
            //ALOGD("error no:%d %s", errno, strerror(errno));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        //ALOGV("[child_connect]recv begin... ret=%d\n", retLen);
        if (retLen == 0) continue; // TODO: check why
        if (retLen < 0) {
            ALOGE("child recv fail, retLen: %d\n", retLen);
            ringbuffer_free(bufferHandle);
            close(sockfd);
            return 0;
        }

        ringbuffer_write(bufferHandle, recvBuf, retLen, RBUF_MODE_BLOCK);

        // check packages, we handle here, for better package handle
        // TODO: wrap in a function:
        // check header
        //ALOGD("current %d: session:%x magic:%x subType:%x size:%x", pendingByData,
        //    curHeader.sessionId, curHeader.magicWord, curHeader.pkgType, curHeader.dataSize);

        if (!pendingByData) {
            // parse header.
            if (ringbuffer_read_avail(bufferHandle) < sizeof(IpcPackageHeader)) {
                continue;
            }
            //ALOGD("Got enough header bytes!");
            char buffer[sizeof(IpcPackageHeader)];
            // Check sync word.
            ringbuffer_read(bufferHandle, buffer, 4, RBUF_MODE_BLOCK);
            if (peekAsSocketWord(buffer) != START_FLAG) {
                ALOGD("Wrong Sync header found! %x", peekAsSocketWord(buffer));
                continue;
            }

            ringbuffer_read(bufferHandle, buffer, sizeof(IpcPackageHeader)-4, RBUF_MODE_BLOCK);
            curHeader.sessionId = peekAsSocketWord(buffer);
            curHeader.magicWord    = peekAsSocketWord(buffer+4); // need check magic or not??
            curHeader.dataSize = peekAsSocketWord(buffer+8);
            curHeader.pkgType  = peekAsSocketWord(buffer+12);
           // ALOGD("data: session:%x magic:%x subType:%x size:%x",
           //     curHeader.sessionId, curHeader.magicWord, curHeader.pkgType, curHeader.dataSize);

            if (eTypeSubtitleExitServ == curHeader.pkgType || 'XEDC' == curHeader.pkgType) {
                ALOGD("exit requested!");
                ringbuffer_free(bufferHandle);
                close(sockfd);
                return -1;
            }
            pendingByData = true; // means, already settup curHeader
        }

        if (pendingByData) {
            // enough data, process it.
            if (ringbuffer_read_avail(bufferHandle) >= curHeader.dataSize) {
                char *payloads = (char *) malloc(curHeader.dataSize +4);
                memcpy(payloads, &curHeader.pkgType, 4); // fill package type
                ringbuffer_read(bufferHandle, payloads+4, curHeader.dataSize, RBUF_MODE_BLOCK);
                {  // notify listener
                    std::lock_guard<std::mutex> guard(mLock);
                     DataListener *listener = mClients.front();
                    if (listener != nullptr) {
                        if (listener->onData(payloads, curHeader.dataSize+4) < 0) {
                            // if returen -1, means client exit, this request clientConnected exit!
                            ringbuffer_free(bufferHandle);
                            free(payloads);
                            close(sockfd);
                            return -1;
                        }
                    }
                }
                free(payloads);
                pendingByData = false;
            }
        }


    }

    ringbuffer_free(bufferHandle);
    close(sockfd);
    return 0;
}


