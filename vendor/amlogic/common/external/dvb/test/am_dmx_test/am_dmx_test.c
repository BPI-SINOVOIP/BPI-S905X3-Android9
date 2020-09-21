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
 * \brief
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_dmx.h>
#include <am_fend.h>
#include <am_pes.h>
#include <string.h>
#include <unistd.h>

#define FEND_DEV_NO   (0)

#define  PAT_TEST
#define  EIT_TEST
#define  NIT_TEST
#define  BAT_TEST


static int s_last_num =-1;

int freq = 0;
int layer = -1;
int src=0;
int dmx=0;
int timeout = 60*3;

static int bat=0;
static int nit=0;
static int user=0;
static int pat=0;
static int eit=0;
static int pall=0;

#define USER_MAX 10
static int u_pid[USER_MAX]={[0 ... USER_MAX-1] = -1};
static int u_para[USER_MAX]={[0 ... USER_MAX-1] = 0};
static char *u_ext[USER_MAX]={[0 ... USER_MAX-1] = NULL};
static int u_bs[USER_MAX]={[0 ... USER_MAX-1] = 0};
static int u_para_g;
static FILE *fp[USER_MAX];
static FILE *fp_e[USER_MAX];

static AM_PES_Handle_t h_pes[USER_MAX];
static char *u_path_g;
static char u_path_g_b[256];
/*
   u_para format:
	d1 - 0:sec 1:pes
	d2 - 1:crc : sec only
	d3 - 1:print
	d4 - 1:swfilter
	d5 - 1:ts_tap :pes only
	d6 - 1:w2file
*/
#define UPARA_TYPE      0xf
#define UPARA_CRC       0xf0
#define UPARA_PR        0xf00
#define UPARA_SF        0xf000
#define UPARA_DMX_TAP   0xf0000
#define UPARA_FILE      0xf00000
#define UPARA_PES2ES    0xf000000

#define get_upara(_i) (u_para[(_i)]? u_para[(_i)] : u_para_g)


static void pes_cb(AM_PES_Handle_t handle, uint8_t *buf, int size) {
	int u = (int)(long)AM_PES_GetUserData(handle);
	printf("pes cb u=%d b=%p, s:%d\n", u, buf, size);
	int ret = fwrite(buf, 1, size, fp_e[u-1]);
	if (ret != size)
		printf("data w lost\n");

}

static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	int u = (int)(long)user_data;

	if(pall) {
		int i;
		printf("data:\n");
		for(i=0;i<len;i++)
		{
			printf("%02x ", data[i]);
			if(((i+1)%16)==0) printf("\n");
		}
		if((i%16)!=0) printf("\n");
	}
#if 1
	if(bat&UPARA_PR) {
		if(data[0]==0x4a) {
			printf("sec:tabid:0x%02x,bunqid:0x%02x%02x,section num:%4d,lat_section_num:%4d\n",data[0],
				data[3],data[4],data[6],data[7]);
		}

	}
	else if(nit&UPARA_PR) {

		if(data[0]==0x40) {
			printf("section:%8d,max:%8d\n",data[6],data[7]);
			if((data[6] !=s_last_num+1)&&(s_last_num!=-1))//第一个或者不连续
			{
				if(s_last_num ==data[7])//上一个是MAX
				{
					if(data[6] != 0)//上一个MAX 这个不是 0
					{
						printf("##drop packet,tabid:0x%4x,cur:%8d,last:%8d,max:%8d\n",data[0],
						data[6],s_last_num,data[7]);
						//stop_section_flag =1;
					}
					else
					{
					}
				}
				else//上一个不是
				{
					printf("##drop packet,tabid:%4x,cur:%8d,last:%8d,max:%8d\n",data[0],
					data[6],s_last_num,data[7]);
					//stop_section_flag =1;
				}

				
			}
			else
			{
				//printf("section:%8d,",sectiondata->m_pucData[6]);
			}
			s_last_num = data[6];
		}
	}
	else if(pat&UPARA_PR) {
		if(data[0]==0x0)
			printf("%02x: %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8]);
	}
	else {
		if(!user_data) {
			printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8]);
			return;
		}

		if(get_upara(u-1)&UPARA_PR)
			printf("[%d:%d %d] %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", u-1, u_pid[u-1], len,
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
		if(get_upara(u-1)&UPARA_FILE){
			{
				int ret = fwrite(data, 1, len, fp[u-1]);
				if (ret != len)
					printf("data w lost\n");
			}
			if (get_upara(u-1)&UPARA_PES2ES) {
				if (!h_pes[u-1]) {
					AM_PES_Para_t para = {.packet = pes_cb, .user_data = (void*)(long)u, .payload_only = AM_TRUE,};
					AM_PES_Create(&h_pes[u-1], &para);
				}
				AM_PES_Decode(h_pes[u-1], data, len);
			}
		}
	}
#endif
}

static int get_section(int dmx, int timeout)
{
#ifdef PAT_TEST
	int fid;
#endif

#ifdef EIT_TEST
	int fid_eit_pf, fid_eit_opf;
#endif

#ifdef NIT_TEST
	int fid_nit;
#endif
#ifdef BAT_TEST
	int fid_bat;
#endif

	int fid_user[USER_MAX];
	int i;

	struct dmx_sct_filter_params param;
	struct dmx_pes_filter_params pparam;
#ifdef PAT_TEST
	if(pat&0xf) {
	printf("start pat...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid));
	AM_TRY(AM_DMX_SetCallback(dmx, fid, dump_bytes, NULL));
	}
#endif

#ifdef EIT_TEST
	if(eit&0xf) {
	printf("start eit...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_eit_pf));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_eit_pf, dump_bytes, NULL));
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_eit_opf));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_eit_opf, dump_bytes, NULL));
	}
#endif

#ifdef NIT_TEST
	if(nit&0xf) {
	printf("start nit...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_nit));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_nit, dump_bytes, NULL));
	}
#endif

#ifdef BAT_TEST
	if(bat&0xf) {
	printf("start bat...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_bat));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_bat, dump_bytes, NULL));
	}
#endif

	
#ifdef PAT_TEST
	if(pat&0xf) {
	memset(&param, 0, sizeof(param));
	param.pid = 0;
	param.filter.filter[0] = 0;
	param.filter.mask[0] = 0xff;
	//param.filter.filter[2] = 0x08;
	//param.filter.mask[2] = 0xff;

	param.flags = DMX_CHECK_CRC;
	if(pat&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid, &param));
	}
#endif

#ifdef EIT_TEST
	if(eit&0xf) {
	memset(&param, 0, sizeof(param));
	param.pid = 0x12;
	param.filter.filter[0] = 0x4E;
	param.filter.mask[0] = 0xff;
	param.flags = DMX_CHECK_CRC;	
	if(eit&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_eit_pf, &param));
	
	memset(&param, 0, sizeof(param));
	param.pid = 0x12;
	param.filter.filter[0] = 0x4F;
	param.filter.mask[0] = 0xff;
	param.flags = DMX_CHECK_CRC;	
	if(eit&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_eit_opf, &param));
	}
#endif

#ifdef NIT_TEST
	if(nit&0xF) {
	memset(&param, 0, sizeof(param));
	param.pid = 0x10;
	param.filter.filter[0] = 0x40;
	param.filter.mask[0] = 0xff;
	if(nit&UPARA_CRC)
		param.flags = DMX_CHECK_CRC;	
	if(nit&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_nit, &param));
	}
#endif

#ifdef BAT_TEST
	if(bat&0xF) {
	memset(&param, 0, sizeof(param));
	param.pid = 0x11;
	param.filter.filter[0] = 0x4a;
	param.filter.mask[0] = 0xff;
	if(bat&UPARA_CRC)
		param.flags = DMX_CHECK_CRC;	
	if(bat&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_bat, &param));
	}
#endif


#ifdef PAT_TEST
	if(pat&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid, 32*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid));
	}
#endif
#ifdef EIT_TEST
	if(eit&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_eit_pf, 32*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid_eit_pf));
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_eit_opf, 32*1024));
	//AM_TRY(AM_DMX_StartFilter(dmx, fid_eit_opf));
	}
#endif

#ifdef NIT_TEST
	if(nit&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_nit, 32*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid_nit));
	}
#endif

#ifdef BAT_TEST
	if(bat&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_bat, 64*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid_bat));
	}
#endif

	for(i=0; i<USER_MAX; i++) {
		if(u_pid[i]!=-1) {
			AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_user[i]));

			AM_TRY(AM_DMX_SetCallback(dmx, fid_user[i], dump_bytes, (void*)(long)(i+1)));

			if(u_bs[i]) {
				AM_TRY(AM_DMX_SetBufferSize(dmx, fid_user[i], u_bs[i]));
				printf("buffersize => %d\n", u_bs[i]);
			}

			if(get_upara(i)&UPARA_TYPE) {/*pes*/
				memset(&pparam, 0, sizeof(pparam));
				pparam.pid = u_pid[i];
				pparam.pes_type = DMX_PES_OTHER;
				pparam.input = DMX_IN_FRONTEND;
				pparam.output = DMX_OUT_TAP;
				if(get_upara(i)&UPARA_DMX_TAP)
					pparam.output = DMX_OUT_TSDEMUX_TAP;
				if(get_upara(i)&UPARA_SF)
					pparam.flags |= 0x100;
				AM_TRY(AM_DMX_SetPesFilter(dmx, fid_user[i], &pparam));

			} else {/*sct*/
				int v[16] = {[0 ... 15] = 0};
				int m[16] = {[0 ... 15] = 0};
				int ii;
				memset(&param, 0, sizeof(param));
				param.pid = u_pid[i];
				if(u_ext[i]) {
				sscanf(u_ext[i], "%x:%x,%x:%x,%x:%x,%x:%x"
					",%x:%x,%x:%x,%x:%x,%x:%x"
					",%x:%x,%x:%x,%x:%x,%x:%x"
					",%x:%x,%x:%x,%x:%x,%x:%x",
					&v[0], &m[0], &v[1], &m[1],
					&v[2], &m[2], &v[3], &m[3],
					&v[4], &m[4], &v[5], &m[5],
					&v[6], &m[6], &v[7], &m[7],
					&v[8], &m[8], &v[9], &m[9],
					&v[10], &m[10], &v[11], &m[11],
					&v[12], &m[12], &v[13], &m[13],
					&v[14], &m[14], &v[15], &m[15]);
				for(ii=0; ii<16; ii++) {
					if(m[ii]) {
						param.filter.filter[ii] = v[ii];
						param.filter.mask[ii] = m[ii];
						printf("ext%d: [%d]%x:%x\n", i, ii, v[ii], m[ii]);
					}
				}
				}
				if(get_upara(i)&UPARA_CRC)
					param.flags = DMX_CHECK_CRC;
				if(get_upara(i)&UPARA_SF)
					param.flags |= 0x100;
				AM_TRY(AM_DMX_SetSecFilter(dmx, fid_user[i], &param));
			}

			if(get_upara(i)&UPARA_FILE) {
				char name[32];
				sprintf(name, "%s/u_%d.dump", u_path_g, i);
				fp[i] = fopen(name, "wb");
				if (fp[i])
					printf("file open:[%s]\n", name);
				sprintf(name, "%s/u_%d.es.dump", u_path_g, i);
				fp_e[i] = fopen(name, "wb");
				if (fp_e[i])
					printf("file open:[%s]\n", name);
			}

			AM_TRY(AM_DMX_StartFilter(dmx, fid_user[i]));
		}
	}

	sleep(timeout);

#ifdef PAT_TEST
	if(pat&0xF) {
	AM_TRY(AM_DMX_StopFilter(dmx, fid));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid));
	}
#endif	
#ifdef EIT_TEST
	if(eit&0xF) {
	AM_TRY(AM_DMX_StopFilter(dmx, fid_eit_pf));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_eit_pf));
	AM_TRY(AM_DMX_StopFilter(dmx, fid_eit_opf));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_eit_opf));
	}
#endif
#ifdef NIT_TEST
	if(nit&0xF){
	AM_TRY(AM_DMX_StopFilter(dmx, fid_nit));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_nit));
	}
#endif	
#ifdef BAT_TEST
	if(bat&0xF) {
	AM_TRY(AM_DMX_StopFilter(dmx, fid_bat));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_bat));
	}
#endif	
	
	for(i=0; i<USER_MAX; i++) {
		if(u_pid[i]!=-1) {
			AM_TRY(AM_DMX_StopFilter(dmx, fid_user[i]));
			AM_TRY(AM_DMX_FreeFilter(dmx, fid_user[i]));
			if((get_upara(i)&UPARA_FILE) && fp[i])
				fclose(fp[i]);
		}
	}
	
	return 0;
}

static int setlayer(int layer/*1/2/4/7*/)
{
	AM_ErrorCode_t ret;

	struct dtv_property p = {.cmd=DTV_ISDBT_LAYER_ENABLED, .u.data = layer};
	struct dtv_properties props = {.num=1, .props=&p};
	printf("AM FEND SetProp layer:%d\n", props.props[0].u.data);
	ret = AM_FEND_SetProp(FEND_DEV_NO, &props);
	return 0;
}

int get_para(char *argv)
{
	#define CASE(name, len, type, var) \
		if(!strncmp(argv, name"=", (len)+1)) { \
			sscanf(&argv[(len)+1], type, &var); \
			printf("param["name"] => "type"\n", var); \
		}
	#define CASESTR(name, len, type, var) \
		if(!strncmp(argv, name"=", (len)+1)) { \
			var = &argv[(len)+1]; \
			printf("param["name"] => "type"\n", var); \
		}

	CASE("freq",      4, "%i", freq)
	else CASE("src",  3, "%i", src)
	else CASE("dmx",  3, "%i", dmx)
	else CASE("pat",  3, "%x", pat)
	else CASE("eit",  3, "%x", eit)
	else CASE("layer",5, "%i", layer)
	else CASE("bat",  3, "%x", bat)
	else CASE("nit",  3, "%x", nit)
	else CASE("timeout", 7, "%i", timeout)
	else CASE("pall", 4, "%i", pall)
	else CASE("pid0", 4, "%i", u_pid[0])
	else CASE("pid1", 4, "%i", u_pid[1])
	else CASE("pid2", 4, "%i", u_pid[2])
	else CASE("pid3", 4, "%i", u_pid[3])
	else CASE("pid4", 4, "%i", u_pid[4])
	else CASE("para0", 5, "%x", u_para[0])
	else CASE("para1", 5, "%x", u_para[1])
	else CASE("para2", 5, "%x", u_para[2])
	else CASE("para3", 5, "%x", u_para[3])
	else CASE("para4", 5, "%x", u_para[4])
	else CASESTR("ext0", 4, "%s", u_ext[0])
	else CASESTR("ext1", 4, "%s", u_ext[1])
	else CASESTR("ext2", 4, "%s", u_ext[2])
	else CASESTR("ext3", 4, "%s", u_ext[3])
	else CASESTR("ext4", 4, "%s", u_ext[4])
	else CASE("bs0", 3, "%i", u_bs[0])
	else CASE("bs1", 3, "%i", u_bs[1])
	else CASE("bs2", 3, "%i", u_bs[2])
	else CASE("bs3", 3, "%i", u_bs[3])
	else CASE("bs4", 3, "%i", u_bs[4])
	else CASE("para", 4, "%x", u_para_g)
	else CASESTR("path", 4, "%s", u_path_g)

	if (u_path_g) {
		char *e = strchr(u_path_g, ' ');
		if (e) {
			int l = e - u_path_g;
			memcpy(u_path_g_b, u_path_g, l);
			u_path_g_b[l] = '\0';
			u_path_g = u_path_g_b;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	AM_DMX_OpenPara_t para;
	AM_FEND_OpenPara_t fpara;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	int ret=0;
	int i;

	memset(&fpara, 0, sizeof(fpara));

	if(argc==1)
	{
		printf(
			"Usage:%s [freq=] [src=] [dmx=] [layer=] [timeout=] [pat=] [eit=] [bat=] [nit=] [pidx=] [parax=] [para=] [extx=] [bsx=] [out=]\n"
			"  default   - src:0 dmx:0 layer:-1 parax:0\n"
			"  x         - 0~4\n"
			"  para      - d6->|111111|<-d1\n"
			"    d1 - 0:sec 1:pes (means enable for pat/eit/bat/nit)\n"
			"    d2 - 1:crc : sec only\n"
			"    d3 - 1:print\n"
			"    d4 - 1:swfilter\n"
			"    d5 - 1:ts tap : pes only\n"
			"    d6 - 1:w2file\n"
			"    d7 - 1:convert pes to es format, pes only\n"
			"  ext       - tid....\n"
			"    eg. 0x82:0xff,0x02:0xff\n"
			"    up to 16 filter data:mask(s)\n"
			"  bs        - buffersize\n"
			"  path      - output path\n"
			, argv[0]);
		return 0;
	}

	for(i=1; i< argc; i++)
		get_para(argv[i]);

#if 1
	if(freq>0)
	{
		AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));

		p.frequency = freq;
#if 0
		p.inversion = INVERSION_AUTO;
		p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
		p.u.ofdm.code_rate_HP = FEC_AUTO;
		p.u.ofdm.code_rate_LP = FEC_AUTO;
		p.u.ofdm.constellation = QAM_AUTO;
		p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
		p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
		p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
#else		
		p.u.qam.symbol_rate = 6875000;
		p.u.qam.fec_inner = FEC_AUTO;
		p.u.qam.modulation = QAM_64;
#endif		
	
		AM_TRY(AM_FEND_Lock(FEND_DEV_NO, &p, &status));
		
		if(status&FE_HAS_LOCK)
			printf("locked\n");
		else
			printf("unlocked\n");
	}
	
	if(layer!=-1)
		setlayer(layer);

#endif	
	memset(&para, 0, sizeof(para));
	//para.use_sw_filter = AM_TRUE;
	//para.dvr_fifo_no = 1;
	AM_TRY(AM_DMX_Open(dmx, &para));

	AM_TRY(AM_DMX_SetSource(dmx, src));
	printf("TS SRC = %d\n", src);

	get_section(dmx, timeout);
	
	AM_DMX_Close(dmx);

	if(freq)
		AM_FEND_Close(FEND_DEV_NO);

	return ret;
}

