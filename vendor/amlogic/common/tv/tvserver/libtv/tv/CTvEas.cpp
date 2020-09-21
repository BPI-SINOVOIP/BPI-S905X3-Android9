/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvEas"

#include "CTvEas.h"

CTvEas *CTvEas::mInstance;
CTvEas *CTvEas::GetInstance()
{
    LOGD("start eas action!\n");
    if (mInstance == NULL) {
        mInstance = new CTvEas();
    }

    return mInstance;
}

CTvEas::CTvEas()
{
    mpObserver          = NULL;
    mDmxId              = INVALID_ID;
}

CTvEas::~CTvEas()
{
    if (mInstance != NULL) {
        delete mInstance;
        mInstance = NULL;
    }
}

/**
 * @Function: StartEasUpdate
 * @Description: Start Update EAS info
 * @Param:
 * @Return:0 success, -1 fail
 */
int CTvEas::StartEasUpdate()
{
#ifdef SUPPORT_ADTV
    int val = 0;
    val = EasCreate(0, 2, 0, NULL);    //2 is demux id which according to DVB moudle!
    if (val < 0) {
        LOGD("EasCreate failed!\n");
        return -1;
    }

    AM_ErrorCode_t ret;
    ret = AM_EPG_ChangeMode(mEasScanHandle, MODE_ADD, SCAN_PSIP_CEA);
    if (ret != DVB_SUCCESS) {
        LOGD("StartEasUpdate failed!\n");
        return -1;
    }
#endif
    LOGD("StartEasUpdate success!\n");
    return 0;
}

/**
 * @Function: StopEasUpdate
 * @Description: Stop Update EAS info
 * @Param:
 * @Return: 0 success, -1 fail
 */
int CTvEas::StopEasUpdate()
{
#ifdef SUPPORT_ADTV
    AM_ErrorCode_t ret;
    ret = AM_EPG_ChangeMode(mEasScanHandle, MODE_REMOVE, SCAN_PSIP_CEA);
    if (ret != DVB_SUCCESS) {
        LOGD("StopEasUpdate failed!\n");
    } else {
        LOGD("StopEasUpdate success!\n");
    }

    int val = 0;
    val = EasDestroy();
    if (val < 0) {
        LOGD("EasDestroy failed!\n");
        return -1;
    }
#endif
    return 0;
}

/**
 * @Function: EasCreate
 * @Description: open dev for EAS and set EAS event
 * @Param: fend_id: fend dev id;
           dmx_id:  demux dev id;
           src:     source;
           textLangs:language;
 * @Return: 0 success, -1 fail
 */
int CTvEas::EasCreate(int fend_id, int dmx_id, int src, char *textLangs)
{
#ifdef SUPPORT_ADTV
    AM_EPG_CreatePara_t para;
    AM_ErrorCode_t ret;
    AM_DMX_OpenPara_t dmx_para;

    LOGD("Opening demux%d ...", dmx_id);
    mDmxId = dmx_id;
    memset(&dmx_para, 0, sizeof(dmx_para));
    ret = AM_DMX_Open(dmx_id, &dmx_para);
    if (ret != DVB_SUCCESS) {
        LOGD("AM_DMX_Open failed");
        return - 1;
    }

    para.fend_dev = fend_id;
    para.dmx_dev  = dmx_id;
    para.source   = src;
    para.hdb      = NULL;

    snprintf(para.text_langs, sizeof(para.text_langs), "%s", textLangs);

    ret = AM_EPG_Create(&para, &mEasScanHandle);
    if (ret != DVB_SUCCESS) {
        LOGD("AM_EPG_Create failed!\n");
        return -1;
    }

    AM_EVT_Subscribe((long)mEasScanHandle, AM_EPG_EVT_NEW_CEA, EasEvtCallback, NULL);

    ret = AM_EPG_SetUserData(mEasScanHandle, (void *)this);
    if (ret != DVB_SUCCESS) {
        LOGD("AM_EPG_SetUserData failed!\n");
        return -1;
    }
#endif
    return 0;
}

/**
 * @Function: EasDestroy
 * @Description: close dev for EAS and reset EAS event
 * @Param:
 * @Return: 0 success, -1 fail
 */
int CTvEas::EasDestroy()
{
#ifdef SUPPORT_ADTV
    AM_ErrorCode_t ret = -1;
    if (mEasScanHandle != NULL) {
        /*unregister eit events notifications*/
        AM_EVT_Unsubscribe((long)mEasScanHandle, AM_EPG_EVT_NEW_CEA, EasEvtCallback, NULL);

        ret = AM_EPG_Destroy(mEasScanHandle);
        mEasScanHandle = NULL;
    }

    if (mDmxId != INVALID_ID) {
        AM_DMX_Close(mDmxId);
        mDmxId = INVALID_ID;
    }

    if (ret != DVB_SUCCESS) {
        LOGD("AM_EPG_SetUserData failed!\n");
        return -1;
    }
#endif
    return 0;
}

/**
 * @Function: EasEvtCallback
 * @Description: EAS event callback function
 * @Param:dev_no: dev id
          event_type:RRT event type
          param:callback data
          user_data:
 * @Return:
 */
void CTvEas::EasEvtCallback(long dev_no, int event_type, void *param, void *user_data __unused)
{
#ifdef SUPPORT_ADTV
    CTvEas *pEas;

    AM_EPG_GetUserData((void *)dev_no, (void **)&pEas);

    if ((pEas == NULL) || (pEas->mpObserver == NULL)) {
        return;
    }

    switch (event_type) {
    case AM_EPG_EVT_NEW_CEA: {
        LOGD("%s: EAS callback!\n", __FUNCTION__);
        dvbpsi_atsc_cea_t *tmp = (dvbpsi_atsc_cea_t *)param;

        int SectionCount = 0;
        int MultistrCount = 0;
        int DescriptorCount = 0;
        int i , j;
        SectionCount = mInstance->GetSectionCount(tmp);
        pEas->mCurEasEv.eas_section_count = SectionCount;
        dvbpsi_atsc_cea_t *tmp2 = (dvbpsi_atsc_cea_t *)param;
        if (tmp2 != NULL) {
            pEas->mCurEasEv.table_id = tmp2->i_table_id;
            pEas->mCurEasEv.extension = tmp2->i_extension;
            pEas->mCurEasEv.version = tmp2->i_version;
            pEas->mCurEasEv.current_next = tmp2->b_current_next;
            pEas->mCurEasEv.sequence_num = tmp2->i_sequence_num;
            pEas->mCurEasEv.protocol_version = tmp2->i_protocol_version;
            pEas->mCurEasEv.eas_event_id = tmp2->i_eas_event_id;
            for (i=0;i<3;i++) {
                pEas->mCurEasEv.eas_orig_code[i]= tmp2->i_eas_orig_code[i];
            }

            pEas->mCurEasEv.eas_event_code_len = tmp2->i_eas_event_code_len;
            for (i=0;i<tmp2->i_eas_event_code_len;i++) {
                pEas->mCurEasEv.eas_event_code[i] = tmp2->i_eas_event_code[i];
            }

            pEas->mCurEasEv.alert_message_time_remaining = tmp2->i_alert_message_time_remaining;
            pEas->mCurEasEv.event_start_time = tmp2->i_event_start_time;
            pEas->mCurEasEv.event_duration = tmp2->i_event_duration;
            pEas->mCurEasEv.alert_priority = tmp2->i_alert_priority;
            pEas->mCurEasEv.details_OOB_source_ID = tmp2->i_details_OOB_source_ID;
            pEas->mCurEasEv.details_major_channel_number = tmp2->i_details_major_channel_number;
            pEas->mCurEasEv.details_minor_channel_number = tmp2->i_details_minor_channel_number;
            pEas->mCurEasEv.audio_OOB_source_ID = tmp2->i_audio_OOB_source_ID;
            pEas->mCurEasEv.location_count = tmp2->i_location_count;
            for (i=0;i<tmp2->i_location_count;i++) {
                pEas->mCurEasEv.location[i].i_state_code = tmp2->location[i].i_state_code;
                pEas->mCurEasEv.location[i].i_country_subdiv = tmp2->location[i].i_country_subdiv;
                pEas->mCurEasEv.location[i].i_country_code = tmp2->location[i].i_country_code;
            }

            pEas->mCurEasEv.exception_count = tmp2->i_exception_count;
            for (i=0;i<tmp2->i_exception_count;i++) {
                pEas->mCurEasEv.exception[i].i_in_band_refer = tmp2->exception[i].i_in_band_refer;
                pEas->mCurEasEv.exception[i].i_exception_major_channel_number = tmp2->exception[i].i_exception_major_channel_number;
                pEas->mCurEasEv.exception[i].i_exception_minor_channel_number = tmp2->exception[i].i_exception_minor_channel_number;
                pEas->mCurEasEv.exception[i].exception_OOB_source_ID = tmp2->exception[i].exception_OOB_source_ID;
            }

            dvbpsi_atsc_cea_multi_str_t *TempMulti1 = tmp2->p_first_multi_text;
            dvbpsi_atsc_cea_multi_str_t *TempMulti2 = tmp2->p_first_multi_text;
            if (TempMulti2 != NULL) {
                MultistrCount = mInstance->GetMultiCount(TempMulti1);
            }
            pEas->mCurEasEv.multi_text_count = MultistrCount;
            for (i=0;i<MultistrCount;i++) {
                while (TempMulti2 != NULL) {
                    for (j=0;j<3;j++) {
                        pEas->mCurEasEv.multi_text[i].lang[j] = TempMulti2->lang[j];
                    }
                    pEas->mCurEasEv.multi_text[i].i_type = TempMulti2->i_type;
                    pEas->mCurEasEv.multi_text[i].i_compression_type = TempMulti2->i_compression_type;
                    pEas->mCurEasEv.multi_text[i].i_mode = TempMulti2->i_mode;
                    pEas->mCurEasEv.multi_text[i].i_number_bytes = TempMulti2->i_number_bytes;
                    for (j=0;j<TempMulti2->i_number_bytes;j++) {
                        pEas->mCurEasEv.multi_text[i].compressed_str[j] = TempMulti2->compressed_str[j];
                    }
                    TempMulti2 = TempMulti2->p_next;
                }
            }

            dvbpsi_descriptor_t *TempDesc1 = tmp2->p_first_descriptor;
            dvbpsi_descriptor_t *TempDesc2 = tmp2->p_first_descriptor;

            if (TempMulti1 != NULL) {
                DescriptorCount = mInstance->GetDescCount(TempDesc1);
            }
            pEas->mCurEasEv.descriptor_text_count = DescriptorCount;

            for (i=0;i<DescriptorCount;i++) {
                while (TempDesc2 != NULL) {
                    pEas->mCurEasEv.descriptor[i].i_tag = TempDesc2->i_tag;
                    pEas->mCurEasEv.descriptor[i].i_length = TempDesc2->i_length;
                    memcpy(pEas->mCurEasEv.descriptor[i].p_data, TempDesc2->p_data, TempDesc2->i_length);
                    TempDesc2 = TempDesc2->p_next;
                }
            }
            pEas->mpObserver->onEvent(pEas->mCurEasEv);
        }
        break;
    }
    default:
        LOGD("%s: other callback!\n", __FUNCTION__);
        break;
    }
#endif
}

#ifdef SUPPORT_ADTV
/**
 * @Function: GetSectionCount
 * @Description: Get EAS Section Count
 * @Param:pData:Address of atsc eas string which need to check count
 * @Return:count result
 */
int CTvEas::GetSectionCount(dvbpsi_atsc_cea_t *pData)
{
    int count  = 0;
    while (pData != NULL) {
        count ++;
        pData = pData->p_next;
    }
    LOGD("section: count = %d\n", count);
    return count;
}

/**
 * @Function: GetDescCount
 * @Description: Get EAS descriptor string Count
 * @Param:pData:Address of atsc eas descriptor string which need to check count
 * @Return:count result
 */
int CTvEas::GetDescCount(dvbpsi_descriptor_t *pData)
{
    int count  = 0;
    while (pData != NULL) {
        count ++;
        pData = pData->p_next;
    }
    LOGD("descriptor: count = %d\n", count);
    return count;
}

/**
 * @Function: GetMultiCount
 * @Description: Get EAS multi string Count
 * @Param:pData:Address of atsc eas multi string which need to check count
 * @Return:count result
 */
int CTvEas::GetMultiCount(dvbpsi_atsc_cea_multi_str_t *pData)
{
    int count  = 0;
    while (pData != NULL) {
        count ++;
        pData = pData->p_next;
    }
    LOGD("multi: count = %d\n", count);
    return count;
}
#endif

