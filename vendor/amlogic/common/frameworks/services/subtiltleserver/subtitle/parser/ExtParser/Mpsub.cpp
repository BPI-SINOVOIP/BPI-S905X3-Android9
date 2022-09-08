#define LOG_TAG "Mpsub"

#include "Mpsub.h"

Mpsub::Mpsub(std::shared_ptr<DataSource> source): ExtParser(source) {
    mMpsubPosition = 0;
    mExtParserUtils = new ExtParserUtils(0, source);
}

Mpsub::~Mpsub() {
    mMpsubPosition = 0;
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * Mpsub::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    float a, b;
    int num = 0;
    char *p, *q;

    do {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
    }
    while (sscanf(line, "%f %f", &a, &b) != 2);
    mMpsubPosition += a * 100;
    current->start = (int)mMpsubPosition;
    mMpsubPosition += b * 100;
    current->end = (int)mMpsubPosition;
    while (num < SUB_MAX_TEXT) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return (num == 0) ? NULL : current;
        }
        p = line;
        while (isspace(*p)) {
            p++;
        }
        if (mExtParserUtils->ExtSubtitleEol(*p) && num > 0) {
            return current;
        }
        if (mExtParserUtils->ExtSubtitleEol(*p)) {
            return NULL;
        }
        for (q = p; !mExtParserUtils->ExtSubtitleEol(*q); q++) ;
        *q = '\0';
        if (strlen(p)) {
            current->text.text[num] = mExtParserUtils->ExtSubtitleStrdup(p);
            current->text.lines = ++num;
        } else {
            return (num == 0) ? NULL : current;
        }
    }
    return NULL;        /* we should have returned before if it's OK */
}

