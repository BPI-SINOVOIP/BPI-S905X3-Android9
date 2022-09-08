#define LOG_TAG "AssParser"
#include <unistd.h>
#include <fcntl.h>

#include<iostream>
#include <sstream>
#include <string>
#include <list>
#include <thread>
#include <algorithm>
#include <functional>

#include <utils/Log.h>

#include "sub_types.h"
#include "AssParser.h"
#include "ParserFactory.h"

#include "streamUtils.h"
#define LOGI ALOGI
#define LOGD ALOGD
#define LOGE ALOGE
#define MIN_HEADER_DATA_SIZE 24


//TODO: move to utils directory

/**
 *  return ascii printed literal value
 */

/*
    retrieve the subtitle content from the decoded buffer.
    If is the origin ass event buffer, also fill the time stamp info.

    normal ASS type:
    Format: Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text
    Dialogue: 0,0:00:00.26,0:00:01.89,Default,,0000,0000,0000,,Don't move

    ssa type and example:
    Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
    Dialogue: Marked=0,0:00:00.26,0:00:01.89,Default,NTP,0000,0000,0000,!Effect,Don't move

    not normal, typically, built in:
    36,0,Default,,0000,0000,0000,,They'll be back from Johns Hopkins...


   here, we only care about the last subtitle content. other feature, like effect and margin, not support

*/
static inline int __getAssSpu(uint8_t*spuBuf, uint32_t length, std::shared_ptr<AML_SPUVAR> spu) {
    const int ASS_EVENT_SECTIONS = 9;
    // *,0,Default,,0000,0000,0000,,      "," num:8
    const int BUILTIN_ASS_EVENT_SECTIONS = 8;
    std::stringstream ss;
    std::string str;
    std::vector<std::string> items; // store the event sections in vector

    ss << spuBuf;

    // if has dialog, is normal, or is not.
    bool isNormal = strncmp((const char *)spuBuf, "Dialogue:", 9) == 0;

    int i = 0;
    while (getline(ss, str, ',')) {
        i++;
        items.push_back(str);

        // keep the last subtitle content not split with ',', the content may has it.
        if (isNormal && i == ASS_EVENT_SECTIONS) {
             break;
        } else if (!isNormal && i == BUILTIN_ASS_EVENT_SECTIONS) {
            break;
        }
    }

    // obviously, invalid dialog from the spec.
    if (i < BUILTIN_ASS_EVENT_SECTIONS) return -1;

    if (isNormal) {
        uint32_t hour, min, sec, ms;
        int count = 0;

        // 1 is start time
        count = sscanf(items[1].c_str(), "%d:%d:%d.%d", &hour, &min, &sec, &ms);
        if (count == 4) {
            spu->pts = (hour * 60 * 60 + min * 60 + sec) * 1000 + ms * 10;
            spu->pts *= 90;
        }

        // 2 is end time.
        count = sscanf(items[2].c_str(), "%d:%d:%d.%d", &hour, &min, &sec, &ms);
        if (count == 4) {
            spu->m_delay = (hour * 60 * 60 + min * 60 + sec) * 1000 + ms * 10;
            spu->m_delay *= 90;
        }
    }

    // get the subtitle content. here we do not need other effect and margin data.
    std::string tempStr = ss.str();
    //fist check the "{\" in the content. If don't find ,just use getline to get the content.
    int nPos = tempStr.find("{\\");
    if (-1 == nPos) {
        getline(ss, str);
    } else {
        str = tempStr.substr(nPos, tempStr.length());
    }
    ALOGV("[%s]-subtitle=%s", ss.str().c_str(), str.c_str());

    // currently not support style control code rendering
    // discard the unsupported {} Style Override control codes
    std::size_t start, end;
    while ((start = str.find("{\\")) != std::string::npos) {
        end = str.find('}', start);
        if (end != std::string::npos) {
            str.erase(start, end-start+1);
        } else {
            break;
        }
    }

    // replacer "\n"
    while ((start = str.find("\\n")) != std::string::npos
            || (start = str.find("\\N")) != std::string::npos) {
        std::string newline = "\n";
        str.replace(start, 2, newline);
    }

    strcpy((char *)spu->spu_data, str.c_str());
    return 0;
}

AssParser::AssParser(std::shared_ptr<DataSource> source) {
    mDataSource = source;
    mParseType = TYPE_SUBTITLE_SSA;
    mRestLen = 0;
    mRestbuf = nullptr;
}

int AssParser::getSpu(std::shared_ptr<AML_SPUVAR> spu) {
    int ret = -1;
    char *spuBuf = NULL;
    uint32_t currentLen, currentPts, currentType, durationPts;

    if (mState == SUB_INIT) {
        mState = SUB_PLAYING;
    } else if (mState == SUB_STOP) {
        ALOGD(" subtitle_status == SUB_STOP \n\n");
        return 0;
    }

    int dataSize = mDataSource->availableDataSize();
    if (dataSize <= 0) {
        return -1;
    } else {
        dataSize += mRestLen;
        currentType = 0;
        spuBuf = new char[dataSize]();
    }


    // Got enough data (MIN_HEADER_DATA_SIZE bytes), then start parse
    while (dataSize >= MIN_HEADER_DATA_SIZE) {
        LOGI("dataSize =%u  mRestLen=%d,", dataSize, mRestLen);

        char *tmpSpuBuffer = spuBuf;
        char *spuBufPiece = tmpSpuBuffer;
        if (mRestLen) {
            memcpy(spuBufPiece, mRestbuf, mRestLen);
        }

        if ((currentType == AV_CODEC_ID_DVD_SUBTITLE || currentType == AV_CODEC_ID_VOB_SUBTITLE)
                && mRestLen > 0) {
            LOGI("decode rest data!\n");
        } else {
            mDataSource->read(spuBufPiece + mRestLen, 16);
            dataSize -= 16;
            tmpSpuBuffer += 16;
        }

        int rdOffset = 0;
        int syncWord = subPeekAsInt32(spuBufPiece + rdOffset);
        rdOffset += 4;
        if (syncWord != AML_PARSER_SYNC_WORD) {
            LOGE("\n wrong subtitle header :%x %x %x %x    %x %x %x %x    %x %x %x %x \n",
                    spuBufPiece[0], spuBufPiece[1], spuBufPiece[2], spuBufPiece[3],
                    spuBufPiece[4], spuBufPiece[5], spuBufPiece[6], spuBufPiece[7],
                    spuBufPiece[8], spuBufPiece[9], spuBufPiece[10], spuBufPiece[11]);
            mDataSource->read(spuBufPiece, dataSize);
            dataSize = 0;
            LOGE("\n\n ******* find wrong subtitle header!! ******\n\n");
            delete[] spuBuf;
            return -1;

        }

        LOGI("\n\n ******* find correct subtitle header ******\n\n");
        // ignore first sync byte: 0xAA/0x77
        currentType = subPeekAsInt32(spuBufPiece + rdOffset) & 0x00FFFFFF;
        rdOffset += 4;
        currentLen = subPeekAsInt32(spuBufPiece + rdOffset);
        rdOffset += 4;
        currentPts = subPeekAsInt32(spuBufPiece + rdOffset);
        rdOffset += 4;
        LOGI("dataSize=%u, currentType:%x, currentPts is %x, currentLen is %d, \n",
                dataSize, currentType, currentPts, currentLen);
        if (currentLen > dataSize) {
            LOGI("currentLen > size");
            mDataSource->read(spuBufPiece, dataSize);
            dataSize = 0;
            delete[] spuBuf;
            return -1;
        }

        // TODO: types move to a common header file, do not use magic number.
        if (currentType == AV_CODEC_ID_DVD_SUBTITLE || currentType == AV_CODEC_ID_VOB_SUBTITLE) {
            mDataSource->read(spuBufPiece + mRestLen + 16, dataSize - mRestLen);
            mRestLen = dataSize;
            dataSize = 0;
            tmpSpuBuffer += currentLen;
            LOGI("currentType=0x17000 or 0x1700a! mRestLen=%d, dataSize=%d,\n", mRestLen, dataSize);
        } else {
            mDataSource->read(spuBufPiece + 16, currentLen + 4);
            dataSize -= (currentLen + 4);
            tmpSpuBuffer += (currentLen + 4);
            mRestLen = 0;
        }


        switch (currentType) {
            case AV_CODEC_ID_VOB_SUBTITLE:   //mkv internel image
                durationPts = subPeekAsInt32(spuBufPiece + rdOffset);
                rdOffset += 4;
                mRestLen -= 4;
                LOGI("durationPts is %d\n", durationPts);
                break;

            case AV_CODEC_ID_TEXT:   //mkv internel utf-8
            case AV_CODEC_ID_SSA:   //mkv internel ssa
            case AV_CODEC_ID_SUBRIP:   //mkv internel SUBRIP
            case AV_CODEC_ID_ASS:   //mkv internel ass
                durationPts = subPeekAsInt32(spuBufPiece + rdOffset);
                rdOffset += 4;
                spu->subtitle_type = TYPE_SUBTITLE_SSA;
                spu->buffer_size = currentLen + 1;  //256*(currentLen/256+1);
                spu->spu_data = new uint8_t[spu->buffer_size]();
                spu->pts = currentPts;
                spu->m_delay = durationPts;
                if (durationPts != 0) {
                    spu->m_delay += currentPts;
                }

                memcpy(spu->spu_data, spuBufPiece + rdOffset, currentLen);
                if (currentType == AV_CODEC_ID_SSA || currentType == AV_CODEC_ID_ASS) {
                    ret = __getAssSpu(spu->spu_data, spu->buffer_size, spu);
                    LOGI("CODEC_ID_SSA  size is:%u ,data is:%s, currentLen=%d\n",
                             spu->buffer_size, spu->spu_data, currentLen);
                } else {
                    ret = 0;
                }

                break;

            case AV_CODEC_ID_MOV_TEXT:
                durationPts = subPeekAsInt32(spuBufPiece + rdOffset);
                rdOffset += 4;
                spu->subtitle_type = TYPE_SUBTITLE_TMD_TXT;
                spu->buffer_size = currentLen + 1;
                spu->spu_data = new uint8_t[spu->buffer_size]();
                spu->pts = currentPts;
                spu->m_delay = durationPts;
                if (durationPts != 0) {
                    spu->m_delay += currentPts;
                }
                rdOffset += 2;
                currentLen -= 2;
                if (currentLen == 0) {
                    ret = -1;
                    break;
                }
                memcpy(spu->spu_data, spuBufPiece + rdOffset, currentLen);
                LOGI("CODEC_ID_TIME_TEXT   size is:    %u ,data is:    %s, currentLen=%d\n",
                        spu->buffer_size, spu->spu_data, currentLen);
                ret = 0;
                break;

            default:
                ALOGD("received invalid type %x", currentType);
                ret = -1;
                break;
        }

        if (ret < 0) break;

        //std::list<std::shared_ptr<AML_SPUVAR>> mDecodedSpu;
        // TODO: add protect? only list operation may no need.
        // TODO: sort
         addDecodedItem(std::shared_ptr<AML_SPUVAR>(spu));
    }

    //LOGI("[%s::%d] error! spuBuf=%x, \n", __FUNCTION__, __LINE__, spuBuf);
    if (spuBuf) {
        delete[] spuBuf;
        spuBuf = NULL;
    }
    return ret;
}


int AssParser::getInterSpu() {
    std::shared_ptr<AML_SPUVAR> spu(new AML_SPUVAR());

    //TODO: commom place
    spu->sync_bytes = AML_PARSER_SYNC_WORD;//0x414d4c55;
    // simply, use new instead of malloc, can automatically initialize the buffer
    spu->useMalloc = true;
    spu->isSimpleText = true;
    int ret = getSpu(spu);

    return ret;
}


int AssParser::parse() {
    while (!mThreadExitRequested) {
        if (getInterSpu() < 0) {
            // advance input and parse failed, wait and retry.
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }
    return 0;
}

void AssParser::dump(int fd, const char *prefix) {
    dprintf(fd, "%s ASS Parser\n", prefix);
    dumpCommon(fd, prefix);
    dprintf(fd, "%s  rest Length=%d\n", prefix, mRestLen);

}



