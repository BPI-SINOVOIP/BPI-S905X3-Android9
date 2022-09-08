#define LOG_TAG "Jacosub"

#include "Jacosub.h"

Jacosub::Jacosub(std::shared_ptr<DataSource> source): ExtParser(source) {
    mExtParserUtils = new ExtParserUtils(0, source);
}

Jacosub::~Jacosub() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * Jacosub::ExtSubtitleParser(subtitle_t *current) {
    char line1[LINE_LEN], line2[LINE_LEN], directive[LINE_LEN], *p, *q;
    unsigned a1, a2, a3, a4, b1, b2, b3, b4, comment = 0;
    static unsigned jacoTimeres = 30;
    static int jacoShift = 0;
    memset(current, 0, sizeof(subtitle_t));
    memset(line1, 0, LINE_LEN);
    memset(line2, 0, LINE_LEN);
    memset(directive, 0, LINE_LEN);

    while (!current->text.text[0]) {
        if (!mExtParserUtils->ExtSubtitlesfileGets(line1)) {
            return NULL;
        }
        if (sscanf(line1, "%u:%u:%u.%u %u:%u:%u.%u %[^\n\r]",
                   &a1, &a2, &a3, &a4, &b1, &b2, &b3, &b4, line2) < 9) {
            if (sscanf(line1, "@%u @%u %[^\n\r]", &a4, &b4, line2) < 3) {
                if (line1[0] == '#') {
                    int hours = 0, minutes = 0, seconds, delta, inverter = 1;
                    unsigned units = jacoShift;
                    switch (toupper(line1[1])) {
                        case 'S':
                            if (isalpha(line1[2])) {
                                delta = 6;
                            } else {
                                delta = 2;
                            }
                            if (sscanf(&line1[delta], "%d",&hours)) {
                                if (hours < 0) {
                                    hours *= -1;
                                    inverter = -1;
                                }
                                if (sscanf(&line1[delta], "%*d:%d",&minutes)) {
                                    if (sscanf(&line1[delta],"%*d:%*d:%d",&seconds)) {
                                        sscanf(&line1[delta],"%*d:%*d:%*d.%d",&units);
                                    } else {
                                        hours = 0;
                                        sscanf(&line1[delta],"%d:%d.%d",&minutes,&seconds,&units);
                                        minutes *= inverter;
                                    }
                                } else {
                                    hours = minutes = 0;
                                    sscanf(&line1[delta],"%d.%d",&seconds,&units);
                                    seconds *= inverter;
                                }
                                jacoShift = ((hours * 3600 + minutes * 60 + seconds) * jacoTimeres + units) * inverter;
                            }
                            break;
                        case 'T':
                            if (isalpha(line1[2])) {
                                delta = 8;
                            } else {
                                delta = 2;
                            }
                            sscanf(&line1[delta], "%u", &jacoTimeres);
                            break;
                    }
                }
                continue;
            } else {
                current->start = (unsigned long)((a4 + jacoShift) * 100.0 / jacoTimeres);
                current->end = (unsigned long)((b4 + jacoShift) * 100.0 / jacoTimeres);
            }
        } else {
            current->start = (unsigned long)(((a1 * 3600 + a2 * 60 + a3) * jacoTimeres + a4 + jacoShift) * 100.0 / jacoTimeres);
            current->end = (unsigned long)(((b1 * 3600 + b2 * 60 + b3) * jacoTimeres + b4 + jacoShift) * 100.0 / jacoTimeres);
        }
        current->text.lines = 0;
        p = line2;
        while ((*p == ' ') || (*p == '\t')) {
            ++p;
        }
        if (isalpha(*p) || *p == '[') {
            int cont, jLength;
            if (sscanf(p, "%s %[^\n\r]", directive, line1) < 2) {
                return (subtitle_t *) ERR;
            }
            jLength = strlen(directive);
            for (cont = 0; cont < jLength; ++cont) {
                if (isalpha(*(directive + cont))) {
                    *(directive + cont) =
                        toupper(*(directive + cont));
                }
            }
            if ((strstr(directive, "RDB") != NULL)
                    || (strstr(directive, "RDC") != NULL)
                    || (strstr(directive, "RLB") != NULL)
                    || (strstr(directive, "RLG") != NULL)) {
                continue;
            }
            if (strstr(directive, "JL") != NULL) {
                current->text.alignment = SUB_ALIGNMENT_BOTTOMLEFT;
            }
            else if (strstr(directive, "JR") != NULL)
            {
                current->text.alignment = SUB_ALIGNMENT_BOTTOMRIGHT;
            } else {
                current->text.alignment = SUB_ALIGNMENT_BOTTOMCENTER;
            }
            strcpy(line2, line1);
            p = line2;
        }
        for (q = line1; (!mExtParserUtils->ExtSubtitleEol(*p))
                && (current->text.lines < SUB_MAX_TEXT); ++p) {
            switch (*p) {
                case '{':
                    comment++;
                    break;
                case '}':
                    if (comment) {
                        --comment;
                        //the next line to get rid of a blank after the comment
                        if ((*(p + 1)) == ' ')
                            p++;
                    }
                    break;
                case '~':
                    if (!comment) {
                        *q = ' ';
                        ++q;
                    }
                    break;
                case ' ':
                case '\t':
                    if ((*(p + 1) == ' ') || (*(p + 1) == '\t')) {
                        break;
                    }
                    if (!comment) {
                        *q = ' ';
                        ++q;
                    }
                    break;
                case '\\':
                    if (*(p + 1) == 'n') {
                        *q = '\0';
                        q = line1;
                        current->text.text[current->text. lines++] = mExtParserUtils->ExtSubtitleStrdup(line1);
                        ++p;
                        break;
                    }
                    if ((toupper(*(p + 1)) == 'C') || (toupper(*(p + 1)) == 'F')) {
                        ++p, ++p;
                        break;
                    }
                    if ((*(p + 1) == 'B') || (*(p + 1) == 'b') || (*(p + 1) == 'D') ||  //actually this means "insert current date here"
                            (*(p + 1) == 'I') || (*(p + 1) == 'i') || (*(p + 1) == 'N') || (*(p + 1) == 'T') || //actually this means "insert current time here"
                            (*(p + 1) == 'U') || (*(p + 1) == 'u')) {
                        ++p;
                        break;
                    }
                    if ((*(p + 1) == '\\') || (*(p + 1) == '~') || (*(p + 1) == '{')) {
                        ++p;
                    } else if (mExtParserUtils->ExtSubtitleEol(*(p + 1))) {
                        if (!mExtParserUtils->ExtSubtitlesfileGets(directive)) {
                            return NULL;
                        }
                        mExtParserUtils->ExtSubtitleTrailspace(directive);
                        strncat(line2, directive, (LINE_LEN > 511) ? LINE_LEN : 511);
                        break;
                    }
                    [[fallthrough]];
                default:
                    if (!comment) {
                        *q = *p;
                        ++q;
                    }
                    break;
            }   //-- switch
        }       //-- for
        *q = '\0';
        current->text.text[current->text.lines] = mExtParserUtils->ExtSubtitleStrdup(line1);
    }           //-- while
    current->text.lines++;
    return current;
}

