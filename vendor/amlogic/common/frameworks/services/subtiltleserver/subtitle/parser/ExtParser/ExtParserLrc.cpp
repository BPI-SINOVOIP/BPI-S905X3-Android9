#define LOG_TAG "ExtParserLrc"

#include "ExtParserLrc.h"

ExtParserLrc::ExtParserLrc(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0, source);
}

ExtParserLrc::~ExtParserLrc() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * ExtParserLrc::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    int a1, a2, a3;
    char *p = NULL, *next, separator;
    int i, len, plen;

    while (!current->text.text[0]) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if ((len = sscanf(line, "[%d:%d.%d%c%n", &a1, &a2, &a3, &separator,&plen)) < 4) {
            continue;
        }
        current->start = a1 * 6000 + a2 * 100 + a3;
        if (!current->start) {
            //continue;
            current->start = 0;
        }
        p = &line[plen];
        i = 0;
        if (*p != '|') {
            next = p;
            while (1) {
                next = mExtParserUtils->ExtSubtiltleReadtext(next, &(current->text.text[i]));
                if (current->text.text[i] == NULL) {
                    return NULL;
                }
                i++;
                if (!next) {
                    break;
                }
                if (i >= SUB_MAX_TEXT) {
                    ALOGD(("Too many lines in a subtitle\n"));
                    current->text.lines = i;
                    while (1) {
                        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
                            return NULL;
                        }
                        if ((len = sscanf(line,"[%d:%d.%d%c%n", &a1,&a2, &a3,&separator,&plen)) < 4) {
                            continue;
                        } else {
                            current->end = a1 * 6000 + a2 * 100 + a3;
                            return current;
                        }
                    }
                    return current;
                }
            }
            current->text.lines = i;
        }
    }
    while (1) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if ((len = sscanf(line, "[%d:%d.%d%c%n", &a1, &a2, &a3, &separator,&plen)) < 4) {
            continue;
        } else {
            current->end = a1 * 6000 + a2 * 100 + a3;
            return current;
        }
    }
    return current;
}

