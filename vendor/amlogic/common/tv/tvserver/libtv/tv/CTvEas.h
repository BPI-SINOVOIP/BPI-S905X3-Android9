/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifdef SUPPORT_ADTV
#include <am_debug.h>
#include <am_epg.h>
#endif
#include "CTvLog.h"
#include "CTvEv.h"
#include <tvutils.h>
#if !defined(_CDTVEAS_H)
#define _CDTVEAS_H

typedef struct cea_location_code_s
{
    int  i_state_code;
    int  i_country_subdiv;    /**<the language of mlti str*/
    int  i_country_code;

} cea_location_code_t;

typedef struct cea_exception_code_s
{
    int  i_in_band_refer;
    int  i_exception_major_channel_number;    /**<the exception major channel num*/
    int  i_exception_minor_channel_number;    /**<the exception minor channel num*/
    int  exception_OOB_source_ID;             /**<the exception oob source id*/
} cea_exception_code_t;

typedef struct cea_multi_str_s
{
    int   lang[3];                 /**<the language of mlti str*/
    int   i_type;                  /**<the str type, alert or nature*/
    int   i_compression_type;      /*!< compression type */
    int   i_mode;                  /*!< mode */
    int   i_number_bytes;          /*!< number bytes */
    int   compressed_str[64];      /**<the compressed str*/
} cea_multi_str_t;

typedef struct descriptor_s
{
     int  i_tag;          /*!< descriptor_tag */
     int  i_length;       /*!< descriptor_length */
     int  p_data[64];    /*!< content */
} descriptor_t;

class CTvEas {
public :
    static const int MODE_ADD    = 0;
    static const int MODE_REMOVE = 1;
    static const int MODE_SET    = 2;

    static const int SCAN_PSIP_CEA = 0x10000;

    static const int INVALID_ID = -1;

    class EasEvent : public CTvEv {
    public:
        EasEvent(): CTvEv(CTvEv::TV_EVENT_EAS)
        {
            eas_section_count = 0;
            table_id = 0;
            extension = 0;
            version = 0;
            current_next = 0;
            sequence_num = 0;
            protocol_version = 0;
            eas_event_id = 0;
            memset(eas_orig_code, 0, sizeof(eas_orig_code));
            eas_event_code_len = 0;
            memset(eas_event_code, 0, sizeof(eas_event_code));
            alert_message_time_remaining = 0;
            event_start_time = 0;
            event_duration = 0;
            alert_priority = 0;
            details_OOB_source_ID = 0;
            details_major_channel_number = 0;
            details_minor_channel_number = 0;
            audio_OOB_source_ID = 0;
            location_count = 0;
            memset(location, 0, sizeof(location));
            exception_count = 0;
            memset(exception, 0, sizeof(exception));
            multi_text_count = 0;
            memset(multi_text, 0, sizeof(multi_text));
            descriptor_text_count = 0;
            memset(descriptor, 0, sizeof(descriptor));
        };
        ~EasEvent()
        {

        };

        int    eas_section_count;                /*!< eas struct count */
        int    table_id;                         /*!< table id */
        int    extension;                        /*!< subtable id */
        int    version;                          /*!< version_number */
        int    current_next;                     /*!< current_next_indicator */
        int    sequence_num;                     /*!< sequence version*/
        int    protocol_version;                 /*!< protocol version*/
        int    eas_event_id;                     /*!< eas event id */
        int    eas_orig_code[3];                 /*!< eas event orig code */
        int    eas_event_code_len;               /*!< eas event code len */
        int    eas_event_code[64];               /*!< eas event code */
        int    alert_message_time_remaining;     /*!< alert msg remain time */
        int    event_start_time;                 /*!< eas event start time */
        int    event_duration;                   /*!< event dur */
        int    alert_priority;                   /*!< alert priority */
        int    details_OOB_source_ID;            /*!< details oob source id */
        int    details_major_channel_number;     /*!< details major channel num */
        int    details_minor_channel_number;     /*!< details minor channel num */
        int    audio_OOB_source_ID;              /*!< audio oob source id */
        int    location_count;                   /*!< location count */
        cea_location_code_t location[64];       /*!< location info */
        int    exception_count;                  /*!< exception count */
        cea_exception_code_t   exception[64];   /*!< exception info */
        int    multi_text_count;                 /*!< multi_text count */
        cea_multi_str_t   multi_text[2];         /*!< nature and alert multi str information structure. */
        int    descriptor_text_count;            /*!< descriptor text count. */
        descriptor_t      descriptor[2];         /*!< descriptor structure. */
    };

    class IObserver {
    public:
        IObserver() {};
        virtual ~IObserver() {};
        virtual void onEvent(const EasEvent &ev) = 0;
    };

    int setObserver(IObserver *ob)
    {
        mpObserver = ob;
        return 0;
    }

    static CTvEas *GetInstance();
    CTvEas();
    ~CTvEas();
    int StartEasUpdate();
    int StopEasUpdate();

    static CTvEas *mInstance;
    void * mEasScanHandle = nullptr;


private:
    int EasCreate(int fend_id, int dmx_id, int src, char *textLangs);
    int EasDestroy();
    static  void EasEvtCallback(long dev_no, int event_type, void *param, void *user_data);
#ifdef SUPPORT_ADTV
    int GetSectionCount(dvbpsi_atsc_cea_t *pData);
    int GetDescCount(dvbpsi_descriptor_t *pData);
    int GetMultiCount(dvbpsi_atsc_cea_multi_str_t *pData);
#endif

    IObserver *mpObserver;
    int mDmxId ;
    EasEvent mCurEasEv;
};
#endif //_CDTVEAS_H
