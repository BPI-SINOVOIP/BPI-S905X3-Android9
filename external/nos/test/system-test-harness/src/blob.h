#ifndef SRC_BLOB_H
#define SRC_BLOB_H

#include <stddef.h>
#include <stdint.h>

struct LITE_BIGNUM {
	uint32_t dmax;              /* Size of d, in 32-bit words. */
	uint32_t d;  /* Word array, little endian format ... */
} __attribute__((packed));

struct LITE_RSA {
	uint32_t e;
	struct LITE_BIGNUM N;
	struct LITE_BIGNUM d;
} __attribute__((packed));

/* TODO: maybe use protos? */
struct blob_params {
	uint32_t tag;
	uint32_t integer;
	uint64_t long_integer;
	/* TODO: save space for opaque data (APP_DATA etc). */
} __attribute__((packed));

struct blob_enforcements {
	uint32_t params_count;
	/* TODO: sizeof(proto-params) instead of constant here. */
	struct blob_params params[20];
} __attribute__((packed));

/* TODO: refactor RSA_MAX_BYTES from dcrypto.h */
struct blob_rsa {
	struct	LITE_RSA rsa;
	uint8_t N_bytes[3072 >> 3];
	uint8_t d_bytes[3072 >> 3];
} __attribute__((packed));

struct blob_ec {
	uint32_t curve;
	uint8_t d[32];
	uint8_t x[32];
	uint8_t y[32];
} __attribute__((packed));

struct blob_sym {
	uint32_t key_bits;
	uint8_t bytes[2048 >> 3];
} __attribute__((packed));

enum blob_alg {
	BLOB_RSA = 1,
	BLOB_EC = 2,
	BLOB_AES = 3,
	BLOB_DES = 4,
	BLOB_HMAC = 5,
};

#define KM_BLOB_MAGIC	0x474F4F47
#define KM_BLOB_VERSION 1

struct km_blob {
	struct {
		uint32_t magic;
		uint32_t version;
		uint32_t id;
		uint32_t iv[4];
	} h __attribute__((packed));
	struct {
		/* TODO: is sw_enforced expected to be managed by h/w? */
		struct blob_enforcements sw_enforced;
		struct blob_enforcements tee_enforced;
		uint32_t algorithm;
		union {
			struct blob_rsa rsa;
			struct blob_ec	ec;
			struct blob_sym sym;
		} key;
		/* TODO: pad to block size. */
	} b __attribute__((packed));
	uint8_t hmac[32];
} __attribute__((packed));

#endif	// SRC_BLOB_H
