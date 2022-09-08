#define LOG_TAG "ExtParser"
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

#include "ExtParser.h"
#include "ParserFactory.h"


#define __DEBUG
#ifdef __DEBUG
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  TRACE()    LOGI("[%s::%d]\n",__FUNCTION__,__LINE__)
#else
#define  LOGI(...)
#define  LOGE(...)
#define  TRACE()
#endif


#define log_print LOGI
//#define log_print LOGE
//#define LOGE ALOGE

//TODO: move to utils directory

ExtParser::ExtParser(std::shared_ptr<DataSource> source) {
    mDataSource = source;
    mParseType = TYPE_SUBTITLE_EXTERNAL;

    mPtsRate = 24;
    mGotPtsRate = true;
    mNoTextPostProcess = 0;  // 1 => do not apply text post-processing
    mState = SUB_INIT;
    mMaxSpuItems = EXTERNAL_MAX_NUMBER_SPU_ITEM;
}

ExtParser::~ExtParser() {
    LOGI("%s", __func__);
    // call back may call parser, parser later destroy
    mSubIndex = 0;
}


subtitle_t * ExtParser ::ExtSubtitleParser(subtitle_t *current) {
    return NULL;
}

void ExtParser::ExtSubtitlePost(subtitle_t *current) {

}

void ExtParser::internal_sub_close(subdata_t *subdata)
{
    int i;
    list_t *entry;
    entry = subdata->list.next;
    while (entry != &subdata->list) {
        subtitle_t *subt = list_entry(entry, subtitle_t, list);
        if (subdata->sub_format == SUB_DIVX) {
            if (subt->subdata) {
                FREE(subt->subdata);
            }
        }
        else {
            for (i = 0; i < subt->text.lines; i++) {
                FREE(subt->text.text[i]);
            }
        }
        entry = entry->next;
        FREE(subt);
    }
    FREE(subdata);
}


#ifdef ENABLE_PROBE_UTF8_UTF16
static unsigned char probe_find_time = 10;
int ExtParser::probe_utf8_utf16(char *probe_buffer, int size) {
    int utf8_count = 0;
    int little_utf16 = 0;
    int big_utf16 = 0;
    int i = 0;
    for (i = 0; i < (size - 5); i++) {
        if (probe_buffer[i] == 0
                && (probe_buffer[i + 1] > 0x20
                    && probe_buffer[i + 1] < 0x7d)) {
            i++;
            big_utf16++;
        }
        else if (probe_buffer[i + 1] == 0 && (probe_buffer[i] > 0x20 && probe_buffer[i] < 0x7d)) {
            i++;
            little_utf16++;
        }
        else if (((probe_buffer[i] & 0xE0) == 0xE0) && ((probe_buffer[i + 1] & 0xC0) == 0x80)
                 && ((probe_buffer[i + 2] & 0xC0) == 0x80)) {
            i += 2;
            utf8_count++;
            if (((probe_buffer[i + 1] >= 0x80)
                    && ((probe_buffer[i + 1] & 0xE0) != 0xE0))
                    || ((probe_buffer[i + 2] >= 0x80)
                        && ((probe_buffer[i + 2] & 0xC0) != 0x80))
                    || ((probe_buffer[i + 3] >= 0x80)
                        && ((probe_buffer[i + 3] & 0xC0) != 0x80)))
                return 0;
        }
        if (big_utf16 >= probe_find_time)
            return 2;
        else if (little_utf16 >= probe_find_time)
            return 3;
        else if (utf8_count >= probe_find_time)
            return 1;
    }
    if (i == (size - 2)) {
        if (big_utf16 > 0)
            return 2;
        else if (little_utf16 > 0)
            return 3;
        else if (utf8_count > 0)
            return 1;
    }
    return 0;
}
#endif

subdata_t * ExtParser::internal_sub_open(unsigned int rate, char *charset) {
    list_t *entry;
    int fd = 0;
    subtitle_t *sub, *sub_read;
    subdata_t *subdata;
    int sub_format = SUB_INVALID;

    //reset_variate();
    mDataSource->lseek(0, SEEK_SET);
    ALOGD("[internal_sub_open]  charset=%s\n", charset);
    //log_print("charset = %s\n", charset);
    if (charset == NULL) {
        char file_format[3];
        mDataSource->read(file_format, 3);
        if (file_format[0] == 0xFF && file_format[1] == 0xFE) {
            subtitle_format = 3;    //little endian
        } else if (file_format[0] == 0xFE && file_format[1] == 0xFF) {
            subtitle_format = 2;    //big endian
        } else if ((file_format[0] == 0xEF) && (file_format[1] == 0xBB)
                 && (file_format[2] == 0xBF)) {
            subtitle_format = 1;
        } else {
            subtitle_format = 0;
        }
        mDataSource->lseek(-3, SEEK_CUR);
#ifdef ENABLE_PROBE_UTF8_UTF16
        if (subtitle_format == 0) {
            char *probe_buffer = NULL;
            probe_buffer = (char *)MALLOC(SUBTITLE_PROBE_SIZE);
            if (probe_buffer != NULL) {
                if (mDataSource->read(probe_buffer, SUBTITLE_PROBE_SIZE) == SUBTITLE_PROBE_SIZE) {
                    subtitle_format = probe_utf8_utf16(probe_buffer, SUBTITLE_PROBE_SIZE);
                }
                FREE(probe_buffer);
                mDataSource->lseek(0, SEEK_SET);
            }
        }
#endif
    } else {
        if (strcmp(charset, "UTF8") == 0) {
            subtitle_format = 1;
        } else if (strcmp(charset, "UTF-16LE") == 0) {
            subtitle_format = 3;
        } else if (strcmp(charset, "UTF-16BE") == 0) {
            subtitle_format = 2;
        } else {
            subtitle_format = 0;
        }
    }
    log_print("subtitle_format = %d\n", subtitle_format);
    subdata = (subdata_t *) MALLOC(sizeof(subdata_t));
    if (!subdata) {
        //close(fd);
        mDataSource->stop();
        log_print("malloc data error, return!\n");
        return NULL;
    }
    memset(subdata, 0, sizeof(subdata_t));
    INIT_LIST_HEAD(&subdata->list);

    mDataSource->lseek(0, SEEK_SET);
    mPtsRate = 15;       //24;//dafault value

    while (1) {
        sub = (subtitle_t *) MALLOC(sizeof(subtitle_t));
        if (!sub) {
            break;
        }
        memset(sub, 0, sizeof(subtitle_t));
        sub->end = rate;
        sub_read = ExtSubtitleParser(sub);
       //sub_read = srp->read(sub);
        if (!sub_read) {
            FREE(sub);
            break;  // EOF
        }
        if (sub_read == ERR) {
            FREE(sub);
            subdata->sub_error++;
        } else {
            // Apply any post processing that needs recoding first
            if (!mNoTextPostProcess) {
                //srp->post(sub_read);
                ExtSubtitlePost(sub_read);
            }
            /* 10ms to pts conversion */
            sub->start = sub_ms2pts(sub->start);
            sub->end = sub_ms2pts(sub->end);
            //log_print("[internal_sub_open]0(%d, %d) %s\n", sub->start, sub->end, sub->text.text[0]);
            log_print("return: %s\n", sub->text.text[0]);
            list_add_tail(&sub->list, &subdata->list);
            subdata->sub_num++;
        }
    }
    /*
       list_for_each(entry, &subdata->list) {
       subtitle_t *subt = list_entry(entry, subtitle_t, list);
       log_print("[internal_sub_open](%d,%d)%s\n",subt->start,subt->end, subt->text.text[0]);
       } */
    //close(fd);
    mDataSource->stop();
    if (subdata->sub_num <= 0) {
        internal_sub_close(subdata);
        return NULL;
    }
    log_print("SUB: Read %d subtitles", subdata->sub_num);
    if (subdata->sub_error) {
        log_print(", %d bad line(s).\n", subdata->sub_error);
    } else {
        log_print((".\n"));
    }
    return subdata;
}

void ExtParser::reset_variate() {
    mGotPtsRate = false;
}

void ExtParser::resetForSeek() {
    mSubIndex = 0;
}

std::shared_ptr<AML_SPUVAR> ExtParser::tryConsumeDecodedItem() {
     std::list<std::shared_ptr<AML_SPUVAR>>::iterator it;
     //std::unique_lock<std::mutex> autolock(mMutex);
     if (mDecodedSpu.size() == 0) {
         __android_log_print(ANDROID_LOG_ERROR, "ExtParser", "no decoder spu, size 0");
         return nullptr;
     }
     it = mDecodedSpu.begin();

     for (int i = 0; i < mSubIndex; i++) {
         if (mSubIndex >= mDecodedSpu.size()) {
             mSubIndex = 0;
             return nullptr;
         }
         ++it;
     }
     mSubIndex++;
     return (std::shared_ptr<AML_SPUVAR>) *it;
}

int ExtParser::getSpu() {
    int ret = -1;
    char *spuBuf = NULL;
    char s[4] ="GBK";
    char *charset;
    charset = s;
    uint32_t currentLen, currentPts, currentType, durationPts;

    if (mState == SUB_INIT) {
        mState = SUB_PLAYING;
    } else if (mState == SUB_STOP) {
        ALOGD(" subtitle_status == SUB_STOP \n\n");
        return 0;
    }
    subdata_t *subdata = NULL;
    subdata = internal_sub_open(0, charset);
    if (subdata == NULL) {
        LOGE("internal_sub_open failed!");
        return -1;
    }
    int i = 0, j = 0;
    list_t *entry;
    char *textBuf = NULL;
    int len = 0;
    list_for_each(entry, &subdata->list) {
        i++;
        subtitle_t *subt = list_entry(entry, subtitle_t, list);

        for (j = 0; j < subt->text.lines; j++) {
            len = len + strlen(subt->text.text[j]) + 1; //+1 indicate "\n" for each line
        }
        textBuf = (char *)malloc(len + 1);
        if (textBuf == NULL) {
            LOGE("malloc text buffer failed!");
            return -1;
        }
        memset(textBuf, 0, len);
        for (j = 0; j < subt->text.lines; j++) {
            strcat(textBuf, subt->text.text[j]);
            strcat(textBuf, "\n");
        }
        LOGE("[getSpu](%d,%d)%s", subt->start, subt->end, textBuf);
        std::shared_ptr<AML_SPUVAR> spu(new AML_SPUVAR());
        spu->pts = subt->start;
        spu->m_delay = subt->end;
        int len = strlen(textBuf);
        spu->spu_data = (unsigned char *)malloc(len);
        memset(spu->spu_data, 0, len);
        memcpy(spu->spu_data, textBuf, len);
        spu->buffer_size = len;
        spu->isExtSub = true;
        addDecodedItem(std::shared_ptr<AML_SPUVAR>(spu));
        free(textBuf);
        usleep(100);
    }
    return ret;
}


int ExtParser::getInterSpu() {
    //std::shared_ptr<AML_SPUVAR> spu(new AML_SPUVAR());

    //TODO: commom place
    //spu->sync_bytes = AML_PARSER_SYNC_WORD;//0x414d4c55;
    // simply, use new instead of malloc, can automatically initialize the buffer
    //spu->useMalloc = true;
    usleep(500);
    int ret = getSpu();

    return ret;
}


int ExtParser::parse() {
    if (!mThreadExitRequested) {
        if (mState == SUB_INIT) {
            mState = SUB_PLAYING;
            getInterSpu();
            // advance input and parse failed, wait and retry.
        }
    }
    return 0;
}
