/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_fend_ctrl.h
 * \brief Frontend control
 *
 * \author jiang zhongming <zhongming.jiang@amlogic.com>
 * \date 2012-05-06: create the document
 ***************************************************************************/

#ifndef _AM_FEND_CTRL_H
#define _AM_FEND_CTRL_H

/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/frontend.h>

#include <am_types.h>
#include <am_mem.h>
#include <am_fend.h>
#include <am_fend_diseqc_cmd.h>

#include <semaphore.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define guard_offset_min -8000
#define guard_offset_max 8000
#define guard_offset_step 8000
#define MAX_SATCR 8
#define MAX_LNBNUM 32

/****************************************************************************
 * Error code definitions
 ****************************************************************************/
 
/**\brief frontend control error code*/
enum AM_FENDCTRL_ErrorCode
{
	AM_FENDCTRL_ERROR_BASE=AM_ERROR_BASE(AM_MOD_FENDCTRL),
	AM_FENDCTRL_ERR_CANNOT_CREATE_THREAD,
	AM_FENDCTRL_ERR_END
};  

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief frontend control event type*/
enum AM_FENDCTRL_EventType
{
	AM_FENDCTRL_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_FENDCTRL),
	AM_FENDCTRL_EVT_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/ 

/**\brief DVB-S frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersSatellite
{
	struct dvb_frontend_parameters para; /**< frontend parameter*/

	AM_Bool_t no_rotor_command_on_tune;  /**< tune with rotor or not*/
	AM_FEND_Polarisation_t polarisation; /**< LNB polarisation*/
}AM_FENDCTRL_DVBFrontendParametersSatellite_t;

/**\brief DVB-C frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersCable
{
	struct dvb_frontend_parameters para; /**< parameter*/
}AM_FENDCTRL_DVBFrontendParametersCable_t;

/**\brief DVB-T frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersTerrestrial
{
	struct dvb_frontend_parameters para; /**< parameter*/
	fe_ofdm_mode_t ofdm_mode;
}AM_FENDCTRL_DVBFrontendParametersTerrestrial_t;

/**\brief ATSC frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersATSC
{
	struct dvb_frontend_parameters para; /**< parameter*/
}AM_FENDCTRL_DVBFrontendParametersATSC_t;

/**\brief ATSC frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersAnalog
{
	struct dvb_frontend_parameters para; /**< parameter*/
}AM_FENDCTRL_DVBFrontendParametersAnalog_t;

/**\brief DTMB frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersDTMB
{
	struct dvb_frontend_parameters para; /**< parameter*/
}AM_FENDCTRL_DVBFrontendParametersDTMB_t;

/**\brief ISDBT frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersISDBT
{
	struct dvb_frontend_parameters para;
}AM_FENDCTRL_DVBFrontendParametersISDBT_t;


/**\brief frontend control module parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParameters{
	union
	{
		AM_FENDCTRL_DVBFrontendParametersSatellite_t sat;
		AM_FENDCTRL_DVBFrontendParametersCable_t cable;
		AM_FENDCTRL_DVBFrontendParametersTerrestrial_t terrestrial; 
		AM_FENDCTRL_DVBFrontendParametersATSC_t atsc;
		AM_FENDCTRL_DVBFrontendParametersAnalog_t analog;
		AM_FENDCTRL_DVBFrontendParametersDTMB_t dtmb;
		AM_FENDCTRL_DVBFrontendParametersISDBT_t isdbt;
	};
    int m_logicalChannelNum;
	int m_type; /**< demod type*/
}AM_FENDCTRL_DVBFrontendParameters_t;
  
/**\brief DiSEqC parameter*/
enum { AA=0, AB=1, BA=2, BB=3, SENDNO=4 /* and 0xF0 .. 0xFF*/  };

/**\brief DiSEqC mode*/
typedef enum { DISEQC_NONE=0, V1_0=1, V1_1=2, V1_2=3, V1_3=4, SMATV=5 }AM_SEC_Diseqc_Mode;

/**\brief Toneburst parameter*/
typedef enum { NO=0, A=1, B=2 }AM_SEC_Toneburst_Param;

/**\brief diseqc param*/
typedef struct AM_SEC_DVBSatelliteDiseqcParameters
{
	unsigned char m_committed_cmd;                /**< committed command*/
	AM_SEC_Diseqc_Mode m_diseqc_mode;             /**< DiSEqC mode*/
	AM_SEC_Toneburst_Param m_toneburst_param;     /**< Toneburst parameter*/

	unsigned char m_repeats;	/**< for cascaded switches*/
	AM_Bool_t m_use_fast;	/**< send no DiSEqC on H/V or Lo/Hi change*/
	AM_Bool_t m_seq_repeat;	/**< send the complete DiSEqC Sequence twice...*/
	unsigned char m_command_order;					/**< low 4 bits for diseqc 1.0, high 4 bits for diseqc > 1.0*/
	/* 	diseqc 1.0)
			0) commited, toneburst
			1) toneburst, committed
		diseqc > 1.0)
			2) committed, uncommitted, toneburst
			3) toneburst, committed, uncommitted
			4) uncommitted, committed, toneburst
			5) toneburst, uncommitted, committed */
	unsigned char m_uncommitted_cmd;	/**< state of the 4 uncommitted switches..*/
}AM_SEC_DVBSatelliteDiseqcParameters_t;

/**\brief 22Khz param*/
typedef enum {	ON=0, OFF=1, HILO=2}AM_SEC_22khz_Signal;

/**\brief 14/18 voltage parameter*/
typedef enum {	_14V=0, _18V=1, _0V=2, HV=3, HV_13=4 }AM_SEC_Voltage_Mode;

/**\brief satellite switch parameter*/
typedef struct AM_SEC_DVBSatelliteSwitchParameters
{
	AM_SEC_Voltage_Mode m_voltage_mode;		/**< voltage param*/
	AM_SEC_22khz_Signal m_22khz_signal;		/**< 22Khz param*/
	unsigned char m_rotorPosNum;			/**< rotor position,0 is disable.. then use gotoxx*/
}AM_SEC_DVBSatelliteSwitchParameters_t;

/**\brief rotor speed property*/
enum { FAST, SLOW };

/**\brief rotor input power parameters*/
typedef struct AM_SEC_DVBSatelliteRotorInputpowerParameters
{
	AM_Bool_t m_use;	/**< can we use rotor inputpower to detect rotor running state ?*/
	unsigned char m_delta;	/**< delta between running and stopped rotor*/
	unsigned int m_turning_speed; /**< SLOW, FAST, or fast turning epoch*/
}AM_SEC_DVBSatelliteRotorInputpowerParameters_t;

/**\brief rotor gotoxx? parameters*/
typedef struct AM_SEC_DVBSatelliteRotorGotoxxParameters
{
	double m_longitude;	/**< longitude for gotoXX? function*/
	double m_latitude;	/**< latitude for gotoXX? function*/
	int m_sat_longitude;	/**< longitude for gotoXX? function of satellite unit-0.1 degree*/
}AM_SEC_DVBSatelliteRotorGotoxxParameters_t;

/**\brief satellite rotor parameters*/
typedef struct AM_SEC_DVBSatelliteRotorParameters
{
	AM_SEC_DVBSatelliteRotorInputpowerParameters_t m_inputpower_parameters; /**< rotor input power parameters*/
	AM_SEC_DVBSatelliteRotorGotoxxParameters_t m_gotoxx_parameters;         /**< rotor gotoxx? paramters*/
	AM_Bool_t m_reset_rotor_status_cache;								/**< rotor reset cache or not*/
	unsigned int m_rotor_move_unit;  										/**< rotor move unit, 00 continuously 01-7F(unit second, e.g 01-one second 02-two second) 80-FF (unit step，e.g FF-one step FE-two step)*/
}AM_SEC_DVBSatelliteRotorParameters_t;

typedef enum { RELAIS_OFF=0, RELAIS_ON }AM_SEC_12V_Relais_State;

/**\brief DVB-S frontend blindscan parameters*/
typedef struct AM_FENDCTRL_DVBFrontendParametersBlindSatellite
{
	AM_FEND_Polarisation_t polarisation;               /**< DVB-S blindscan polarisation*/
	AM_FEND_Localoscollatorfreq_t ocaloscollatorfreq;  /**< DVB-S blindscan ocaloscollator frequency*/
}AM_FENDCTRL_DVBFrontendParametersBlindSatellite_t;

/**\brief LNB parameters*/
typedef struct AM_SEC_DVBSatelliteLNBParameters
{
	AM_SEC_12V_Relais_State m_12V_relais_state;	/**< 12V relais output on/off*/

	unsigned int m_lof_hi,	/**< for 2 band universal lnb 10600 Mhz (high band offset frequency)*/
				m_lof_lo,	/**< for 2 band universal lnb  9750 Mhz (low band offset frequency)*/
				m_lof_threshold;	/**< for 2 band universal lnb 11750 Mhz (band switch frequency)*/

	AM_Bool_t m_increased_voltage; /**< use increased voltage ( 14/18V )*/

	AM_SEC_DVBSatelliteSwitchParameters_t m_cursat_parameters; /**< satellite switch parameters*/
	AM_SEC_DVBSatelliteDiseqcParameters_t m_diseqc_parameters; /**< Diseqc parameters*/
	AM_SEC_DVBSatelliteRotorParameters_t m_rotor_parameters;   /**< satellite rotor parameters*/

	int m_prio; // to override automatic tuner management ... -1 is Auto

	int SatCR_positions;                /**< unicable*/
	int SatCR_idx;                      /**< unicable cannel index*/
	unsigned int SatCRvco;              /**< unicable cannel vco*/
	unsigned int UnicableTuningWord;    /**< unicable tuning word*/
	unsigned int UnicableConfigWord;    /**< unicable config word*/
	int old_frequency;                  /**< old frequency*/
	int old_polarisation;               /**< old polarisation*/
	int old_orbital_position;           /**< old satellite orbital position*/
	int guard_offset_old;               /**< old guard offet*/
	int guard_offset;                   /**< guard offset*/
	int LNBNum;                         /**< LNB1 LNB2*/

	AM_FENDCTRL_DVBFrontendParametersBlindSatellite_t b_para; /**< blindscan parameters*/

	AM_Bool_t sec_blind_flag; /**< blindscan flag*/

	pthread_mutex_t    lock; /**< LNB parameters lock*/
}AM_SEC_DVBSatelliteLNBParameters_t;

/**\brief satellite control command delay*/
typedef enum{
	DELAY_AFTER_CONT_TONE_DISABLE_BEFORE_DISEQC=0,  /**< delay after continuous tone disable before diseqc command*/
	DELAY_AFTER_FINAL_CONT_TONE_CHANGE, /**< delay after continuous tone change before tune*/
	DELAY_AFTER_FINAL_VOLTAGE_CHANGE, /**< delay after voltage change at end of complete sequence*/
	DELAY_BETWEEN_DISEQC_REPEATS, /**< delay between repeated diseqc commands*/
	DELAY_AFTER_LAST_DISEQC_CMD, /**< delay after last diseqc command*/
	DELAY_AFTER_TONEBURST, /**< delay after toneburst*/
	DELAY_AFTER_ENABLE_VOLTAGE_BEFORE_SWITCH_CMDS, /**< delay after enable voltage before transmit toneburst/diseqc*/
	DELAY_BETWEEN_SWITCH_AND_MOTOR_CMD, /**< delay after transmit toneburst / diseqc and before transmit motor command*/
	DELAY_AFTER_VOLTAGE_CHANGE_BEFORE_MEASURE_IDLE_INPUTPOWER, /**< delay after voltage change before measure idle input power*/
	DELAY_AFTER_ENABLE_VOLTAGE_BEFORE_MOTOR_CMD, /**< delay after enable voltage before transmit motor command*/
	DELAY_AFTER_MOTOR_STOP_CMD, /**< delay after transmit motor stop*/
	DELAY_AFTER_VOLTAGE_CHANGE_BEFORE_MOTOR_CMD, /**< delay after voltage change before transmit motor command*/
	DELAY_BEFORE_SEQUENCE_REPEAT, /**< delay before the complete sequence is repeated (when enabled)*/
	MOTOR_COMMAND_RETRIES, /**< max transmit tries of rotor command when the rotor dont start turning (with power measurement)*/
	MOTOR_RUNNING_TIMEOUT, /**< max motor running time before timeout*/
	DELAY_AFTER_VOLTAGE_CHANGE_BEFORE_SWITCH_CMDS, /**< delay after change voltage before transmit toneburst/diseqc*/
	DELAY_AFTER_DISEQC_RESET_CMD, /**< delay after diseqc reset command*/
	DELAY_AFTER_DISEQC_PERIPHERIAL_POWERON_CMD, /**< delay after diseqc peripheral power on command*/
	SEC_CMD_MAX_PARAMS
}AM_SEC_Cmd_Param_t;

/**\brief diseqc command*/
typedef enum{
	TYPE_SEC_PREFORTUNERDEMODDEV = 0,	/**< diseqc prepare for tuner and demod*/
	TYPE_SEC_LNBSSWITCHCFGVALID = 31,	/**< diseqc LNB and Switch config is valid*/
	TYPE_SEC_POSITIONERSTOP,				/**< diseqc rotor positioner stop*/
	TYPE_SEC_POSITIONERDISABLELIMIT,		/**< diseqc rotor positioner disable limit*/
	TYPE_SEC_POSITIONEREASTLIMIT,		/**< diseqc rotor positioner east limit*/
	TYPE_SEC_POSITIONERWESTLIMIT,		/**< diseqc rotor positioner west limit*/
	TYPE_SEC_POSITIONEREAST,				/**< diseqc rotor positioner east*/
	TYPE_SEC_POSITIONERWEST,				/**< diseqc rotor positioner west*/
	TYPE_SEC_POSITIONERSTORE,			/**< diseqc rotor store position*/
	TYPE_SEC_POSITIONERGOTO,				/**< diseqc rotor positioner goto*/
	TYPE_SEC_POSITIONERGOTOX				/**< diseqc rotor positioner gotox(longitude/latitude)*/
}AM_SEC_Cmd_t;

/**\brief diseqc async infomation*/
typedef struct AM_SEC_AsyncInfo
{
	int                dev_no;        /**< device number*/
	AM_Bool_t          short_circuit; /**< Frontend short circuit status*/
	AM_Bool_t          sec_monitor_enable_thread; /**< diseqc monitor thread enable status*/
	pthread_t          sec_monitor_thread;        /**< diseqc monitor thread*/
	AM_Bool_t          enable_thread; /**< diseqc thread enable status*/
	pthread_t          thread;        /**< diseqc thread*/
	pthread_mutex_t    lock;          /**< diseqc thread lock*/
	pthread_cond_t     cond;          /**< diseqc thread cond*/
	AM_Bool_t          preparerunning;/**< diseqc thread prepare running status*/
	AM_Bool_t          prepareexitnotify; /**< diseqc thread prepare exit notify status*/
	sem_t sem_running; /**< diseqc thread semaphore*/
	AM_FENDCTRL_DVBFrontendParametersBlindSatellite_t *sat_b_para; /**< blindscan parameters*/
	AM_FENDCTRL_DVBFrontendParametersSatellite_t sat_para; /**< frontend parameters*/
	AM_Bool_t sat_para_valid; /**< frontend tune valid status*/
	fe_status_t *sat_status; /**< frontend tune status*/
	unsigned int sat_tunetimeout; /**< frontend tune timeout*/
}AM_SEC_AsyncInfo_t;

/**\brief diseqc parameters*/
typedef struct AM_SEC_DVBSatelliteEquipmentControl
{
	AM_SEC_DVBSatelliteLNBParameters_t m_lnbs; /**< LNB parmeters*/
	AM_Bool_t m_canMeasureInputPower; /**< inputpower status*/
	AM_SEC_Cmd_Param_t m_params[SEC_CMD_MAX_PARAMS]; /**< diseqc delay command*/

	AM_SEC_Cmd_t sec_cmd; /**< diseqc command*/

	AM_SEC_AsyncInfo_t m_sec_asyncinfo; /**< diseqc async infomation*/
}AM_SEC_DVBSatelliteEquipmentControl_t;


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/* sec control interface */ 

/**\brief diseqc parameters setting
 * \param dev_no frontend device number
 * \param[in] para diseqc parameters
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_SEC_SetSetting(int dev_no, const AM_SEC_DVBSatelliteEquipmentControl_t *para); 

/**\brief get diseqc setting parameters
 * \param dev_no frontend device number
 * \param[out] para diseqc parameters
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_SEC_GetSetting(int dev_no, AM_SEC_DVBSatelliteEquipmentControl_t *para);

/**\brief diseqc reset rotor cache status
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_SEC_ResetRotorStatusCache(int dev_no);

/**\brief diseqc reset cache
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern void  AM_SEC_Cache_Reset(int dev_no);

/**\brief diseqc prepare blindscan
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_SEC_PrepareBlindScan(int dev_no);

/**\brief diseqc diseqc frequency convert fucntion(centre_freq -> tp_freq)
 * \param dev_no frontend device number
 * \param centre_freq unit KHZ
 * \param[out] tp_freq unit KHZ
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_SEC_FreqConvert(int dev_no, unsigned int centre_freq, unsigned int *tp_freq);

/**\brief filter invalid tp frequency
 * \param dev_no frontend device number
 * \param tp_freq unit KHZ
 * \return
 *   - AM_TRUE do filter
 *   - AM_FALSE not filter
 */
extern AM_Bool_t AM_SEC_FilterInvalidTp(int dev_no, unsigned int tp_freq);

/**\brief diseqc execute command
 * \param dev_no frontend device commond
 * \param para diseqc frontend parameters
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_SEC_ExecSecCmd(int dev_no, const AM_FENDCTRL_DVBFrontendParameters_t *para); 

extern AM_ErrorCode_t AM_SEC_DumpSetting(void);

/* frontend control interface */ 

/**\brief frontend control set parameters
 * \param dev_no frontend device number
 * \param[in] para frontend parameters
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FENDCTRL_SetPara(int dev_no, const AM_FENDCTRL_DVBFrontendParameters_t *para); 

/**\brief frontend control try to lock and wait lock status
 * \param dev_no frontend device number
 * \param[in] para frontend parameter
 * \param[out] status frontend lock status
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FENDCTRL_Lock(int dev_no, const AM_FENDCTRL_DVBFrontendParameters_t *para, fe_status_t *status);


#ifdef __cplusplus
}
#endif

#endif

