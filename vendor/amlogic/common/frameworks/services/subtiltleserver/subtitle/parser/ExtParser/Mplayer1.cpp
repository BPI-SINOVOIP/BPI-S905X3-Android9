#define LOG_TAG "Mplayer1"

#include "Mplayer1.h"

Mplayer1::Mplayer1(std::shared_ptr<DataSource> source): ExtParser(source) {
    mPtsRate = 15;
    mGotPtsRate = true;
    mExtParserUtils = new ExtParserUtils(0, source);
}

Mplayer1::~Mplayer1() {
    mGotPtsRate = false;
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * Mplayer1::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    char line2[LINE_LEN + 1];
    char *p, *next;
    int tmp;
    int i;

    do {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line))
            return NULL;
    }
    while ((sscanf
            (line, "%d,%d,%d,%[^\r\n]", &(current->start),
             &(current->end), &tmp, line2) < 4));
    //parse pts rate
    if ((current->start && current->start == 1)
            || (current->start && current->end
                && current->start == current->end == 1)) {
        mPtsRate = atoi(line2);
        mGotPtsRate = true;
    }
    current->start = (current->start * 100) / mPtsRate;
    current->end = (current->end * 100) / mPtsRate;
    p = line2;
    next = p, i = 0;
    while (1) {
        next = mExtParserUtils->ExtSubtiltleReadtext(next, &(current->text.text[i]));
        if (!next) {
            break;
        }
        if (current->text.text[i] == ERR) {
            return NULL;
        }
        i++;
        if (i >= SUB_MAX_TEXT) {
            ALOGD(("Too many lines in a subtitle\n"));
            current->text.lines = i;
            return current;
        }
    }
    current->text.lines = ++i;
    return current;
}

