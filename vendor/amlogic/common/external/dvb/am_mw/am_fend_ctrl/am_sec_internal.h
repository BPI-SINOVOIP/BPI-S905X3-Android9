/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_sec_internal.h
 * \brief SEC卫星设备控制模块内部头文件
 *
 * \author jiang zhongming <zhongming.jiang@amlogic.com>
 * \date 2012-05-06: create the document
 ***************************************************************************/

#ifndef _AM_SEC_INTERNAL_H
#define _AM_SEC_INTERNAL_H

#include "list.h"

#include "am_types.h"

#include "am_fend_ctrl.h"

#include "am_fend.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define MAX_DISEQC_LENGTH  16

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Diseqc命令*/
typedef struct eDVBDiseqcCommand
{
	int len;
	unsigned char data[MAX_DISEQC_LENGTH];
}eDVBDiseqcCommand_t;

/**\brief 卫星设备控制命令模式*/
enum AM_SEC_CMD_MODE{ modeStatic, modeDynamic };

/**\brief 卫星设备控制命令*/
enum AM_SEC_CMD{
	NONE, SLEEP, SET_VOLTAGE, SET_TONE, GOTO,
	SEND_DISEQC, SEND_TONEBURST, SET_FRONTEND,
	SET_TIMEOUT, IF_TIMEOUT_GOTO, 
	IF_VOLTAGE_GOTO, IF_NOT_VOLTAGE_GOTO,
	SET_POWER_LIMITING_MODE,
	SET_ROTOR_DISEQC_RETRYS, IF_NO_MORE_ROTOR_DISEQC_RETRYS_GOTO,
	MEASURE_IDLE_INPUTPOWER, MEASURE_RUNNING_INPUTPOWER,
	IF_MEASURE_IDLE_WAS_NOT_OK_GOTO, IF_INPUTPOWER_DELTA_GOTO,
	UPDATE_CURRENT_ROTORPARAMS, INVALIDATE_CURRENT_ROTORPARMS,
	UPDATE_CURRENT_SWITCHPARMS, INVALIDATE_CURRENT_SWITCHPARMS,
	IF_ROTORPOS_VALID_GOTO,
	IF_TUNER_LOCKED_GOTO,
	IF_TONE_GOTO, IF_NOT_TONE_GOTO,
	START_TUNE_TIMEOUT,
	SET_ROTOR_MOVING,
	SET_ROTOR_STOPPED,
	DELAYED_CLOSE_FRONTEND
};

/**\brief 卫星设备控制命令马达信息*/
typedef struct rotor
{
	union {
		int deltaA;   // difference in mA between running and stopped rotor
		int lastSignal;
	};
	int okcount;  // counter
	int steps;    // goto steps
	int direction;
	int dev_no;
}rotor_t;

/**\brief 卫星设备控制命令匹配信息*/
typedef struct pair
{
	union
	{
		int voltage;
		int tone;
		int val;
	};
	int steps;
}pair_t;

/**\brief 卫星设备控制命令及数据*/
typedef struct eSecCommand
{
	/* Use kernel list is not portable,  I suggest add a module in future*/
	struct list_head head;

	int cmd;

	union
	{
		int val;
		int steps;
		int timeout;
		int voltage;
		int tone;
		int toneburst;
		int msec;
		int mode;
		rotor_t measure;
		eDVBDiseqcCommand_t diseqc;
		pair_t compare;
	};
}eSecCommand_t;

/**\brief 卫星设备控制数据*/
typedef enum {
	NEW_CSW,
	NEW_UCSW,
	NEW_TONEBURST,
	CSW,                  // state of the committed switch
	UCSW,                 // state of the uncommitted switch
	TONEBURST,            // current state of toneburst switch
	NEW_ROTOR_CMD,        // prev sent rotor cmd
	NEW_ROTOR_POS,        // new rotor position (not validated)
	ROTOR_CMD,            // completed rotor cmd (finalized)
	ROTOR_POS,            // current rotor position
	LINKED_PREV_PTR,      // prev double linked list (for linked FEs)
	LINKED_NEXT_PTR,      // next double linked list (for linked FEs)
	SATPOS_DEPENDS_PTR,   // pointer to FE with configured rotor (with twin/quattro lnb)
	FREQ_OFFSET,          // current frequency offset
	CUR_VOLTAGE,          // current voltage
	CUR_VOLTAGE_INC,      // current voltage increased
	CUR_TONE,             // current continuous tone
	SATCR,                // current SatCR
	SEC_TIMEOUTCOUNT,	// needed for timeout
	CUR_LOCALOSCILLATOR,
	NUM_DATA_ENTRIES
}AM_SEC_FEND_DATA;


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 准备锁频卫星设备控制
 * \param dev_no 前端设备号
 * \param para 前端设备参数
 * \param status 前端设备状态
 * \param tunetimeout 超时时间
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend_ctrl.h)
 */
extern AM_ErrorCode_t AM_SEC_PrepareTune(int dev_no, const AM_FENDCTRL_DVBFrontendParametersSatellite_t *para, fe_status_t *status, unsigned int tunetimeout);

/**\brief 卫星设备控制命令执行,测试使用
 * \param dev_no 前端设备号
 * \param str 卫星设备控制命令字符串
 * \return
 *   - 无
 */
extern void AM_SEC_SetCommandString(int dev_no, const char *str);

#ifdef __cplusplus
}
#endif

#endif

