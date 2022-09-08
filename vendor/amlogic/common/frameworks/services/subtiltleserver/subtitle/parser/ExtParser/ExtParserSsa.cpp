#define LOG_TAG "ExtParserSsa"

#include<iostream>
#include <sstream>
#include "ExtParserSsa.h"

ExtParserSsa::ExtParserSsa(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0, source);
}

ExtParserSsa::~ExtParserSsa() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

/*
    retrieve the subtitle content from the decoded buffer.
    If is the origin ass event buffer, also fill the time stamp info.

    ASS type:
    Format: Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text
    Dialogue: 0,0:00:00.26,0:00:01.89,Default,,0000,0000,0000,,Don't move

    ssa type and example:
    Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
    Dialogue: Marked=0,0:00:00.26,0:00:01.89,Default,NTP,0000,0000,0000,!Effect,Don't move
*/
subtitle_t * ExtParserSsa::ExtSubtitleParser(subtitle_t *current) {
    /*
     * Sub Station Alpha v4 (and v2?) scripts have 9 commas before subtitle
     * other Sub Station Alpha scripts have only 8 commas before subtitle
     * Reading the "ScriptType:" field is not reliable since many scripts appear
     * w/o it
     *
     * http://www.scriptclub.org is a good place to find more examples
     * http://www.eswat.demon.co.uk is where the SSA specs can be found
     */
    const int ASS_EVENT_SECTIONS = 9;

    char line[LINE_LEN + 1];
    while (mExtParserUtils->ExtSubtitlesfileGets(line)) {
        std::stringstream ss;
        std::string str;
        std::vector<std::string> items; // store the event sections in vector
        ss << line;
        int count = 0;
        for (count=0; count<ASS_EVENT_SECTIONS; count++) {
            if (!getline(ss, str, ',')) break;
            items.push_back(str);
        }

        if (count < ASS_EVENT_SECTIONS) continue;

         // get the subtitle content. here we do not need other effect and margin data.
        getline(ss, str);

         // currently not support style control code rendering
         // discard the unsupported {} Style Override control codes
        std::size_t start, end;
        while ((start = str.find("{\\")) != std::string::npos) {
            end = str.find('}', start);
            if (end != std::string::npos) {
                str.erase(start, end-start+1);
            } else {
                break;
            }
        }

        // replacer "\n"
        while ((start = str.find("\\n")) != std::string::npos
                || (start = str.find("\\N")) != std::string::npos) {
            std::string newline = "\n";
            str.replace(start, 2, newline);
        }

        uint32_t hour, min, sec, ms;
        // 1 is start time
        if ( sscanf(items[1].c_str(), "%d:%d:%d.%d", &hour, &min, &sec, &ms) != 4) continue;
        current->start = (hour * 60 * 60 + min * 60 + sec) * 100 + ms;

        // 2 is end time.
        if ( sscanf(items[2].c_str(), "%d:%d:%d.%d", &hour, &min, &sec, &ms) != 4) continue;
        current->end = (hour * 60 * 60 + min * 60 + sec) * 100 + ms;

        current->text.text[0] = ::strdup(str.c_str());
        current->text.lines++;
        return current;
    }

    return nullptr;
}

void ExtParserSsa::ExtSubtitlePost(subtitle_t *sub) {
    int l = sub->text.lines;
    char *so, *de, *start;
    while (l) {
        /* eliminate any text enclosed with {}, they are font and color settings */
        so = de = sub->text.text[--l];
        while (*so) {
            if (*so == '{' && so[1] == '\\') {
                for (start = so; *so && *so != '}'; so++) ;
                if (*so) {
                    so++;
                } else {
                    so = start;
                }
            }
            if (*so) {
                *de = *so;
                so++;
                de++;
            }
        }
        *de = *so;
    }
}


