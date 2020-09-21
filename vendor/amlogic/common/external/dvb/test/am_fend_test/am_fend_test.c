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
 * \brief DVB前端测试
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-08: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_fend.h>
#include <am_fend_diseqc_cmd.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <am_misc.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define FEND_DEV_NO    (0)

#define MAX_DISEQC_LENGTH  16

static int ofdm_mode = 0;
static unsigned int blindscan_process = 0;

#define scanf(a...) \
	do { \
		char c; \
		scanf(a); \
		while ( (c = getchar()) != '\n' && c != EOF ) ; \
	}while(0)

/****************************************************************************
 * Functions
 ***************************************************************************/

static void fend_cb(int dev_no, struct dvb_frontend_event *evt, void *user_data)
{
	fe_status_t status;
	int ber, snr, strength;
	struct dvb_frontend_info info;

	AM_FEND_GetInfo(dev_no, &info);
	if(info.type == FE_QAM) {
		printf("cb parameters: freq:%d srate:%d modulation:%d fec_inner:%d\n",
			evt->parameters.frequency, evt->parameters.u.qam.symbol_rate,
			evt->parameters.u.qam.modulation, evt->parameters.u.qam.fec_inner);
	} else if(info.type == FE_OFDM) {
		printf("cb parameters: freq:%d bandwidth:%d \n",
			evt->parameters.frequency, evt->parameters.u.ofdm.bandwidth);
	} else if(info.type == FE_QPSK) {	
		printf("cb parameters: * can get fe type qpsk! *\n");
	} else if(info.type == FE_ANALOG){
		//printf("cb parameters: std:%#X\n", evt->parameters.u.analog.std);
		struct dvb_frontend_parameters para;
		AM_FEND_GetPara(dev_no, &para);
		printf("cb parameters: std:%#X\n", para.u.analog.std);
	} else if(info.type == FE_ATSC) {
		printf("cb parameters: freq:%d modulation:%d\n",
			evt->parameters.frequency,
			evt->parameters.u.vsb.modulation);
	}else {
		printf("cb parameters: * can not get fe type! *\n");
	}
	printf("cb status: 0x%x\n", evt->status);
	
	AM_FEND_GetStatus(dev_no, &status);
	AM_FEND_GetBER(dev_no, &ber);
	AM_FEND_GetSNR(dev_no, &snr);
	AM_FEND_GetStrength(dev_no, &strength);
	
	printf("cb status: 0x%0x ber:%d snr:%d, strength:%d\n", status, ber, snr, strength);
}

static void blindscan_cb(int dev_no, AM_FEND_BlindEvent_t *evt, void *user_data)
{
	if(evt->status == AM_FEND_BLIND_START)
	{
		printf("++++++blindscan_start %u\n", evt->freq);
	}
	else if(evt->status == AM_FEND_BLIND_UPDATEPROCESS)
	{
		blindscan_process = evt->process;
		printf("++++++blindscan_process %u\n", blindscan_process);
	}
	else if(evt->status == AM_FEND_BLIND_UPDATETP)
	{
		printf("++++++blindscan_tp\n");
	}
}

static void SetDiseqcCommandString(int dev_no, const char *str)
{
	struct dvb_diseqc_master_cmd diseqc_cmd;
	
	if (!str)
		return;
	diseqc_cmd.msg_len=0;
	int slen = strlen(str);
	if (slen % 2)
	{
		AM_DEBUG(1, "%s", "invalid diseqc command string length (not 2 byte aligned)");
		return;
	}
	if (slen > MAX_DISEQC_LENGTH*2)
	{
		AM_DEBUG(1, "%s", "invalid diseqc command string length (string is to long)");
		return;
	}
	unsigned char val=0;
	int i=0; 
	for (i=0; i < slen; ++i)
	{
		unsigned char c = str[i];
		switch(c)
		{
			case '0' ... '9': c-=48; break;
			case 'a' ... 'f': c-=87; break;
			case 'A' ... 'F': c-=55; break;
			default:
				AM_DEBUG(1, "%s", "invalid character in hex string..ignore complete diseqc command !");
				return;
		}
		if ( i % 2 )
		{
			val |= c;
			diseqc_cmd.msg[i/2] = val;
		}
		else
			val = c << 4;
	}
	diseqc_cmd.msg_len = slen/2;

	AM_FEND_DiseqcSendMasterCmd(dev_no, &diseqc_cmd); 

	return;
}

static void sec(int dev_no)
{
	int sec = -1;
	AM_Bool_t sec_exit = AM_FALSE;
	
	printf("sec_control\n");
	while(1)
	{
		printf("-----------------------------\n");
		printf("DiseqcResetOverload-0\n");
		printf("DiseqcSendBurst-1\n");
		printf("SetTone-2\n");
		printf("SetVoltage-3\n");
		printf("EnableHighLnbVoltage-4\n");
		printf("Diseqccmd_ResetDiseqcMicro-5\n");
		printf("Diseqccmd_StandbySwitch-6\n");
		printf("Diseqccmd_PoweronSwitch-7\n");
		printf("Diseqccmd_SetLo-8\n");
		printf("Diseqccmd_SetVR-9\n");
		printf("Diseqccmd_SetSatellitePositionA-10\n");
		printf("Diseqccmd_SetSwitchOptionA-11\n");
		printf("Diseqccmd_SetHi-12\n");
		printf("Diseqccmd_SetHL-13\n");
		printf("Diseqccmd_SetSatellitePositionB-14\n");
		printf("Diseqccmd_SetSwitchOptionB-15\n");
		printf("Diseqccmd_SetSwitchInput-16\n");
		printf("Diseqccmd_SetLNBPort4-17\n");
		printf("Diseqccmd_SetLNBPort16-18\n");		
		printf("Diseqccmd_SetChannelFreq-19\n");
		printf("Diseqccmd_SetPositionerHalt-20\n");
		printf("Diseqccmd_EnablePositionerLimit-21\n");
		printf("Diseqccmd_DisablePositionerLimit-22\n");
		printf("Diseqccmd_SetPositionerELimit-23\n");
		printf("Diseqccmd_SetPositionerWLimit-24\n");
		printf("Diseqccmd_PositionerGoE-25\n");
		printf("Diseqccmd_PositionerGoW-26\n");	
		printf("Diseqccmd_StorePosition-27\n");
		printf("Diseqccmd_GotoPositioner-28\n");	
		printf("Diseqccmd_GotoxxAngularPositioner-29\n");
		printf("Diseqccmd_GotoAngularPositioner-30\n");			
		printf("Diseqccmd_SetODUChannel-31\n");
		printf("Diseqccmd_SetODUPowerOff-32\n");	
		printf("Diseqccmd_SetODUUbxSignalOn-33\n");
		printf("Diseqccmd_SetString-34\n");
		printf("Exit sec-35\n");
		printf("-----------------------------\n");
		printf("select\n");
		scanf("%d", &sec);
		switch(sec)
		{
			case 0:
				AM_FEND_DiseqcResetOverload(dev_no); 
				break;
				
			case 1:
				{
					int minicmd;
					printf("minicmd_A-0/minicmd_B-1\n");
					scanf("%d", &minicmd);
					AM_FEND_DiseqcSendBurst(dev_no,minicmd);
					break;
				}
				
			case 2:
				{
					int tone;
					printf("on-0/off-1\n");
					scanf("%d", &tone);				
					AM_FEND_SetTone(dev_no, tone); 
					break;
				}

			case 3:
				{
					int voltage;
					printf("v13-0/v18-1/v_off-2\n");
					scanf("%d", &voltage);
					AM_FEND_SetVoltage(dev_no, voltage);				
					break;
				}

			case 4:
				{
					int enable;
					printf("disable-0/enable-1/\n");
					scanf("%d", &enable);				
					AM_FEND_EnableHighLnbVoltage(dev_no, (long)enable);  
					break;
				}

			case 5:
				AM_FEND_Diseqccmd_ResetDiseqcMicro(dev_no);
				break;

			case 6:
				AM_FEND_Diseqccmd_StandbySwitch(dev_no);
				break;
				
			case 7:
				AM_FEND_Diseqccmd_PoweronSwitch(dev_no);
				break;
				
			case 8:
				AM_FEND_Diseqccmd_SetLo(dev_no);
				break;
				
			case 9:
				AM_FEND_Diseqccmd_SetVR(dev_no); 
				break;
				
			case 10:
				AM_FEND_Diseqccmd_SetSatellitePositionA(dev_no); 
				break;
				
			case 11:
				AM_FEND_Diseqccmd_SetSwitchOptionA(dev_no);
				break;
				
			case 12:
				AM_FEND_Diseqccmd_SetHi(dev_no);
				break;
				
			case 13:
				AM_FEND_Diseqccmd_SetHL(dev_no);
				break;
				
			case 14:
				AM_FEND_Diseqccmd_SetSatellitePositionB(dev_no); 
				break;
				
			case 15:
				AM_FEND_Diseqccmd_SetSwitchOptionB(dev_no); 
				break;
				
			case 16:
				{
					int input;
					printf("s1ia-1/s2ia-2/s3ia-3/s4ia-4/s1ib-5/s2ib-6/s3ib-7/s4ib-8/\n");
					scanf("%d", &input);	
					AM_FEND_Diseqccmd_SetSwitchInput(dev_no, input);
					break;
				}
				
			case 17:
				{
					int lnbport, polarisation, local_oscillator_freq;
					printf("lnbport-1-4:AA=0, AB=1, BA=2, BB=3, SENDNO=4\n");
					scanf("%d", &lnbport);
					printf("polarisation:H-0/V-1/NO-2\n");
					scanf("%d", &polarisation);
					printf("local_oscillator_freq:L-0/H-1/NO-2\n");
					scanf("%d", &local_oscillator_freq);				
					AM_FEND_Diseqccmd_SetLNBPort4(dev_no, lnbport, polarisation, local_oscillator_freq);
					break;
				}

			case 18:
				{
					int lnbport, polarisation, local_oscillator_freq;
					printf("lnbport-1-16:0xF0 .. 0xFF\n");
					scanf("%d", &lnbport);
					printf("polarisation:H-0/V-1/NO-2\n");
					scanf("%d", &polarisation);
					printf("polarisation:L-0/H-1/NO-2\n");
					scanf("%d", &local_oscillator_freq);				
					AM_FEND_Diseqccmd_SetLNBPort16(dev_no, lnbport, polarisation, local_oscillator_freq);
					break;
				}
                                                                  
			case 19:
				{
					int freq;
					printf("frequency(KHz): ");		
					scanf("%d", &freq);				
					AM_FEND_Diseqccmd_SetChannelFreq(dev_no, freq);
					break;
				}

			case 20:
				AM_FEND_Diseqccmd_SetPositionerHalt(dev_no);
				break;                                                                  

			case 21:
				AM_FEND_Diseqccmd_EnablePositionerLimit(dev_no);
				break;

			case 22:
				AM_FEND_Diseqccmd_DisablePositionerLimit(dev_no);
				break;

			case 23:
				AM_FEND_Diseqccmd_SetPositionerELimit(dev_no);
				break;

			case 24:
				AM_FEND_Diseqccmd_SetPositionerWLimit(dev_no);
				break;
				
			case 25:
				{
					unsigned char unit;
					printf("unit continue-0 second-1-127 step-128-255: ");		
					scanf("%d", &unit);				
					AM_FEND_Diseqccmd_PositionerGoE(dev_no, unit);
					break;
				}

			case 26:
				{
					unsigned char unit;
					printf("unit continue-0 second-1-127 step-128-255: ");		
					scanf("%d", &unit);				
					AM_FEND_Diseqccmd_PositionerGoW(dev_no, unit);
					break;
				}

			case 27:
				{
					unsigned char position;
					printf("position 0-255: ");		
					scanf("%d", &position);							
					AM_FEND_Diseqccmd_StorePosition(dev_no, position);
					break;
				}

			case 28:
				{
					unsigned char position;
					printf("position 0-255: ");		
					scanf("%d", &position);				
					AM_FEND_Diseqccmd_GotoPositioner(dev_no, position); 
					break;
				}

			case 29:
				{
					double local_longitude, local_latitude, satellite_longitude;
					printf("local_longitude: ");		
					scanf("%lf", &local_longitude);
					printf("local_latitude: ");		
					scanf("%lf", &local_latitude);
					printf("satellite_longitude: ");		
					scanf("%lf", &satellite_longitude);				
					AM_FEND_Diseqccmd_GotoxxAngularPositioner(dev_no, local_longitude, local_latitude, satellite_longitude);
	                            break; 
				}   

			case 30:
				{
					double local_longitude, local_latitude, satellite_longitude;
					printf("local_longitude: ");		
					scanf("%lf", &local_longitude);
					printf("local_latitude: ");		
					scanf("%lf", &local_latitude);
					printf("satellite_longitude: ");		
					scanf("%lf", &satellite_longitude);				
					AM_FEND_Diseqccmd_GotoAngularPositioner(dev_no, local_longitude, local_latitude, satellite_longitude);
	                            break; 
				}                                 

			case 31:
				{
					unsigned char ub_number;
					printf("ub_number 0-7: ");		
					scanf("%d", &ub_number);	
					unsigned char inputbank_number;
					printf("inputbank_number 0-7: ");		
					scanf("%d", &inputbank_number);	
					int transponder_freq;
					printf("transponder_freq(KHz): ");		
					scanf("%d", &transponder_freq);	
					int oscillator_freq;
					printf("oscillator_freq(KHz): ");		
					scanf("%d", &oscillator_freq);	
					int ub_freq;
					printf("ub_freq(KHz): ");		
					scanf("%d", &ub_freq);					
					AM_FEND_Diseqccmd_SetODUChannel(dev_no, ub_number, inputbank_number, transponder_freq, oscillator_freq, ub_freq);
					break;
				}

			case 32:
				{
					unsigned char ub_number;
					printf("ub_number 0-7: ");		
					scanf("%d", &ub_number);					
					AM_FEND_Diseqccmd_SetODUPowerOff(dev_no, ub_number); 
					break;
				}

			case 33:
				AM_FEND_Diseqccmd_SetODUUbxSignalOn(dev_no);
                            break;                           

			case 34:
				{
					char str[13] = {0};

					printf("diseqc cmd str: ");		
					scanf("%s", str);						
					SetDiseqcCommandString(dev_no, str);
				}
				break;

			case 35:
				sec_exit = AM_TRUE;
				break;
				
			default:
				break;
		}

		sec = -1;

		if(sec_exit == AM_TRUE)
		{
			break;
		}

		usleep(15 *1000);
		
	}

	return;
}


static void show_cmds(void)
{
	printf("Commands:\n");
	printf("\tlock\n");
	printf("\tinfo\n");
	printf("\tstat\n");
	printf("\topen\n");
	printf("\tquit\n");
	printf(">");
	fflush(stdout);
}





static int open_fend(int *id, int *mode)
{
	AM_FEND_OpenPara_t para;

	if(*id>=0){
		AM_FEND_Close(*id);
		*id=-1;
		*mode =-1;
	}

	memset(&para, 0, sizeof(para));
	
	printf("Input fontend id, id=-1 quit\n");
	printf("id: ");
	scanf("%d", id);
	if(*id<0)
		return -2;

	printf("Input fontend mode: (0-DVBC, 1-DVBT, 2-DVBS, 3-ISDBT, 4-DTMB, 5-DVBT2, 6-ANALOG,7-ATSC)\n");
	printf("mode(0/1/2/3/4/5/6/7): ");
	scanf("%d", mode);
	para.mode = (*mode==0)?FE_QAM : 
			(*mode==1)? FE_OFDM : 
			(*mode==2)? FE_QPSK :
			(*mode==3)? FE_ISDBT :
			(*mode==4)? FE_DTMB :
			(*mode==5)? FE_OFDM :
			(*mode==6)? FE_ANALOG :
			(*mode==7)? FE_ATSC :
				FE_OFDM;

	AM_TRY(AM_FEND_Open(*id, &para));

	AM_FEND_SetMode(*id, para.mode);

	return 0;
}

static int lock_fend(int id, int mode)
{
	fe_status_t status;

	static int blind_scan = 0;
	static struct dvb_frontend_parameters blindscan_para[128];
	unsigned int count = 128;

	static struct dvb_frontend_parameters p;
	static int freq, srate, qam;
	static int bw;
	static int std, afc, afc_range;

	/*(0-DVBC, 1-DVBT, 2-DVBS, 3-ISDBT, 4-DTMB, 5-DVBT2, 6-ANALOG, 7-ATSC)*/

	if(mode==-1)
		return -1;

	if(mode == 2) {
		printf("blindscan(0/1): ");
		scanf("%d", &blind_scan);
		if(blind_scan == 1)
		{
			AM_FEND_BlindScan(id, blindscan_cb, (void *)(long)id, 950000000, 2150000000);
			while(1){
				if(blindscan_process == 100){
					blindscan_process = 0;
					break;
				}
				//printf("wait process %u\n", blindscan_process);
				usleep(500 * 1000);
			}

			AM_FEND_BlindExit(id); 

			printf("start AM_FEND_BlindGetTPInfo\n");
			
			AM_FEND_BlindGetTPInfo(id, blindscan_para, count);

			printf("dump TPInfo: %d\n", count);

			int i = 0;
			
			printf("\n\n");
			for(i=0; i < count; i++)
			{
				printf("Ch%2d: RF: %4d SR: %5d ",i+1, (blindscan_para[i].frequency/1000),(blindscan_para[i].u.qpsk.symbol_rate/1000));
				printf("\n");
			}	

			blind_scan = 0;
		}
	}
	
	AM_TRY(AM_FEND_SetCallback(id, fend_cb, NULL));
	
	printf("input frontend parameters, frequency=0 quit\n");
	if(mode != 2) {
		printf("frequency(Hz): ");
	} else {
		sec(id);
		printf("frequency(KHz): ");
	}
	
	scanf("%d", &freq);
	if(!freq)
		return -2;
	
	if(mode==0) {
		printf("symbol rate(kbps): ");
		scanf("%d", &srate);
		printf("QAM(16/32/64/128/256): ");
		scanf("%d", &qam);
		
		p.frequency = freq;
		p.u.qam.symbol_rate = srate*1000;
		p.u.qam.fec_inner = FEC_AUTO;
		switch(qam)
		{
			case 16:
				p.u.qam.modulation = QAM_16;
			break;
			case 32:
				p.u.qam.modulation = QAM_32;
			break;
			case 64:
			default:
				p.u.qam.modulation = QAM_64;
			break;
			case 128:
				p.u.qam.modulation = QAM_128;
			break;
			case 256:
				p.u.qam.modulation = QAM_256;
			break;
		}
	}else if(mode==1 || mode==3 || mode==4 || mode==5){
		printf("BW[8/7/6/5(AUTO) MHz]: ");
		scanf("%d", &bw);

		if(mode==1) {
			printf("T2?[0/1]: ");
			scanf("%d", &ofdm_mode);
			
		} else if(mode == 5) {
			printf("set to T2\n");
			ofdm_mode = OFDM_DVBT2;
		}
			
		p.frequency = freq;
		switch(bw)
		{
			case 8:
			default:
				p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
			break;
			case 7:
				p.u.ofdm.bandwidth = BANDWIDTH_7_MHZ;
			break;
			case 6:
				p.u.ofdm.bandwidth = BANDWIDTH_6_MHZ;
			break;
			case 5:
				p.u.ofdm.bandwidth = BANDWIDTH_AUTO;
			break;
		}

		p.u.ofdm.code_rate_HP = FEC_AUTO;
		p.u.ofdm.code_rate_LP = FEC_AUTO;
		p.u.ofdm.constellation = QAM_AUTO;
		p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
		p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
		p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
	}else if (mode == 6) {
		typedef __u64 v4l2_std_id;

		/* one bit for each */
		#define V4L2_STD_PAL_B          ((v4l2_std_id)0x00000001)
		#define V4L2_STD_PAL_B1         ((v4l2_std_id)0x00000002)
		#define V4L2_STD_PAL_G          ((v4l2_std_id)0x00000004)
		#define V4L2_STD_PAL_H          ((v4l2_std_id)0x00000008)
		#define V4L2_STD_PAL_I          ((v4l2_std_id)0x00000010)
		#define V4L2_STD_PAL_D          ((v4l2_std_id)0x00000020)
		#define V4L2_STD_PAL_D1         ((v4l2_std_id)0x00000040)
		#define V4L2_STD_PAL_K          ((v4l2_std_id)0x00000080)

		#define V4L2_STD_PAL_M          ((v4l2_std_id)0x00000100)
		#define V4L2_STD_PAL_N          ((v4l2_std_id)0x00000200)
		#define V4L2_STD_PAL_Nc         ((v4l2_std_id)0x00000400)
		#define V4L2_STD_PAL_60         ((v4l2_std_id)0x00000800)

		#define V4L2_STD_NTSC_M         ((v4l2_std_id)0x00001000)	/* BTSC */
		#define V4L2_STD_NTSC_M_JP      ((v4l2_std_id)0x00002000)	/* EIA-J */
		#define V4L2_STD_NTSC_443       ((v4l2_std_id)0x00004000)
		#define V4L2_STD_NTSC_M_KR      ((v4l2_std_id)0x00008000)	/* FM A2 */

		#define V4L2_STD_SECAM_B        ((v4l2_std_id)0x00010000)
		#define V4L2_STD_SECAM_D        ((v4l2_std_id)0x00020000)
		#define V4L2_STD_SECAM_G        ((v4l2_std_id)0x00040000)
		#define V4L2_STD_SECAM_H        ((v4l2_std_id)0x00080000)
		#define V4L2_STD_SECAM_K        ((v4l2_std_id)0x00100000)
		#define V4L2_STD_SECAM_K1       ((v4l2_std_id)0x00200000)
		#define V4L2_STD_SECAM_L        ((v4l2_std_id)0x00400000)
		#define V4L2_STD_SECAM_LC       ((v4l2_std_id)0x00800000)

		/* ATSC/HDTV */
		#define V4L2_STD_ATSC_8_VSB     ((v4l2_std_id)0x01000000)
		#define V4L2_STD_ATSC_16_VSB    ((v4l2_std_id)0x02000000)

		/*COLOR MODULATION TYPE*/
		#define V4L2_COLOR_STD_PAL	((v4l2_std_id)0x04000000)
		#define V4L2_COLOR_STD_NTSC	((v4l2_std_id)0x08000000)
		#define V4L2_COLOR_STD_SECAM	((v4l2_std_id)0x10000000)

		/*para2 is finetune data */
		printf("std(-1=default): ");
		scanf("%d", &std);
		printf("afc_range(0=off, -1=default): ");
		scanf("%d", &afc_range);
		if (std == -1)
			std = V4L2_COLOR_STD_PAL | V4L2_STD_PAL_D | V4L2_STD_PAL_D1  | V4L2_STD_PAL_K;

		if (afc_range == -1)
			afc_range = 1000000;
		afc = 0;
		if (afc_range == 0)
			afc &= ~ANALOG_FLAG_ENABLE_AFC;
		else
			afc |= ANALOG_FLAG_ENABLE_AFC;

	        p.u.analog.std   =   std;
	        p.u.analog.afc_range = afc_range;
		p.u.analog.flag = afc;
		p.frequency = freq;

	        printf("freq=%dHz, std=%#X, flag=%x, afc_range=%d\n", freq, std, afc, afc_range);

	} else if (mode==7){
		printf("MODULATION(8:VSB8/16:VSB16/64:QAM64/128:QAM128/256:QAM256): ");
		scanf("%d", &qam);
		switch (qam)
		{
			case 8:
			default:
				p.u.vsb.modulation = VSB_8;
			break;
			case 16:
				p.u.vsb.modulation = VSB_16;
			break;
			case 64:
				p.u.vsb.modulation = QAM_64;
			break;
			case 128:
				p.u.vsb.modulation = QAM_128;
			break;
			case 256:
				p.u.vsb.modulation = QAM_256;
			break;
		}
		p.frequency = freq;
	} else {
		printf("dvb sx test\n");

		p.frequency = freq;

		printf("symbol rate: ");
		scanf("%d", &(p.u.qpsk.symbol_rate));
	}
#if 0
	if (mode == 10) {
		AM_TRY(AM_FEND_SetPara(id, &p));
	} else
#else
	{
		/*add set sub sys*/
		if (mode == 1 || mode == 5) {
			struct dtv_properties prop;
			struct dtv_property property;

			prop.num = 1;
			prop.props = &property;
			memset(&property, 0, sizeof(property));
			property.cmd = DTV_DELIVERY_SUB_SYSTEM;
			property.u.data = ofdm_mode;
			AM_FEND_SetProp(id, &prop);
			
		}
		AM_TRY(AM_FEND_Lock(id, &p, &status));
		printf("lock status: 0x%x\n", status);
	}
#endif

	return 0;
}

static int info_fend(int id)
{
	struct dvb_frontend_info info;

	AM_TRY(AM_FEND_GetInfo(id,&info));


	printf("info:\n");
	#define P(_f, _a) printf("\t%s: "_f"\n", #_a, _a)
	P("%d", info.type);
	P("0x%x", info.caps);
	printf("\t");
	#define CHK(_cap) if((info.caps)&(_cap)) printf(" %s", #_cap)
	CHK(FE_IS_STUPID);
	CHK(FE_CAN_INVERSION_AUTO);
	CHK(FE_CAN_FEC_1_2);
	CHK(FE_CAN_FEC_2_3);
	CHK(FE_CAN_FEC_3_4);
	CHK(FE_CAN_FEC_4_5);
	CHK(FE_CAN_FEC_5_6);
	CHK(FE_CAN_FEC_6_7);
	CHK(FE_CAN_FEC_7_8);
	CHK(FE_CAN_FEC_8_9);
	CHK(FE_CAN_FEC_AUTO	);
	CHK(FE_CAN_QPSK	);
	CHK(FE_CAN_QAM_16);
	CHK(FE_CAN_QAM_32);
	CHK(FE_CAN_QAM_64);
	CHK(FE_CAN_QAM_128);
	CHK(FE_CAN_QAM_256);
	CHK(FE_CAN_QAM_AUTO);
	CHK(FE_CAN_TRANSMISSION_MODE_AUTO);
	CHK(FE_CAN_BANDWIDTH_AUTO);
	CHK(FE_CAN_GUARD_INTERVAL_AUTO);
	CHK(FE_CAN_HIERARCHY_AUTO);
	CHK(FE_CAN_8VSB);
	CHK(FE_CAN_16VSB);
	CHK(FE_HAS_EXTENDED_CAPS);
	CHK(FE_CAN_MULTISTREAM);
	CHK(FE_CAN_TURBO_FEC);
	CHK(FE_CAN_2G_MODULATION);
	CHK(FE_NEEDS_BENDING);
	CHK(FE_CAN_RECOVER);
	CHK(FE_CAN_MUTE_TS);
	printf("\n");
	P("%s", info.name);
	P("%u", info.frequency_min);
	P("%u", info.frequency_max);
	P("%u", info.frequency_stepsize);
	P("%u", info.frequency_tolerance);
	P("%u", info.symbol_rate_min);
	P("%u", info.symbol_rate_max);
	P("%u", info.symbol_rate_tolerance);
	P("%u", info.notifier_delay);
	return 0;
}

static int stat_fend(int id)
{
	fe_status_t status;
	int strength, snr, ber;
	int err1, err2, err3, err4;
	err1 = AM_FEND_GetStatus(id,&status);
	err2 = AM_FEND_GetStrength(id,&strength);
	err3 = AM_FEND_GetBER(id,&ber);
	err4 = AM_FEND_GetSNR(id,&snr);

	if(!err1)
		printf("\tStat: 0x%x\n", status);
	else
		printf("\tStat: FAIL\n");

	if(!err2)
		printf("\tstrength: %d\n", strength);
	else
		printf("\tstrength: FAIL\n");

	if(!err3)
		printf("\tber: %d\n", ber);
	else
		printf("\tber: FAIL\n");

	if(!err4)
		printf("\tsnr: %d\n", snr);
	else
		printf("\tsnr: FAIL\n");
	return 0;
}

static int dvbt2_plp(int id)
{
	int set_not_get = 0;
	uint8_t plp_ids[256];
	uint8_t plp = 0;
	int err;
	int i;

	struct dtv_properties prop;
	struct dtv_property property;

	printf("get/set(0/1): ");
	scanf("%d", &set_not_get);

	if (!set_not_get) {
		prop.num = 1;
		prop.props = &property;

		memset(&property, 0, sizeof(property));
		property.cmd = DTV_DVBT2_PLP_ID;
		property.u.buffer.reserved1[1] = UINT_MAX;/*get plps*/
		property.u.buffer.reserved2 = plp_ids;
		err = AM_FEND_GetProp(id, &prop);
		plp = property.u.buffer.reserved1[0];
		if(!err) {
			printf("DVB-T2 get %d PLPs:\n\t", plp);
			for(i=0; i<plp; i++)
				printf("%d ", plp_ids[i]);
			printf("\n\n");
		} else
			printf("DVB-T2 get plps FAIL.err(0x%x)\n", err);
	} else {
		printf("input plp to set:");
		scanf("%d", &plp);
		memset(&property, 0, sizeof(property));
		property.cmd = DTV_DVBT2_PLP_ID;
		property.u.data = plp;

		err = AM_FEND_SetProp(id, &prop);
		if(!err)
			printf("DVB-T2 Set PLP%d ", property.u.data);
		else
			printf("DVB-T2 Set PLP%d FAIL.err(0x%x)\n", property.u.data, err);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fe_id=-1;
	int mode=-1;

	char buf[64], last[64];
	char *cmd=NULL;

	int is_dvbt2 = 0;

	if(open_fend(&fe_id, &mode)!=0)
		return 0;

	if(mode == 5)
		is_dvbt2 = 1;

	while(1)
	{
		show_cmds();
		cmd = fgets(buf, sizeof(buf), stdin);
		if(!cmd)
			continue;

		if((cmd[0]=='\n') && (cmd[0]!=last[0]))
			cmd = last;
		else
			memcpy(last, cmd, sizeof(last));

		if(!strncmp(cmd, "open", 4)){
			if(open_fend(&fe_id, &mode)==-2)
				break;
		} else if (!strncmp(cmd, "lock", 4)) {
			lock_fend(fe_id, mode);
		} else if (!strncmp(cmd, "stat", 4)) {
			stat_fend(fe_id);
		} else if (!strncmp(cmd, "info", 4)) {
			info_fend(fe_id);
		} else if (!strncmp(cmd, "quit", 4)) {
			if(fe_id>=0){
				AM_FEND_CloseEx(fe_id, strncmp(cmd, "quit reset", 10)? AM_FALSE : AM_TRUE);
				fe_id = -1;
				mode = -1;
				break;
			}
		} else if (is_dvbt2 && !strncmp(cmd, "plp", 3)) {
			dvbt2_plp(fe_id);
		} else {
			continue;
		}
	}
	
	
	return 0;
}

