#define  LOG_TAG    "DvdParser"

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <list>
#include <thread>
#include <algorithm>
#include <functional>

#include <utils/Log.h>

#include "streamUtils.h"
#include "sub_types.h"
#include "DvdParser.h"
#include "ParserFactory.h"
#include "VideoInfo.h"

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

typedef enum {
    FSTA_DSP = 0,
    STA_DSP = 1,
    STP_DSP = 2,
    SET_COLOR = 3,
    SET_CONTR = 4,
    SET_DAREA = 5,
    SET_DSPXA = 6,
    CHG_COLCON = 7,
    CMD_END = 0xFF,
} CommandID;

/**
 * Locale-independent conversion of ASCII isspace.
 */
static inline int __isspace(int c) {
    return c==' ' || c=='\f' || c=='\n' || c=='\r' || c=='\t' || c=='\v';
}

static void save2BitmapFile(const char *filename, uint32_t *bitmap, int w, int h) {
    LOGI("save2BitmapFile:%s\n",filename);
    FILE *f;
    char fname[40];

    snprintf(fname, sizeof(fname), "%s.bmp", filename);
    f = fopen(fname, "w");
    if (!f) {
        perror(fname);
        return;
    }
    fprintf(f, "P6 %d %d %d", w, h, 255);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int v = bitmap[y * w + x];
            putc((v >> 16) & 0xff, f);
            putc((v >> 8) & 0xff, f);
            putc((v >> 0) & 0xff, f);
        }
    }
    fclose(f);
}

static inline unsigned short doDCSQC(unsigned char *pdata, unsigned char *pend) {
    unsigned short cmdDelay, cmdDelaynew;
    unsigned short temp;
    unsigned short cmdAddress;
    int Done, stoped;

    cmdDelay = *pdata++;
    cmdDelay <<= 8;
    cmdDelay += *pdata++;
    cmdAddress = *pdata++;
    cmdAddress <<= 8;
    cmdAddress += *pdata++;
    cmdDelaynew = 0;
    Done = 0;
    stoped = 0;
    while (!Done) {
        switch (*pdata) {
            case FSTA_DSP:
                pdata++;
                break;
            case STA_DSP:
                pdata++;
                break;
            case STP_DSP:
                pdata++;
                stoped = 1;
                break;
            case SET_COLOR:
                pdata += 3;
                break;
            case SET_CONTR:
                pdata += 3;
                break;
            case SET_DAREA:
                pdata += 7;
                break;
            case SET_DSPXA:
                pdata += 7;
                break;
            case CHG_COLCON:
                temp = *pdata++;
                temp = temp << 8;
                temp += *pdata++;
                pdata += temp;
                break;
            case CMD_END:
                pdata++;
                Done = 1;
                break;
            default:
                pdata = pend;
                Done = 1;
                break;
        }
    }
    if ((pdata < pend) && (stoped == 0))
        cmdDelaynew = doDCSQC(pdata, pend);
    return cmdDelaynew > cmdDelay ? cmdDelaynew : cmdDelay;
}

static inline int getSpuCmd(std::shared_ptr<AML_SPUVAR> subFrame) {
    unsigned short temp;
    unsigned char *pCmdData;
    unsigned char *pCmdEnd;
    unsigned char dataByte0, dataByte1;
    unsigned char spuCmd;
    if (subFrame->cmd_offset >= subFrame->length) {
        LOGI("cmd_offset bigger than frame_size\n\n");
        return -1;  //cmd offset > frame size
    }
    pCmdData = (unsigned char *)(subFrame->spu_data);
    pCmdEnd = pCmdData + subFrame->length;
    pCmdData += subFrame->cmd_offset;
    pCmdData += 4;
    while (pCmdData < pCmdEnd) {
        spuCmd = *pCmdData++;
        switch (spuCmd) {
            case FSTA_DSP:
                subFrame->display_pending = 2;
                break;
            case STA_DSP:
                subFrame->display_pending = 1;
                break;
            case STP_DSP:
                subFrame->display_pending = 0;
                break;
            case SET_COLOR:
                temp = *pCmdData++;
                subFrame->spu_color = temp << 8;
                temp = *pCmdData++;
                subFrame->spu_color += temp;
                break;
            case SET_CONTR:
                temp = *pCmdData++;
                subFrame->spu_alpha = temp << 8;
                temp = *pCmdData++;
                subFrame->spu_alpha += temp;
                break;
            case SET_DAREA:
                dataByte0 = *pCmdData++;
                dataByte1 = *pCmdData++;
                subFrame->spu_start_x = ((dataByte0 & 0x3f) << 4) | (dataByte1 >> 4);
                dataByte0 = *pCmdData++;
                subFrame->spu_width = ((dataByte1 & 0x0f) << 8) | (dataByte0);
                subFrame->spu_width = subFrame->spu_width - subFrame->spu_start_x + 1;
                dataByte0 = *pCmdData++;
                dataByte1 = *pCmdData++;
                subFrame->spu_start_y = ((dataByte0 & 0x3f) << 4) | (dataByte1 >> 4);
                dataByte0 = *pCmdData++;
                subFrame->spu_height = ((dataByte1 & 0x0f) << 8) | (dataByte0);
                subFrame->spu_height = (unsigned short)((int)subFrame->spu_height - (int)subFrame->spu_start_y + 1);
                if (subFrame->spu_width > VOB_SUB_WIDTH) {
                    subFrame->spu_width = VOB_SUB_WIDTH;
                }
                if (subFrame->spu_height > VOB_SUB_HEIGHT) {
                    subFrame->spu_height = VOB_SUB_HEIGHT;
                }
                LOGI("subFrame->spu_width = %d, subFrame->spu_height = %d \n", subFrame->spu_width, subFrame->spu_height);
                break;
            case SET_DSPXA:
                temp = *pCmdData++;
                subFrame->top_pxd_addr = temp << 8;
                temp = *pCmdData++;
                subFrame->top_pxd_addr += temp;
                temp = *pCmdData++;
                subFrame->bottom_pxd_addr = temp << 8;
                temp = *pCmdData++;
                subFrame->bottom_pxd_addr += temp;
                break;
            case CHG_COLCON:
                temp = *pCmdData++;
                temp = temp << 8;
                temp += *pCmdData++;
                pCmdData += temp;
                /*
                   uVobSPU.disp_colcon_addr = uVobSPU.point + uVobSPU.point_offset;
                   uVobSPU.colcon_addr_valid = 1;
                   temp = uVobSPU.disp_colcon_addr + temp - 2;

                   uSPU.point = temp & 0x1fffc;
                   uSPU.point_offset = temp & 3;
                 */
                break;
            case CMD_END:
                if (pCmdData <= (pCmdEnd - 6)) {
                    if ((subFrame->m_delay = doDCSQC(pCmdData, pCmdEnd - 6)) > 0)
                        subFrame->m_delay = subFrame->m_delay*1024 + subFrame->pts;
                }
                LOGI("getSpuCmd parser to the end\n\n");
                return 0;
            default:
                return -1;
        }
    }
    LOGI("getSpuCmd can not parser complete\n\n");
    return -1;
}


static unsigned short DecodeRL(unsigned short RLData, unsigned short *pixelnum, unsigned short *pixeldata) {
    unsigned short nData = RLData;
    unsigned short nShiftNum;
    unsigned short nDecodedBits;
    if (nData & 0xc000)
        nDecodedBits = 4;
    else if (nData & 0x3000)
        nDecodedBits = 8;
    else if (nData & 0x0c00)
        nDecodedBits = 12;
    else
        nDecodedBits = 16;

    nShiftNum = 16 - nDecodedBits;
    *pixeldata = (nData >> nShiftNum) & 0x0003;
    *pixelnum = nData >> (nShiftNum + 2);
    return nDecodedBits;
}

static unsigned short GetWordFromPixBuffer(unsigned short bitpos, unsigned short *pixelIn) {
    unsigned char hi = 0, lo = 0, hi_ = 0, lo_ = 0;
    char *tmp = (char *)pixelIn;
    hi = *(tmp + 0);
    lo = *(tmp + 1);
    hi_ = *(tmp + 2);
    lo_ = *(tmp + 3);
    if (bitpos == 0) {
        return (hi << 0x8 | lo);
    } else {
        // this overflow is intented, cast to u16 to avoid asan report it
        return ((unsigned short)((hi << 0x8 | lo) << bitpos) |
                (unsigned short)((hi_ << 0x8 | lo_) >> (16 - bitpos)));
    }
}

int getInterSpuSize(std::shared_ptr<AML_SPUVAR> spu) {
    int subtitleWidth = spu->spu_width;
    int subtitleHeight = spu->spu_height;
    if (subtitleWidth * subtitleHeight == 0) {
        return 0;
    }

    int bufferWidth = subtitleWidth;
    LOGI("buffer width is %d\n", bufferWidth);
    LOGI("buffer height is %d\n", subtitleHeight);
    return bufferWidth * subtitleHeight;
}


int fillResizedData(unsigned char *dstData, int *srcData, std::shared_ptr<AML_SPUVAR> spu) {
    if (spu->resize_size == getInterSpuSize(spu)) {
        memcpy(dstData, srcData, spu->resize_size * 4);
        return 0;
    }
    int yStart = spu->resize_ystart;
    int xStart = spu->resize_xstart;
    int yEnd = yStart + spu->resize_height;
    int resizeWidth = spu->resize_width;
    int bufferWidth = spu->spu_width;
    int bufferWidthSize = bufferWidth;
    int *resizeSrcData = srcData + bufferWidthSize * yStart;
    int i = yStart;
    for (; i < yEnd; i++) {
        memcpy(dstData + (resizeWidth * (i - yStart)),
               resizeSrcData + (bufferWidthSize * (i - yStart)) +
               xStart, resizeWidth * 4);
    }
    return 0;
}


void DvdParser::checkDebug() {
       //dump dvd subtitle bitmap
       char value[PROPERTY_VALUE_MAX] = {0};
       memset(value, 0, PROPERTY_VALUE_MAX);
       property_get("vendor.subtitle.dump", value, "false");
       if (!strcmp(value, "true")) {
           mDumpSub = true;
       }
}


DvdParser::DvdParser(std::shared_ptr<DataSource> source) {
    LOGI("%s", __func__);
    mDataSource = source;
    mParseType = TYPE_SUBTITLE_VOB;
    mDumpSub = false;

    mRestLen = 0;
    mRestbuf = nullptr;
    mHasPalette = false;

    for (int i = 0; i < 16; i++) {
        mPalette[i] = 0;
    }
}

DvdParser::~DvdParser() {
    LOGI("%s", __func__);
    stopParse();
}


void DvdParser::parsePalette(char *p) {
    int i;
    mHasPalette = 1;
    for (i = 0; i < 16; i++) {
        mPalette[i] = strtoul(p, &p, 16);
        while (*p == ',' || __isspace(*p))
            p++;
    }
}

int DvdParser::parseExtradata(char *extradata, int extradataSize) {
    char *dataorig, *data;
    if (!extradata || !extradataSize) {
        return 1;
    }
    extradata += 5;
    extradataSize -= 5;
    dataorig = data = (char *)malloc(extradataSize + 1);
    if (!data) return 1;

    memcpy(data, extradata, extradataSize);
    data[extradataSize] = '\0';
    for (;;) {
        int pos = strcspn(data, "\n\r");
        if (pos == 0 && *data == 0)
            break;

        if (strncmp("palette:", data, 8) == 0) {
            parsePalette(data + 8);
        }
        data += pos;
        data += strspn(data, "\n\r");
    }
    free(dataorig);
    return 1;
}


unsigned int *DvdParser::parseInterSpu(int *buffer, std::shared_ptr<AML_SPUVAR> spu) {
    LOGI("enter parser_inter_sup \n\n");
    unsigned char *data = NULL;
    unsigned *resultBuf = (unsigned *)buffer;
    unsigned index = 0, index1 = 0;
    unsigned char n = 0;
    unsigned short bufferWidth, bufferHeight;
    int startHeight = -1, endHeight = 0;

    bufferWidth = spu->spu_width;
    bufferHeight = spu->spu_height;

    int xStart = bufferWidth, xEnd = 0;

    unsigned dataByte = (((bufferWidth * 2) + 15) >> 4) << 1;
    //LOGI("dataByte is %d\n\n",dataByte);
    int bufferWidthSize = bufferWidth;

    unsigned short subtitleAlpha = spu->spu_alpha;
    LOGI("subtitleAlpha is %x\n\n", subtitleAlpha);
    unsigned int RGBA_Pal[4];
    RGBA_Pal[0] = RGBA_Pal[1] = RGBA_Pal[2] = RGBA_Pal[3] = 0;

    if (subtitleAlpha & 0xf000 && subtitleAlpha & 0x0f00 && subtitleAlpha & 0x00f0) {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[2] = 0xff000000;
        RGBA_Pal[3] = 0xff000000;
    } else if (subtitleAlpha == 0xfe0) {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[2] = 0xff000000;
        RGBA_Pal[3] = 0;
    } else {
        RGBA_Pal[1] = 0xffffffff;
        RGBA_Pal[3] = 0xff000000;
    }
    for (int i = 0; i < bufferHeight; i++) {
        if (i & 1) {
            data = spu->spu_data + (i >> 1) * dataByte + (VOB_SUB_SIZE / 2);
        } else {
            data = spu->spu_data + (i >> 1) * dataByte;
        }

        index = 0;
        for (int j = 0; j < bufferWidth; j++) {
            index1 = index % 2 ? index - 1 : index + 1;
            n = data[index1];
            index++;
            if (startHeight < 0) {
                startHeight = i;
                //startHeight = (startHeight%2)?(startHeight-1):startHeight;
            }
            endHeight = i;
            if (j < xStart) xStart = j;

            resultBuf[i * (bufferWidthSize) + j] = RGBA_Pal[(n >> 6) & 0x3];
            if (++j >= bufferWidth) break;

            resultBuf[i * (bufferWidthSize) + j] = RGBA_Pal[(n >> 4) & 0x3];
            if (++j >= bufferWidth) break;

            resultBuf[i * (bufferWidthSize) + j] = RGBA_Pal[(n >> 2) & 0x3];
            if (++j >= bufferWidth) break;

            resultBuf[i * (bufferWidthSize) + j] = RGBA_Pal[n & 0x3];
            if (j > xEnd) xEnd = j;
        }
    }
    spu->resize_xstart = xStart;
    spu->resize_ystart = startHeight;
    spu->resize_width = bufferWidth;
    spu->resize_height = endHeight - startHeight + 1;
    spu->resize_size = spu->resize_height * spu->resize_width;
    LOGI("resize startx is %d\n\n",spu->resize_xstart);
    LOGI("resize starty is %d\n\n",spu->resize_ystart);
    LOGI("resize height is %d\n\n",spu->resize_height);
    LOGI("resizeWidth is %d\n\n",spu->resize_width);

    return (resultBuf + startHeight * bufferWidthSize);
}


void DvdParser::fillResize(std::shared_ptr<AML_SPUVAR> spu) {
    int subSize = getInterSpuSize(spu);
    if (subSize <= 0) {
        LOGE("subSize invalid \n\n");
        return;
    }

    int *internalSubData = NULL;
    internalSubData = (int *)malloc(subSize * 4);
    if (internalSubData == NULL) {
        LOGE("malloc subSize fail \n\n");
        return;
    }
    memset(internalSubData, 0x0, subSize * 4);
    parseInterSpu(internalSubData, spu);
    if (spu->spu_data) {
        free(spu->spu_data);
        spu->spu_data = NULL;
    }

    spu->spu_data = (unsigned char *)malloc(spu->resize_size * 4);

    spu->spu_width = spu->resize_width;
    spu->spu_height = spu->resize_height;
    spu->buffer_size = spu->resize_size * 4; // actuall buffer size.

    fillResizedData(spu->spu_data, internalSubData, spu);
    free(internalSubData);
}



unsigned char DvdParser::spuFillPixel(unsigned short *pixelIn, char *pixelOut,
                             std::shared_ptr<AML_SPUVAR> subFrame, int n)
{
    unsigned short nPixelNum = 0, nPixelData = 0;
    unsigned short nRLData, nBits;
    unsigned short nDecodedPixNum = 0;
    unsigned short PXDBufferBitPos = 0, WrOffset = 16;
    unsigned short change_data = 0;
    unsigned short PixelDatas[4] = { 0, 1, 2, 3 };
    unsigned short rownum = subFrame->spu_width;
    unsigned short height = subFrame->spu_height;
    unsigned short _alpha = subFrame->spu_alpha;
    static unsigned short *ptrPXDWrite;
    memset(pixelOut, 0, VOB_SUB_SIZE / 2);
    ptrPXDWrite = (unsigned short *)pixelOut;
    if (_alpha & 0xF) {
        _alpha = _alpha >> 4;
        change_data++;
        while (_alpha & 0xF) {
            change_data++;
            _alpha = _alpha >> 4;
        }
        PixelDatas[0] = change_data;
        PixelDatas[change_data] = 0;
        if (n == 2) {
            subFrame->spu_alpha = (subFrame->spu_alpha & 0xFFF0) | (0x000F << (change_data<<2));
        }
    }

    for (int j = 0; j < height/2; j++) {
        while (nDecodedPixNum < rownum) {
            //ALOGD("%p %d j=%d  PXDBufferBitPos=%d", pixelIn, nDecodedPixNum, j, PXDBufferBitPos);
            nRLData = GetWordFromPixBuffer(PXDBufferBitPos, pixelIn);
            nBits = DecodeRL(nRLData, &nPixelNum, &nPixelData);
            PXDBufferBitPos += nBits;
            if (PXDBufferBitPos >= 16) {
                PXDBufferBitPos -= 16;
                pixelIn++;
            }

            if (nPixelNum == 0) {
                nPixelNum = rownum - nDecodedPixNum % rownum;
            }

            if (change_data) {
                nPixelData = PixelDatas[nPixelData];
            }

            for (int i = 0; i<nPixelNum; i++) {
                WrOffset -= 2;
                *ptrPXDWrite |= nPixelData << WrOffset;
                if (WrOffset == 0) {
                    WrOffset = 16;
                    ptrPXDWrite++;
                }
            }
            nDecodedPixNum += nPixelNum;
        }

        //Rule 6
        if (PXDBufferBitPos == 4) {
            PXDBufferBitPos = 8;
        } else if (PXDBufferBitPos == 12) {
            PXDBufferBitPos = 0;
            pixelIn++;
        }

        if (WrOffset != 16) {
            WrOffset = 16;
            ptrPXDWrite++;
        }
        nDecodedPixNum -= rownum;
    }
    return 0;
}

int DvdParser::getVobSpu(char *spuBuffer, int *bufsize, unsigned length, std::shared_ptr<AML_SPUVAR> spu) {
    int rdOffset = 0, wrOffset;
    unsigned currentLen = length;
    int ret = -1;
    char *pixDataOdd = NULL;
    char *pixDataEven = NULL;
    unsigned short *ptrPXDRead;

    if (spuBuffer[0] == 'E' && spuBuffer[1] == 'X' && spuBuffer[2] == 'T' && spuBuffer[3] == 'R' && spuBuffer[4] == 'A') {
        LOGI("## extradata %x,%x,%x,%x, %x,%x,%x,%x, %x,%x,%x,%x, %x,%x,%x,%x, ",
                spuBuffer[0], spuBuffer[1], spuBuffer[2], spuBuffer[3],
                spuBuffer[4], spuBuffer[5], spuBuffer[6], spuBuffer[7],
                spuBuffer[8], spuBuffer[9], spuBuffer[10], spuBuffer[11],
                spuBuffer[12], spuBuffer[13], spuBuffer[14], spuBuffer[15]);
        parseExtradata(spuBuffer, currentLen);
        rdOffset += currentLen;
        currentLen = 0;
        *bufsize -= rdOffset;
        return 0;
    }

    spu->length = spuBuffer[0] << 8;
    spu->length |= spuBuffer[1];
    spu->cmd_offset = spuBuffer[2] << 8;
    spu->cmd_offset |= spuBuffer[3];
    memset(spu->spu_data, 0, VOB_SUB_SIZE);

    wrOffset = 0;
    while (spu->length - wrOffset > 0) {
        if (!currentLen) {
            LOGI("currentLen is zero\n\n");


            int syncWord = subPeekAsInt32(spuBuffer + rdOffset);
            rdOffset += 4;

            if (AML_PARSER_SYNC_WORD != syncWord) {
                ALOGE("Wrong sync header: %x! ignore....", syncWord);
                *bufsize -= rdOffset;
                return rdOffset; // wrong head, ignore and retry
            }

            rdOffset++;//0xaa or 0x77
            rdOffset += 3; // 3 bytes for type

            currentLen = subPeekAsInt32(spuBuffer + rdOffset);
            rdOffset += 4;

            // skip 4 bytes for pts
            rdOffset += 4;
        }

        if ((wrOffset + currentLen) <= spu->length) {
            memcpy(spu->spu_data + wrOffset, spuBuffer + rdOffset, currentLen);
            rdOffset += currentLen;
            wrOffset += currentLen;
            currentLen = 0;
        } else {
            break;
        }

        if (wrOffset == spu->length) {
            getSpuCmd(spu);
            spu->frame_rdy = 1;
            break;
        }
    }

    *bufsize -= rdOffset;

    // if one frame data is ready, decode it.
    LOGI("spu->frame_rdy is %d, mRestLen=%d, rdOffset=%d", spu->frame_rdy, *bufsize, rdOffset);
    if (spu->frame_rdy == 1) {
        pixDataOdd = (char *)malloc(VOB_SUB_SIZE / 2);
        ptrPXDRead = (unsigned short *)(spu->spu_data + spu->top_pxd_addr);

        LOGI("pixDataOdd is %p ptrPXDRead=%p top_pxd_addr=%d", pixDataOdd, ptrPXDRead, spu->top_pxd_addr);
        spuFillPixel(ptrPXDRead, pixDataOdd, spu, 1);

        pixDataEven = (char *)malloc(VOB_SUB_SIZE / 2);
        ptrPXDRead = (unsigned short *)(spu->spu_data + spu->bottom_pxd_addr);

        LOGI("pixDataEven is %p ptrPXDRead=%p bottom_pxd_addr=%d", pixDataEven, ptrPXDRead, spu->bottom_pxd_addr);
        spuFillPixel(ptrPXDRead, pixDataEven, spu, 2);

        memset(spu->spu_data, 0, VOB_SUB_SIZE);
        LOGI("###copy spu_data###\n\n");

        memcpy(spu->spu_data, pixDataOdd, VOB_SUB_SIZE / 2);
        memcpy(spu->spu_data + VOB_SUB_SIZE / 2, pixDataEven, VOB_SUB_SIZE / 2);
        ret = 0;
    }

    if (pixDataOdd) {
        free(pixDataOdd);
    }

    if (pixDataEven) {
        free(pixDataEven);
    }

    return rdOffset;
}



int DvdParser::getSpu(std::shared_ptr<AML_SPUVAR> spu) {
    int ret = -1, size;
    char *spuBuffer = NULL;
    unsigned currentLen, currentPts, currentType, durationPts;

    if (mState == SUB_INIT) {
        mState = SUB_PLAYING;
    } else if (mState == SUB_STOP) {
        ALOGD(" subtitle_status == SUB_STOP \n\n");
        return 0;
    }

    size = mDataSource->availableDataSize();

    if (size <= 0) {
        return -1;
    } else {
        size += mRestLen;
        currentType = 0;
        spuBuffer = (char *)malloc(size);
    }

    unsigned int sizeflag = size;
    char *tmpSpuBuf = spuBuffer;

    while (sizeflag > 30) {
        LOGI("sizeflag =%u  mRestLen=%d,", sizeflag,  mRestLen);
        if (sizeflag <= 16) {
            ret = -1;
            LOGI(" sizeflag is too little ");
            free(spuBuffer);
            return -1;
        }

        char *spuBufPiece = tmpSpuBuf;
        if (mRestLen > 0) {
            memcpy(spuBufPiece, mRestbuf, mRestLen);
        }

        if ((currentType == 0x17000 || currentType == 0x1700a) && mRestLen > 0) {
            LOGI("decode rest data!\n");
        } else {
            mDataSource->read(spuBufPiece + mRestLen, 16);
            sizeflag -= 16;
            tmpSpuBuf += 16;
        }

        int rdOffset = 0;
        int syncWord = subPeekAsInt32(spuBuffer + rdOffset);
        rdOffset += 4;

        if (AML_PARSER_SYNC_WORD != syncWord) {
            LOGI("\n wrong subtitle header : %x %x %x %x    %x %x %x %x    %x %x %x %x \n",
                    spuBufPiece[0], spuBufPiece[1], spuBufPiece[2], spuBufPiece[3],
                    spuBufPiece[4], spuBufPiece[5], spuBufPiece[6], spuBufPiece[7],
                    spuBufPiece[8], spuBufPiece[9], spuBufPiece[10], spuBufPiece[11]);
            mDataSource->read(spuBufPiece, sizeflag);
            sizeflag = 0;
            LOGI("\n\n ******* find wrong subtitle header!! ******\n\n");
            free(spuBuffer);
            return -1;
        }
        // ignore first sync byte: 0xAA/0x77
        currentType = subPeekAsInt32(spuBuffer + rdOffset) & 0x00FFFFFF;
        rdOffset += 4;
        currentLen = subPeekAsInt32(spuBuffer + rdOffset);
        rdOffset += 4;
        currentPts = subPeekAsInt32(spuBuffer + rdOffset);
        rdOffset += 4;

        // update restLen
        if (mRestLen) mRestLen -= 4*4;

        LOGI("sizeflag=%u, currentType:%x, currentPts is %x, currentLen is %d, \n",
                sizeflag, currentType, currentPts, currentLen);

        if (currentLen > sizeflag) {
            LOGI("currentLen > size");
            mDataSource->read(spuBufPiece, sizeflag);
            sizeflag = 0;
            free(spuBuffer);
            return -1;

        }

        if (currentType == 0x17000 || currentType == 0x1700a) {
            mDataSource->read(spuBufPiece + mRestLen + 16, sizeflag - mRestLen);
            mRestLen = sizeflag;
            sizeflag = 0;
            tmpSpuBuf += currentLen;
            LOGI("currentType=0x17000 or 0x1700a! mRestLen=%d, sizeflag=%d,\n", mRestLen, sizeflag);
        } else {
            mDataSource->read(spuBufPiece + 16, currentLen + 4);
            sizeflag -= (currentLen + 4);
            tmpSpuBuf += (currentLen + 4);
            mRestLen = 0;
        }

         ALOGD("currentType? %x", currentType);

        switch (currentType) {
            case 0x1700a:   //mkv internel image
                durationPts = subPeekAsInt32(spuBuffer + rdOffset);
                rdOffset += 4;
                mRestLen -= 4;
                LOGI("durationPts is %d\n", durationPts);
                [[fallthrough]];
            case 0x17000:   //vob internel image
                //init
                spu->subtitle_type = TYPE_SUBTITLE_VOB;
                spu->buffer_size = VOB_SUB_SIZE;
                spu->spu_data = (unsigned char *)malloc(VOB_SUB_SIZE);
                memset(spu->spu_data, 0, VOB_SUB_SIZE);
                ALOGD("alloc spuData:%p size:%d", spu->spu_data, spu->buffer_size);
                spu->pts = currentPts;
                ret = getVobSpu(spuBufPiece + rdOffset, &mRestLen, currentLen, spu);
                if (currentType == 0x17000 || currentType == 0x1700a) {
                    LOGI("## ret=%d, mRestLen=%d, sizeflag=%d, mRestbuf=%p, %x, ---\n",
                            ret, mRestLen, sizeflag, mRestbuf, mRestbuf?mRestbuf[0]:0);
                    if (mRestLen < 0) {
                        LOGI("Warning mRestLen <0, set to 0\n");
                        mRestLen = 0;
                    }
                    if (mRestLen) {
                        if (mRestbuf) {
                            free(mRestbuf);
                            mRestbuf = NULL;
                        }

                        mRestbuf = (char *)malloc(mRestLen);
                        ALOGD("spuBufPiece: %p, rdoff:%d ret:%d, mRestLen=%d", spuBufPiece, rdOffset, ret, mRestLen);
                        memcpy(mRestbuf, spuBufPiece + rdOffset + ret, mRestLen);

                        int syncWord = subPeekAsInt32(mRestbuf);
                        if ((AML_PARSER_SYNC_WORD == syncWord) && ((mRestbuf[4] == 0xaa) || (mRestbuf[4] == 0x77))) {
                            LOGI("## sub header found ! mRestbuf=%p, %x, ---\n", mRestbuf, mRestbuf[0]);
                            sizeflag = mRestLen;
                        } else {
                            LOGI("## no header found, free mRestbuf! ---\n");
                            free(mRestbuf);
                            mRestbuf = NULL;
                            mRestLen = sizeflag = 0;
                        }
                    }
                } else {
                    mRestLen = 0;
                }
                break;
            default:
                ret = -1;
                break;
        }
        LOGI("get_spu ret: %d\n", ret);

        if (ret < 0) break;

        fillResize(spu);

        if (mDumpSub) {
            char filename[64];
            snprintf(filename, sizeof(filename), "./data/subtitleDump/dvd(%d)", spu->pts);
            save2BitmapFile(filename, (uint32_t *)spu->spu_data, spu->spu_width, spu->spu_height);
        }

        if (spu->spu_data && spu->buffer_size > 0) {
            if (spu->spu_origin_display_w <= 0 || spu->spu_origin_display_h <= 0) {
                spu->spu_origin_display_w = VideoInfo::Instance()->getVideoWidth();
                spu->spu_origin_display_h = VideoInfo::Instance()->getVideoHeight();
            }
            if (spu->spu_origin_display_w <= 0 || spu->spu_origin_display_h <= 0) {
                spu->spu_origin_display_w = 720;
                spu->spu_origin_display_h = 576;
            }
            addDecodedItem(std::shared_ptr<AML_SPUVAR>(spu));
        } else {
            ALOGE("decoded invalid subtitle!");
        }
    }

    if (spuBuffer) free(spuBuffer);

    return ret;
}


int DvdParser::getInterSpu() {
    std::shared_ptr<AML_SPUVAR> spu(new AML_SPUVAR());

    spu->sync_bytes = AML_PARSER_SYNC_WORD;
    return getSpu(spu);
}


int DvdParser::parse() {
    while (!mThreadExitRequested) {
        if (getInterSpu() < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    return 0;
}

void DvdParser::dump(int fd, const char *prefix) {
    dprintf(fd, "%s Dvd(VOB) Parser\n", prefix);
    dumpCommon(fd, prefix);
}

