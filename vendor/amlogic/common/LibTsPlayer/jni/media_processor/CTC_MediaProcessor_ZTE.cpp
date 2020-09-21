/* 
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#define LOG_NDEBUG 0
#define LOG_TAG "CTC_MediaProcessor_ZTE"
#include "CTC_Log.h"
#include <cutils/properties.h>

#include <CTC_MediaProcessor_ZTE.h>
#include "ITsPlayer.h"

namespace aml {

CTC_MediaProcessor_ZTE* GetMediaProcessor_ZTE(int channelNo, CTC_InitialParameter* initialParam)
{
    if (initialParam != nullptr) {
        if ((initialParam->interfaceExtension&CTC_EXTENSION_ZTE) == 0) {
            ALOGW("interfaceExtension does not set correctly!");
            initialParam->interfaceExtension |= CTC_EXTENSION_ZTE;
        }
    }

    return new CTC_MediaProcessor_ZTE(channelNo, initialParam);
}

CTC_MediaProcessor_ZTE::CTC_MediaProcessor_ZTE(int channelNo, CTC_InitialParameter* initialParam)
: CTC_MediaProcessor(channelNo, initialParam)
{

}

CTC_MediaProcessor_ZTE::~CTC_MediaProcessor_ZTE()
{

}








}
