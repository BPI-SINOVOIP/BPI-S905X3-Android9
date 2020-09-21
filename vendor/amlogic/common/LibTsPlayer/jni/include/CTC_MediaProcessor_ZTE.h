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

#ifndef _CTC_MEDIAPROCESSOR_ZTE_H_
#define _CTC_MEDIAPROCESSOR_ZTE_H_

#include "CTC_MediaProcessor.h"

namespace aml {
class CTC_MediaProcessor_ZTE;



CTC_MediaProcessor_ZTE* GetMediaProcessor_ZTE(int channelNo, CTC_InitialParameter* initialParam = nullptr);


class CTC_MediaProcessor_ZTE : public CTC_MediaProcessor
{
public:
    ~CTC_MediaProcessor_ZTE();

    /**
     * \brief SwitchAudioTrack_ZTE
     *
     * \param pAudioPara
     */
    void SwitchAudioTrack_ZTE(PAUDIO_PARA_T pAudioPara);

#ifdef TELECOM_QOS_SUPPORT
    void RegisterParamEvtCb(void *hander, IPTV_PLAYER_PARAM_Evt_e enEvt, IPTV_PLAYER_PARAM_EVENT_CB  pfunc);
#endif

private:
    friend CTC_MediaProcessor_ZTE* GetMediaProcessor_ZTE(int channelNo, CTC_InitialParameter* initialParam);
	CTC_MediaProcessor_ZTE(int channelNo, CTC_InitialParameter* initialParam);

private:
    CTC_MediaProcessor_ZTE(const CTC_MediaProcessor_ZTE&);
    CTC_MediaProcessor_ZTE& operator=(const CTC_MediaProcessor_ZTE&);
};



}
#endif
