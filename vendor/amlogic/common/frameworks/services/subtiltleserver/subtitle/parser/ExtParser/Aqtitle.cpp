#define LOG_TAG "Aqtitle"

#include "Aqtitle.h"

Aqtitle::Aqtitle(std::shared_ptr<DataSource> source): ExtParser(source) {
    mDataSource = source;
    mPtsRate = 15;
    mExtParserUtils = new ExtParserUtils(0, mDataSource);
}

Aqtitle::~Aqtitle() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * Aqtitle::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    char *next;
    int i;
    ALOGD("Aqtitle ExtSubtitleParser");
    while (1) {
        // try to locate next subtitle
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if (!(sscanf(line, "-->> %d", &(current->start)) < 1)) {
            break;
        }
    }
    if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
        return NULL;
    }
    mExtParserUtils->ExtSubtiltleReadtext((char *)&line, &current->text.text[0]);
    current->text.lines = 1;
    current->end = current->start;  // will be corrected by next subtitle
    if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
        return current;
    }
    next = line, i = 1;
    while (1) {
        next = mExtParserUtils->ExtSubtiltleReadtext(next, &(current->text.text[i]));
        if (!next) {
            break;
        }
        if (strlen(next) == 0) {
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
    if ((current->text.text[0] == NULL) && (current->text.text[1] == NULL)) {
        // void subtitle -> end of previous marked and exit
        return NULL;
    }
    while (1) {
        // try to locate next subtitle
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        if (!(sscanf(line, "-->> %d", &(current->end)) < 1)) {
            break;
        }
    }
    current->start = (current->start * 100) / mPtsRate;
    current->end = (current->end * 100) / mPtsRate;
    return current;
}
