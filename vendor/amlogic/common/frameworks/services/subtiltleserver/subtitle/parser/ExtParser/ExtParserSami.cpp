#define LOG_TAG "ExtParserSami"

#include "ExtParserSami.h"

ExtParserSami::ExtParserSami(std::shared_ptr<DataSource> source): ExtParser(source) {
    mSubSlackTime = 20000;
    mNoTextPostProcess = 0;
    mExtParserUtils = new ExtParserUtils(0, source);
}

ExtParserSami::~ExtParserSami() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * ExtParserSami::ExtSubtitleParser(subtitle_t *current) {
    static char line[LINE_LEN + 1];
    static char *s = NULL, *slacktime_s;
    char text[LINE_LEN + 1], *p = NULL, *q;
    int state;
    current->text.lines = current->start = current->end = 0;
    current->text.alignment = SUB_ALIGNMENT_BOTTOMCENTER;
    state = 0;
    /* read the first line */
    if (!s) {
        s = mExtParserUtils->ExtSubtitlesfileGets(line);
        if (!s) {
            return 0;
        }
    }
    do {
        switch (state) {
            case 0: /* find "START=" or "Slacktime:" */
                slacktime_s = mExtParserUtils->ExtSubtitleStristr(s, "Slacktime:");
                if (slacktime_s) {
                    mSubSlackTime = strtol(slacktime_s + 10, NULL, 0) / 10;
                }
                s = mExtParserUtils->ExtSubtitleStristr(s, "Start=");
                if (s) {
                    current->start = strtol(s + 6, &s, 0) / 10;
                    /* eat '>' */
                    for (; *s != '>' && *s != '\0'; s++) ;
                    s++;
                    state = 1;
                    continue;
                }
                break;
            case 1: /* find (optionnal) "<P", skip other TAGs */
                for (; *s == ' ' || *s == '\t'; s++) ;  /* strip blanks, if any */
                if (*s == '\0')
                    break;
                if (*s != '<') {
                    state = 3;
                    p = text;
                    continue;
                }
                /* not a TAG */
                s++;
                if (*s == 'P' || *s == 'p') {
                    s++;
                    state = 2;
                    continue;
                }
                /* found '<P' */
                for (; *s != '>' && *s != '\0'; s++) ;  /* skip remains of non-<P> TAG */
                if (*s == '\0')
                    break;
                s++;
                continue;
            case 2: /* find ">" */
                s = strchr(s, '>');
                if (s) {
                    s++;
                    state = 3;
                    p = text;
                    continue;
                }
                break;
            case 3: /* get all text until '<' appears */
                if (*s == '\0')
                    break;
                else if (!strncasecmp(s, "<br>", 4)) {
                    *p = '\0';
                    p = text;
                    mExtParserUtils->ExtSubtitleTrailspace(text);
                    if (text[0] != '\0')
                        current->text.text[current->text.lines++] = mExtParserUtils->ExtSubtitleStrdup(text);
                    s += 4;
                } else if ((*s == '{') && !mNoTextPostProcess) {
                    state = 5;
                    ++s;
                    continue;
                } else if (*s == '<') {
                    state = 4;
                } else if (!strncasecmp(s, "&nbsp;", 6)) {
                    *p++ = ' ';
                    s += 6;
                } else if (*s == '\t') {
                    *p++ = ' ';
                    s++;
                } else if (*s == '\r' || *s == '\n') {
                    s++;
                } else
                    *p++ = *s++;
                /* skip duplicated space */
                if (p > text + 2)
                    if (*(p - 1) == ' ' && *(p - 2) == ' ')
                        p--;
                continue;
            case 4: /* get current->end or skip <TAG> */
                q = mExtParserUtils->ExtSubtitleStristr(s, "Start=");
                if (q) {
                    current->end = strtol(q + 6, &q, 0) / 10 - 1;
                    *p = '\0';
                    mExtParserUtils->ExtSubtitleTrailspace(text);
                    if (text[0] != '\0')
                        current->text.text[current->text.lines++] = mExtParserUtils->ExtSubtitleStrdup(text);
                    if (current->text.lines > 0) {
                        state = 99;
                        break;
                    }
                    state = 0;
                    continue;
                }
                s = strchr(s, '>');
                if (s) {
                    s++;
                    state = 3;
                    continue;
                }
                break;
            case 5: /* get rid of {...} text, but read the alignment code */
                if ((*s == '\\') && (*(s + 1) == 'a')
                        && !mNoTextPostProcess)
                {
                    if (mExtParserUtils->ExtSubtitleStristr(s, "\\a1") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_BOTTOMLEFT;
                        s = s + 3;
                    }
                    if (mExtParserUtils->ExtSubtitleStristr(s, "\\a2") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_BOTTOMCENTER;
                        s = s + 3;
                    } else if (mExtParserUtils->ExtSubtitleStristr(s, "\\a3") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_BOTTOMRIGHT;
                        s = s + 3;
                    } else if ((mExtParserUtils->ExtSubtitleStristr(s, "\\a4") != NULL)
                             || (mExtParserUtils->ExtSubtitleStristr(s, "\\a5") != NULL)
                             || (mExtParserUtils->ExtSubtitleStristr(s, "\\a8") != NULL)) {
                        current->text.alignment = SUB_ALIGNMENT_TOPLEFT;
                        s = s + 3;
                    } else if (mExtParserUtils->ExtSubtitleStristr(s, "\\a6") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_TOPCENTER;
                        s = s + 3;
                    } else if (mExtParserUtils->ExtSubtitleStristr(s, "\\a7") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_TOPRIGHT;
                        s = s + 3;
                    } else if (mExtParserUtils->ExtSubtitleStristr(s, "\\a9") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_MIDDLELEFT;
                        s = s + 3;
                    } else if (mExtParserUtils->ExtSubtitleStristr(s, "\\a10") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_MIDDLECENTER;
                        s = s + 4;
                    } else if (mExtParserUtils->ExtSubtitleStristr(s, "\\a11") != NULL) {
                        current->text.alignment = SUB_ALIGNMENT_MIDDLERIGHT;
                        s = s + 4;
                    }
                }
                if (*s == '}')
                    state = 3;
                ++s;
                continue;
        }
        /* read next line */
        if (state != 99)
            s = mExtParserUtils->ExtSubtitlesfileGets(line);
        if (state != 99 && !s) {
            if (current->start > 0) {
                break;  // if it is the last subtitle
            } else {
                return 0;
            }
        }
    }
    while (state != 99);
    /* For the last subtitle */
    if (current->end <= 0) {
        current->end = current->start + mSubSlackTime;
        *p = '\0';
        mExtParserUtils->ExtSubtitleTrailspace(text);
        if (text[0] != '\0') {
            current->text.text[current->text.lines++] = mExtParserUtils->ExtSubtitleStrdup(text);
        } else
            return 0;
    }
    return current;
}

