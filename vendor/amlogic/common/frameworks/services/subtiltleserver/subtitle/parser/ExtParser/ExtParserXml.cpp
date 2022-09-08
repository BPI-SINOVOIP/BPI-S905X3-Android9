#define LOG_TAG "ExtParserXml"

#include "ExtParserXml.h"
#include "tinyxml.h"
#include "tinystr.h"

ExtParserXml::ExtParserXml(std::shared_ptr<DataSource> source): ExtParser(source) {
    mDataSource = source;
    mExtParserUtils = new ExtParserUtils(0, source);
    mSubIndex = 0;
}

ExtParserXml::~ExtParserXml() {
    if (mExtParserUtils != NULL) {
        delete mExtParserUtils;
        mExtParserUtils = NULL;
    }
}

subtitle_t * ExtParserXml::ExtSubtitleParser(subtitle_t *current) {
    char line[LINE_LEN + 1];
    char line2[LINE_LEN + 1];
    char *p, *next;
    int i;
    int size = mDataSource->availableDataSize();
    char *rdBuffer = new char[size]();
    mDataSource->read(rdBuffer, size);
    //log_print("internal_sub_read_line_xml size:%d, buffer:%s\n", size, rdBuffer);
    parseXml(rdBuffer);
    return NULL;
}

int ExtParserXml::parseXml(char *buffer) {
    TiXmlDocument doc;
    doc.Parse(buffer);

    TiXmlElement* root = doc.FirstChildElement();
    if (root == NULL) {
        ALOGD("Failed to load file: No root element.\n");
        doc.Clear();
        return -1;
    }
    subtitle_t *sub;
    int timeStart, end;

    for (TiXmlElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement()) {
        TiXmlElement* e1 = elem->FirstChildElement("Number");
        const char* num = e1->GetText();
        ALOGD("parseXml num:%s\n", num);

        TiXmlElement* e2 = e1->NextSiblingElement();
        const char* startMilliseconds = e2->GetText();
        ALOGD("parseXml startMilliseconds:%s\n", startMilliseconds);
        int timeStart = atoi(startMilliseconds);


        TiXmlElement* e3 = e2->NextSiblingElement();
        const char* endMilliseconds = e3->GetText();
        ALOGD("parseXml endMilliseconds:%s\n", endMilliseconds);
        int timeEnd = atoi(endMilliseconds);

        TiXmlElement* e4 = e3->NextSiblingElement();
        const char* text = e4->GetText();
        ALOGD("parseXml text:%s\n", text);

        sub = (subtitle_t *) MALLOC(sizeof(subtitle_t));
        if (!sub) {
            break;
        }
        memset(sub, 0, sizeof(subtitle_t));
        //sub->end = rate;
        /* ms to pts conversion */
        sub->start = timeStart * 90;
        sub->end = timeEnd * 90;

        ALOGD("[parseXml](%d,%d)%s", sub->start, sub->end, text);
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

