#ifndef __SUBTITLE_USERDATA_AFD_H__
#define __SUBTITLE_USERDATA_AFD_H__

#include "sub_types2.h"
#include "ParserFactory.h"
#include "ParserEventNotifier.h"
#include <thread>


#define USERDATA_DEVICE_NUM 0 // userdata device number

class UserDataAfd /*: public Parser*/{
public:
    UserDataAfd();
    virtual ~UserDataAfd();
    //virtual void dump(int fd, const char *prefix);
    int start(ParserEventNotifier *notify);
    int stop();
    void run();

    int parse() {return -1;};
    void dump(int fd, const char *prefix) {return;};
    void notifyCallerAfdChange(int afd);
    static inline UserDataAfd *getCurrentInstance();

    int mNewAfdValue;


private:
    static UserDataAfd *sInstance;
    ParserEventNotifier *mNotifier;
    std::shared_ptr<std::thread> mThread;
};


#endif

