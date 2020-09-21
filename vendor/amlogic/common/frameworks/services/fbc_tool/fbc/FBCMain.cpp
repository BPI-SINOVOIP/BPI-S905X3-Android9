/*
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "FBCCMD.h"
#include "CFbcLog.h"
#include "CFbcProtocol.h"

#define LOG_TAG "FBC"
#define LOG_FBC_TAG "fbc_tool"


int main(int argc, char **argv)
{
    if (argc < 2) {
        LOGI("Usage:");
        LOGI("   fbc reboot");
        LOGI("   fbc suspend");
        LOGI("   .");
        LOGI("   .");
        LOGI("   .");
        return -1;
    }

    CSerialPort serialPort;
    unsigned char read_buf[14];
    unsigned char write_buf[14];
    unsigned char value = 0; //reboot type is normal
    int crc32value;
    int idx = 0;

    memset(read_buf, 0, sizeof(read_buf));
    memset(write_buf, 0, sizeof(write_buf));

    switch (check_cmd(argv[1])) {
        case reboot:
            write_buf[0] = 0x5a;
            write_buf[1] = 0x5a;
            write_buf[2] = 14;
            write_buf[3] = 0x0;
            write_buf[4] = 0x0;
            write_buf[5] = 0x1; // fbc cmd: reboot

            //parameter
            write_buf[6] = (value >> 0) & 0xFF;
            write_buf[7] = (value >> 8) & 0xFF;
            write_buf[8] = (value >> 16) & 0xFF;
            write_buf[9] = (value >> 24) & 0xFF;
            break;
        case suspend:
            LOGE("Unsurport command!!!");
            return -1;
            break;
        default:
            LOGE("Unsurport command!!!");
            return -1;
            break;
    }

    //crc32 little Endian
    crc32value = GetFbcProtocolInstance()->Calcrc32(0, write_buf, 10);
    write_buf[10] = (crc32value >> 0) & 0xFF;
    write_buf[11] = (crc32value >> 8) & 0xFF;
    write_buf[12] = (crc32value >> 16) & 0xFF;
    write_buf[13] = (crc32value >> 24) & 0xFF;

    if (serialPort.OpenDevice(SERIAL_C) < 0) {
        LOGE("open serialport failed!!!\n");
        return -1;
    } else {
        serialPort.setup_serial(115200);
    }

    LOGD("send cmd to fbc ..........\n");
    serialPort.writeFile(write_buf, 14);

/*
    LOGD("read status from fbc ........\n");
    serialPort.readFile(read_buf, 14);

    for (idx = 0; idx < 14; idx++) {
        LOGD("the data is:0x%x\n", read_buf[idx]);
    }
*/
    serialPort.CloseDevice();


    return 0;
}
