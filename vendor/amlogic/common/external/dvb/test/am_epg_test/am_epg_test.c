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
/**\file am_monitor_test.c 
 * \brief 监控模块测试程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/
#define USE_PRINTF
#define AM_DEBUG_LEVEL 1

#define FEND_DEV_NO 0
#define DMX_DEV_NO 0

#include <stdio.h>
#include <am_debug.h>
#include <am_dmx.h>
#include <am_epg.h>


#ifdef USE_PRINTF
#ifdef AM_DEBUG
#undef AM_DEBUG
#endif
#define AM_DEBUG(_level,_fmt...) do {printf(_fmt);printf("\n");}while(0)
#endif

static AM_Bool_t disable_default_proc = AM_TRUE;

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

static void tune(int freq, int p1, int p2, int p3, int p4)
{
	struct dvb_frontend_parameters p;
	fe_status_t status;

	p.u.qam.symbol_rate = p1;
	p.u.qam.modulation = p2;
	p.u.qam.fec_inner = p3;

	p.frequency = freq;
	AM_FEND_Lock(FEND_DEV_NO, &p, &status);
}


static void epg_evt_call_back(long dev_no, int event_type, void *param, void *user_data)
{
	switch (event_type)
	{
		case AM_EPG_EVT_NEW_PAT:
		{
			AM_DEBUG(1, "got event AM_EPG_EVT_NEW_PAT ");
			break;
		}
		case AM_EPG_EVT_NEW_PMT:
		{
			AM_DEBUG(1, "got event AM_EPG_EVT_NEW_PMT ");
			break;
		}
		case AM_EPG_EVT_NEW_CAT:
		{
			AM_DEBUG(1, "got event AM_EPG_EVT_NEW_CAT ");
			break;
		}
		case AM_EPG_EVT_NEW_SDT:
		{
			AM_DEBUG(1, "got event AM_EPG_EVT_NEW_SDT ");
			break;
		}
		case AM_EPG_EVT_NEW_TDT:
		{
			dvbpsi_tot_t *tot = (dvbpsi_tot_t*)param;
			dvbpsi_descriptor_t *descr;
			
			AM_DEBUG(1, "got event AM_EPG_EVT_NEW_TDT ");
			
			AM_SI_LIST_BEGIN(tot->p_first_descriptor, descr)
				if (descr->i_tag == AM_SI_DESCR_LOCAL_TIME_OFFSET && descr->p_decoded)
				{
					dvbpsi_local_time_offset_dr_t *plto = (dvbpsi_local_time_offset_dr_t*)descr->p_decoded;

					plto = plto;
					AM_DEBUG(2, "Local time offset Descriptor");
				}
			AM_SI_LIST_END()
			
			break;
		}
		case AM_EPG_EVT_NEW_NIT:
		{
			AM_DEBUG(1, "got event AM_EPG_EVT_NEW_NIT ");
			break;
		}
		case AM_EPG_EVT_NEW_EIT:
		{
			dvbpsi_eit_t *eit = (dvbpsi_eit_t*)param;
			dvbpsi_eit_event_t *event;
			dvbpsi_descriptor_t *descr;
			char name[256+1];
			
			AM_DEBUG(1, "got event AM_EPG_EVT_NEW_EIT ");
			AM_DEBUG(1, "Service id 0x%x:", eit->i_service_id);
			AM_SI_LIST_BEGIN(eit->p_first_event, event)
				AM_SI_LIST_BEGIN(event->p_first_descriptor, descr)
					if (descr->i_tag == AM_SI_DESCR_SHORT_EVENT && descr->p_decoded)
					{
						dvbpsi_short_event_dr_t *pse = (dvbpsi_short_event_dr_t*)descr->p_decoded;

						AM_SI_ConvertDVBTextCode((char*)pse->i_event_name, pse->i_event_name_length,\
								name, 256);
						name[256] = 0;
						AM_DEBUG(1, "\tevent_id 0x%x, name '%s'", event->i_event_id, name);
						break;
					}
				AM_SI_LIST_END()
			AM_SI_LIST_END()
			break;
		}
		default:
			break;
	}
}

static void table_update (AM_EPG_Handle_t handle, int type, void *tables, void *user_data)
{
/*Use the following macros to fix type disagree*/
#define dvbpsi_stt_t stt_section_info_t
#define dvbpsi_mgt_t mgt_section_info_t
#define dvbpsi_psip_eit_t eit_section_info_t
#define dvbpsi_psip_ett_t ett_section_info_t
#define dvbpsi_rrt_t rrt_section_info_t
#define dvbpsi_vct_t vct_section_info_t

#define list_for_t(l, v, t) for ((v)=(t*)(l); (v)!=NULL; (v)=(v)->p_next)
#define list_for(l, v) for ((v)=(l); (v)!=NULL; (v)=(v)->p_next)

	switch(type) {
		case AM_EPG_TAB_PAT:{
			dvbpsi_pat_t *pat;
			list_for_t(tables, pat, dvbpsi_pat_t) {
				printf("T [PAT:0x%02x][ts:0x%04x] v[0x%x] cn[%d]\n", pat->i_table_id, pat->i_ts_id, pat->i_version, pat->b_current_next);
				dvbpsi_pat_program_t *prog;
				list_for(pat->p_first_program, prog)
					printf("\t program: [pid:0x%04x] [pn:0x%04x]\n", prog->i_pid, prog->i_number);
			}
		} break;
		case AM_EPG_TAB_PMT:{
			dvbpsi_pmt_t *pmt;
			list_for_t(tables, pmt, dvbpsi_pmt_t) {
				printf("T [PMT:0x%02x][pn:0x%04x] v[0x%x] cn[%d]\n", pmt->i_table_id, pmt->i_program_number, pmt->i_version, pmt->b_current_next);
				dvbpsi_pmt_es_t *es;
				list_for(pmt->p_first_es, es)
					printf("\t es: [pid:0x%04x] [type:%d]\n", es->i_pid, es->i_type);
			}
		} break;
		case AM_EPG_TAB_CAT:{
			dvbpsi_cat_t *cat;
			list_for_t(tables, cat, dvbpsi_cat_t)
				printf("T [CAT:0x%02x]            v[0x%x] cn[%d]\n", cat->i_table_id, cat->i_version, cat->b_current_next);
		} break;
		case AM_EPG_TAB_SDT:{
			dvbpsi_sdt_t *sdt;
			list_for_t(tables, sdt, dvbpsi_sdt_t) {
				printf("T [SDT:0x%02x][ts:0x%04x][nt:0x%04x] v[0x%x] cn[%d]\n", sdt->i_table_id, sdt->i_ts_id, sdt->i_network_id, sdt->i_version, sdt->b_current_next);
				dvbpsi_sdt_service_t *srv;
				list_for(sdt->p_first_service, srv) {
					printf("\t srv: [pn:0x%04x] [ca:%d] [running:%d]\n", srv->i_service_id, srv->b_free_ca, srv->i_running_status);
					dvbpsi_descriptor_t *dr;
					list_for(srv->p_first_descriptor, dr) {
						if(dr->i_tag == AM_SI_DESCR_SERVICE) {
							dvbpsi_service_dr_t *dr_srv = (dvbpsi_service_dr_t *)dr->p_decoded;
							char name[64] = "\0";
							AM_SI_ConvertDVBTextCode((char*)dr_srv->i_service_name, dr_srv->i_service_name_length,\
								name, 64);
							name[63] = 0;
							printf("\t\t dr_srv [type:%d] [name:%s]\n", dr_srv->i_service_type, name);
						}
					}
				}
			}
		} break;
		case AM_EPG_TAB_NIT:{
			dvbpsi_nit_t *nit;
			list_for_t(tables, nit, dvbpsi_nit_t) {
				printf("T [NIT:0x%02x][nt:0x%04x] v[0x%x] cn[%d]\n", nit->i_table_id, nit->i_network_id, nit->i_version, nit->b_current_next);
				dvbpsi_nit_ts_t *ts;
				list_for(nit->p_first_ts, ts)
					printf("\t ts: [tid:0x%04x] [onid:0x%04x]\n", ts->i_ts_id, ts->i_orig_network_id);
			}
		} break;
		case AM_EPG_TAB_TDT:{
			dvbpsi_tot_t *tot;
			list_for_t(tables, tot, dvbpsi_tot_t)
				printf("T [TDT:0x%02x][utc:0x%llx]\n", tot->i_table_id, tot->i_utc_time);
		} break;
		case AM_EPG_TAB_EIT:{
			dvbpsi_eit_t *eit;
			list_for_t(tables, eit, dvbpsi_eit_t)
				printf("T [EIT:0x%02x][pn:0x%04x][nt:0x%04x] v[0x%x] cn[%d]\n", eit->i_table_id, eit->i_service_id, eit->i_network_id, eit->i_version, eit->b_current_next);
		} break;
		case AM_EPG_TAB_STT:{
			dvbpsi_stt_t *stt;
			list_for_t(tables, stt, dvbpsi_stt_t)
				printf("T [STT:0x%02x][ts:0x%04x][utc:0x%04x]\n", stt->i_table_id, stt->utc_time);
		} break;
		case AM_EPG_TAB_RRT:{
			dvbpsi_rrt_t *rrt;
			list_for_t(tables, rrt, dvbpsi_rrt_t)
				printf("T [RRT:0x%02x][rr:0x%04x][dd:0x%04x] v[0x%x]\n", rrt->i_table_id, rrt->rating_region, rrt->dimensions_defined, rrt->version_number);
		} break;
		case AM_EPG_TAB_MGT:{
			dvbpsi_mgt_t *mgt;
			list_for_t(tables, mgt, dvbpsi_mgt_t)
				printf("T [MGT:0x%02x][td:0x%04x][ca:0x%04x] v[0x%x]\n", mgt->i_table_id, mgt->tables_defined, mgt->is_cable, mgt->version_number);
		} break;
		case AM_EPG_TAB_VCT:{
			dvbpsi_vct_t *vct;
			list_for_t(tables, vct, dvbpsi_vct_t)
				printf("T [VCT:0x%02x][ts:0x%04x][ch:0x%04x] v[0x%x]\n", vct->i_table_id, vct->transport_stream_id, vct->num_channels_in_section, vct->version_number);
		} break;
		case AM_EPG_TAB_PSIP_EIT:{
			dvbpsi_psip_eit_t *peit;
			list_for_t(tables, peit, dvbpsi_psip_eit_t)
				printf("T [PEIT:0x%02x][sr:0x%04x][ev:0x%04x] v[0x%x]\n", peit->i_table_id, peit->source_id, peit->num_events_in_section, peit->version_number);
		} break;
		case AM_EPG_TAB_PSIP_ETT:{
			dvbpsi_psip_ett_t *pett;
			list_for_t(tables, pett, dvbpsi_psip_ett_t)
				printf("T [PETT:0x%02x][sr:0x%04x][ta:0x%04x] v[0x%x]\n", pett->i_table_id, pett->source_id, pett->table_id_extension, pett->version_number);
		} break;

	}
}

static int disable_defproc_reg_cb(AM_EPG_Handle_t hmon)
{
	printf("disable default table procedure.\n");

	AM_EPG_DisableDefProc(hmon, AM_TRUE);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_PAT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_PMT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_CAT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_SDT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_NIT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_TDT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_EIT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_STT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_RRT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_MGT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_VCT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_PSIP_EIT, table_update, NULL);
	AM_EPG_SetTablesCallback(hmon, AM_EPG_TAB_PSIP_ETT, table_update, NULL);

	disable_default_proc = AM_TRUE;
	return 0;
}

static int start_epg_test()
{
	int mode, op, para1, para2;
	char buf[256];
	char buf1[32];
	AM_Bool_t go = AM_TRUE;
	AM_EPG_CreatePara_t epara;
	AM_EPG_Handle_t hmon;

	epara.fend_dev = FEND_DEV_NO;
	epara.dmx_dev = DMX_DEV_NO;
	epara.source = 0;
	
	AM_TRY(AM_EPG_Create(&epara, &hmon));

	disable_defproc_reg_cb(hmon);

	/*注册通知事件*/
	AM_EVT_Subscribe((long)hmon, AM_EPG_EVT_NEW_PAT, epg_evt_call_back, NULL);
	AM_EVT_Subscribe((long)hmon, AM_EPG_EVT_NEW_PMT, epg_evt_call_back, NULL);
	AM_EVT_Subscribe((long)hmon, AM_EPG_EVT_NEW_CAT, epg_evt_call_back, NULL);
	AM_EVT_Subscribe((long)hmon, AM_EPG_EVT_NEW_SDT, epg_evt_call_back, NULL);
	AM_EVT_Subscribe((long)hmon, AM_EPG_EVT_NEW_NIT, epg_evt_call_back, NULL);
	AM_EVT_Subscribe((long)hmon, AM_EPG_EVT_NEW_TDT, epg_evt_call_back, NULL);
	AM_EVT_Subscribe((long)hmon, AM_EPG_EVT_NEW_EIT, epg_evt_call_back, NULL);

	printf("------------------------------------------\n");
	printf("commands:\n");
	printf("\t'start [all pat pmt cat sdt nit tdt eit4e eit4f eit50 eit60]'	start recv table\n");
	printf("\t'stop [all pat pmt cat sdt nit tdt eit4e eit4f eit50 eit60]'	stop recv table\n");
	printf("\t'seteitpf [check interval in ms]'	set the eit pf auto check interval\n");
	printf("\t'seteitsche [check interval in ms]'	set the eit schedule auto check interval\n");
	printf("\t'defproc [on/off]'	diable default table procedure \n");
	printf("\t'mon [ts_id] [sid]'  monitor service\n");
	printf("\t'tune [frequency in hz]'	tune the frequency\n");
	printf("\t'utc'	get current UTC time\n");
	printf("\t'local'	get current local time\n");
	printf("\t'offset [offset value in hour]'		set the local offset\n");
	printf("\t'quit'\n");
	printf("------------------------------------------\n");
	
	while (go)
	{
		if (gets(buf))
		{
			if(!strncmp(buf, "quit", 4))
			{
				go = AM_FALSE;
				continue;
			}
			else if(!strncmp(buf, "start", 5) || !strncmp(buf, "stop", 4))
			{
				sscanf(buf+5, "%s", buf1);
				if (!strncmp(buf1, "pat", 3))
				{
					mode = AM_EPG_SCAN_PAT;
				}
				else if (!strncmp(buf1, "pmt", 3))
				{
					mode = AM_EPG_SCAN_PMT;
				}
				else if (!strncmp(buf1, "cat", 3))
				{
					mode = AM_EPG_SCAN_CAT;
				}
				else if (!strncmp(buf1, "sdt", 3))
				{
					mode = AM_EPG_SCAN_SDT;
				}
				else if (!strncmp(buf1, "nit", 3))
				{
					mode = AM_EPG_SCAN_NIT;
				}
				else if (!strncmp(buf1, "tdt", 3))
				{
					mode = AM_EPG_SCAN_TDT;
				}
				else if (!strncmp(buf1, "eit4e", 5))
				{
					mode = AM_EPG_SCAN_EIT_PF_ACT;
				}
				else if (!strncmp(buf1, "eit4f", 5))
				{
					mode = AM_EPG_SCAN_EIT_PF_OTH;
				}
				else if (!strncmp(buf1, "eit50", 5))
				{
					mode = AM_EPG_SCAN_EIT_SCHE_ACT;
				}
				else if (!strncmp(buf1, "eit60", 5))
				{
					mode = AM_EPG_SCAN_EIT_SCHE_OTH;
				}
				else if (!strncmp(buf1, "eitall", 6))
				{
					mode = AM_EPG_SCAN_EIT_PF_ACT|AM_EPG_SCAN_EIT_PF_OTH|AM_EPG_SCAN_EIT_SCHE_ACT|AM_EPG_SCAN_EIT_SCHE_OTH;
				}
				else if (!strncmp(buf1, "all", 3))
				{
					mode = 0xFFFFFFFF;
				}
				else
				{
					AM_DEBUG(1, "Invalid command");
					continue;
				}

				if(!strncmp(buf, "start", 5))
					op = AM_EPG_MODE_OP_ADD;
				else
					op = AM_EPG_MODE_OP_REMOVE;

				AM_EPG_ChangeMode(hmon, op, mode);
			}
			else if(!strncmp(buf, "seteitpf", 8))
			{
				sscanf(buf+8, "%d", &para1);
				AM_EPG_SetEITPFCheckDistance(hmon, para1);
			}
			else if(!strncmp(buf, "seteitsche", 10))
			{
				sscanf(buf+10, "%d", &para1);
				AM_EPG_SetEITScheCheckDistance(hmon, para1);
			}
			else if(!strncmp(buf, "tune", 4))
			{
				sscanf(buf+4, "%d", &para1);
				tune(para1, 6875*1000, QAM_64, FEC_AUTO, 0);
			}
			else if(!strncmp(buf, "utc", 3))
			{
				int t;
				struct tm *tim;
				
				AM_EPG_GetUTCTime(&t);
			
				tim = gmtime((time_t*)&t);								
				AM_DEBUG(1, "UTC Time: %d-%02d-%02d %02d:%02d:%02d", 
						   tim->tm_year + 1900, tim->tm_mon + 1,
						   tim->tm_mday, tim->tm_hour,
						   tim->tm_min, tim->tm_sec);	
			}
			else if(!strncmp(buf, "local", 5))
			{
				int t;
				struct tm *tim;

				AM_EPG_GetUTCTime(&t);
				AM_EPG_UTCToLocal(t, &t);
			
				tim = gmtime((time_t*)&t);								
				AM_DEBUG(1, "Local Time: %d-%02d-%02d %02d:%02d:%02d", 
						   tim->tm_year + 1900, tim->tm_mon+1,
						   tim->tm_mday, tim->tm_hour,
						   tim->tm_min, tim->tm_sec);	
			}
			else if(!strncmp(buf, "offset", 6))
			{
				int offset;

				sscanf(buf+6, "%d", &offset);
				AM_EPG_SetLocalOffset(offset * 3600);
			}
			else if(!strncmp(buf, "defproc", 7))
			{
				sscanf(buf+7, "%s", buf1);
				if (!strncmp(buf1, "off", 3))
				{
					disable_defproc_reg_cb(hmon);
				}
				else if (!strncmp(buf1, "on", 2))
				{
					AM_EPG_DisableDefProc(hmon, AM_FALSE);
					disable_default_proc = AM_FALSE;
				}
			}
			else if(!strncmp(buf, "mon", 3))
			{
				int i = sscanf(buf+3, "%i %i", &para1, &para2);
				if (i == 2)
				{
					AM_EPG_MonitorServiceByID(hmon, para1, para2, NULL);
					/*need reset*/
					AM_EPG_ChangeMode(hmon, AM_EPG_MODE_OP_REMOVE, AM_EPG_SCAN_PAT | AM_EPG_SCAN_PMT);
					AM_EPG_ChangeMode(hmon, AM_EPG_MODE_OP_ADD, AM_EPG_SCAN_PAT | AM_EPG_SCAN_PMT);
				}
			}
		}
	}

	AM_DEBUG(1, "Start destroy EPG...");
	AM_TRY(AM_EPG_Destroy(hmon));
	AM_DEBUG(1, "OK");

	AM_EVT_Unsubscribe((long)hmon, AM_EPG_EVT_NEW_PAT, epg_evt_call_back, NULL);
	AM_EVT_Unsubscribe((long)hmon, AM_EPG_EVT_NEW_PMT, epg_evt_call_back, NULL);
	AM_EVT_Unsubscribe((long)hmon, AM_EPG_EVT_NEW_CAT, epg_evt_call_back, NULL);
	AM_EVT_Unsubscribe((long)hmon, AM_EPG_EVT_NEW_SDT, epg_evt_call_back, NULL);
	AM_EVT_Unsubscribe((long)hmon, AM_EPG_EVT_NEW_NIT, epg_evt_call_back, NULL);
	AM_EVT_Unsubscribe((long)hmon, AM_EPG_EVT_NEW_TDT, epg_evt_call_back, NULL);
	AM_EVT_Unsubscribe((long)hmon, AM_EPG_EVT_NEW_EIT, epg_evt_call_back, NULL);

	return 0;
}

int main(int argc, char **argv)
{
	AM_FEND_OpenPara_t fpara;
	AM_DMX_OpenPara_t para;
	int freq = 0;

	if (argc > 1)
		sscanf(argv[1], "%d", &freq);

	if (freq > 0) {
		memset(&fpara, 0, sizeof(fpara));
		AM_FEND_Open(FEND_DEV_NO, &fpara);

		tune(freq, 6875*1000, QAM_64, FEC_AUTO, 0);
	}

	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO, &para));
	
	start_epg_test();

	AM_DMX_Close(DMX_DEV_NO);
	AM_FEND_Close(FEND_DEV_NO);
	return 0;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

