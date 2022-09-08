#define LOG_TAG "Mplayer2"

#include "Mplayer2.h"

Mplayer2::Mplayer2(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0, source);
}

Mplayer2::~Mplayer2() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * Mplayer2::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    char line2[LINE_LEN + 1];
    char *p, *next;
    int i;

    do {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line))
            return NULL;
            ALOGD("internal_sub_read_line_mpl2 :%s\n", line);
    }
    while ((sscanf
            (line, "[%d][%d]%[^\r\n]", &(current->start), &(current->end),
             line2) < 3));
    current->start *= 10;
    current->end *= 10;
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

