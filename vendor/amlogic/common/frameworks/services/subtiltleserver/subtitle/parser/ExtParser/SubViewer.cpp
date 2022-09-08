#define LOG_TAG "SubViewer"

#include "SubViewer.h"

SubViewer::SubViewer(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0 , source);
}

SubViewer::~SubViewer() {
    mGotPtsRate = false;
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * SubViewer::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    int a1, a2, a3, a4, b1, b2, b3, b4;
    char *p = NULL;
    int i, len;
    while (!current->text.text[0]) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if ((len = sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
                 &a1, &a2, &a3, (char *)&i, &a4, &b1, &b2, &b3, (char *)&i, &b4)) < 10) {
            continue;
        }
        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        current->end = b1 * 360000 + b2 * 6000 + b3 * 100 + b4 / 10;
        for (i = 0; i < SUB_MAX_TEXT;) {
            if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
                break;
            }
            len = 0;
            for (p = line; *p != '\n' && *p != '\r' && *p;
                    p++, len++) ;
            if (len) {
                int j = 0, skip = 0;
                char *curptr = current->text.text[i] = (char *)MALLOC(len + 1);
                if (!current->text.text[i]) {
                    return NULL;
                }
                for (; j < len; j++) {
                    /* let's filter html tags ::atmos */
                    if (line[j] == '>') {
                        skip = 0;
                        continue;
                    }
                    if (line[j] == '<') {
                        skip = 1;
                        continue;
                    }
                    if (skip) {
                        continue;
                    }
                    *curptr = line[j];
                    curptr++;
                }
                *curptr = '\0';
                i++;
            }
            else {
                break;
            }
        }
        current->text.lines = i;
    }
    return current;
}

