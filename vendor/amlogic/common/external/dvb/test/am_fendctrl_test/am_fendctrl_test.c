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
 * \brief DVB前端控制测试
 *
 * \author jiang zhongming <zhongming.jiang@amlogic.com>
 * \date 2012-05-19: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_fend.h>
#include <am_fend_ctrl.h>
#include <string.h>
#include <stdio.h>
#include <am_misc.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define FEND_DEV_NO    (0)

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

static void sec(int dev_no)
{
	int sec = -1;
	AM_Bool_t sec_exit = AM_FALSE;
	AM_SEC_DVBSatelliteEquipmentControl_t para; 
	
	printf("sec_control\n");
	while(1)
	{
		printf("-----------------------------\n");
		printf("SEC_SetSetting-0\n");
		printf("SEC_SettingClear-1\n");
		printf("Set LNB Lof-2\n");
		printf("Set LNB Increased Voltage-3\n");
		printf("Set Voltage Mode-4\n");
		printf("Set 22khz Signal-5\n");
		printf("Set Rotor Pos Num-6\n");
		printf("Set Diseqc-7\n");
		printf("Set Rotor-8\n");
		printf("Set unicable-9\n");
		printf("Dump sec-10\n");
		printf("Exit sec-11\n");
		printf("-----------------------------\n");
		printf("select\n");
		scanf("%d", &sec);
		switch(sec)
		{
			case 0:
				AM_SEC_SetSetting(dev_no, &para);
				break;
				
			case 1:
				{
					memset(&para, 0, sizeof(AM_SEC_DVBSatelliteEquipmentControl_t));
					break;
				}
				
			case 2:
				{
					printf("LNB m_lof_hi(KHz)\n");
					scanf("%d", &(para.m_lnbs.m_lof_hi));	

					printf("LNB m_lof_lo(KHz)\n");
					scanf("%d", &(para.m_lnbs.m_lof_lo));

					printf("LNB m_lof_threshold(KHz)\n");
					scanf("%d", &(para.m_lnbs.m_lof_threshold));
					break;
				}

			case 3:
				{
					int increased_voltage; 
					printf("LNB m_increased_voltage disable-0/enable-1\n");
					scanf("%d", &(increased_voltage));				
					para.m_lnbs.m_increased_voltage = increased_voltage;
					break;
				}

			case 4:
				{
					int voltage_mode; 
					printf("Voltage Mode _14V=0, _18V=1, _0V=2, HV=3, HV_13=4\n");
					scanf("%d", &(voltage_mode));	
					para.m_lnbs.m_cursat_parameters.m_voltage_mode = voltage_mode;
					break;
				}

			case 5:
				{
					int _22khz_signal;
					printf("22khz Signal ON=0, OFF=1, HILO=2\n");
					scanf("%d", &(_22khz_signal));	
					para.m_lnbs.m_cursat_parameters.m_22khz_signal = _22khz_signal;
					break;
				}
				break;

			case 6:
				{
					int rotorPosNum;
					printf("Rotor Pos Num 0-disable,gotoxx 1-255-position\n");
					scanf("%d", &(rotorPosNum));	
					para.m_lnbs.m_cursat_parameters.m_rotorPosNum = rotorPosNum;
					break;
				}
				
			case 7:
				{
					int diseqc;
					
					printf("m_diseqc_mode DISEQC_NONE=0, V1_0=1, V1_1=2, V1_2=3, SMATV=4\n");
					scanf("%d", &(diseqc));	
					para.m_lnbs.m_diseqc_parameters.m_diseqc_mode = diseqc;

					if(para.m_lnbs.m_diseqc_parameters.m_diseqc_mode > 0)
					{
						printf("m_toneburst_param NO=0, A=1, B=2\n");
						scanf("%d", &(diseqc));		
						para.m_lnbs.m_diseqc_parameters.m_toneburst_param =diseqc;
					
						printf("m_committed_cmd AA=0, AB=1, BA=2, BB=3, SENDNO=4\n");
						scanf("%d", &(diseqc));	
						para.m_lnbs.m_diseqc_parameters.m_committed_cmd = diseqc;

						if(para.m_lnbs.m_diseqc_parameters.m_diseqc_mode > 1)
						{
							printf("m_committed_cmd SENDNO=4, 0xF0 .. 0xFF\n");
							scanf("%x", &(diseqc));		
							para.m_lnbs.m_diseqc_parameters.m_uncommitted_cmd = diseqc;

							printf("m_repeats\n");
							scanf("%d", &(diseqc));	
							para.m_lnbs.m_diseqc_parameters.m_repeats = diseqc;

							printf("m_command_order\n");
							printf("diseqc 1.0)\n");
							printf("0) commited, toneburst\n");
							printf("1) toneburst, committed\n");
							printf("diseqc > 1.0)\n");
							printf("2) committed, uncommitted, toneburst\n");
							printf("3) toneburst, committed, uncommitted\n");
							printf("4) uncommitted, committed, toneburst\n");
							printf("5) toneburst, uncommitted, committed\n");
							scanf("%d", &(diseqc));	
							para.m_lnbs.m_diseqc_parameters.m_command_order = diseqc;

							printf("m_seq_repeat disable-0/enable-1\n");
							scanf("%d", &(diseqc));	
							para.m_lnbs.m_diseqc_parameters.m_seq_repeat = diseqc;
						}
						else
						{
							printf("m_use_fast disable-0/enable-1\n");
							scanf("%d", &(diseqc));		
							para.m_lnbs.m_diseqc_parameters.m_use_fast = diseqc;
						}
					}
					break;
				}
				
			case 8:
				{
					if((para.m_lnbs.m_diseqc_parameters.m_diseqc_mode == 3) && (para.m_lnbs.m_cursat_parameters.m_rotorPosNum == 0))
					{
						float degree;
						printf("m_longitude degree\n");
						scanf("%f", &(degree));		
						para.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_longitude = degree;

						printf("m_latitude degree\n");
						scanf("%f", &(degree));		
						para.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_latitude = degree;

						printf("m_sat_longitude 0.1 degree\n");
						scanf("%d", &(para.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_sat_longitude));			
					}		
					break;
				}
				
			case 9:
				{
					printf("SatCR_idx  0-(max_satcr-1)\n");
					scanf("%d", &(para.m_lnbs.SatCR_idx));
					printf("para.m_lnbs.SatCR_idx:%d\n", para.m_lnbs.SatCR_idx);

					if(para.m_lnbs.SatCR_idx >= 0 && para.m_lnbs.SatCR_idx <= 7)
					{
						#if 0
						printf("SatCR_positions \n");
						scanf("%d", &(para.m_lnbs.SatCR_positions));			
						#endif
						
						printf("SatCRvco (KHz)\n");
						scanf("%d", &(para.m_lnbs.SatCRvco));

						printf("LNBNum 0-1\n");
						scanf("%d", &(para.m_lnbs.LNBNum));	
					}
					break;
				}                  

			case 10:
				AM_SEC_DumpSetting();
				break;

			case 11:
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
		
	}
	
	return;
}

int main(int argc, char **argv)
{
	AM_FEND_OpenPara_t para;
	AM_Bool_t loop=AM_TRUE;
	fe_status_t status;
	int fe_id=-1;	
	
	while(loop)
	{
		AM_FENDCTRL_DVBFrontendParameters_t fenctrl_para;
		int mode, current_mode;
		int freq, srate, qam;
		int bw;
		char buf[64], name[64];
		
		memset(&para, 0, sizeof(para));
		
		printf("Input fontend id, id=-1 quit\n");
		printf("id: ");
		scanf("%d", &fe_id);
		if(fe_id<0)
			return 0;
		
		para.mode = AM_FEND_DEMOD_DVBS;

		if(para.mode != AM_FEND_DEMOD_DVBS){
			printf("Input fontend mode: (0-DVBC, 1-DVBT)\n");
			printf("mode(0/1): ");
			scanf("%d", &mode);
			
			sprintf(name, "/sys/class/amlfe-%d/mode", fe_id);
			if(AM_FileRead(name, buf, sizeof(buf))==AM_SUCCESS) {
				if(sscanf(buf, ":%d", &current_mode)==1) {
					if(current_mode != mode) {
						int r;
						printf("Frontend(%d) dose not match the mode expected, change it?: (y/N) ", fe_id);
						getchar();/*CR*/
						r=getchar();
						if((r=='y') || (r=='Y'))
							para.mode = (mode==0)?AM_FEND_DEMOD_DVBC : 
										(mode==1)? AM_FEND_DEMOD_DVBT : 
										AM_FEND_DEMOD_AUTO;
					}
				}
			}
		}else{
			fenctrl_para.m_type = AM_FEND_DEMOD_DVBS;
			mode = 2;
		}

		AM_TRY(AM_FEND_Open(/*FEND_DEV_NO*/fe_id, &para));
		
		AM_TRY(AM_FEND_SetCallback(/*FEND_DEV_NO*/fe_id, fend_cb, NULL));
		
		printf("input frontend parameters, frequency=0 quit\n");
		if(para.mode != AM_FEND_DEMOD_DVBS){
			printf("frequency(Hz): ");
		}
		else{
			sec(fe_id);
			
			printf("frequency(KHz): ");
		}
		
		scanf("%d", &freq);
		if(freq!=0)
		{
			if(mode==0) {
				printf("symbol rate(kbps): ");
				scanf("%d", &srate);
				printf("QAM(16/32/64/128/256): ");
				scanf("%d", &qam);
				
				fenctrl_para.cable.para.frequency = freq;
				fenctrl_para.cable.para.u.qam.symbol_rate = srate*1000;
				fenctrl_para.cable.para.u.qam.fec_inner = FEC_AUTO;
				switch(qam)
				{
					case 16:
						fenctrl_para.cable.para.u.qam.modulation = QAM_16;
					break;
					case 32:
						fenctrl_para.cable.para.u.qam.modulation = QAM_32;
					break;
					case 64:
					default:
						fenctrl_para.cable.para.u.qam.modulation = QAM_64;
					break;
					case 128:
						fenctrl_para.cable.para.u.qam.modulation = QAM_128;
					break;
					case 256:
						fenctrl_para.cable.para.u.qam.modulation = QAM_256;
					break;
				}
			}else if(mode==1){
				printf("BW[8/7/6/5(AUTO) MHz]: ");
				scanf("%d", &bw);

				fenctrl_para.terrestrial.para.frequency = freq;
				switch(bw)
				{
					case 8:
					default:
						fenctrl_para.terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
					break;
					case 7:
						fenctrl_para.terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_7_MHZ;
					break;
					case 6:
						fenctrl_para.terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_6_MHZ;
					break;
					case 5:
						fenctrl_para.terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_AUTO;
					break;
				}

				fenctrl_para.terrestrial.para.u.ofdm.code_rate_HP = FEC_AUTO;
				fenctrl_para.terrestrial.para.u.ofdm.code_rate_LP = FEC_AUTO;
				fenctrl_para.terrestrial.para.u.ofdm.constellation = QAM_AUTO;
				fenctrl_para.terrestrial.para.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
				fenctrl_para.terrestrial.para.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
				fenctrl_para.terrestrial.para.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
			}else{
				printf("dvb sx test\n");

				int polar;
				printf("polar: ");
				scanf("%d", &(polar));
				fenctrl_para.sat.polarisation = polar;

				fenctrl_para.sat.para.frequency = freq;

				printf("symbol rate: ");
				scanf("%d", &(fenctrl_para.sat.para.u.qpsk.symbol_rate));
			}

			int sync = 0;
			printf("async-0, sync-1: ");			
			scanf("%d", &(sync));

			if(sync == 0)
			{
				AM_TRY(AM_FENDCTRL_SetPara(/*FEND_DEV_NO*/fe_id, &fenctrl_para));
			}
			else
			{
				AM_TRY(AM_FENDCTRL_Lock(/*FEND_DEV_NO*/fe_id, &fenctrl_para, &status));
				printf("lock status: 0x%x\n", status);
			}

		}
		else
		{
			loop = AM_FALSE;
		}

		printf("fend close: ");
		scanf("%d", &(fe_id));
		
		AM_TRY(AM_FEND_Close(/*FEND_DEV_NO*/fe_id));
	}
	
	
	return 0;
}

