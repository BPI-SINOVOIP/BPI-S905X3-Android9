#ifndef _ION_IF_H_
#define _ION_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <utils/threads.h>

namespace android {
struct IONBufferNode {
    int share_fd;
    int ion_handle;
    uint8_t* vaddr;
    size_t size;
    bool IsUsed;
};
#define BUFFER_NUM 6
class IONInterface {
private:
    static int mIONDevice_fd;
    IONBufferNode mPicBuffers[BUFFER_NUM];
    static IONInterface* mIONInstance;
    static Mutex IonLock;
    static int mCount;
private:
    IONInterface();
public:
    ~IONInterface();
    static IONInterface* get_instance();
    uint8_t* alloc_buffer(size_t size, int* share_fd);
    void free_buffer(int share_fd);
    void put_instance();
    int release_node(IONBufferNode* pBuffer);
};
}
#endif
