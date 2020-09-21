/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __CSERIAL_STREAM__
#define __CSERIAL_STREAM__
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include "CFile.h"
static const int speed_arr[] = {B230400, B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, B38400, B19200, B9600, B4800, B2400, B1200, B300};
static const int name_arr[] = { 230400, 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200, 9600, 4800, 2400, 1200, 300};
static const char *DEV_PATH_S0 = "/dev/ttyS0";
static const char *DEV_PATH_S1 = "/dev/ttyS1";
static const char *DEV_PATH_S2 = "/dev/ttyS2";

enum SerialDeviceID {
    SERIAL_A = 0,
    SERIAL_B,
    SERIAL_C,
};

class CSerialPort: public CFile {
public:
    CSerialPort();
    ~CSerialPort();

    int OpenDevice(int serial_dev_id);
    int CloseDevice();

    int writeFile(const unsigned char *pData, int uLen);
    int readFile(void*pBuf, int uLen);
    int set_opt(int speed, int db, int sb, char pb, int overtime, bool raw_mode);
    int setup_serial(unsigned int baud_rate);
    int getDevId()
    {
        return mDevId;
    };

private:
    int setdatabits(struct termios *s, int db);
    int setstopbits(struct termios *s, int sb);
    int setparity(struct termios *s, char pb);
    int set_Parity (int fd, int databits, int stopbits, int parity);
    void set_speed (int fd, int speed);

    int mDevId;
};
#endif
