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
 * \brief 智能卡测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-05: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <am_smc.h>
#include <string.h>
#include <unistd.h>

#define SMC_DEV_NO (0)

static void smc_cb(int dev_no, AM_SMC_CardStatus_t status, void *data)
{
	if(status==AM_SMC_CARD_IN)
	{
		printf("cb: card in!\n");
	}
	else
	{
		printf("cb: card out!\n");
	}
}

static int smc_test(AM_Bool_t sync)
{
	AM_SMC_OpenPara_t para;
	uint8_t atr[AM_SMC_MAX_ATR_LEN];
	int i, len;
	AM_SMC_CardStatus_t status;
	uint8_t sbuf[5]={0x80, 0x44, 0x00, 0x00, 0x08};
	uint8_t rbuf[256];
	int rlen = sizeof(rbuf);
	
	memset(&para, 0, sizeof(para));
	para.enable_thread = !sync;
	AM_TRY(AM_SMC_Open(SMC_DEV_NO, &para));
	
	if(!sync)
	{
		AM_SMC_SetCallback(SMC_DEV_NO, smc_cb, NULL);
	}
	
	printf("please insert a card\n");
	do {
		AM_TRY(AM_SMC_GetCardStatus(SMC_DEV_NO, &status));
		usleep(100000);
	} while(status==AM_SMC_CARD_OUT);
	
	printf("card in\n");
	
	len = sizeof(atr);
	AM_TRY(AM_SMC_Reset(SMC_DEV_NO, atr, &len));
	printf("ATR: ");
	for(i=0; i<len; i++)
	{
		printf("%02x ", atr[i]);
	}
	printf("\n");
/*	
	AM_TRY(AM_SMC_TransferT0(SMC_DEV_NO, sbuf, sizeof(sbuf), rbuf, &rlen));
	printf("send: ");
	for(i=0; i<sizeof(sbuf); i++)
	{
		printf("%02x ", sbuf[i]);
	}
	printf("\n");
	
	printf("recv: ");
	for(i=0; i<rlen; i++)
	{
		printf("%02x ", rbuf[i]);
	}
	printf("\n");
*/	
	AM_TRY(AM_SMC_Close(SMC_DEV_NO));
	
	return 0;
}

int get_para(char *argv, AM_SMC_Param_t *ppara)
{
	#define CASE(name, len) \
		if(!strncmp(argv, #name"=", (len)+1)) { \
			sscanf(&argv[(len)+1], "%i", &ppara->name); \
			printf("param["#name"] => %d\n", ppara->name); \
		}

	CASE(f, 1)
	else CASE(d, 1)
	else CASE(n, 1)
	else CASE(bwi, 3)
	else CASE(cwi, 3)
	else CASE(bgt, 3)
	else CASE(freq, 4)
	else CASE(recv_invert, 10)
	else CASE(recv_lsb_msb, 12)
	else CASE(recv_parity, 11)
	else CASE(recv_no_parity, 14)
	else CASE(xmit_invert, 10)
	else CASE(xmit_lsb_msb, 11)
	else CASE(xmit_retries, 11)
	else CASE(xmit_repeat_dis, 15)

	return 0;
}

int main(int argc, char **argv)
{
	AM_SMC_Param_t para;	
	AM_SMC_OpenPara_t openpara;

	int i;

	memset(&openpara, 0, sizeof(openpara));
	memset(&para, 0, sizeof(para));

	AM_TRY(AM_SMC_Open(SMC_DEV_NO, &openpara));
	AM_TRY(AM_SMC_GetParam(SMC_DEV_NO, &para));

	for(i=1; i< argc; i++)
		get_para(argv[i], &para);

	AM_TRY(AM_SMC_SetParam(SMC_DEV_NO, &para));
	AM_TRY(AM_SMC_Close(SMC_DEV_NO));

	printf("sync mode test\n");
	smc_test(AM_TRUE);
/*	
	printf("async mode test\n");
	smc_test(AM_FALSE);
*/
	return 0;
}

