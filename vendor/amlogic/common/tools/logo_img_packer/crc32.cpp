/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 */
/*
 * crc32.c
 *
 *  Created on: 2013-5-31
 *      Author: binsheng.xu@amlogic.com
 */
#include "res_pack_i.h"

#define BUFSIZE     1024*16

static unsigned int crc_table[256];


static void init_crc_table(void)
{
    unsigned int c;
    unsigned int i, j;

    for (i = 0; i < 256; i++) {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[i] = c;
    }
}

//generate crc32 with buffer data
unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size)
{
    unsigned int i;
    for (i = 0; i < size; i++) {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ;
}

//Generate crc32 value with file steam, which from 'offset' to end if checkSz==0
unsigned calc_img_crc(FILE* fp, off_t offset, unsigned checkSz)
{

    unsigned char buf[BUFSIZE];

    unsigned int crc = 0xffffffff;
    unsigned MaxCheckLen = 0;
    unsigned totalLenToCheck = 0;

    if (fp == NULL) {
        fprintf(stderr,"bad param!!\n");
        return 0;
    }

    fseeko(fp, 0, SEEK_END);
    MaxCheckLen  = ftell(fp);
    MaxCheckLen -= offset;
    if(!checkSz){
            checkSz = MaxCheckLen;
    }
    else if(checkSz > MaxCheckLen){
            fprintf(stderr, "checkSz %u > max %u\n", checkSz, MaxCheckLen);
            return 0;
    }
    //fprintf(stderr,"L%d, checkSz=%u\n", __LINE__, checkSz);

    init_crc_table();
    fseeko(fp,offset,SEEK_SET);

    while(totalLenToCheck < checkSz)
    {
            int nread;
            unsigned leftLen = checkSz - totalLenToCheck;
            int thisReadSz = leftLen > BUFSIZE ? BUFSIZE : leftLen;

            nread = fread(buf,1, thisReadSz, fp);
            if (nread < 0) {
                    fprintf(stderr,"%d:read %s.\n", __LINE__, strerror(errno));
                    return 0;
            }
            crc = crc32(crc, buf, thisReadSz);

            totalLenToCheck += thisReadSz;
    }
    
    return crc;
}

int check_img_crc(FILE* fp, off_t offset, const unsigned orgCrc, unsigned checkSz/* checkSz from offset */)
{
    const unsigned genCrc = calc_img_crc(fp, offset, checkSz);

    if(genCrc != orgCrc)
    {
        fprintf(stderr,"%d:genCrc 0x%x != orgCrc 0x%x, error(%s).\n", __LINE__, genCrc, orgCrc, strerror(errno));
        return -1;
    }

    return 0;
}


