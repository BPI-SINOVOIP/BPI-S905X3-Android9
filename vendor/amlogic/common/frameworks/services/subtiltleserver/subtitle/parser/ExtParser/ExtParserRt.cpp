#define LOG_TAG "ExtParserRt"

#include "ExtParserRt.h"

ExtParserRt::ExtParserRt(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0, source);
}

ExtParserRt::~ExtParserRt() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * ExtParserRt::ExtSubtitleParser(subtitle_t *current) {
    //TODO: This format uses quite rich (sub/super)set of xhtml
    // I couldn't check it since DTD is not included.
    // WARNING: full XML parses can be required for proper parsing
    char line[LINE_LEN + 1];
    int a1, a2, a3, a4, b1, b2, b3, b4;
    char *p = NULL, *next = NULL;
    int i, len, plen;

    while (!current->text.text[0]) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line)) {
            return NULL;
        }
        //TODO: it seems that format of time is not easily determined, it may be 1:12, 1:12.0 or 0:1:12.0
        //to describe the same moment in time. Maybe there are even more formats in use.
        //if ((len=sscanf (line, "<Time Begin=\"%d:%d:%d.%d\" End=\"%d:%d:%d.%d\"",&a1,&a2,&a3,&a4,&b1,&b2,&b3,&b4)) < 8)
        plen = a1 = a2 = a3 = a4 = b1 = b2 = b3 = b4 = 0;
        if (((len =
                    sscanf(line,
                           "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d.%d\"%*[^<]<clear/>%n",
                           &a3, &a4, &b3, &b4, &plen)) < 4)
                &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",
                             &a3, &a4, &b2, &b3, &b4, &plen)) < 5)
                &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n",
                             &a2, &a3, &b2, &b3, &plen)) < 4)
                &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d:%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",
                             &a2, &a3, &b2, &b3, &b4, &plen)) < 5) &&
                //          ((len=sscanf (line, "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d\"%*[^<]<clear/>%n",&a2,&a3,&a4,&b2,&b3,&plen)) < 5) &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\" %*[Ee]nd=\"%d:%d.%d\"%*[^<]<clear/>%n",
                             &a2, &a3, &a4, &b2, &b3, &b4, &plen)) < 6)
                &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\" %*[Ee]nd=\"%d:%d:%d.%d\"%*[^<]<clear/>%n",
                             &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4,
                             &plen)) < 8) &&
                //now try it without end time
                ((len =
                     sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d.%d\"%*[^<]<clear/>%n",
                             &a3, &a4, &plen)) < 2)
                &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d:%d\"%*[^<]<clear/>%n",
                             &a2, &a3, &plen)) < 2)
                &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d:%d.%d\"%*[^<]<clear/>%n",
                             &a2, &a3, &a4, &plen)) < 3)
                &&
                ((len =
                      sscanf(line,
                             "<%*[tT]ime %*[bB]egin=\"%d:%d:%d.%d\"%*[^<]<clear/>%n",
                             &a1, &a2, &a3, &a4, &plen)) < 4))
            continue;
        current->start = a1 * 360000 + a2 * 6000 + a3 * 100 + a4 / 10;
        current->end = b1 * 360000 + b2 * 6000 + b3 * 100 + b4 / 10;
        if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0) {
            current->end = current->start + 200;
        }
        p = line;
        p += plen;
        i = 0;
        // TODO: I don't know what kind of convention is here for marking multiline subs, maybe <br/> like in xml?
        next = strstr(line, "<clear/>");
        if (next && strlen(next) > 8) {
            next += 8;
            i = 0;
            while (1) {
                next = mExtParserUtils->ExtSubtiltleReadtext(next, &(current->text. text[i]));
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
        }
        current->text.lines = i + 1;
    }
    return current;
}

