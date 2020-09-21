/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Description:
 */

#ifndef __USB_FMT_C__
#define __USB_FMT_C__

#include <utils/Log.h>
#include "usb_fmt.h"
#include "DebugUtils.h"

struct usb_fmt_s{
        uint32_t                p;
        struct v4l2_frmsize_discrete d;
}usb_fmt_t;

struct usb_device_s {
        uint16_t        idVendor;
        uint16_t        idProduct;
        uint32_t        num; 
        struct usb_fmt_s *uf;
}usb_device_t;


static struct usb_fmt_s id_046d_082b[] = {
        {
                .p = V4L2_PIX_FMT_MJPEG,
                {
                        .width = 640,
                        .height = 480,
                }
        },{
                .p = V4L2_PIX_FMT_MJPEG,
                {
                        .width = 320,
                        .height = 240,
                }
        },
};

static struct usb_device_s aml_usb_pixelfmt[] = {
        {
                .idVendor = 0x046d,
                .idProduct= 0x082b,
                .num    = ARRAY_SIZE(id_046d_082b),
                .uf     = id_046d_082b,
        },
};

extern "C" uint32_t query_aml_usb_pixelfmt(uint16_t idVendor, uint16_t idProduct,
                uint16_t w, uint16_t h)
{
        int i;
        struct usb_fmt_s *uf;
        int num = 0;
#if 0
        CAMHAL_LOGIB("idVendor=%x, idProduct=%x, w=%d, h=%d\n",
                     idVendor,idProduct, w, h);
#endif

        for (i = 0; i<(int)ARRAY_SIZE(aml_usb_pixelfmt); i++){
#if 0
                CAMHAL_LOGIB("i=%d,idVendor=%x, idProduct=%x\n",
                             i, aml_usb_pixelfmt[i].idVendor,
                                aml_usb_pixelfmt[i].idProduct);
#endif

                if((aml_usb_pixelfmt[i].idVendor == idVendor) &&
                   (aml_usb_pixelfmt[i].idProduct == idProduct)){
                        num =aml_usb_pixelfmt[i].num;
                        uf = aml_usb_pixelfmt[i].uf;
                        break;
                }
        }

        if ((0 == num) || (NULL == uf)){
                return 0;
        }

        for (i = 0; i < num; i++){
#if 0
                CAMHAL_LOGIB("i=%d, w=%d, h=%d\n", i, uf[i].d.width, uf[i].d.height);
#endif
                if((w == uf[i].d.width) && (h == uf[i].d.height)){
                        return uf[i].p;
                }
        }
        return 0;
}
#endif
