#define LOG_TAG "SubViewer2"

#include "SubViewer2.h"

SubViewer2::SubViewer2(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0,  source);
}

SubViewer2::~SubViewer2() {
    mGotPtsRate = false;
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * SubViewer2::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    int a1, a2, a3, a4;
    char *p = NULL;
    int i, len;
    static int matchNum = 0;
    while (!current->text.text[0]) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if (line[0] != '{') {
            ALOGD(("[internal_sub_read_line_subviewer2]-1-\n"));
            continue;
        }
        if ((len = sscanf(line, "{T %d:%d:%d:%d", &a1, &a2, &a3, &a4)) < 4) {
            continue;
        }
        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        for (i = 0; i < SUB_MAX_TEXT;) {
            if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
                break;
            }
            if (line[0] == '}') {
                if (matchNum % 2 == 0) {
                    matchNum++;
                    continue;
                    //break;
                } else {
                    matchNum++;
                    break;
                }
            }
            if (line[0] == '{') {
                 if ((len = sscanf(line, "{T %d:%d:%d:%d", &a1, &a2, &a3, &a4)) < 4) {
                      continue;
                 }
                 current->end = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
                 matchNum++;
                 break;
            }
            len = 0;
            for (p = line; *p != '\n' && *p != '\r' && *p;
                    ++p, ++len) ;
            if (len) {
                current->text.text[i] = (char *)MALLOC(len + 1);
                if (!current->text.text[i]) {
                    return NULL;
                }
                strncpy(current->text.text[i], line, len);
                current->text.text[i][len] = '\0';
                ++i;
            } else {
                break;
            }
        }
        current->text.lines = i;
    }
    return current;

}

