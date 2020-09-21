/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
#include "atsc/atsc_types.h"
#include "atsc/atsc_stt.h"

#define MAKE_SHORT_HL(exp)		            ((INT16U)((exp##_hi<<8)|exp##_lo))
#define MAKE_WORD_HML(exp)			((INT32U)(((exp##_hi)<<24)|((exp##_mh)<<16)|((exp##_ml)<<8)|exp##_lo))
#define secs_Between_1Jan1970_6Jan1980 	(315964800)

#if (ATSC_EPG_TIME_RELATE)  // for atsc epg
static INT32U g_gps_seconds = 0;
static INT32U g_gps_seconds_offset = 0;

INT32U GetCurrentGPSSeconds(void)
{
	return g_gps_seconds;
}

INT32U GetCurrentGPSOffset(void)
{
	return g_gps_seconds_offset;
}
#endif

#if 0
INT32S get_timezone_secondes(void)
{
	INT8S hour = 0;
	INT8S minute = 0;
	INT8S second = 0;
	INT32S total_seconds = 0;
	
	app_time_get_offset(&hour, &minute, &second);
	total_seconds = hour*3600 + minute*60 + second;
	
	return total_seconds;
}
#endif

stt_section_info_t *atsc_psip_new_stt_info(void)
{
	stt_section_info_t *tmp = NULL;

    tmp = AMMem_malloc(sizeof(stt_section_info_t));
    if(tmp)
    {
        tmp->utc_time = 0;
        tmp->p_next = NULL;
    }

    return tmp;
}

void   atsc_psip_free_stt_info(stt_section_info_t *info)
{
	AMMem_free(info);
}

//this is the seconds from 1970 1.1 00:00:00 to now
static void util_convert_data_time(long seconds)
{
    int left_seconds = 0,left_days;
    int hour, minute, second;
    int year, month=0, date, idx;
    int leap_year = 0, year_now;
    int month_days[12] =          {31,59,90,120,151,181,212,243,273,304,334,365};
    int month_leap_days[12] = {31,60,91,121,152,182,213,244,274,305,335,366};
    int year_now_leap = 0;
    //long seconds;
    int base_year = 1970;
    
    //seconds = atoi(argv[1]);
    
    //year, we using 365.25 days as one year
    year = seconds/(365*24*3600+25*24*36);
    //AM_DEBUG(1,"The seonds is:%ld, the year is:%d\n", seconds, year);
    year_now = base_year+year;

    leap_year = 0;
    year_now_leap = 0;
    left_days = 0;
    //AM_DEBUG(1,"The year_now is:%d\n", year_now);
    for(idx = base_year; idx <= year_now; idx++)
    {
        if(idx%400==0 || ((idx%4==0) && (idx%100!=0))){
            if(idx == year_now){
                //AM_DEBUG(1,"The current year is a leap year.\n");
                year_now_leap = 1;
            }else{
                leap_year++;
                //AM_DEBUG(1,"the %d is a leap year, total leap year:%d.\n", idx, leap_year);
            }
        }
    }
    
    left_days = (seconds - (365*24*3600)*year - leap_year*24*3600)/(24*3600);
    //AM_DEBUG(1,"The left days is:%d\n", left_days);
    //AM_DEBUG(1,"The year now is:%d, leap_year:%d\n", year_now, leap_year);
    //AM_DEBUG(1,"the year is:%d, seconds is:%ld, left_days:%d\n",year, seconds, left_days);

    //month
    if(year_now_leap > 0){
        //AM_DEBUG(1,"The current year is a leap year................\n");
        for(idx = 0; idx < 12; idx++)
            month_days[idx] = month_leap_days[idx];
    }
    
    left_seconds = (seconds-3600*24*leap_year - year*365*24*3600);
    left_days = left_seconds/(3600*24);
    //AM_DEBUG(1,"222 the left days is:%d, left_seconds:%d.\n", left_days, left_seconds);
    for(idx = 0; idx < 12; idx++)
    {
        //Feb should be checked here, but 2014 is not a leap year, so, hehe
        if(month_days[idx] >= left_days)
        {
            month = idx+1;
            //AM_DEBUG(1,"The left days is:%d, month days is:%d, month is:%d\n", left_days, month_days[idx], month);
            break;
        }
    }

    //day
    if(idx == 0){
        date = left_days;
    }else{
        date = left_days - month_days[idx-1];
    }

    //HH:MM:SS
    left_seconds = seconds%(3600*24);
    hour = left_seconds/3600;
    left_seconds = seconds%(3600);
    minute = left_seconds/60;
    second = seconds%(60);
    AM_DEBUG(1,"The time is:(%04d)-(%02d/%02d)-(%02d:%02d:%02d)\n",year+base_year, month, date, hour, minute, second);
}


INT32S atsc_psip_parse_stt(INT8U* data, INT32U length, stt_section_info_t *info)
{
    stt_section_t *sect = NULL;
    INT32U tmp_time;
    INT8U GPS_UTC_offset;

    UNUSED(length);

    if(data && info)
    {
        sect = (stt_section_t*)data;

        if(sect->table_id == ATSC_PSIP_STT_TID)
        {
            tmp_time = MAKE_WORD_HML(sect->system_time);
            GPS_UTC_offset = sect->GPS_UTC_offset;
            //AM_DEBUG(1,"the GPS time is:%d, GPS_UTC_offset:%d", tmp_time, GPS_UTC_offset);
            util_convert_data_time(tmp_time);
#if 1
            info->utc_time = tmp_time + secs_Between_1Jan1970_6Jan1980 - GPS_UTC_offset;
            util_convert_data_time(info->utc_time);
#else	
            info->utc_time = tmp_time + secs_Between_1Jan1970_6Jan1980 - GPS_UTC_offset + get_timezone_secondes();
#endif
#if ATSC_EPG_TIME_RELATE
            g_gps_seconds = tmp_time;
            g_gps_seconds_offset = GPS_UTC_offset;
#endif
            return 0;
        }
    }

    return -1;
}

void atsc_psip_dump_stt_time(INT32U utc_time)
{
#ifndef __ROM_
    if(utc_time)
    {
        AM_TRACE("\r\n============= STT INFO =============\r\n");

        AM_TRACE("%d, 0x%08x", utc_time, utc_time);
        
        AM_TRACE("\r\n================ END ===============\r\n");
    }
#endif

    return ;
}
