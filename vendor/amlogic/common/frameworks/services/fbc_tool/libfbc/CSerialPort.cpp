/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "FBC"
#define LOG_FBC_TAG "CSerialPort"

#include "CSerialPort.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <errno.h>

static int com_read_data(int hComm, unsigned char *pData, unsigned int uLen)
{
    char inbuff[uLen];
    char buff[uLen];
    char tempbuff[uLen];
    memset(inbuff, '\0', uLen);
    memset(buff, '\0', uLen);
    memset(tempbuff, '\0', uLen);

    if (hComm < 0) {
        return -1;
    }

    char *p = inbuff;

    fd_set readset;
    struct timeval tv;
    int MaxFd = 0;

    unsigned int c = 0;
    int z;

    do {
        FD_ZERO(&readset);
        FD_SET(hComm, &readset);
        MaxFd = hComm + 1;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        do {
            z = select(MaxFd, &readset, 0, 0, &tv);
        } while (z == -1 && errno == EINTR);

        if (z == -1) {
            hComm = -1;
            break;
        }

        if (z == 0) {
            hComm = -1;
            break;
        }

        if (FD_ISSET(hComm, &readset)) {
            z = read(hComm, buff, uLen - c);
            c += z;

            if (z > 0) {
                if (z < (signed int) uLen) {
                    buff[z + 1] = '\0';
                    memcpy(p, buff, z);
                    p += z;
                } else {
                    memcpy(inbuff, buff, z);
                }

                memset(buff, '\0', uLen);
            } else {
                hComm = -1;
            }

            if (c >= uLen) {
                hComm = -1;
                break;
            }
        }
    } while (hComm >= 0);

    memcpy(pData, inbuff, c);
    p = NULL;
    return c;
}

CSerialPort::CSerialPort()
{
    mDevId = -1;
}

//close it
CSerialPort::~CSerialPort()
{
}

int CSerialPort::OpenDevice(int serial_dev_id)
{
    int tmp_ret = 0;
    const char *dev_file_name = NULL;

    if (getFd() < 0) {
        if (serial_dev_id == SERIAL_A) {
            dev_file_name = DEV_PATH_S0;
        } else if (serial_dev_id == SERIAL_B) {
            dev_file_name = DEV_PATH_S1;
        } else if (serial_dev_id == SERIAL_C) {
            dev_file_name = DEV_PATH_S2;
        }

        if (dev_file_name != NULL) {
            mDevId = serial_dev_id;
            tmp_ret = openFile(dev_file_name);
        }
    }

    return tmp_ret;
}

int CSerialPort::CloseDevice()
{
    mDevId = -1;
    closeFile();
    return 0;
}

void CSerialPort::set_speed (int fd, int speed)
{
    int i;
    int status;
    struct termios Opt;
    tcgetattr (fd, &Opt);
    for (i = 0; i < (int)(sizeof (speed_arr) / sizeof (int)); i++) {
        if (speed == name_arr[i]) {
            tcflush (fd, TCIOFLUSH);
            cfsetispeed (&Opt, speed_arr[i]);
            cfsetospeed (&Opt, speed_arr[i]);
            status = tcsetattr (fd, TCSANOW, &Opt);
            if (status != 0) {
                perror ("tcsetattr fd1");
                return;
            }
            tcflush (fd, TCIOFLUSH);
        }
    }
}

int CSerialPort::set_Parity (int fd, int databits, int stopbits, int parity)
{
    struct termios options;
    if (tcgetattr (fd, &options) != 0) {
        perror ("SetupSerial 1");
        return (0);
    }
    options.c_cflag &= ~CSIZE;
    switch (databits) {
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        fprintf (stderr, "Unsupported data size\n");
        return (0);
    }
    switch (parity) {
    case 'n':
    case 'N':
        options.c_cflag &= ~PARENB; /* Clear parity enable */
        options.c_iflag &= ~INPCK;  /* Enable parity checking */
        break;
    case 'o':
    case 'O':
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;   /* Disnable parity checking */
        break;
    case 'e':
    case 'E':
        options.c_cflag |= PARENB;  /* Enable parity */
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;   /* Disnable parity checking */
        break;
    case 'S':
    case 's':           /*as no parity */
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        fprintf (stderr, "Unsupported parity\n");
        return (0);
    }

    switch (stopbits) {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        fprintf (stderr, "Unsupported stop bits\n");
        return (0);
    }
    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;
    tcflush (fd, TCIFLUSH);
    options.c_cc[VTIME] = 150;
    options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
    //qd to set raw mode, which is copied from web
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                         | INLCR | IGNCR | ICRNL | IXON);
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_cflag &= ~(CSIZE | PARENB);
    options.c_cflag |= CS8;

    if (tcsetattr (fd, TCSANOW, &options) != 0) {
        perror ("SetupSerial 3");
        return (0);
    }
    return (1);
}

int CSerialPort::setup_serial(unsigned int baud_rate)
{
    set_speed(mFd, baud_rate);
    set_Parity(mFd, 8, 1, 'N');
    return 0;
}

int CSerialPort::set_opt(int speed, int db, int sb, char pb, int overtime, bool raw_mode)
{
    int i = 0;
    struct termios old_cfg, new_cfg;
    if (mFd <= 0) {
        //LOGE("not open dev, when set opt");
        return -1;
    }
    //first get it
    if (tcgetattr(mFd, &old_cfg) != 0) {
        //LOGE("get serial attr error mFd = %d(%s)!\n", mFd, strerror(errno));
        return -1;
    }

    //set speed
    for (i = 0; i < (int)(sizeof(speed_arr) / sizeof(int)); i++) {
        if (speed == name_arr[i]) {
            cfsetispeed(&new_cfg, speed_arr[i]);
            cfsetospeed(&new_cfg, speed_arr[i]);
            break;
        }
    }

    setdatabits(&new_cfg, db);
    setstopbits(&new_cfg, sb);
    setparity(&new_cfg, pb);

    if (overtime >= 0) {
        new_cfg.c_cc[VTIME] = overtime / 100;
        new_cfg.c_cc[VMIN] = 0; /* Update the options and do it NOW */
    }

    if (raw_mode) {
        cfmakeraw(&new_cfg);
    }

    //clear
    tcflush(mFd, TCIOFLUSH);
    if (tcsetattr(mFd, TCSANOW, &new_cfg) < 0) {
        //LOGE("%s, set serial attr error(%s)!\n", "TV", strerror(errno));
        return -1;
    }
    //clear,let be avail
    tcflush(mFd, TCIOFLUSH);
    return 0;
}

int CSerialPort::writeFile(const unsigned char *pData, int uLen)
{
    unsigned int len;
    len = write(mFd, pData, uLen);
    if (len == uLen) {
        return len;
    } else {
        tcflush(mFd, TCOFLUSH);
        //LOGE("write data failed and tcflush hComm\n");
        return -1;
    }
}

int CSerialPort::readFile(void *pBuf, int uLen)
{
    //using non-block mode
    return com_read_data(mFd, (unsigned char *)pBuf, uLen);
}

int CSerialPort::setdatabits(struct termios *s, int db)
{
    if (db == 5) {
        s->c_cflag = (s->c_cflag & ~CSIZE) | (CS5 & CSIZE);
    } else if (db == 6) {
        s->c_cflag = (s->c_cflag & ~CSIZE) | (CS6 & CSIZE);
    } else if (db == 7) {
        s->c_cflag = (s->c_cflag & ~CSIZE) | (CS7 & CSIZE);
    } else if (db == 8) {
        s->c_cflag = (s->c_cflag & ~CSIZE) | (CS8 & CSIZE);
    } else {
        //LOGE("Unsupported data size!\n");
    }
    return 0;
}

int CSerialPort::setstopbits(struct termios *s, int sb)
{
    if (sb == 1) {
        s->c_cflag &= ~CSTOPB;
    } else if (sb == 2) {
        s->c_cflag |= CSTOPB;
    } else {
        //LOGE("Unsupported stop bits!\n");
    }
    return 0;
}

int CSerialPort::setparity(struct termios *s, char pb)
{
    if (pb == 'n' || pb == 'N') {
        s->c_cflag &= ~PARENB; /* Clear parity enable */
        s->c_cflag &= ~INPCK; /* Enable parity checking */
    } else if (pb == 'o' || pb == 'O') {
        s->c_cflag |= (PARODD | PARENB);
        s->c_cflag |= INPCK; /* Disable parity checking */
    } else if (pb == 'e' || pb == 'E') {
        s->c_cflag |= PARENB; /* Enable parity */
        s->c_cflag &= ~PARODD;
        s->c_iflag |= INPCK; /* Disable parity checking */
    } else if (pb == 's' || pb == 'S') {
        s->c_cflag &= ~PARENB;
        s->c_cflag &= ~CSTOPB;
        s->c_cflag |= INPCK; /* Disable parity checking */
    } else {
        //LOGE("Unsupported parity!\n");
    }
    return 0;
}

