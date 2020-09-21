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
/**\file am_si_test.c
 * \brief SI 测试程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-18: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_dmx.h>
#include <am_fend.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "am_si.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define FEND_DEV_NO   (0)
#define DMX_DEV_NO    (0)

enum 
{
	SI_TEST_PAT,
	SI_TEST_CAT,
	SI_TEST_NIT,
	SI_TEST_PMT,
	SI_TEST_SDT,
	SI_TEST_EIT,
	SI_TEST_TOT
};

typedef struct
{
	int dev_no;
	int hfilter;
	uint16_t pid;
	uint8_t table_id;
	uint8_t cur_sec;
	uint8_t last_sec;
	void (*print)(void *);
	uint8_t data[4096];
}SITest_Section_t;



static void si_test_table_start_next_section(int t);
static void si_test_restart_section(int t);

static void print_pat(void *p_table)
{
	dvbpsi_pat_t *t = (dvbpsi_pat_t*)p_table;
	dvbpsi_pat_program_t *pg;

	assert(t);

	printf("This is a PAT, Expand:\n");
	printf("\ttable_id\t0x%x\n", t->i_table_id);	
	printf("\tts_id\t0x%x\n", t->i_ts_id);
	printf("\tversion\t0x%x\n", t->i_version);
	printf("\tcur_next\t0x%x\n", t->b_current_next);
	printf("\tPrograms:\n");
	//printf("\t\t0x%x\n");
	AM_SI_LIST_BEGIN(t->p_first_program, pg)
		printf("\t\tprogram number\t0x%x\n", pg->i_number);
		printf("\t\tpid\t\t0x%x\n", pg->i_pid);
		printf("\n");
	AM_SI_LIST_END()
}

static void print_sdt(void *p_table)
{
	dvbpsi_sdt_t *t = (dvbpsi_sdt_t*)p_table;
	dvbpsi_sdt_service_t *srv;
	dvbpsi_descriptor_t *descr;
	char nbuf[256];

	assert(t);

	printf("This is a SDT, Expand:\n");
	printf("\ttable_id\t0x%x\n", t->i_table_id);	
	printf("\tts_id\t0x%x\n", t->i_ts_id);
	printf("\tversion\t0x%x\n", t->i_version);
	printf("\tcur_next\t0x%x\n", t->b_current_next);
	printf("\tnet_id\t0x%x\n", t->i_network_id);
	printf("\tServices:\n");
	//printf("\t\t0x%x\n");
	AM_SI_LIST_BEGIN(t->p_first_service, srv)
		printf("\t\tservice_id\t0x%x\n", srv->i_service_id);
		printf("\t\teit_schedule\t0x%x\n", srv->b_eit_schedule);
		printf("\t\teit_present\t0x%x\n", srv->b_eit_present);
		printf("\t\trunning_status\t0x%x\n", srv->i_running_status);
		printf("\t\tfree_ca_mode\t0x%x\n", srv->b_free_ca);
		printf("\t\tDescriptors:\n");
		
		AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
			printf("\t\t\ttag\t0x%x\n", descr->i_tag);
			printf("\t\t\tlen\t0x%x\n", descr->i_length);
			printf("\t\t\tdecoded\t0x%x\n", (int)descr->p_decoded);
			if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE)
			{
				dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t*)descr->p_decoded;
				
				printf("\t\t\t*Service Descriptor*\n");
				printf("\t\t\tservice_type\t0x%x\n", psd->i_service_type);
				snprintf(nbuf, psd->i_service_provider_name_length, \
						"%s",psd->i_service_provider_name);
				printf("\t\t\tprovider_name\t\"%s\"\n", nbuf);
				snprintf(nbuf, psd->i_service_name_length, \
						"%s",psd->i_service_name);
				printf("\t\t\tservice_name\t\"%s\"\n", nbuf);
			}
			printf("\n");
		AM_SI_LIST_END()
		
		printf("\n");
	AM_SI_LIST_END()
}

static SITest_Section_t si_sections[] = 
{
	{DMX_DEV_NO, -1, AM_SI_PID_PAT, AM_SI_TID_PAT, 0, 0, print_pat},
	{DMX_DEV_NO, -1, AM_SI_PID_CAT, AM_SI_TID_CAT, 0, 0, NULL},
	{DMX_DEV_NO, -1, AM_SI_PID_NIT, AM_SI_TID_NIT_ACT, 0, 0, NULL},
	{DMX_DEV_NO, -1, 0x1000, 		AM_SI_TID_PMT, 0, 0, NULL},
	{DMX_DEV_NO, -1, AM_SI_PID_SDT, AM_SI_TID_SDT_ACT, 0, 0, print_sdt},
	{DMX_DEV_NO, -1, AM_SI_PID_EIT, AM_SI_TID_EIT_PF_ACT, 0, 0, NULL},
	{DMX_DEV_NO, -1, AM_SI_PID_TOT, AM_SI_TID_TOT, 0, 0, NULL},
};

static int get_sec_by_hfilter(int hfilter)
{
	int i;

	for (i=0; i<AM_ARRAY_SIZE(si_sections); i++)
	{
		if (si_sections[i].hfilter == hfilter)
		{
			return i;
		}
	}

	return -1;
}

static void si_section_callback(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	AM_SI_Handle_t hsi;
	int t;
	AM_SI_SectionHeader_t header;
	void *p_table;
	
	AM_DEBUG(1, "GET A SECTION LEN: %d", len);

	t = get_sec_by_hfilter(fid);
	if (t == -1)
	{
		AM_DEBUG(1, "Unknown filter id 0x%x", fid);
		return;
	}

	/*Copy the data*/
	memcpy(si_sections[t].data, data, len);
	
	AM_SI_Create(&hsi);
	AM_SI_GetSectionHeader(hsi, (uint8_t*)si_sections[t].data, (uint16_t)len, &header);

	/*Print the header*/
	AM_DEBUG(1, "SectionHeader::\n\ttable_id\t\t0x%x\n\tsyntax_indicator\t0x%x\n\tprv_indicator\t\t0x%x\n\tlength\t\t\t0x%x\n\textension\t\t0x%x\n\tverion\t\t\t0x%x\n\tcur_next\t\t0x%x\n\tsec_num\t\t\t0x%x\n\tlast_sec_num\t\t0x%x",
	header.table_id,header.syntax_indicator,header.private_indicator,
	header.length, header.extension, header.version, 
	header.cur_next_indicator, header.sec_num, header.last_sec_num);

	/*Decode*/
	if (AM_SI_DecodeSection(hsi, si_sections[t].pid, 
							(uint8_t*)si_sections[t].data, 
							(uint16_t)len, &p_table) == AM_SUCCESS)
	{
		AM_DEBUG(1, "Section Successfully decoded");
		/*Print detail*/
		if (si_sections[t].print)
			si_sections[t].print(p_table);
		AM_SI_ReleaseSection(hsi, ((dvbpsi_psi_section_t*)p_table)->i_table_id, (void*)p_table);
	}
	else
	{
		AM_DEBUG(1, "Section  decoded failed");
	}
		
	AM_SI_Destroy(hsi);

	si_sections[t].last_sec = header.last_sec_num;
	
	/*Start next section receive*/
	si_test_table_start_next_section(t);
}



static void si_test_table_start_next_section(int t)
{
	SITest_Section_t *psec;
	struct dmx_sct_filter_params param;
	struct dmx_pes_filter_params pparam;
	
	if (t < 0 || t >= AM_ARRAY_SIZE(si_sections))
		return;
	
	psec = &si_sections[t];
	if (psec->hfilter == -1)
		return;
	if (psec->cur_sec > psec->last_sec && psec->hfilter != -1)
	{
		AM_DMX_StopFilter(psec->dev_no, psec->hfilter);
		AM_DMX_FreeFilter(psec->dev_no, psec->hfilter);
		psec->hfilter = -1;
		return;
	}

	AM_DMX_StopFilter(psec->dev_no, psec->hfilter);
	
	memset(&param, 0, sizeof(param));

	param.pid = psec->pid;
	param.filter.filter[0] = psec->table_id;
	param.filter.mask[0] = 0xff;
	param.filter.filter[4] = psec->cur_sec;
	param.filter.mask[4] = 0xff;

	param.flags = DMX_CHECK_CRC;
	
	AM_DMX_SetSecFilter(psec->dev_no, psec->hfilter, &param);

	AM_DMX_StartFilter(psec->dev_no, psec->hfilter);

	psec->cur_sec++;
	
}

static void si_test_restart_section(int t)
{
	SITest_Section_t *psec;
	
	if (t < 0 || t >= AM_ARRAY_SIZE(si_sections))
		return;
	
	psec = &si_sections[t];
	if (psec->hfilter != -1)
	{
		AM_DMX_StopFilter(psec->dev_no, psec->hfilter);
		AM_DMX_FreeFilter(psec->dev_no, psec->hfilter);
		psec->hfilter = -1;
	}

	AM_DMX_AllocateFilter(psec->dev_no, &psec->hfilter);
	
	AM_DMX_SetCallback(psec->dev_no, psec->hfilter, si_section_callback, NULL);

	AM_DMX_SetBufferSize(psec->dev_no, psec->hfilter, 32*1024);

	psec->cur_sec = psec->last_sec = 0;

	si_test_table_start_next_section(t);
}

static void si_test_release_all_filter()
{
	int i;

	for (i=0; i<AM_ARRAY_SIZE(si_sections); i++)
	{
		if (si_sections[i].hfilter != -1)
		{
			AM_DMX_StopFilter(si_sections[i].dev_no, si_sections[i].hfilter);
			AM_DMX_FreeFilter(si_sections[i].dev_no, si_sections[i].hfilter);
		}
	}
}


static void start_si_test()
{
	int go = 1;
	char buf[256];

	printf("********************\n");
	printf("* commands:\n");
	printf("* quit\n");
	printf("* pat\n");
	printf("* pmt\n");
	printf("* cat\n");
	printf("* sdt\n");
	printf("* eit\n");
	printf("* nit\n");
	printf("* tot\n");
	printf("********************\n");
	
	while (go)
	{
		if (fgets(buf, sizeof(buf), stdin))
		{
			if (!strncmp(buf, "pat", 3))
				si_test_restart_section(SI_TEST_PAT);
			else if (!strncmp(buf, "pmt", 3))
				si_test_restart_section(SI_TEST_PMT);
			else if (!strncmp(buf, "cat", 3))
				si_test_restart_section(SI_TEST_CAT);
			else if (!strncmp(buf, "nit", 3))
				si_test_restart_section(SI_TEST_NIT);
			else if (!strncmp(buf, "eit", 3))
				si_test_restart_section(SI_TEST_EIT);
			else if (!strncmp(buf, "sdt", 3))
				si_test_restart_section(SI_TEST_SDT);
			else if (!strncmp(buf, "tot", 3))
				si_test_restart_section(SI_TEST_TOT);
			else if (!strncmp(buf, "quit", 4))
				go = 0;
		}
	}

	si_test_release_all_filter();
}

int main(int argc, char **argv)
{
	AM_DMX_OpenPara_t para;
	AM_FEND_OpenPara_t fpara;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	int freq = 0;
	
	memset(&fpara, 0, sizeof(fpara));
	AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));
	
	if(argc>1)
	{
		sscanf(argv[1], "%d", &freq);
	}
	
	if(!freq)
	{
		freq = 618000;
	}
	
	p.frequency = freq;
	p.u.qam.symbol_rate = 6875*1000;
	p.u.qam.modulation = QAM_64;
	p.u.qam.fec_inner = FEC_AUTO;

	printf("Try lock %d MHz...\n", p.frequency/1000);
	//sleep(1);
	AM_TRY(AM_FEND_Lock(FEND_DEV_NO, &p, &status));
	
	if(status&FE_HAS_LOCK)
	{
		printf("locked\n");
	}
	else
	{
		printf("unlocked, there will be no sections received\n");
	}
	
	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO, &para));
	
	start_si_test();
	
	AM_DMX_Close(DMX_DEV_NO);
	AM_FEND_Close(FEND_DEV_NO);
	
	return 0;
}


/****************************************************************************
 * API functions
 ***************************************************************************/

