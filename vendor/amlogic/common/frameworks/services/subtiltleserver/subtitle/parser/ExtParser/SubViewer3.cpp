#define LOG_TAG "SubViewer3"

#include "SubViewer3.h"

SubViewer3::SubViewer3(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0, source);
}

SubViewer3::~SubViewer3() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * SubViewer3::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    int a0, a1, a2, a3, a4, b1, b2, b3, b4;
    char *p = NULL, *q = NULL;
    int len;
    while (1) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if (sscanf(line, "%d  %d:%d:%d,%d  %d:%d:%d,%d",
                   &a0, &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4) < 9) {
            continue;
        }
        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4;
        current->end = b1 * 360000 + b2 * 6000 + b3 * 100 + b4;
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        p = q = line;
        for (current->text.lines = 1;
                current->text.lines < SUB_MAX_TEXT;
                current->text.lines++) {
            for (q = p, len = 0;
                    *p && *p != '\r' && *p != '\n' && *p != '|'
                    && strncmp(p, "[br]", 4); p++, len++) ;
            current->text.text[current->text.lines - 1] =
                (char *)MALLOC(len + 1);
            if (!current->text.text[current->text.lines - 1]) {
                return NULL;
            }
            strncpy(current->text.text[current->text.lines - 1], q,
                    len);
            current->text.text[current->text.lines - 1][len] = '\0';
            if (!*p || *p == '\r' || *p == '\n') {
                break;
            }
            if (*p == '|') {
                p++;
            } else {
                while (*p++ != ']') ;
            }
        }
        break;
    }
    return current;

}

