#pragma once

enum {
    eTypeSubtitleTotal      = 'STOT',
    eTypeSubtitleStartPts   = 'SPTS',
    eTypeSubtitleRenderTime = 'SRDT',
    eTypeSubtitleType       = 'STYP',

    eTypeSubtitleTypeString = 'TPSR',
    eTypeSubtitleLangString = 'LGSR',
    eTypeSubtitleData       = 'PLDT',

    eTypeSubtitleResetServ  = 'CDRT',
    eTypeSubtitleExitServ   = 'CDEX',
};


struct IpcPackageHeader {
    int syncWord;
    int sessionId;
    int magicWord;
    int dataSize;
    int pkgType;
};

const static unsigned int START_FLAG = 0xF0D0C0B1;
const static unsigned int MAGIC_FLAG = 0xCFFFFFFB;

static inline unsigned int peekAsSocketWord(const char *buffer) {
    return buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
}

