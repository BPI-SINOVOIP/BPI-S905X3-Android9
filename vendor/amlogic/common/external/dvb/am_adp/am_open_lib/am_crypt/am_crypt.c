#include <string.h>
#include "des.h"

static void av_des_crypt_ts_packet(AVDES* d, uint8_t* dst, const uint8_t *src, int decrypt)
{
	memmove(dst, src, 4);
	av_des_crypt(d, dst + 4, src + 4, 24, NULL, decrypt);
}


void *AM_CRYPT_des_open(const uint8_t *key, int key_bits)
{
	AVDES *d = (AVDES *)malloc(sizeof(AVDES));
	if (d) {
		int ret = av_des_init(d, key, key_bits);
		if (ret)
			return NULL;
	}
	return d;
}

int AM_CRYPT_des_close(void *cryptor)
{
	free(cryptor);
	return 0;
}

void AM_CRYPT_des_crypt(void* cryptor, uint8_t* dst, const uint8_t *src, int len, uint8_t *iv, int decrypt)
{
	av_des_crypt_ts_packet((AVDES *)cryptor, dst, src, decrypt);
}
