#define LOG_TAG "Subrip09"

#include "Subrip09.h"

Subrip09::Subrip09(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0, source);
}

Subrip09::~Subrip09() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * Subrip09::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    int a1, a2, a3;
    char *next = NULL;
    int i, len;
    while (1) {
        // try to locate next subtitle
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if (!((len = sscanf(line, "[%d:%d:%d]", &a1, &a2, &a3)) < 3)) {
            break;
        }
    }
    current->start = a1 * 360000 + a2 * 6000 + a3 * 100;
    /*if (internal_previous_subrip09_sub != NULL)
       {
       internal_previous_subrip09_sub->end = current->start - 1;
       }

       internal_previous_subrip09_sub = current; */
    if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
        return NULL;
    }
    next = line, i = 0;
    current->text.text[0] = NULL;   // just to be sure that string is clear
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
    current->text.lines = i + 1;
    if ((current->text.text[0] == NULL) && (i == 0)) {
        // void subtitle -> end of previous marked and exit
        return NULL;
    }
    while (1) {
        // try to locate next subtitle
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if (!((len = sscanf(line, "[%d:%d:%d]", &a1, &a2, &a3)) < 3)) {
            break;
        }
    }
    current->end = a1 * 360000 + a2 * 6000 + a3 * 100;
    return current;

}

