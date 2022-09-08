#ifndef __SUBTITLE_FILESOURCE_H__
#define __SUBTITLE_FILESOURCE_H__


#include <string>

#include "DataSource.h"

#define LINE_LEN   1024*5


class FileSource : public DataSource {

public:
    FileSource() {mFd = -1;}
    FileSource(int fd);
    virtual ~FileSource();

    size_t totalSize();
    // on need for this
    //virtual int onData(const char*buffer, int len) { return 0;}

    bool start();
    bool stop();
    size_t lseek(int offSet, int whence);
    bool isFileAvailble();
    virtual size_t availableDataSize();
    virtual size_t read(void *buffer, size_t size);

    virtual int onData(const char*buffer, int len) {
        return -1;
    }

    virtual void dump(int fd, const char *prefix);

private:
    int mFd;

};

#endif
