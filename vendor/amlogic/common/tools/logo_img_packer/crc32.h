/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 */
/*
 * crc32.h
 *
 *  Created on: 2013-5-31
 *      Author: binsheng.xu@amlogic.com
 */

#ifndef CRC32_H_
#define CRC32_H_

unsigned int crc32(unsigned int crc,unsigned char *buffer, unsigned int size);

unsigned calc_img_crc(FILE* fd, off_t offset, unsigned checkSz);

int check_img_crc(FILE* fp, off_t offset, const unsigned orgCrc, unsigned checkSz);

#endif /* CRC32_H_ */
