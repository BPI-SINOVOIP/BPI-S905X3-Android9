#ifndef SRC_TEST_KEYS_H
#define SRC_TEST_KEYS_H

#include <stddef.h>
#include <stdint.h>

namespace test_data {

extern const uint8_t RSA_3_512_D[512 >> 3];
extern const uint8_t RSA_3_512_N[512 >> 3];
extern const uint8_t RSA_512_D[512 >> 3];
extern const uint8_t RSA_512_N[512 >> 3];
extern const uint8_t RSA_768_D[768 >> 3];
extern const uint8_t RSA_768_N[768 >> 3];
extern const uint8_t RSA_1024_D[1024 >> 3];
extern const uint8_t RSA_1024_N[1024 >> 3];
extern const uint8_t RSA_2048_D[2048 >> 3];
extern const uint8_t RSA_2048_N[2048 >> 3];
extern const uint8_t RSA_3072_D[3072 >> 3];
extern const uint8_t RSA_3072_N[3072 >> 3];
extern const uint8_t RSA_4096_D[4096 >> 3];
extern const uint8_t RSA_4096_N[4096 >> 3];

const struct {
  const uint32_t e;
  const uint8_t *d;
  const uint8_t *n;
  const size_t size;
} TEST_RSA_KEYS[] = {
  {3,     RSA_3_512_D, RSA_3_512_N, sizeof(RSA_3_512_N)},
  {65537, RSA_512_D,   RSA_512_N,   sizeof(RSA_512_N)},
  {65537, RSA_768_D,   RSA_768_N,   sizeof(RSA_768_N)},
  {65537, RSA_1024_D,  RSA_1024_N,  sizeof(RSA_1024_N)},
  {65537, RSA_2048_D,  RSA_2048_N,  sizeof(RSA_2048_N)},
  {65537, RSA_3072_D,  RSA_3072_N,  sizeof(RSA_3072_N)},
  // TODO: update transport to accept larger messages.
  //  {RSA_4096_D, RSA_4096_N, sizeof(RSA_4096_N)},
};



}  // namespace test_data

#endif  // SRC_TEST_KEYS_H
