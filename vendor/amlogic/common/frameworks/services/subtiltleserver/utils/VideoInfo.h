#pragma once

class VideoInfo {
public:
    virtual ~VideoInfo() {}
    virtual int getVideoWidth() = 0;
    virtual int getVideoHeight() = 0;
    virtual int getVideoFormat() = 0;


    static VideoInfo *Instance();
private:
    static VideoInfo *mInstance;
};

