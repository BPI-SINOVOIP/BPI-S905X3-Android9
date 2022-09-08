#define LOG_TAG "Ttml"

#include "Ttml.h"
#include "tinyxml.h"
#include "tinystr.h"

Ttml::Ttml(std::shared_ptr<DataSource> source): ExtParser(source) {
    mDataSource = source;
}

Ttml::~Ttml() {

}

subtitle_t * Ttml::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    char line2[LINE_LEN + 1];
    char *p, *next;
    int i;
    int size = mDataSource->availableDataSize();
    char *rdBuffer = new char[size]();
    mDataSource->read(rdBuffer, size);
    //log_print("internal_sub_read_line_xml size:%d, buffer:%s\n", size, rdBuffer);
    parseTtml(rdBuffer);
    return NULL;
}

int Ttml::parseTtml(char *buffer) {
    TiXmlDocument doc;
    doc.Parse(buffer);
    TiXmlElement* tt = doc.RootElement();
    if (tt == NULL) {
        ALOGD("Failed to load file: No tt element.\n");
        doc.Clear();
        return -1;
    }

    TiXmlElement* head = tt->FirstChildElement();
    if (head == NULL) {
        ALOGD("Failed to load file: No head element.\n");
        doc.Clear();
        return -1;
    }

    TiXmlElement* body = head->NextSiblingElement();
    if (body == NULL) {
        ALOGD("Failed to load file: No body element.\n");
        doc.Clear();
        return -1;
    }

    TiXmlElement* div = body->FirstChildElement();
    if (div == NULL) {
        ALOGD("Failed to load file: No div element.\n");
        doc.Clear();
        return -1;
    }

    subtitle_t *sub;
    int timeStart, end;


    for (TiXmlElement* elem = div->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()) {
        const char* begin= elem->Attribute("begin");
        ALOGD("parseTtml begin:%s\n", begin);
        int timeStart = atoi(begin) * 1000 * 90;

        const char* end = elem->Attribute("end");
        ALOGD("parseTtml end:%s\n", end);
        int timeEnd = atoi(end) * 1000 * 90;

        TiXmlNode* e3 = elem->FirstChild();
        const char* text= e3 ->ToText()->Value();

        sub = (subtitle_t *) MALLOC(sizeof(subtitle_t));
        if (!sub) {
            break;
        }
        memset(sub, 0, sizeof(subtitle_t));
        sub->start = timeStart;
        sub->end = timeEnd;


        ALOGD("[parseTtml](%d,%d)%s", sub->start, sub->end, text);
        std::shared_ptr<AML_SPUVAR> spu(new AML_SPUVAR());
        spu->pts = sub->start;
        spu->m_delay = sub->end;
        int len = strlen(text);
        spu->spu_data = (unsigned char *)malloc(len);
        memset(spu->spu_data, 0, len);
        memcpy(spu->spu_data, text, len);
        spu->buffer_size = len;
        spu->isExtSub = true;
        addDecodedItem(std::shared_ptr<AML_SPUVAR>(spu));
    }
    doc.Clear();
    return 1;
}


