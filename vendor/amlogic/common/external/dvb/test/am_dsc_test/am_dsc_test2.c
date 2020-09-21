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
 * \brief 解扰器测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-08: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_dsc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fcntl.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define PID_PATH "/sys/class/amstream/ports"

/****************************************************************************
 * API functions
 ***************************************************************************/

extern int inject_file_and_rec_open(char *inject_name,int vpid, int apid, char *record_name);
extern int inject_file_and_rec_close(void);

int main(int argc, char **argv)
{
	AM_DSC_OpenPara_t dsc_para;
	int dsccv, dscca;
	int vpid=0, apid=0;
	char buf[1024];
	char *p = buf;
	int fd = open(PID_PATH, O_RDONLY);
	int dsc = 0, src = 0;
	int ret;
	int aes = 0, des = 0, sm4 = 0,iv = 0;
	int odd_type = AM_DSC_KEY_TYPE_ODD;
	int even_type = AM_DSC_KEY_TYPE_EVEN;
	int odd_iv_type = 0;
	int even_iv_type = 0;
	char inject_name[256];
	int inject_file = 0;
	char record_name[256];
	int record_file = 0;
	int mode = 0;

	int i;
	for (i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "vid", 3))
			sscanf(argv[i], "vid=%i", &vpid);
		else if (!strncmp(argv[i], "aid", 3))
			sscanf(argv[i], "aid=%i", &apid);
		else if (!strncmp(argv[i], "dsc", 3))
			sscanf(argv[i], "dsc=%i", &dsc);
		else if (!strncmp(argv[i], "src", 3))
			sscanf(argv[i], "src=%i", &src);
		else if (!strncmp(argv[i], "aes", 3))
			aes = 1;
		else if (!strncmp(argv[i], "des", 3))
			des = 1;
		else if (!strncmp(argv[i], "sm4", 3))
			sm4 = 1;
		else if (!strncmp(argv[i], "mode", 4))
			sscanf(argv[i],"mode=%i", &mode);
		else if (!strncmp(argv[i], "inject", 6)) {
			memset(inject_name, 0, sizeof(inject_name));
			sscanf(argv[i], "inject=%s", &inject_name);
			inject_file = 1;
		}
		else if (!strncmp(argv[i], "rec", 3)) {
			memset(record_name, 0, sizeof(record_name));
			sscanf(argv[i], "rec=%s", &record_name);
			record_file = 1;
		}
		else if (!strncmp(argv[i], "help", 4)) {
			printf("Usage: %s [vid=pid] [aid=pid] [dsc=n] [src=n] [aes|des|sm4] [mode=0/1/2,0:ecb,1:cbc,2:idsa]\n", argv[0]);
			printf("\t [inject=xxx] [rec=xxx] \n");
			printf("\t inject file and record for verify the descram function \n");
			printf("\t if no v/a specified, will set to current running v/a\n");
			exit(0);
		}
	}

	printf("use dsc[%d] src[%d]\n", dsc, src);
	if (aes) {
		printf("aes mode\n");
		odd_type = AM_DSC_KEY_TYPE_AES_ODD;
		even_type = AM_DSC_KEY_TYPE_AES_EVEN;
	} else if (des) {
		printf("des mode\n");
		odd_type = AM_DSC_KEY_TYPE_DES_ODD;
		even_type = AM_DSC_KEY_TYPE_DES_EVEN;
	} else if (sm4) {
		printf("sm4 mode\n");
		odd_type = AM_DSC_KEY_TYPE_SM4_ODD;
		even_type = AM_DSC_KEY_TYPE_SM4_EVEN;
	} else {
		printf("csa mode\n");
		odd_type = AM_DSC_KEY_TYPE_ODD;
		even_type = AM_DSC_KEY_TYPE_EVEN;
	}

	memset(&dsc_para, 0, sizeof(dsc_para));
	AM_TRY(AM_DSC_Open(dsc, &dsc_para));

	printf("DSC [%d] Set Source [%d]\n", dsc, src);

	ret = AM_DSC_SetSource(dsc, src);
	if(src==AM_DSC_SRC_BYPASS)
		goto end;

	if (!vpid && !apid) {
		if (fd<0) {
			printf("Can not open "PID_PATH);
			goto end;
		}
		read(fd, buf, 1024);
		p = strstr(buf, "amstream_mpts");
		while (p >= buf && p < (buf+1024))
		{
			while((p[0]!='V') && (p[0]!='A'))
				p++;
			if(p[0]=='V' && p[1]=='i' && p[2]=='d' && p[3]==':')
				sscanf(&p[4], "%d", &vpid);
			else if(p[0]=='A' && p[1]=='i' && p[2]=='d' && p[3]==':')
				sscanf(&p[4], "%d", &apid);
			if(vpid>0 && apid>0)
				break;
			p++;
		}
	}

	if(vpid>0 || apid>0) {
		char aes_key_ecb[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
//		char aes_key_cbc_iv[16] = {0x49, 0x72, 0x64, 0x65, 0x74, 0x6F, 0xA9, 0x43, 0x6F,0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74};
		char aes_key_cbc_iv[16] = {0};
		char aes_key_cbc[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
		char aes_key_cbc_isda[16] = { 0xb2, 0x8e, 0xd9, 0x82, 0x9d, 0x91, 0xe6, 0x5d,0x8c, 0x15, 0x33, 0x51, 0xf7, 0x67, 0x0d, 0x4a};
		char aes_key_cbc_isda_iv[16] = {0};

		char sm4_ecb_key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
		char sm4_cbc_key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
		char sm4_cbc_key_iv[16] = {0};
		char sm4_isda_key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};;
		char sm4_isda_key_iv[16] = {0};

		char des_key[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
		/*char csa_key[8] = {0x11, 0x22, 0x33, 0x66, 0x55, 0x66, 0x77, 0x32};*/
		//char csa_key[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
		char csa_key_odd[8] = {0xe6, 0x2a, 0x3b, 0x4b, 0xd0, 0x0e, 0x38, 0x16};
		char csa_key_even[8] = {0xe6, 0x3c, 0x7c, 0x9e, 0x00, 0x43, 0xc6, 0x09};

		char *odd_key = NULL;
		char *even_key = NULL;
		char *key_iv = NULL;

		if (aes) {
			if (mode == 0) {
				odd_key = aes_key_ecb;
				even_key = aes_key_ecb;
				printf("use AES ecb key\n");
			} else if (mode == 1) {
				odd_key = aes_key_cbc;
				even_key = aes_key_cbc;
				key_iv = aes_key_cbc_iv;
				odd_iv_type = AM_DSC_KEY_TYPE_AES_IV_ODD;
				even_iv_type = AM_DSC_KEY_TYPE_AES_IV_EVEN;
				printf("use AES cbc key\n");
			} else if (mode == 2 ) {
				odd_key = aes_key_cbc_isda;
				even_key = aes_key_cbc_isda;
				key_iv = aes_key_cbc_isda_iv;
				printf("use AES isda key\n");
				odd_iv_type = AM_DSC_KEY_TYPE_AES_IV_ODD;
				even_iv_type = AM_DSC_KEY_TYPE_AES_IV_EVEN;
			} else {
				printf("mode is invalid\n");
				return -1;
			}
		} else if (des) {
			odd_key = des_key;
			even_key = des_key;
			printf("use DES key\n");
		} else if (sm4) {
			if (mode == 0) {
				odd_key = sm4_ecb_key;
				even_key = sm4_ecb_key;
				printf("use SM4 ecb key\n");
			} else if (mode == 1) {
				odd_key = sm4_cbc_key;
				even_key = sm4_cbc_key;
				key_iv = sm4_cbc_key_iv;
				odd_iv_type = AM_DSC_KEY_TYPE_SM4_ODD_IV;
				even_iv_type = AM_DSC_KEY_TYPE_SM4_EVEN_IV;
				printf("use SM4 cbc key\n");
			} else if (mode == 2 ) {
				odd_key = sm4_isda_key;
				even_key = sm4_isda_key;
				key_iv = sm4_isda_key_iv;
				odd_iv_type = AM_DSC_KEY_TYPE_SM4_ODD_IV;
				even_iv_type = AM_DSC_KEY_TYPE_SM4_EVEN_IV;
				printf("use SM4 isda key\n");
			} else {
				printf("mode is invalid\n");
				return -1;
			}
		} else {
			odd_key = csa_key_odd;
			even_key = csa_key_even;
			printf("use CSA key\n");
		}

#define AM_CHECK_ERR(_x) do {\
		AM_ErrorCode_t ret = (_x);\
		if (ret != AM_SUCCESS)\
			printf("ERROR (0x%x) %s\n", ret, #_x);\
	} while(0)

		if(vpid>0) {
			AM_CHECK_ERR(AM_DSC_AllocateChannel(dsc, &dsccv));
			AM_CHECK_ERR(AM_DSC_SetChannelPID(dsc, dsccv, vpid));
			if (mode == 2) {
				AM_DSC_SetMode(dsc,dsccv,DSC_DSC_IDSA);
			}
			if (key_iv)
			{
				AM_CHECK_ERR(AM_DSC_SetKey(dsc,dsccv,odd_iv_type, (const uint8_t*)key_iv));
				AM_CHECK_ERR(AM_DSC_SetKey(dsc,dsccv,even_iv_type, (const uint8_t*)key_iv));
			}
			AM_CHECK_ERR(AM_DSC_SetKey(dsc,dsccv,odd_type, (const uint8_t*)odd_key));
			AM_CHECK_ERR(AM_DSC_SetKey(dsc,dsccv,even_type, (const uint8_t*)even_key));
			printf("set default key for pid[%d]\n", vpid);
		}
		if(apid>0) {
			AM_CHECK_ERR(AM_DSC_AllocateChannel(dsc, &dscca));
			AM_CHECK_ERR(AM_DSC_SetChannelPID(dsc, dscca, apid));
			if (mode == 2) {
				AM_DSC_SetMode(dsc,dscca,DSC_DSC_IDSA);
			}
			if (key_iv)
			{
				AM_CHECK_ERR(AM_DSC_SetKey(dsc,dscca,odd_iv_type, (const uint8_t*)key_iv));
				AM_CHECK_ERR(AM_DSC_SetKey(dsc,dscca,even_iv_type, (const uint8_t*)key_iv));
			}
			AM_CHECK_ERR(AM_DSC_SetKey(dsc,dscca,odd_type, (const uint8_t*)odd_key));
			AM_CHECK_ERR(AM_DSC_SetKey(dsc,dscca,even_type, (const uint8_t*)even_key));
			printf("set default key for pid[%d]\n", apid);
		}

		if (inject_file) {
			inject_file_and_rec_open(inject_name,vpid,apid,record_file ? record_name:NULL);
		}

		while(fgets(buf, 256, stdin))
		{
			if(!strncmp(buf, "quit", 4))
			{
				goto end;
			}
		}
	} else {
		printf("No A/V playing.\n");
	}

end:
	AM_DSC_Close(dsc);
	if (inject_file)
		inject_file_and_rec_close();

	return 0;
}
