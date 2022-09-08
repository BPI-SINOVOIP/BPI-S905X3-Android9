#define LOG_TAG "ExtParserFactory"

#include "ParserFactory.h"
#include "ExtParserFactory.h"
#include "ExtParserUtils.h"
#include "ExtParserXml.h"
#include "Ttml.h"
#include "Vplayer.h"
#include "Aqtitle.h"
#include "ExtParserSsa.h"
#include "Jacosub.h"
#include "ExtParserLrc.h"
#include "Mircodvd.h"
#include "Subrip.h"
#include "SubViewer.h"
#include "ExtParserSami.h"
#include "ExtParserRt.h"
#include "ExtParserPjs.h"
#include "Mpsub.h"
#include "SubViewer2.h"
#include "SubViewer3.h"
#include "Subrip09.h"
#include "Mplayer1.h"
#include "Mplayer2.h"
#include "ExtParserEbuttd.h"
#include "tinyxml.h"

int ExtParserFactory::getExtSubtitleformat(std::shared_ptr<DataSource> source) {
    char line[LINE_LEN + 1];
    int i, j = 0;
    char p;
    char q[512];
    //j = sscanf("00:02:21,606 --> 00:02:23,073",
    //           "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d",
    //           &i, &i, &i, (char *)&i, &i, &i, &i, &i, (char *)&i, &i);
    //j = sscanf("abc", "[aef]bc", (char *)&i);
    j = sscanf("abc", "%[abc]", (char *)&i);
    std::shared_ptr<ExtParserUtils> pExtUtils = std::shared_ptr<ExtParserUtils>(new ExtParserUtils(0, source));
    if (pExtUtils == NULL) {
        ALOGD("can't get ext subtitle format");
        return SUB_INVALID;
    }
    while (j < 100) {
        j++;
        if (!pExtUtils->ExtSubtitlesfileGets(line)) {
            return SUB_INVALID;
        }
        if (sscanf(line, "{%d}{%d}", &i, &i) == 2) {
            return SUB_MICRODVD;
        }
        if (sscanf(line, "{%d}{}", &i) == 1) {
            return SUB_MICRODVD;
        }
        if (sscanf(line, "%d,%d,%d", &i, &i, &i) == 3) {
            return SUB_MPL1;
        }
        if (sscanf(line, "[%d][%d]", &i, &i) == 2) {
            return SUB_MPL2;
        }
        if (sscanf(line, "%d:%d:%d.%d,%d:%d:%d.%d", &i, &i, &i, &i, &i, &i,&i, &i) == 8) {
            return SUB_SUBRIP;
        }
        if (sscanf(line, "%d:%d:%d%[,.:]%d --> %d:%d:%d%[,.:]%d", &i, &i, &i, (char *)&i, &i, &i, &i, &i, (char *)&i, &i) == 10) {
            return SUB_SUBVIEWER;
        }
        if (sscanf(line, "{T %d:%d:%d:%d", &i, &i, &i, &i) == 4) {
            return SUB_SUBVIEWER2;
        }
        if (sscanf(line, "%d  %d:%d:%d,%d  %d:%d:%d,%d", &i, &i, &i, &i, &i, &i, &i, &i, &i) == 9) {
            return SUB_SUBVIEWER3;
        }
        if (strstr(line, "<SAMI>") || strstr(line, "<sami>")) {
            return SUB_SAMI;
        }
        if (sscanf(line, "%d:%d:%d.%d %d:%d:%d.%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8) {
            return SUB_JACOSUB;
        }
        if (sscanf(line, "@%d @%d", &i, &i) == 2) {
            return SUB_JACOSUB;
        }
        if (sscanf(line, "%d:%d:%d:", &i, &i, &i) == 3) {
            return SUB_VPLAYER;
        }
        if (sscanf(line, "[%d:%d.%d:", &i, &i, &i) == 3) {
            return SUB_LRC;
        }
        if (sscanf(line, "%d:%d:%d ", &i, &i, &i) == 3) {
            return SUB_VPLAYER;
        }
        if (strstr(line, "<?xml")) {
            //return SUB_XML;
            return getXmlExtentFormt(source);
        }
        if (strstr(line, "<tt")) {
            return SUB_TTML;
        }
        //TODO: just checking if first line of sub starts with "<" is WAY
        // too weak test for RT
        // Please someone who knows the format of RT... FIX IT!!!
        // It may conflict with other sub formats in the future (actually it doesn't)
        if (*line == '<') {
            return SUB_RT;
        }
        if (!memcmp(line, "Dialogue: Marked", 16)) {
            return SUB_SSA;
        }
        if (!memcmp(line, "Dialogue: ", 10)) {
            return SUB_SSA;
        }
        if (sscanf(line, "%d,%d,\"%c", &i, &i, (char *)&i) == 3) {
            return SUB_PJS;
        }
        if (sscanf(line, "%d,%d, \"%c", &i, &i, (char *)&i) == 3) {
            return SUB_PJS;
        }
        if (sscanf(line, "FORMAT=%d", &i) == 1) {
            return SUB_MPSUB;
        }
        if (sscanf(line, "FORMAT=TIM%c", &p) == 1 && p == 'E') {
            return SUB_MPSUB;
        }
        if (strstr(line, "-->>")) {
            return SUB_AQTITLE;
        }
        if (sscanf(line, "[%d:%d:%d]", &i, &i, &i) == 3) {
            return SUB_SUBRIP09;
        }
    }
    return SUB_INVALID; // too many bad lines

}

std::shared_ptr<Parser> ExtParserFactory::CreateExtSubObject(std::shared_ptr<DataSource> source) {
    int subFormat = getExtSubtitleformat(source);
    ALOGD("detect ext subtitle format = %d", subFormat);
    switch (subFormat) {
        case SUB_MICRODVD://0
            return std::shared_ptr<Parser> (new Mircodvd(source));
        case SUB_SUBRIP://1
            return std::shared_ptr<Parser> (new Subrip(source));
        case SUB_SUBVIEWER://2
            return std::shared_ptr<Parser> (new SubViewer(source));
        case SUB_SAMI://3
            return std::shared_ptr<Parser> (new ExtParserSami(source));
        case SUB_VPLAYER://4
            return std::shared_ptr<Parser> (new Vplayer(source));
        case SUB_RT://5
            return std::shared_ptr<Parser> (new ExtParserRt(source));
        case SUB_SSA://6
            return std::shared_ptr<Parser> (new ExtParserSsa(source));
        case SUB_PJS://7
            return std::shared_ptr<Parser> (new ExtParserPjs(source));
        case SUB_MPSUB://8
            return std::shared_ptr<Parser> (new Mpsub(source));
        case SUB_AQTITLE://9
            return std::shared_ptr<Parser> (new Aqtitle(source));
        case SUB_SUBVIEWER2://10
            return std::shared_ptr<Parser> (new SubViewer2(source));
        case SUB_SUBVIEWER3://11
            return std::shared_ptr<Parser> (new SubViewer3(source));
        case SUB_SUBRIP09://12
            return std::shared_ptr<Parser> (new Subrip09(source));
        case SUB_JACOSUB://13
           return std::shared_ptr<Parser> (new Jacosub(source));
        case SUB_MPL1://14
            return std::shared_ptr<Parser> (new Mplayer1(source));
        case SUB_MPL2://15
            return std::shared_ptr<Parser> (new Mplayer2(source));
        case SUB_XML://16
            return std::shared_ptr<Parser> (new ExtParserXml(source));
        case SUB_TTML://17
            return std::shared_ptr<Parser> (new Ttml(source));
        case SUB_LRC://18
            return std::shared_ptr<Parser> (new ExtParserLrc(source));
        case SUB_EBUTTD:
            return std::shared_ptr<Parser> (new ExtParserEbuttd(source));
        default:
            ALOGD("ext subtitle format is invaild! format = %d", subFormat);
            return NULL;
    }
}

int ExtParserFactory::getXmlExtentFormt(std::shared_ptr<DataSource> source) {
    int type = SUB_XML;
    TiXmlDocument doc;

    source->lseek(0, SEEK_SET);
    int size = source->availableDataSize();
    char *rdBuffer = new char[size]();
    source->read(rdBuffer, size);
    doc.Parse(rdBuffer);

    if (doc.FirstChildElement("tt") || doc.FirstChildElement("tt:tt")) {
        TiXmlElement* tt = doc.RootElement();
        if (tt->Attribute("xmlns:ebuttm")) {
            type = SUB_EBUTTD;
        } else {
            type = SUB_TTML;
        }
    }

    doc.Clear();
    delete[] rdBuffer;
    return type;
}
