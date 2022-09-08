#define LOG_TAG "ExtParserPjs"

#include "ExtParserPjs.h"

ExtParserPjs::ExtParserPjs(std::shared_ptr<DataSource> source): ExtParser(source) {
    mPtsRate = 15;
    mExtParserUtils = new ExtParserUtils(0, source);
}

ExtParserPjs::~ExtParserPjs() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

/*
 * PJS subtitles reader.
 * That's the "Phoenix Japanimation Society" format.
 * I found some of them in http://www.scriptsclub.org/ (used for anime).
 * The time is in tenths of second.
 */
subtitle_t * ExtParserPjs::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    char text[LINE_LEN + 1], *s, *d;

    if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
        return NULL;
    }
    /* skip spaces */
    for (s = line; *s && isspace(*s); s++) ;
    /* allow empty lines at the end of the file */
    if (*s == 0) {
        return NULL;
    }
    /* get the time */
    if (sscanf(s, "%d,%d,", &(current->start), &(current->end)) < 2) {
        return NULL;
    }
    /* the files I have are in tenths of second */
    //current->start *= 10;
    //current->end *= 10;
    current->start = (current->start * 100) / mPtsRate;
    current->end = (current->end * 100) / mPtsRate;
    /* walk to the beggining of the string */
    for (; *s; s++) {
        if (*s == ',')
            break;
    }
    if (*s) {
        for (s++; *s; s++)
            if (*s == ',')
                break;
        if (*s)
            s++;
    }
    /* skip spaces */
    for (; *s && isspace(*s); s++) ;
    if (*s != '"') {
        return NULL;
    }
    /* copy the string to the text buffer */
    for (s++, d = text; *s && *s != '"'; s++, d++) {
        *d = *s;
    }
    *d = 0;
    current->text.text[0] = mExtParserUtils->ExtSubtitleStrdup(text);
    current->text.lines = 1;
    return current;
}

