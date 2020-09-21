#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 节目管理
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <string.h>
#include <assert.h>
#include <am_debug.h>
#include "dtv.h"
#include <am_iconv.h>
#include <am_scan.h>
#include <am_cfg.h>
#include <am_av.h>
#include <am_fend.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/*节目存储文件名*/
#define PROGRAM_FILE "prog.dat"

/****************************************************************************
 * Global data
 ***************************************************************************/

struct dvb_frontend_parameters      ts_param;
DTV_Program_t programs[DTV_PROG_COUNT];
int           program_count;
int           curr_program;
const fe_type_t	  fe_type = FE_QAM;

static int	  curr_read_prog;
static struct dvb_frontend_parameters      last_ts_param;

static void test_print_all_programs()
{
	int i;

	printf("frequency %d:\n", ts_param.frequency);
	for (i=0; i<program_count; i++)
	{
		printf("%d. [%s]:vid_pid %d, aud_pid %d\n", programs[i].chan_num, \
													programs[i].name,\
													programs[i].video_pid,\
													programs[i].audio_pid);
	}
}

/**\brief 将搜索节目存储到文件*/
static int store_program_to_file(void)
{
	AM_CFG_OutputContext_t cx;
	int i;
	char buf[256];
	
	AM_TRY(AM_CFG_BeginOutput(PROGRAM_FILE, &cx));
	AM_CFG_StoreDec(&cx, "frequency", ts_param.frequency);
	if (fe_type == FE_QAM)
	{
		AM_CFG_StoreDec(&cx, "qam_symbolrate", ts_param.u.qam.symbol_rate);
		AM_CFG_StoreDec(&cx, "qam_modulation", ts_param.u.qam.modulation);
	}
	else if (fe_type == FE_OFDM)
	{
		AM_CFG_StoreDec(&cx, "ofdm_modulation", ts_param.u.ofdm.constellation);
	}
	
	AM_CFG_StoreDec(&cx, "prog_cnt", program_count);
	for (i=0; i<program_count; i++)
	{
		sprintf(buf, "service%03d", i+1);
		AM_CFG_BeginSection(&cx, buf);
		AM_CFG_StoreDec(&cx, "chan_num", programs[i].chan_num);
		AM_CFG_StoreStr(&cx, "name", programs[i].name);
		AM_CFG_StoreDec(&cx, "video_pid", programs[i].video_pid);
		AM_CFG_StoreDec(&cx, "audio_pid", programs[i].audio_pid);
		AM_CFG_EndSection(&cx);
	}
	
	AM_CFG_EndOutput(&cx);

	return AM_SUCCESS;
} 

/**\brief 读取key*/
static AM_ErrorCode_t read_key(void *user_data, const char *key, const char *value)
{
	
	if (!strcmp(key, "frequency"))
	{
		AM_CFG_Value2Int(value, (int*)&ts_param.frequency);
	}
	else if (!strcmp(key, "qam_symbolrate"))
	{
		AM_CFG_Value2Int(value, (int*)&ts_param.u.qam.symbol_rate);
	}
	else if (!strcmp(key, "qam_modulation"))
	{
		AM_CFG_Value2Int(value, (int*)&ts_param.u.qam.modulation);
	}
	else if (!strcmp(key, "ofdm_modulation"))
	{
		AM_CFG_Value2Int(value, (int*)&ts_param.u.ofdm.constellation);
	}
	else if (!strcmp(key, "prog_cnt"))
	{
		AM_CFG_Value2Int(value, &program_count);
	}
	else if (!strcmp(key, "chan_num") && (curr_read_prog < DTV_PROG_COUNT))
	{
		AM_CFG_Value2Int(value, &programs[curr_read_prog].chan_num);
	}
	else if (!strcmp(key, "name") && (curr_read_prog < DTV_PROG_COUNT))
	{
		strncpy(programs[curr_read_prog].name, value, DTV_PROG_NAME_LEN - 1);
	}
	else if (!strcmp(key, "video_pid") && (curr_read_prog < DTV_PROG_COUNT))
	{
		AM_CFG_Value2Int(value, &programs[curr_read_prog].video_pid);
	}
	else if (!strcmp(key, "audio_pid") && (curr_read_prog < DTV_PROG_COUNT))
	{
		AM_CFG_Value2Int(value, &programs[curr_read_prog].audio_pid);
	}

	return AM_SUCCESS;
}

/**\brief 读取section*/
static AM_ErrorCode_t read_section(void *user_data, const char *section)
{
	if (!strncmp(section, "service", 7))
		curr_read_prog++;

	return AM_SUCCESS;
}

/**\brief 从文件中加载节目数据*/
static int load_program_from_file(void)
{
	curr_read_prog = -1;
	return AM_CFG_Input(PROGRAM_FILE, read_section, read_key, NULL, NULL);
}

/**\brief GB2312转UTF-8*/
static int convertcode(char *inbuf,int inlen,char *outbuf,int outlen)
{
    iconv_t handle;

    char **pin=&inbuf;

    char **pout=&outbuf;

	if (!inbuf || !outbuf || inlen == 0 || outlen == 0)
		return -1;
		
    handle=iconv_open("utf-8","gb2312");

    if (handle == (iconv_t)-1)
    	return -1;

    memset(outbuf,0,outlen);

    if(iconv(handle,pin,(size_t *)&inlen,pout,(size_t *)&outlen) == -1)
    {
        printf("iconv error!\n");
        iconv_close(handle);
        return -1;
    }

    return iconv_close(handle);
} 

/**\brief 节目存储处理函数，由Scan模块在搜索完成时调用*/
void dtv_program_store(AM_SCAN_Result_t *result)
{
	dvbpsi_pmt_t *pmt;
	dvbpsi_sdt_t *sdt;
	dvbpsi_pmt_es_t *es;
	dvbpsi_sdt_service_t *srv;
	dvbpsi_descriptor_t *descr;
	DTV_Program_t *prog;
	
	if (result == NULL)
		return ;

	/*未搜索到任何节目,不进行存储操作*/
	if (result->tses == NULL || result->tses->pmts == NULL)
		return ;

	/*清除原来节目*/
	program_count = 0;
	curr_program = -1;

	/*存储频点信息*/
	ts_param = result->tses->fend_para;

	AM_SI_LIST_BEGIN(result->tses->pmts, pmt)
		/*添加节目*/
		if (program_count >= DTV_PROG_COUNT)
		{
			AM_DEBUG(1, "Too many programs!");
			return;
		}
		prog = &programs[program_count];
		prog->chan_num = program_count + 1;
		prog->video_pid = prog->audio_pid = 0x1fff;
		sprintf(prog->name, "电视节目%03d", prog->chan_num);
		/*检查ES流*/
		AM_SI_LIST_BEGIN(pmt->p_first_es, es)
			/*video pid*/
			if ((es->i_type == 0x1 || es->i_type == 0x2) && prog->video_pid == 0x1fff)
				prog->video_pid = (es->i_pid >= 0x1fff) ? 0x1fff : es->i_pid;
			/*audio pid*/
			if ((es->i_type == 0x3 || es->i_type == 0x4) && prog->audio_pid == 0x1fff)
				prog->audio_pid = (es->i_pid >= 0x1fff) ? 0x1fff : es->i_pid;	
		AM_SI_LIST_END()

		/*SDT*/
		AM_SI_LIST_BEGIN(result->tses->sdts, sdt)
			AM_SI_LIST_BEGIN(result->tses->sdts->p_first_service, srv)
				if (srv->i_service_id == pmt->i_program_number)
				{
					AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
						if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE)
						{
							dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t*)descr->p_decoded;
						
							/*取节目名称*/
							if (psd->i_service_name_length > 0)
							{
								convertcode((char*)psd->i_service_name, psd->i_service_name_length,\
											prog->name, sizeof(prog->name));
								
							}
							/*跳出多层循环*/
							goto SDT_END;
						}
					AM_SI_LIST_END()
				}
			AM_SI_LIST_END()
		AM_SI_LIST_END()
		
SDT_END:
		/*不存储音视频PID都无效的节目*/
		if ((prog->video_pid != 0x1fff) || (prog->audio_pid != 0x1fff))
			program_count++;
			
	AM_SI_LIST_END()

	if (program_count > 0)
		curr_program = 0;

	AM_DEBUG(1, "Total searched %d programs, storing programs to file...", program_count);
	store_program_to_file();
	AM_DEBUG(1, "Done");
		
	test_print_all_programs();
}


/****************************************************************************
 * API functions
 ***************************************************************************/
int dtv_program_init(void)
{
	/*初始化节目数据*/
	program_count = 0;
	curr_program = -1;

	AM_DEBUG(1, "Loading programs from file...");
	/*从文件中加载节目信息*/
	load_program_from_file();

	memset(&last_ts_param, 0, sizeof(last_ts_param));

	if (program_count > 0)
		curr_program = 0;
		
	test_print_all_programs();
	
	return 0;
}

int dtv_program_quit(void)
{
	return 0;
}

/**\brief 取得指定频道号的节目信息*/
int dtv_program_get_info_by_chan_num(int chan_num, DTV_Program_t *info)
{
	int i;
	
	assert(info);

	for (i=0; i<program_count; i++)
	{
		if (programs[i].chan_num == chan_num)
		{
			*info = programs[i];
			return 0;
		}
	}

	AM_DEBUG(1, "Cannot get program info of chan num %d", chan_num);
	return -1;
}

/**\brief 获取当前播放节目的信息*/
int dtv_program_get_curr_info(DTV_Program_t *info)
{
	assert(info);
	
	memset(info, 0, sizeof(DTV_Program_t));
	if (program_count <= 0)
		return -1;
	if (curr_program < 0 || curr_program >= program_count)
		return -1;

	*info = programs[curr_program];

	return 0;
}

/**\brief 播放当前节目*/
int dtv_program_play(void)
{
	DTV_Program_t info;

	/*获取当前播放节目信息*/
	if (dtv_program_get_curr_info(&info) < 0)
	{
		AM_DEBUG(1, "Cannot get curr program, cannot play");
		return -1;
	}

	/*前端参数发生变化，需要重新锁定频点*/
	//if (memcmp(&ts_param, &last_ts_param, sizeof(ts_param)))
	{
		fe_status_t status;

		AM_DEBUG(1, "FEnd param changed, try lock new param(%u)...",ts_param.frequency);
		AM_FEND_Lock(DTV_FEND_DEV_NO, &ts_param, &status);
		AM_DEBUG(1, "FEnd %s", (status & FE_HAS_LOCK) ? "Locked" : "Unlocked");
		last_ts_param = ts_param;
	}


	AM_AV_StopTS(DTV_AV_DEV_NO);

	if (AM_AV_StartTS(0, info.video_pid, 0x1fff, 0, 0) != AM_SUCCESS)

	{
		AM_DEBUG(1, "AM_AV_StartTS Err!");
		return -1;
	}

	return 0;
}


int dtv_program_play_by_channel_no(int channel_no)
{
	DTV_Program_t info;

	/*获取当前播放节目信息*/
	if (dtv_program_get_info_by_chan_num(channel_no,&info) < 0)
	{
		AM_DEBUG(1, "Cannot get curr program, cannot play");
		return -1;
	}

	/*前端参数发生变化，需要重新锁定频点*/
	//if (memcmp(&ts_param, &last_ts_param, sizeof(ts_param)))
	{
		fe_status_t status;

		AM_DEBUG(1, "FEnd param changed, try lock new param(%u)...",ts_param.frequency);
		AM_FEND_Lock(DTV_FEND_DEV_NO, &ts_param, &status);
		AM_DEBUG(1, "FEnd %s", (status & FE_HAS_LOCK) ? "Locked" : "Unlocked");
		last_ts_param = ts_param;
	}
	
	AM_AV_StopTS(DTV_AV_DEV_NO);

	if (AM_AV_StartTS(0, info.video_pid, 0x1fff, 0, 0) != AM_SUCCESS)
	{
		AM_DEBUG(1, "AM_AV_StartTS Err!");
		return -1;
	}
	
	curr_program = channel_no-1;
		
	return 0;
}



/**\brief 停止播放当前节目*/
int dtv_program_stop(void)
{
	return AM_AV_StopTS(0);
}

/**\brief 播放下一个频道*/
int dtv_program_channel_up(void)
{
	if (program_count <= 0)
	{
		curr_program = -1;
		return -1;
	}

	/*设置下一个节目为当前播放节目*/
	curr_program++;
	if (curr_program < 0 || curr_program >= program_count)
		curr_program = 0;

	/*播放*/
	return dtv_program_play();
}

/**\brief 播放上一个频道*/
int dtv_program_channel_down(void)
{
	if (program_count <= 0)
	{
		curr_program = -1;
		return -1;
	}

	/*设置上一个节目为当前播放节目*/
	curr_program--;
	if (curr_program < 0 || curr_program >= program_count)
		curr_program = program_count - 1;
	
	/*播放*/
	return dtv_program_play();
}

