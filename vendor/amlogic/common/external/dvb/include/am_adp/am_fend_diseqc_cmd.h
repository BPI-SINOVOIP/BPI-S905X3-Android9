/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Diseqc protocol command
 *
 * \author jiang zhongming <zhongming.jiang@amlogic.com>
 * \date 2012-03-20: create the document
 ***************************************************************************/

#ifndef _AM_FEND_DISEQC_CMD_H
#define _AM_FEND_DISEQC_CMD_H

#include "am_types.h"
#include "am_evt.h"
/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/frontend.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Diseqc error code*/
enum AM_FEND_DISEQCCMD_ErrorCode
{
	AM_FEND_DISEQCCMD_ERROR_BASE=AM_ERROR_BASE(AM_MOD_FEND_DISEQCCMD),
	AM_FEND_DISEQCCMD_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief DVB-S/S2 frontend switch input type*/
typedef enum AM_FEND_SWITCHINPUT
{
	AM_FEND_SWITCH1INPUTA,
	AM_FEND_SWITCH2INPUTA, 
	AM_FEND_SWITCH3INPUTA,
	AM_FEND_SWITCH4INPUTA,
	AM_FEND_SWITCH1INPUTB,
	AM_FEND_SWITCH2INPUTB,
	AM_FEND_SWITCH3INPUTB,
	AM_FEND_SWITCH4INPUTB	
}AM_FEND_Switchinput_t; 

/**\brief DVB-S/S2 frontend polraisation*/
typedef enum AM_FEND_POLARISATION
{
	AM_FEND_POLARISATION_H,
	AM_FEND_POLARISATION_V,
	AM_FEND_POLARISATION_NOSET
}AM_FEND_Polarisation_t; 

/**\brief DVB-S/S2 frontend local oscillator frequency*/
typedef enum AM_FEND_LOCALOSCILLATORFREQ
{
	AM_FEND_LOCALOSCILLATORFREQ_L,
	AM_FEND_LOCALOSCILLATORFREQ_H,
	AM_FEND_LOCALOSCILLATORFREQ_NOSET
}AM_FEND_Localoscollatorfreq_t;

/**\brief 22Khz parameter, match AM_SEC_22khz_Signal*/
typedef enum {	AM_FEND_ON=0, AM_FEND_OFF=1 }AM_FEND_22khz_Signal; // 22 Khz

/**\brief voltage parameter, match AM_SEC_Voltage_Mode*/
typedef enum {	AM_FEND_13V=0, AM_FEND_18V=1 }AM_FEND_Voltage_Mode; // 13/18 V

/**\brief DVB-S/S2 Frontend LOCAL OSCILLATOR FREQ TABLE*/
typedef enum AM_FEND_LOT
{
	AM_FEND_LOT_STANDARD_NONE = 0x00,
	AM_FEND_LOT_STANDARD_UNKNOWN,
	AM_FEND_LOT_STANDARD_9750MHZ,
	AM_FEND_LOT_STANDARD_10000MHZ,
	AM_FEND_LOT_STANDARD_10600MHZ,
	AM_FEND_LOT_STANDARD_10750MHZ,
	AM_FEND_LOT_STANDARD_11000MHZ,
	AM_FEND_LOT_STANDARD_11250MHZ,	
	AM_FEND_LOT_STANDARD_11475MHZ,
	AM_FEND_LOT_STANDARD_20250MHZ,
	AM_FEND_LOT_STANDARD_5150MHZ,	
	AM_FEND_LOT_STANDARD_1585MHZ,
	AM_FEND_LOT_STANDARD_13850MHZ,
	AM_FEND_LOT_WIDEBAND_NONE = 0x10,
	AM_FEND_LOT_WIDEBAND_10000MHZ,
	AM_FEND_LOT_WIDEBAND_10200MHZ,
	AM_FEND_LOT_WIDEBAND_13250MHZ,
	AM_FEND_LOT_WIDEBAND_13450MHZ
}AM_FEND_LOT_t;
                
/**\brief DVB-S/S2 Frontend RF type*/
typedef enum AM_FEND_RFType
{
	AM_FEND_RFTYPE_STANDARD,
	AM_FEND_RFTYPE_WIDEBAND
}AM_FEND_RFType_t;
                

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief reset Diseqc micro controller(Diseqc1.0 M*R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_ResetDiseqcMicro(int dev_no);

/**\brief standby the switch Diseqc(Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_StandbySwitch(int dev_no);

/**\brief poweron the switch Diseqc(Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_PoweronSwitch(int dev_no);

/**\brief Set Low Local Oscillator Freq(Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetLo(int dev_no);

/**\brief set polarisation(Diseqc1.0 R)
 * \param dev_no forntend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetVR(int dev_no); 

/**\brief choose satellite A (Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetSatellitePositionA(int dev_no); 

/**\brief choose Switch Option A (Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetSwitchOptionA(int dev_no);

/**\brief set High Local Oscillator Frequency(Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On Success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetHi(int dev_no);

/**\brief set horizontal polarisation(Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetHL(int dev_no);

/**\brief set satellite B (Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetSatellitePositionB(int dev_no); 

/**\brief set Switch Option B (Diseqc1.0 R)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetSwitchOptionB(int dev_no); 

/**\brief set Switch input (Diseqc1.1 R)
 * \param dev_no frontend device number
 * \param switch_input switch input parameter
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetSwitchInput(int dev_no, AM_FEND_Switchinput_t switch_input);

/**\brief set LNB1-LNB4/polarisation/local oscillator (Diseqc1.0 M)
 * \param dev_no frontend device number
 * \param lnbport LNB1-LNB4/LNBPort:AA=0, AB=1, BA=2, BB=3, SENDNO=4
 * \param polarisation H/V
 * \param local_oscillator_freq H/L
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetLNBPort4(int dev_no, int lnbport, 
			AM_FEND_Polarisation_t polarisation,
			AM_FEND_Localoscollatorfreq_t local_oscillator_freq);
                                                                 
/**\brief set LNB Port LNB1-LNB16 (Diseqc1.1 M)
 * \param dev_no frontend device number
 * \param lnbport LNB1-LNB16,LNBPort In 0xF0 .. 0xFF
 * \param polarisation H/V
 * \param local_oscillator_freq H/L
 * \return
 *   - AM_SUCCESS On Success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetLNBPort16(int dev_no, int lnbport,
			AM_FEND_Polarisation_t polarisation,
			AM_FEND_Localoscollatorfreq_t local_oscillator_freq);
                                                                  
/**\brief set channel frequency (Diseqc1.1 M)
 * \param dev_no frontend device number
 * \param freq unit KHZ 
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetChannelFreq(int dev_no, int freq);

/**\brief set ositioner halt (Diseqc1.2 M)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetPositionerHalt(int dev_no);

/**\brief enable positioner limit (Diseqc1.2 M)
 * \param dev_no frontend deivce number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_EnablePositionerLimit(int dev_no);
																  
/**\brief disable positioner limit (Diseqc1.2 M)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_DisablePositionerLimit(int dev_no);

/**\brief enable positioner limit and store east limit (Diseqc1.2 M)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetPositionerELimit(int dev_no);

/**\brief enable positioner limit and store west limit(Diseqc1.2 M)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetPositionerWLimit(int dev_no);

/**\brief set positioner to go to east (Diseqc1.2 M)
 * \param dev_no frontend device number
 * \param unit 00 continuously 01-7F(unit second, e.g 01-one second 02-two second) 80-FF (unit step，e.g FF-one step FE-two step)
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_PositionerGoE(int dev_no, unsigned char unit);

/**\brief set positioner to go to west (Diseqc1.2 M)
 * \param dev_no frontend device number
 * \param unit 00 continuously 01-7F(unit second, e.g 01-one second 02-two second) 80-FF (unit step，e.g FF-one step FE-two step)
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_PositionerGoW(int dev_no, unsigned char unit);

/**\brief store position (Diseqc1.2 M)
 * \param dev_no frontend device number
 * \param position (e.g 1,2...)0 for reference.  actually it's not store ?
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_StorePosition(int dev_no, unsigned char position);

/**\brief set positioner to last position (Diseqc1.2 M)
 * \param dev_no frontend device number
 * \param position (e.g 1,2...) 0 for reference
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_GotoPositioner(int dev_no, unsigned char position); 

/**\brief position satellite with angular (longitude/latitude)
 * \param dev_no frontend device number
 * \param local_longitude
 * \param local_latitude
 * \param satellite_longitude
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_GotoxxAngularPositioner(int dev_no, double local_longitude, double local_latitude, double satellite_longitude);

/**\brief position satellite with angular (USALS(another name Diseqc1.3) Diseqc extention)
 * \param dev_no frontend device number
 * \param local_longitude
 * \param local_latitude
 * \param satellite_longitude
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_GotoAngularPositioner(int dev_no, double local_longitude, double local_latitude, double satellite_longitude);
                                                                       
/**\brief set ODU channel (Diseqc extention)
 * \param dev_no frontend device number
 * \param ub_number user band number(0-7)
 * \param inputbank_number input bank number (0-7)
 * \param transponder_freq unit KHZ
 * \param oscillator_freq unit KHZ
 * \param ub_freq unit KHZ    
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUChannel(int dev_no, unsigned char ub_number, unsigned char inputbank_number,
																int transponder_freq, int oscillator_freq, int ub_freq);
       
/**\brief set ODU UB to power off (Diseqc extention)
 * \param dev_no frontend device numner
 * \param ub_number user band number(0-7)  
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUPowerOff(int dev_no, unsigned char ub_number); 

/**\brief set ODU UB to signal on (Diseqc extention)
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUUbxSignalOn(int dev_no);
                                                                 
/**\brief set ODU config(Diseqc extention)
 * \param dev_no frontend device number
 * \param ub_number user band number(0-7)
 * \param satellite_position_count
 * \param input_bank_count input bank count
 * \param rf_type frontend rf type
 * \param ub_count user band count
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetODUConfig(int dev_no, unsigned char ub_number, 
																unsigned char satellite_position_count,
																unsigned char input_bank_count,
																AM_FEND_RFType_t rf_type,
																unsigned char ub_count);
                                                                  
/**\brief se ODU local oscillator (Diseqc extention)
 * \param dev_no frontend device number
 * \param ub_number user band number(0-7)    
 * \param lot parameter in local oscillator table
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Diseqccmd_SetODULoFreq(int dev_no, unsigned char ub_number, AM_FEND_LOT_t lot);                                                                  
                                                                               

/**\brief produce satellite angular (USALS(another name Diseqc1.3) Diseqc extention)
 * \param dev_no frontend device number
 * \param local_longitude
 * \param local_latitude
 * \param satellite_longitude
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern int AM_ProduceAngularPositioner(int dev_no, double local_longitude, double local_latitude, double satellite_longitude);
																		
#ifdef __cplusplus
}
#endif

#endif
