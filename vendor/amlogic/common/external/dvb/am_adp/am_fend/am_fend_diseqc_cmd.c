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
 * \brief Diseqc 协议命令
 *
 * \author jiang zhongming <zhongming.jiang@amlogic.com>
 * \date 2012-03-20: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 3

#include <am_debug.h>

#include "am_fend_diseqc_cmd.h"
#include "am_fend.h"
#include "am_rotor_calc.h"

#include <string.h>
#include <unistd.h>
#include <math.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS (0xE0)
#define FEND_DISEQC_CMD_FRAMING_CMDNOREPLYREPEATTRANS (0xE1)

#define FEND_DISEQC_CMD_ADDR_ANYDEVICE (0x00)
#define FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV (0x10)
#define FEND_DISEQC_CMD_ADDR_ANYPOSITIONER (0x30)
#define FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER (0x31)
#define FEND_DISEQC_CMD_ADDR_ELAVATIONPOSITIONER (0x32)
#define FEND_DISEQC_CMD_ADDR_SUBSCRIBERCONTROLLED (0x71)

/****************************************************************************
 * Static data
 ***************************************************************************/

/****************************************************************************
 * Static functions
 ***************************************************************************/

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 重置Diseqc微控制器(Diseqc1.0 M*R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_ResetDiseqcMicro(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x00;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 待机外围的Switch设备(Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_StandbySwitch(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x02;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 供电外围的Switch设备(Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_PoweronSwitch(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x03;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择低本振频率(Low Local Oscillator Freq)(Diseqc1.0 R)
 * \param dev_no 前端设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetLo(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x20;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择垂直极化(polarisation)(Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetVR(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x21;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择卫星A (Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetSatellitePositionA(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x22;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择Switch Option A (Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetSwitchOptionA(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x23;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择高本振频率(High Local Oscillator Freq)(Diseqc1.0 R)
 * \param dev_no 前端设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetHi(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x24;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择水平极化(polarisation)(Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetHL(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x25;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择卫星B (Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetSatellitePositionB(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x26;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择Switch Option B (Diseqc1.0 R)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetSwitchOptionB(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x27;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择Switch input (Diseqc1.1 R)
 * \param dev_no 前端设备号  
 * \param switch_input switch输入
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetSwitchInput(int dev_no, AM_FEND_Switchinput_t switch_input)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;

	switch(switch_input)
	{
		case AM_FEND_SWITCH1INPUTA:
			cmd.msg[2] = 0x28;	
			break;
			
		case AM_FEND_SWITCH2INPUTA:
			cmd.msg[2] = 0x29;
			break;	
			
		case AM_FEND_SWITCH3INPUTA:
			cmd.msg[2] = 0x2A;
			break;	
			
		case AM_FEND_SWITCH4INPUTA:
			cmd.msg[2] = 0x2B;
			break;
			
		case AM_FEND_SWITCH1INPUTB:
			cmd.msg[2] = 0x2C;
			break;
			
		case AM_FEND_SWITCH2INPUTB:
			cmd.msg[2] = 0x2D;
			break;	
			
		case AM_FEND_SWITCH3INPUTB:
			cmd.msg[2] = 0x2E;
			break;	
			
		case AM_FEND_SWITCH4INPUTB:
			cmd.msg[2] = 0x2F;
			break;	
			
		default:
			cmd.msg[2] = 0x28;
			break;
	}

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 选择LNB1-LNB4\极性\本振频率 (Diseqc1.0 M) 
 * \param dev_no 前端设备号
 * \param lnbport LNB1-LNB4对应LNBPort取值AA=0, AB=1, BA=2, BB=3, SENDNO=4
 * \param polarisation 垂直水平极性 
 * \param local_oscillator_freq 高低本振频率   
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetLNBPort4(int dev_no, int lnbport, 
																AM_FEND_Polarisation_t polarisation, 
																AM_FEND_Localoscollatorfreq_t local_oscillator_freq)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	if((lnbport < 0) || (lnbport > 4)){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
		return ret;
	}

	if(lnbport == 4)
	{
		return ret;
	}
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;
	cmd.msg[2] = 0x38;

#if 0
	switch(lnbport)
	{
		case 1:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF0;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF1;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF2;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF3;
			}else{
				cmd.msg[3] = 0xF0;
			}
			break;
			
		case 2:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF4;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF5;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF6;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF7;
			}else{
				cmd.msg[3] = 0xF4;
			}			
			break;
			
		case 3:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF8;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF9;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xFA;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xFB;
			}else{
				cmd.msg[3] = 0xF8;
			}			
			break;
			
		case 4:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xFC;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xFD;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xFE;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xFF;
			}else{
				cmd.msg[3] = 0xFC;
			}			
			break;	
			
		default:
			cmd.msg[3] = 0xF0;
			break;
	}
#else
	/*above code is correct, but complex*/
	int band = 0;
	int csw = lnbport; 

	if(local_oscillator_freq != AM_FEND_LOCALOSCILLATORFREQ_NOSET){
		if ( local_oscillator_freq & AM_FEND_LOCALOSCILLATORFREQ_H)
			band |= 1;
	}

	if(polarisation != AM_FEND_POLARISATION_NOSET){
		if (!(polarisation & AM_FEND_POLARISATION_V))
			band |= 2;
	}

	if ( lnbport < 4 )
		csw = 0xF0 | (csw << 2);

	if (lnbport <= 4)
		csw |= band;	


	if(local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_NOSET){
		csw &= 0xEF;
	}

	if(polarisation == AM_FEND_POLARISATION_NOSET){
		csw &= 0xDF;
	}

	cmd.msg[3]  = csw;
#endif

	AM_DEBUG(1, "AM_FEND_Diseqccmd_SetLNBPort4 %x\n", cmd.msg[3] );

	cmd.msg_len = 4;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}
                                                                 
/**\brief 选择LNB1-LNB16 (Diseqc1.1 M) 
 * \param dev_no 前端设备号
 * \param lnbport LNB1-LNB16对应LNBPort取值0xF0 .. 0xFF
 * \param polarisation 垂直水平极性 
 * \param local_oscillator_freq 高低本振频率 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetLNBPort16(int dev_no, int lnbport, 
					AM_FEND_Polarisation_t polarisation, 
					AM_FEND_Localoscollatorfreq_t local_oscillator_freq)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	UNUSED(polarisation);
	UNUSED(local_oscillator_freq);

	if((lnbport < 0xF0) || (lnbport > 0xFF)){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
		return ret;
	}	
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_LNBSWITCHSMATV;

#if 0 /*polarisation and local_oscillator_freq set in AM_FEND_Diseqccmd_SetLNBPort4, we are not need complex*/
	cmd.msg[2] = 0x38;

	switch(lnbport)
	{
		case 1:
		case 5:
		case 9:
		case 13:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF0;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF1;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF2;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF3;
			}else{
				cmd.msg[3] = 0xF2;
			}			
			
			break;
			
		case 2:
		case 6:
		case 10:
		case 14:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF4;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF5;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF6;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF7;
			}else{
				cmd.msg[3] = 0xF6;
			}	
			
			break;
			
		case 3:
		case 7:
		case 11:
		case 15:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xF8;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xF9;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xFA;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xFB;
			}else{
				cmd.msg[3] = 0xFA;
			}	
			break;
			
		case 4:
		case 8:
		case 12:
		case 16:
			if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xFC;
			}else if((polarisation == AM_FEND_POLARISATION_V) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xFD;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_L)){
				cmd.msg[3] = 0xFE;
			}else if((polarisation == AM_FEND_POLARISATION_H) && (local_oscillator_freq == AM_FEND_LOCALOSCILLATORFREQ_H)){
				cmd.msg[3] = 0xFF;
			}else{
				cmd.msg[3] = 0xFE;
			}
			break;		
			
		default:
			cmd.msg[3] = 0xF2;
			break;
	}
	
	cmd.msg_len = 4;

	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
		return ret;
	}
#endif

	cmd.msg[2] = 0x39;

#if 0
	switch(lnbport)
	{
		case 1:
			cmd.msg[3] = 0xF0;
			break;
			
		case 2:
			cmd.msg[3] = 0xF1;
			break;
			
		case 3:
			cmd.msg[3] = 0xF2;
			break;
			
		case 4:
			cmd.msg[3] = 0xF3;
			break;	

		case 5:
			cmd.msg[3] = 0xF4;
			break;
			
		case 6:
			cmd.msg[3] = 0xF5;
			break;
			
		case 7:
			cmd.msg[3] = 0xF6;
			break;
			
		case 8:
			cmd.msg[3] = 0xF7;
			break;	

		case 9:
			cmd.msg[3] = 0xF8;
			break;
			
		case 10:
			cmd.msg[3] = 0xF9;
			break;
			
		case 11:
			cmd.msg[3] = 0xFA;
			break;
			
		case 12:
			cmd.msg[3] = 0xFB;
			break;	

		case 13:
			cmd.msg[3] = 0xFC;
			break;
			
		case 14:
			cmd.msg[3] = 0xFD;
			break;
			
		case 15:
			cmd.msg[3] = 0xFE;
			break;
			
		case 16:
			cmd.msg[3] = 0xFF;
			break;				
			
		default:
			cmd.msg[3] = 0xF0;
			break;
	}
#endif

	cmd.msg[3] = lnbport;

	cmd.msg_len = 4;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_SetLNBPort16 %x\n", cmd.msg[3] );	
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}
                                                                  
/**\brief 设置channel频率 (Diseqc1.1 M) 
 * \param dev_no 前端设备号
 * \param Freq unit KHZ 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetChannelFreq(int dev_no, int freq)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));
	int reamin_freq = 0;
	unsigned char remain1 = 0, remain2 = 0;

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_SUBSCRIBERCONTROLLED;
	cmd.msg[2] = 0x58;

	/*freq BCD*/
	reamin_freq = freq /100;

	/*10GHZ*/
	remain1 = reamin_freq / (1000 * 100);
	reamin_freq = reamin_freq % (1000 * 100);
	/*GHZ*/
	remain2 = reamin_freq / (1000 * 10);
	reamin_freq = reamin_freq % (1000 * 10);	
	cmd.msg[3] = (remain1 << 4) | remain2;

	/*100MHZ*/
	remain1 = reamin_freq / 1000;
	reamin_freq = reamin_freq % 1000;
	/*10MHZ*/
	remain2 = reamin_freq / 100;
	reamin_freq = reamin_freq % 100;		
	cmd.msg[4] = (remain1 << 4) | remain2;

	/*MHZ*/
	remain1 = reamin_freq / 10;
	reamin_freq = reamin_freq % 10;
	/*100KHZ*/
	remain2 = reamin_freq;	
	cmd.msg[5] = (remain1 << 4) | remain2;

	if(cmd.msg[5] == 0){
		cmd.msg_len = 5;
	}else{
		cmd.msg_len = 6;
	}
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 停止定位器(positioner)移动 (Diseqc1.2 M)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetPositionerHalt(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_SetPositionerHalt\n");
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x60;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 允许定位器(positioner)限制 (Diseqc1.2 M)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_EnablePositionerLimit(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x6A;
	cmd.msg[3] = 0x00;

	cmd.msg_len = 4;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}
																  
/**\brief 禁止定位器(positioner)限制 (Diseqc1.2 M)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_DisablePositionerLimit(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_DisablePositionerLimit\n");
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x63;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 允许定位器(positioner)限制并存储方向东的限制 (Diseqc1.2 M)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetPositionerELimit(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_SetPositionerELimit\n");
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x66;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 允许定位器(positioner)限制并存储方向西的限制 (Diseqc1.2 M)
 * \param dev_no 前端设备号 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetPositionerWLimit(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_SetPositionerWLimit\n");
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x67;

	cmd.msg_len = 3;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 定位器(positioner)向东移动 (Diseqc1.2 M) 
 * \param dev_no 前端设备号
 * \param unit 00 continuously 01-7F(单位second, e.g 01-one second 02-two second) 80-FF (单位step，e.g FF-one step FE-two step)
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_PositionerGoE(int dev_no, unsigned char unit)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_PositionerGoE\n");
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYREPEATTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x68;
	cmd.msg[3] = unit;
	
	cmd.msg_len = 4;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 定位器(positioner)向西移动 (Diseqc1.2 M) 
 * \param dev_no 前端设备号
 * \param unit 00 continuously 01-7F(单位second, e.g 01-one second 02-two second) 80-FF (单位step，e.g FF-one step FE-two step)  
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_PositionerGoW(int dev_no, unsigned char unit)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_PositionerGoW\n");
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYREPEATTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x69;
	cmd.msg[3] = unit;

	cmd.msg_len = 4;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 存储定位信息 (Diseqc1.2 M) 
 * \param dev_no 前端设备号
 * \param position 位置 (e.g 1,2...)
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_StorePosition(int dev_no, unsigned char position)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_StorePosition %d\n", position);
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x6A;
	cmd.msg[3] = position;

	cmd.msg_len = 4;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 定位(positioner)定位到之前存储的定位信息位置 (Diseqc1.2 M) 
 * \param dev_no 前端设备号
 * \param position 位置 (e.g 1,2...)
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_GotoPositioner(int dev_no, unsigned char position)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_DEBUG(1, "AM_FEND_Diseqccmd_GotoPositioner %d\n", position);
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x6B;
	cmd.msg[3] = position;

	cmd.msg_len = 4;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 定位器(positioner)根据经纬度定位到卫星 (gotoxx Diseqc extention) 
 * \param dev_no 前端设备号
 * \param local_longitude 本地经度
 * \param local_latitude 本地纬度
 * \param satellite_longitude 卫星经度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_GotoxxAngularPositioner(int dev_no, double local_longitude, double local_latitude, double satellite_longitude)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x6E;

	double a = atan(tan(local_longitude - satellite_longitude)/sin(local_latitude));
	int sign = 1;

	AM_DEBUG(1, "local_longitude=%f local_latitude=%f satellite_longitude=%f GotoAngular=%f", local_longitude, local_latitude, satellite_longitude, a );
	
	if(a >= 1E-7){
		sign = 1;
	}else if(a <= -1E-7){
		sign = -1;
	}else{
		sign = 0;
	}

	int angular = 0;
	double angular_remainder = 0.0;
	unsigned char remain1 = 0, remain2 = 0, remain3 = 0, remain4 = 0;

	if(sign == 1){
	}else if(sign == -1){
		a = fabs(a);
	}else{
		return ret;
	}

	angular = (int)(floor(a));
	angular_remainder = a - (double)angular;

	if(angular/256 == 1){
		/*this condition isn't happen because satellite cover 1/3 area of earth*/
		if(sign == 1){
			remain1 = 0x1;

			angular = angular%256;
		}else if(sign == -1){
			remain1 = 0x0;
			sign = 1;
			angular = 360 - angular;
		}
		
	}else{
		if(sign == 1){
			remain1 = 0x0;
		}else if(sign == -1){
			remain1 = 0xF;

			angular = 256 - angular;
		}		
	}

	if(angular/16 > 0){
		remain2 = angular/16;
		angular = angular%16;
	}

	if(angular > 0){
		remain3 = angular;
	}

	if((angular_remainder < 0.1) && (angular_remainder > -0.1)){
		remain4 = 0x0;
	}else if((angular_remainder < 0.15) && (angular_remainder > -0.15)){
		remain4 = 0x2;
	}else if((angular_remainder < 0.2) && (angular_remainder > -0.2)){
		remain4 = 0x3;
	}else if((angular_remainder < 0.35) && (angular_remainder > -0.35)){
		remain4 = 0x5;
	}else if((angular_remainder < 0.4) && (angular_remainder > -0.4)){
		remain4 = 0x6;
	}else if((angular_remainder < 0.55) && (angular_remainder > -0.55)){
		remain4 = 0x8;
	}else if((angular_remainder < 0.65) && (angular_remainder > -0.65)){
		remain4 = 0xA;
	}else if((angular_remainder < 0.7) && (angular_remainder > -0.7)){
		remain4 = 0xB;
	}else if((angular_remainder < 0.85) && (angular_remainder > -0.85)){
		remain4 = 0xD;
	}else{
		remain4 = 0xE;
	}
	
	cmd.msg[3] = (remain1 << 4) | remain2;
	cmd.msg[4] = (remain3 << 4) | remain4;		

	cmd.msg_len = 5;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 定位器(positioner)根据经纬度定位到卫星 (USALS(another name Diseqc1.3) Diseqc extention) 
 * \param dev_no 前端设备号
 * \param local_longitude 本地经度
 * \param local_latitude 本地纬度
 * \param satellite_longitude 卫星经度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_GotoAngularPositioner(int dev_no, double local_longitude, double local_latitude, double satellite_longitude)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	int RotorCmd = 0;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_POLARAZIMUTHPOSITIONER;
	cmd.msg[2] = 0x6E;

#if 0
	if ( satellite_longitude < 0 )
		satellite_longitude = 360 + satellite_longitude;	

	if ( local_longitude < 0 )
		local_longitude = 360 + local_longitude;


	double satHourAngle = AM_CalcSatHourangle( satellite_longitude, local_latitude, local_longitude );
	AM_DEBUG(1, "satHourAngle %f\n", satHourAngle);

	static int gotoXTable[10] =
		{ 0x00, 0x02, 0x03, 0x05, 0x06, 0x08, 0x0A, 0x0B, 0x0D, 0x0E };

	if (local_latitude >= 0) // Northern Hemisphere
	{
		int tmp=(int)round( fabs( 180 - satHourAngle ) * 10.0 );
		RotorCmd = (tmp/10)*0x10 + gotoXTable[ tmp % 10 ];

		if (satHourAngle < 180) // the east
			RotorCmd |= 0xE000;
		else					// west
			RotorCmd |= 0xD000;
	}
	else // Southern Hemisphere
	{
		if (satHourAngle < 180) // the east
		{
			int tmp=(int)round( fabs( satHourAngle ) * 10.0 );
			RotorCmd = (tmp/10)*0x10 + gotoXTable[ tmp % 10 ];
			RotorCmd |= 0xD000;
		}
		else // west
		{
			int tmp=(int)round( fabs( 360 - satHourAngle ) * 10.0 );
			RotorCmd = (tmp/10)*0x10 + gotoXTable[ tmp % 10 ];
			RotorCmd |= 0xE000;
		}
	}
	AM_DEBUG(1, "RotorCmd = %04x", RotorCmd);	
#endif

	RotorCmd = AM_ProduceAngularPositioner(dev_no, local_longitude, local_latitude, satellite_longitude);

	cmd.msg[3] = ((RotorCmd & 0xFF00) / 0x100);
	cmd.msg[4] = RotorCmd & 0xFF;			

	cmd.msg_len = 5;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}
 
/**\brief 设置ODU channel (Diseqc extention) 
 * \param dev_no 前端设备号
 * \param ub_number user band number(0-7)
 * \param inputbank_number input bank number (0-7)
 * \param transponder_freq unit KHZ
 * \param oscillator_freq unit KHZ
 * \param ub_freq unit KHZ    
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUChannel(int dev_no, unsigned char ub_number, unsigned char inputbank_number,
																int transponder_freq, int oscillator_freq, int ub_freq)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	if((ub_number > 7) || (inputbank_number > 7)){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
		return ret;
	}
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));
	unsigned short T = 0;
	float F_T = transponder_freq/1000;
	float F_O = oscillator_freq/1000;
	float F_UB = ub_freq/1000;
	unsigned char T1 = 0, T2 = 0;

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x5A;

	T = round((fabs(F_T -F_O) + F_UB)/4.0) -350;
	T = T & 0x0BFF;
	
	T1 = (T >> 8) & 0xFF;
	T2 = T & 0xFF;
	
	cmd.msg[3] = ((ub_number << 5) | (inputbank_number << 2)) | T1;
	cmd.msg[4] = T2;

	cmd.msg_len = 5;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}
       
/**\brief 关闭ODU UB (Diseqc extention) 
 * \param dev_no 前端设备号
 * \param ub_number user band number(0-7)  
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUPowerOff(int dev_no, unsigned char ub_number)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	if(ub_number > 7){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
		return ret;
	}	
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x5A;
	cmd.msg[3] = ub_number << 5;
	cmd.msg[4] = 0x00;

	cmd.msg_len = 5;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 打开所有ODU UB (Diseqc extention)
 * \param dev_no 前端设备号  
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUUbxSignalOn(int dev_no)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x5B;
	cmd.msg[3] = 0x00;
	cmd.msg[4] = 0x00;

	cmd.msg_len = 5;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}
                                                                 
/**\brief 设置ODU (Diseqc extention) 
 * \param dev_no 前端设备号
 * \param ub_number user band number(0-7)   
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUConfig(int dev_no, unsigned char ub_number, 
															unsigned char satellite_position_count,
															unsigned char input_bank_count,
															AM_FEND_RFType_t rf_type,
															unsigned char ub_count)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	if(ub_number > 7){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
		return ret;
	}	
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x5B;
	cmd.msg[3] = (ub_number << 5) | 0x01;

	if(satellite_position_count == 1){
		if(input_bank_count == 4){
			if(rf_type == AM_FEND_RFTYPE_STANDARD){
				if(ub_count == 2){
					cmd.msg[4] = 0x10;
				}else if(ub_count == 4){
					cmd.msg[4] = 0x11;
				}else if(ub_count == 6){
					cmd.msg[4] = 0x12;
				}else if(ub_count == 8){
					cmd.msg[4] = 0x13;
				}
			}else if(rf_type == AM_FEND_RFTYPE_WIDEBAND){			
			}
		}else if(input_bank_count == 2){
			if(rf_type == AM_FEND_RFTYPE_STANDARD){
			}else if(rf_type == AM_FEND_RFTYPE_WIDEBAND){
				if(ub_count == 2){
					cmd.msg[4] = 0x14;
				}else if(ub_count == 4){
					cmd.msg[4] = 0x15;
				}else if(ub_count == 6){
					cmd.msg[4] = 0x16;
				}else if(ub_count == 8){
					cmd.msg[4] = 0x17;
				}			
			}
		}
	}else if(satellite_position_count == 2){
		if(input_bank_count == 8){
			if(rf_type == AM_FEND_RFTYPE_STANDARD){
				if(ub_count == 2){
					cmd.msg[4] = 0x18;
				}else if(ub_count == 4){
					cmd.msg[4] = 0x19;
				}else if(ub_count == 6){
					cmd.msg[4] = 0x1A;
				}else if(ub_count == 8){
					cmd.msg[4] = 0x1B;
				}					
			}else if(rf_type == AM_FEND_RFTYPE_WIDEBAND){
			}			
		}else if(input_bank_count == 4){
			if(rf_type == AM_FEND_RFTYPE_STANDARD){
			}else if(rf_type == AM_FEND_RFTYPE_WIDEBAND){
				if(ub_count == 2){
					cmd.msg[4] = 0x1C;
				}else if(ub_count == 4){
					cmd.msg[4] = 0x1D;
				}else if(ub_count == 6){
					cmd.msg[4] = 0x1E;
				}else if(ub_count == 8){
					cmd.msg[4] = 0x1F;
				}				
			}		
		}	
	}

	
	cmd.msg_len = 5;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}
                                                                  
/**\brief 设置ODU 本振(oscillator)频率 (Diseqc extention) 
 * \param dev_no 前端设备号
 * \param ub_number user band number(0-7)    
 * \param lot LOCAL OSCILLATOR FREQ TABLE select
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
AM_ErrorCode_t AM_FEND_Diseqccmd_SetODULoFreq(int dev_no, unsigned char ub_number, AM_FEND_LOT_t lot)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	if(ub_number > 7){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
		return ret;
	}	
	
	struct dvb_diseqc_master_cmd cmd;
	memset(&cmd, 0, sizeof(struct dvb_diseqc_master_cmd));

	cmd.msg[0] = FEND_DISEQC_CMD_FRAMING_CMDNOREPLYFIRSTTRANS;
	cmd.msg[1] = FEND_DISEQC_CMD_ADDR_ANYDEVICE;
	cmd.msg[2] = 0x5B;
	cmd.msg[3] = (ub_number << 5) | 0x02;
	cmd.msg[4] = lot;

	cmd.msg_len = 5;
	
	if(AM_FEND_DiseqcSendMasterCmd(dev_no, &cmd) != AM_SUCCESS){
		ret = AM_FEND_DISEQCCMD_ERROR_BASE;
	}

	return ret;
}

/**\brief 根据经纬度生成到卫星方位角 (USALS(another name Diseqc1.3) Diseqc extention) 
 * \param dev_no 前端设备号
 * \param local_longitude 本地经度
 * \param local_latitude 本地纬度
 * \param satellite_longitude 卫星经度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_diseqc_cmd.h)
 */
int AM_ProduceAngularPositioner(int dev_no, double local_longitude, double local_latitude, double satellite_longitude)
{
	int RotorCmd = 0;

	UNUSED(dev_no);

	if ( satellite_longitude < 0 )
		satellite_longitude = 360 + satellite_longitude;	

	if ( local_longitude < 0 )
		local_longitude = 360 + local_longitude;


	double satHourAngle = AM_CalcSatHourangle( satellite_longitude, local_latitude, local_longitude );
	AM_DEBUG(1, "satHourAngle %f\n", satHourAngle);

	static int gotoXTable[10] =
		{ 0x00, 0x02, 0x03, 0x05, 0x06, 0x08, 0x0A, 0x0B, 0x0D, 0x0E };

	if (local_latitude >= 0) // Northern Hemisphere
	{
		int tmp=(int)round( fabs( 180 - satHourAngle ) * 10.0 );
		RotorCmd = (tmp/10)*0x10 + gotoXTable[ tmp % 10 ];

		if (satHourAngle < 180) // the east
			RotorCmd |= 0xE000;
		else					// west
			RotorCmd |= 0xD000;
	}
	else // Southern Hemisphere
	{
		if (satHourAngle < 180) // the east
		{
			int tmp=(int)round( fabs( satHourAngle ) * 10.0 );
			RotorCmd = (tmp/10)*0x10 + gotoXTable[ tmp % 10 ];
			RotorCmd |= 0xD000;
		}
		else // west
		{
			int tmp=(int)round( fabs( 360 - satHourAngle ) * 10.0 );
			RotorCmd = (tmp/10)*0x10 + gotoXTable[ tmp % 10 ];
			RotorCmd |= 0xE000;
		}
	}
	AM_DEBUG(1, "AM_ProduceAngularPositioner RotorCmd = %04x", RotorCmd);	

	return RotorCmd;
}


