#define LOG_TAG "Mircodvd"

#include "Mircodvd.h"

Mircodvd::Mircodvd(std::shared_ptr<DataSource> source): ExtParser(source) {
    mPtsRate = 15;
    mGotPtsRate = true;
    mExtParserUtils = new ExtParserUtils(0, source);
}

Mircodvd::~Mircodvd() {
    mGotPtsRate = false;
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * Mircodvd::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    char line2[LINE_LEN + 1];
    char *p, *next;
    int i;
    if (!mGotPtsRate) {
        //fps sysfs default 0
        /*mPtsRate = get_subt5itle_fps() / 100;
        if (get_subtitle_fps() % 100) {
            mPtsRate ++;
        }*/
        if (mPtsRate <= 0) {
            mPtsRate = 24; //30;
        }
        mGotPtsRate = true;
        ALOGD("--------ExtSubReadLineMicrodvd---get frame rate: %d--\n", mPtsRate);
    }
    current->end = 0;
    do {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            ALOGD("ExtSubtitleGets is null");
            return NULL;
        }
    }
    while ((sscanf(line, "{%d}{}%[^\r\n]", &(current->start), line2) < 2)
            &&
            (sscanf
             (line, "{%d}{%d}%[^\r\n]", &(current->start), &(current->end),
              line2) < 3));
    if ((current->start && current->start == 1)
            || (current->start && current->end
                && current->start == current->end == 1)) {
        mPtsRate = atoi(line2);
    }
    p = line2;
    next = p, i = 0;
    while (1) {
        next = mExtParserUtils->ExtSubtiltleReadtext(next, &(current->text.text[i]));
        if (!next) {
            break;
        }
        if (current->text.text[i] == ERR) {
            return (subtitle_t *)ERR;
        }
        i++;
        if (i >= SUB_MAX_TEXT) {
            ALOGD(("Too many lines in a subtitle\n"));
            current->text.lines = i;
            return current;
        }
    }
    current->text.lines = ++i;
    current->start = current->start * 100 / mPtsRate;
    current->end = current->end * 100 / mPtsRate;
    ALOGD("time  %d %d \n", current->start, current->end);
    return current;

}

