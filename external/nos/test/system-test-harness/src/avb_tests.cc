#include <memory>

#include "gtest/gtest.h"
#include "avb_tools.h"
#include "nugget_tools.h"
#include "nugget/app/avb/avb.pb.h"
#include "Avb.client.h"
#include <avb.h>
#include <application.h>
#include <nos/AppClient.h>
#include <nos/NuggetClientInterface.h>
#include "util.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

using std::cout;
using std::string;
using std::unique_ptr;

using namespace nugget::app::avb;
using namespace avb_tools;

namespace {

class AvbTest: public testing::Test {
 protected:
  static unique_ptr<nos::NuggetClientInterface> client;
  static unique_ptr<test_harness::TestHarness> uart_printer;

  static void SetUpTestCase();
  static void TearDownTestCase();

  virtual void SetUp(void);

  int ProductionResetTest(uint32_t selector, uint64_t nonce,
                          const uint8_t *device_data, size_t data_len,
                          const uint8_t *signature, size_t signature_len);
  int SignChallenge(const struct ResetMessage *message,
                    uint8_t *signature, size_t *siglen);

  int Load(uint8_t slot, uint64_t *version);
  int Store(uint8_t slot, uint64_t version);
  int SetDeviceLock(uint8_t locked);
  int SetBootLock(uint8_t locked);
  int SetOwnerLock(uint8_t locked, const uint8_t *metadata, size_t size);
  int GetOwnerKey(uint32_t offset, uint8_t *metadata, size_t *size);
  int SetCarrierLock(uint8_t locked, const uint8_t *metadata, size_t size);

 public:
  const uint64_t LAST_NONCE = 0x4141414141414140ULL;
  const uint64_t VERSION = 1;
  const uint64_t NONCE = 0x4141414141414141ULL;
  const uint8_t DEVICE_DATA[AVB_DEVICE_DATA_SIZE] = {
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
  };
  const uint8_t SIGNATURE[AVB_SIGNATURE_SIZE] = {
    0x68, 0x86, 0x9a, 0x16, 0xca, 0x62, 0xea, 0xa9,
    0x9b, 0xa0, 0x51, 0x03, 0xa6, 0x00, 0x3f, 0xe8,
    0xf1, 0x43, 0xe6, 0xb7, 0xde, 0x76, 0xfe, 0x21,
    0x65, 0x87, 0x78, 0xe5, 0x1d, 0x11, 0x6a, 0xe1,
    0x7b, 0xc6, 0x2e, 0xe2, 0x96, 0x25, 0x48, 0xa7,
    0x09, 0x43, 0x2c, 0xfd, 0x28, 0xa9, 0x66, 0x8a,
    0x09, 0xd5, 0x83, 0x3b, 0xde, 0x18, 0x5d, 0xef,
    0x50, 0x12, 0x8a, 0x8d, 0xfb, 0x2d, 0x46, 0x20,
    0x69, 0x55, 0x4e, 0x86, 0x63, 0xf6, 0x10, 0xe3,
    0x59, 0x3f, 0x55, 0x72, 0x18, 0xcb, 0x60, 0x80,
    0x0d, 0x2e, 0x2f, 0xfc, 0xc2, 0xbf, 0xda, 0x3f,
    0x4f, 0x2b, 0x6b, 0xf1, 0x5d, 0x28, 0x6b, 0x2b,
    0x9b, 0x92, 0xf3, 0x4e, 0xf2, 0xb6, 0x23, 0x8e,
    0x50, 0x64, 0xf6, 0xee, 0xc7, 0x78, 0x6a, 0xe0,
    0xed, 0xce, 0x2c, 0x1f, 0x0a, 0x47, 0x43, 0x5c,
    0xe4, 0x69, 0xc5, 0xc1, 0xf9, 0x52, 0x8c, 0xed,
    0xfd, 0x71, 0x8f, 0x9a, 0xde, 0x62, 0xfc, 0x21,
    0x07, 0xf9, 0x5f, 0xe1, 0x1e, 0xdc, 0x65, 0x95,
    0x15, 0xc8, 0xe7, 0xf2, 0xce, 0xa9, 0xd0, 0x55,
    0xf1, 0x18, 0x89, 0xae, 0xe8, 0x47, 0xd8, 0x8a,
    0x1f, 0x68, 0xa8, 0x6f, 0x5e, 0x5c, 0xda, 0x3d,
    0x98, 0xeb, 0x82, 0xf8, 0x1f, 0x7a, 0x43, 0x6d,
    0x3a, 0x7c, 0x36, 0x76, 0x4f, 0x55, 0xa4, 0x55,
    0x5f, 0x52, 0x47, 0xa5, 0x71, 0x17, 0x7b, 0x73,
    0xaa, 0x5c, 0x85, 0x94, 0xb6, 0xe2, 0x37, 0x1f,
    0x22, 0x29, 0x46, 0x59, 0x20, 0x1f, 0x49, 0x36,
    0x50, 0xa9, 0x60, 0x5d, 0xeb, 0x99, 0x3f, 0x92,
    0x31, 0xa0, 0x1d, 0xad, 0xdb, 0xde, 0x40, 0xf6,
    0xaf, 0x9c, 0x36, 0xe4, 0x0c, 0xf4, 0xcc, 0xaf,
    0x9f, 0x8b, 0xf9, 0xe6, 0x12, 0x53, 0x4e, 0x58,
    0xeb, 0x9a, 0x57, 0x08, 0x89, 0xa5, 0x4f, 0x7c,
    0xb9, 0x78, 0x07, 0x02, 0x17, 0x2c, 0xce, 0xb8,
  };
};

unique_ptr<nos::NuggetClientInterface> AvbTest::client;
unique_ptr<test_harness::TestHarness> AvbTest::uart_printer;

void AvbTest::SetUpTestCase() {
  uart_printer = test_harness::TestHarness::MakeUnique();

  client = nugget_tools::MakeNuggetClient();
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void AvbTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();

  uart_printer = nullptr;
}

void AvbTest::SetUp(void)
{
  bool bootloader;
  bool production;
  uint8_t locks[4];
  int code;

  avb_tools::BootloaderDone(client.get());  // We don't need BL for setup.
  // Perform a challenge/response. If this fails, either
  // the reset path is broken or the image is probably not
  // TEST_IMAGE=1.
  // Note: the reset tests are not safe on -UTEST_IMAGE unless
  //       the storage can be reflashed.
  ResetProduction(client.get());

  code = Reset(client.get(), ResetRequest::LOCKS, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), &bootloader, &production, locks);
  EXPECT_EQ(bootloader, false);
  EXPECT_EQ(production, false);
  EXPECT_EQ(locks[BOOT], 0x00);
  EXPECT_EQ(locks[CARRIER], 0x00);
  EXPECT_EQ(locks[DEVICE], 0x00);
  EXPECT_EQ(locks[OWNER], 0x00);
}

static const uint8_t kResetKeyPem[] =
"-----BEGIN RSA PRIVATE KEY-----\n\
MIIEpAIBAAKCAQEAo0IoAa5cK7XyAj7u1jFStsfEcxkgAZVF9VWKzH1bofKxLioA\n\
r5Lo4D33glKehxkOlDo6GkBj1PoI8WuvYYvEUyxJNUdlVpa1C2lbewEL0rfyBrZ9\n\
4cp0ZeUknymYHn3ynW4Z8sYMlj7BNxGttV/jbxtgtT5WHJ+hg5/4/ifCPucN17Bt\n\
heUKIBoAjy6DlB/pMg1NUQ82DaASMFe89mEzI9Zk4CRtkWjEhknY0bYm46U1ABJb\n\
YmIsHlxdADskvWFDmq8CfJr/jXstTXxZeqaxPPdSP+WPwXN/ku5W7qkF2qimEKiy\n\
DYHzY65JhfWmHOLLGNuz6iHkq93uzkKsGIIPGQIDAQABAoIBABGTvdrwetv56uRz\n\
AiPti4pCV9RMkDWbbLzNSPRbStJU3t6phwlgN9Js2YkefBLvj7JF0pug8x6rDOtx\n\
PKCz+5841Wj3FuILt9JStZa4th0p0NUIMOVudrnBwf+g6s/dn5FzmTeaOyCyAPt8\n\
28b7W/FKcU8SNxM93JXfU1+JyFAdREqsXQfqLhCAXBb46fs5D8hg0c99RdWuJSuY\n\
HKyVXDrjmYAHS5qUDeMx0OY/q1qM03FBvHekvJ78WPpUKF7B/gYH9lBHIHE0KLJY\n\
JR6kKkzN/Ii6BsSubKKeuNntzlzd2ukvFdX4uc42dDpIXPdaQAn84KRYN7++KoGz\n\
2LqtAAECgYEAzQt5kt9c+xDeKMPR92XQ4uMeLxTufBei1PFGZbJnJT8aEMkVhKT/\n\
Pbh1Z8OnN9YvFprDNpEilUm7xyffrE7p0pI1/qiBXZExy6pIcGAo4ZcB8ayN0JV3\n\
k+RilE73x+sKAyWOm82b273PiyHNsQI4flkO5Ev9rpZbPMKlvZYsmxkCgYEAy9RR\n\
RwxwCpvFi3um0Rwz7CI2uvVXGaLVXyR2oNGLcJG7AhusYi4FX6XJQ3vAgiDmzu/c\n\
SaEF9w7uqeueIUA7L7njYP1ojfJAUJEtQRfVJF2tDntN5YgYUTsx8n3IKTs4xFT4\n\
dBthKo16zzLv92+8m4sWJhFW2zzFFLwk+G5jlAECgYEAln1piSZus8Y5h2nRXOZZ\n\
XWyb5qpSLrmaRPegV1uM4IVjuBYduPDwdHhBkxrCS/TjMo/73ry+yRsIuq7FN03j\n\
xyyQfItoByhdh8E+0VuCJbATOTEQFJre3KiuwXMD4LLc8lpKRIevcKPrA46XzOZ4\n\
WCM9DsnHMrAf3oRt6KujqWECgYEAyu43fWUEp4suwg/5pXdOumnV040vinZzuKW0\n\
9aeqDAkLBq5Gkfj/oJqOJoGux9+5640i5KtMJQzY0JOke7ZXNsz7dDTXQ3tMTOo9\n\
A/GWYv5grWpVw5AbpcQpliNkhKhRfCactfwMYTE6c89i2haE0NdI1d2te9ik3l/y\n\
7uP4gAECgYA3u2CumlkjHq+LbMK6Ry+Ajyy4ssM5PKJUqSogJDUkHsG/C7yaCdq/\n\
Ljt2x2220H0pM9885C3PpKxoYwRih9dzOamnARJocAElp2b5AQB+tKddlMdwx1pQ\n\
0IMGQ3fBYkDFLGYDk7hGYkLLlSJCZwi64xKmmfEwl83RL6JDSFupDg==\n\
-----END RSA PRIVATE KEY-----";

static RSA *GetResetKey()
{
  BIO *bio = BIO_new_mem_buf((void*)kResetKeyPem, sizeof(kResetKeyPem) - 1);
  RSA *rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, 0, NULL);
  BIO_free(bio);
  return rsa;
}

int AvbTest::ProductionResetTest(uint32_t selector, uint64_t nonce,
                                 const uint8_t *device_data, size_t data_len,
                                 const uint8_t *signature, size_t signature_len)
{
  ProductionResetTestRequest request;
  request.set_selector(selector);
  request.set_nonce(nonce);
  if (signature && signature_len) {
    request.set_signature(signature, signature_len);
  }
  if (device_data && data_len) {
    request.set_device_data(device_data, data_len);
  }
  Avb service(*client);
  return service.ProductionResetTest(request, nullptr);
}

int AvbTest::SignChallenge(const struct ResetMessage *message,
                           uint8_t *signature, size_t *maxsig)
{
  size_t siglen = *maxsig;
  RSA *key = GetResetKey();
  if (!key)
    return -1;
  EVP_PKEY *pkey = EVP_PKEY_new();
  if (!pkey)
    return -1;
  if (!EVP_PKEY_set1_RSA(pkey, key))
    return -1;
  EVP_MD_CTX md_ctx;
  EVP_MD_CTX_init(&md_ctx);
  EVP_PKEY_CTX *pkey_ctx;
  if (!EVP_DigestSignInit(&md_ctx, &pkey_ctx, EVP_sha256(), NULL, pkey))
    return -1;
  if (!EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING))
    return -1;
  if (!EVP_DigestSignUpdate(&md_ctx, (uint8_t *)message, sizeof(*message)))
    return -1;
  EVP_DigestSignFinal(&md_ctx, NULL, &siglen);
  if (siglen > *maxsig) {
    std::cerr << "Signature length too long: " << siglen << " > "
              << *maxsig << std::endl;
    return -2;
  }
  *maxsig = siglen;
  int code = EVP_DigestSignFinal(&md_ctx, signature, &siglen);
  if (!code) {
    std::cerr << "OpenSSL error: " <<  ERR_get_error() << ": "
              << ERR_reason_error_string(ERR_get_error()) << std::endl;
    return -3;
  }
  EVP_MD_CTX_cleanup(&md_ctx);
  EVP_PKEY_free(pkey);
  RSA_free(key);
  // Feed it in
  return 0;
}

int AvbTest::Load(uint8_t slot, uint64_t *version)
{
  LoadRequest request;
  LoadResponse response;
  int code;

  request.set_slot(slot);

  Avb service(*client);
  code = service.Load(request, &response);
  if (code == APP_SUCCESS) {
    *version = response.version();
  }

  return code;
}

int AvbTest::Store(uint8_t slot, uint64_t version)
{
  StoreRequest request;

  request.set_slot(slot);
  request.set_version(version);

  Avb service(*client);
  return service.Store(request, nullptr);
}

int AvbTest::SetDeviceLock(uint8_t locked)
{
  SetDeviceLockRequest request;

  request.set_locked(locked);

  Avb service(*client);
  return service.SetDeviceLock(request, nullptr);
}

int AvbTest::SetBootLock(uint8_t locked)
{
  SetBootLockRequest request;

  request.set_locked(locked);

  Avb service(*client);
  return service.SetBootLock(request, nullptr);
}

int AvbTest::SetOwnerLock(uint8_t locked, const uint8_t *metadata, size_t size)
{
  SetOwnerLockRequest request;

  request.set_locked(locked);
  if (metadata != NULL && size > 0) {
    request.set_key(metadata, size);
  }

  Avb service(*client);
  return service.SetOwnerLock(request, nullptr);
}

int AvbTest::GetOwnerKey(uint32_t offset, uint8_t *metadata, size_t *size)
{
  GetOwnerKeyRequest request;
  GetOwnerKeyResponse response;
  size_t i;
  int code;

  request.set_offset(offset);
  if (size != NULL)
    request.set_size(*size);

  Avb service(*client);
  code = service.GetOwnerKey(request, &response);

  if (code == APP_SUCCESS) {
    auto chunk = response.chunk();

    if (metadata != NULL) {
      for (i = 0; i < chunk.size(); i++)
        metadata[i] = chunk[i];
    }
    if (size != NULL)
      *size = chunk.size();
  }

  return code;
}

int AvbTest::SetCarrierLock(uint8_t locked, const uint8_t *metadata, size_t size)
{
  CarrierLockRequest request;

  request.set_locked(locked);
  if (metadata != NULL && size > 0) {
    request.set_device_data(metadata, size);
  }

  Avb service(*client);
  return service.CarrierLock(request, nullptr);
}

// Tests

TEST_F(AvbTest, CarrierLockTest)
{
  uint8_t carrier_data[AVB_DEVICE_DATA_SIZE];
  uint8_t locks[4];
  int code;

  // Test we can set and unset the carrier lock in non production mode
  memset(carrier_data, 0, sizeof(carrier_data));

  code = SetCarrierLock(0x12, carrier_data, sizeof(carrier_data));
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[CARRIER], 0x12);

  code = SetCarrierLock(0, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  // Set production mode
  avb_tools::SetBootloader(client.get());
  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");
  avb_tools::BootloaderDone(client.get());

  // Test we cannot set or unset the carrier lock in production mode
  code = SetCarrierLock(0x12, carrier_data, sizeof(carrier_data));
  ASSERT_EQ(code, APP_ERROR_AVB_AUTHORIZATION);

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[CARRIER], 0x00);

  code = SetCarrierLock(0, NULL, 0);
  ASSERT_EQ(code, APP_ERROR_AVB_AUTHORIZATION);
}

TEST_F(AvbTest, CarrierUnlockTest)
{
  CarrierLockTestRequest request;
  CarrierLockTestResponse response;
  CarrierUnlock *token;

  token = new CarrierUnlock();
  token->set_nonce(NONCE);
  token->set_version(VERSION);
  token->set_signature(SIGNATURE, sizeof(SIGNATURE));

  request.set_last_nonce(LAST_NONCE);
  request.set_version(VERSION);
  request.set_device_data(DEVICE_DATA, sizeof(DEVICE_DATA));
  request.set_allocated_token(token);

  Avb service(*client);
  ASSERT_NO_ERROR(service.CarrierLockTest(request, &response), "");

  // The nonce is covered by the signature, so changing it should trip the
  // signature verification
  token->set_nonce(NONCE + 1);
  ASSERT_EQ(service.CarrierLockTest(request, &response), APP_ERROR_AVB_AUTHORIZATION);
}

TEST_F(AvbTest, DeviceLockTest)
{
  bool bootloader;
  bool production;
  uint8_t locks[4];
  int code;

  // Test cannot set the lock
  avb_tools::SetBootloader(client.get());
  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  code = SetDeviceLock(0x12);
  ASSERT_EQ(code, APP_ERROR_AVB_HLOS);

  // Test can set lock
  ResetProduction(client.get());
  avb_tools::SetBootloader(client.get());

  code = SetDeviceLock(0x34);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), &bootloader, &production, locks);
  ASSERT_TRUE(bootloader);
  ASSERT_FALSE(production);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Test cannot set while set
  code = SetDeviceLock(0x56);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Test can unset
  code = SetDeviceLock(0x00);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[DEVICE], 0x00);
}

TEST_F(AvbTest, SetDeviceLockIsIdempotent) {
  ASSERT_NO_ERROR(SetDeviceLock(0x65), "");
  ASSERT_NO_ERROR(SetDeviceLock(0x65), "");
}

TEST_F(AvbTest, BootLockTest)
{
  uint8_t locks[4];
  int code;
  // Test production logic.
  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  // Test cannot set lock
  code = SetBootLock(0x12);
  ASSERT_EQ(code, APP_ERROR_AVB_BOOTLOADER);

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[BOOT], 0x00);

  // Show the bootloader setting and unsetting.
  avb_tools::SetBootloader(client.get());
  code = SetBootLock(0x12);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[BOOT], 0x12);

  code = SetBootLock(0x0);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[BOOT], 0x00);

  // Test cannot unset lock while carrier set
  ResetProduction(client.get());
  code = Reset(client.get(), ResetRequest::LOCKS, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  code = SetCarrierLock(0x34, DEVICE_DATA, sizeof(DEVICE_DATA));
  ASSERT_NO_ERROR(code, "");

  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  // Can lock when carrier lock is set.
  avb_tools::SetBootloader(client.get());
  code = SetBootLock(0x56);
  ASSERT_NO_ERROR(code, "");

  // Cannot unlock.
  code = SetBootLock(0x0);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  // Or change the value.
  code = SetBootLock(0x42);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[CARRIER], 0x34);
  ASSERT_EQ(locks[BOOT], 0x56);

  // Clear the locks to show device lock enforcement.
  ResetProduction(client.get());
  code = Reset(client.get(), ResetRequest::LOCKS, NULL, 0);
  ASSERT_NO_ERROR(code, "");
  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");
  avb_tools::SetBootloader(client.get());

  // Need to be in the HLOS.
  code = SetDeviceLock(0x78);
  ASSERT_EQ(code, APP_ERROR_AVB_HLOS);

  avb_tools::BootloaderDone(client.get());
  code = SetDeviceLock(0x78);
  ASSERT_NO_ERROR(code, "");

  // We can move to a locked state when
  // device lock is true.
  avb_tools::SetBootloader(client.get());
  code = SetBootLock(0x9A);
  ASSERT_NO_ERROR(code, "");

  // But we can't move back.
  code = SetBootLock(0x0);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[DEVICE], 0x78);
  ASSERT_EQ(locks[BOOT], 0x9A);
}

TEST_F(AvbTest, SetBootLockIsIdempotent) {
  ASSERT_NO_ERROR(SetBootLock(0x12), "");
  ASSERT_NO_ERROR(SetBootLock(0x12), "");
}

TEST_F(AvbTest, SetBootLockAfterWipingUserData) {
  // This is a sequence of commands that the bootloader will issue
  ASSERT_NO_ERROR(SetProduction(client.get(), true, NULL, 0), "");
  avb_tools::SetBootloader(client.get());
  ASSERT_TRUE(nugget_tools::WipeUserData(client.get()));
  ASSERT_NO_ERROR(SetBootLock(0xdc), "");
}

TEST_F(AvbTest, OwnerLockTest)
{
  uint8_t owner_key[AVB_METADATA_MAX_SIZE];
  uint8_t chunk[AVB_CHUNK_MAX_SIZE];
  uint8_t locks[4];
  int code;
  size_t i;

  for (i = 0; i < AVB_METADATA_MAX_SIZE; i += 2) {
    owner_key[i + 0] = (i >> 8) & 0xFF;
    owner_key[i + 1] = (i >> 8) & 0xFF;
  }

  // This should pass when BOOT lock is not set
  code = SetOwnerLock(0x65, owner_key, sizeof(owner_key));
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[OWNER], 0x65);

  for (i = 0; i < AVB_METADATA_MAX_SIZE; i += AVB_CHUNK_MAX_SIZE) {
    size_t size = sizeof(chunk);
    size_t j;

    code = GetOwnerKey(i, chunk, &size);
    ASSERT_NO_ERROR(code, "");
    ASSERT_EQ(size, sizeof(chunk));
    for (j = 0; j < size; j++) {
      ASSERT_EQ(chunk[j], owner_key[i + j]);
    }
  }

  // Test setting the lock while set fails
  code = SetOwnerLock(0x87, owner_key, sizeof(owner_key));
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[OWNER], 0x65);

  // Clear it
  code = SetOwnerLock(0x00, owner_key, sizeof(owner_key));
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[OWNER], 0x00);

  // Set the boot lock
  avb_tools::SetBootloader(client.get());
  code = SetBootLock(0x43);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, NULL, locks);
  ASSERT_EQ(locks[BOOT], 0x43);

  // Test setting the lock while BOOT is locked fails
  code = SetOwnerLock(0x21, owner_key, sizeof(owner_key));
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);
}

TEST_F(AvbTest, ProductionMode)
{
  bool production;
  uint8_t locks[4];
  int code;

  // Check we're not in production mode
  GetState(client.get(), NULL, &production, NULL);
  ASSERT_FALSE(production);

  // Set some lock values to make sure production doesn't affect them
  avb_tools::SetBootloader(client.get());
  code = SetOwnerLock(0x11, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  code = SetBootLock(0x22);
  ASSERT_NO_ERROR(code, "");

  code = SetCarrierLock(0x33, DEVICE_DATA, sizeof(DEVICE_DATA));
  ASSERT_NO_ERROR(code, "");

  code = SetDeviceLock(0x44);
  ASSERT_NO_ERROR(code, "");

  // Set production mode with a DUT hash
  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), NULL, &production, locks);
  ASSERT_TRUE(production);
  ASSERT_EQ(locks[OWNER], 0x11);
  ASSERT_EQ(locks[BOOT], 0x22);
  ASSERT_EQ(locks[CARRIER], 0x33);
  ASSERT_EQ(locks[DEVICE], 0x44);

  // Test production cannot be turned off.
  code = SetProduction(client.get(), false, NULL, 0);
  ASSERT_EQ(code, APP_ERROR_AVB_AUTHORIZATION);
  avb_tools::BootloaderDone(client.get());
  code = SetProduction(client.get(), false, NULL, 0);
  ASSERT_EQ(code, APP_ERROR_AVB_AUTHORIZATION);
}

TEST_F(AvbTest, Rollback)
{
  uint64_t value = ~0ULL;
  int code, i;

  // Test we cannot change values in normal mode
  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");
  for (i = 0; i < 8; i++) {
    code = Store(i, 0xFF00000011223344 + i);
    ASSERT_EQ(code, APP_ERROR_AVB_BOOTLOADER);

    code = Load(i, &value);
    ASSERT_NO_ERROR(code, "");
    ASSERT_EQ(value, 0x00ULL);
  }

  // Test we can change values in bootloader mode
  avb_tools::SetBootloader(client.get());
  for (i = 0; i < 8; i++) {
    code = Store(i, 0xFF00000011223344 + i);
    ASSERT_NO_ERROR(code, "");

    code = Load(i, &value);
    ASSERT_NO_ERROR(code, "");
    ASSERT_EQ(value, 0xFF00000011223344 + i);

    code = Store(i, 0x8800000011223344 - i);
    ASSERT_NO_ERROR(code, "");

    code = Load(i, &value);
    ASSERT_NO_ERROR(code, "");
    ASSERT_EQ(value, 0x8800000011223344 - i);
  }
}

TEST_F(AvbTest, Reset)
{
  bool bootloader;
  bool production;
  uint8_t locks[4];
  int code;

  // Set some locks and production mode*/
  avb_tools::SetBootloader(client.get());
  code = SetBootLock(0x12);
  ASSERT_NO_ERROR(code, "");

  code = SetDeviceLock(0x34);
  ASSERT_NO_ERROR(code, "");

  code = SetProduction(client.get(), true, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  avb_tools::BootloaderDone(client.get());

  GetState(client.get(), &bootloader, &production, locks);
  ASSERT_FALSE(bootloader);
  ASSERT_TRUE(production);
  ASSERT_EQ(locks[BOOT], 0x12);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Try reset, should fail
  code = Reset(client.get(), ResetRequest::LOCKS, NULL, 0);
  ASSERT_EQ(code, APP_ERROR_AVB_DENIED);

  GetState(client.get(), &bootloader, &production, locks);
  ASSERT_FALSE(bootloader);
  ASSERT_TRUE(production);
  ASSERT_EQ(locks[BOOT], 0x12);
  ASSERT_EQ(locks[DEVICE], 0x34);

  // Disable production, try reset, should pass
  ResetProduction(client.get());
  code = Reset(client.get(), ResetRequest::LOCKS, NULL, 0);
  ASSERT_NO_ERROR(code, "");

  GetState(client.get(), &bootloader, &production, locks);
  ASSERT_FALSE(bootloader);
  ASSERT_FALSE(production);
  ASSERT_EQ(locks[BOOT], 0x00);
  ASSERT_EQ(locks[DEVICE], 0x00);
}

TEST_F(AvbTest, GetResetChallengeTest)
{
  int code;
  uint32_t selector;
  uint64_t nonce;
  uint8_t data[32];
  uint8_t empty[32];
  size_t len = sizeof(data);

  memset(data, 0, sizeof(data));
  memset(empty, 0, sizeof(empty));
  code = GetResetChallenge(client.get(), &selector, &nonce, data, &len);
  ASSERT_LE(sizeof(data), len);
  ASSERT_NO_ERROR(code, "");
  EXPECT_NE(0ULL, nonce);
  EXPECT_EQ((uint32_t)ResetToken::CURRENT, selector);
  EXPECT_EQ(32UL, len);
  // We didn't set a device id.
  EXPECT_EQ(0, memcmp(data, empty, sizeof(empty)));
}

// TODO(drewry) move to new test suite since this is unsafe on
//  non-TEST_IMAGE builds.
TEST_F(AvbTest, ResetProductionValid)
{
  static const uint8_t kDeviceHash[] = {
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8
  };
  struct ResetMessage message;
  int code;
  uint32_t selector;
  size_t len = sizeof(message.data);
  uint8_t signature[2048/8 + 1];
  size_t siglen = sizeof(signature);

  // Lock in a fixed device hash
  code = SetProduction(client.get(), true, kDeviceHash, sizeof(kDeviceHash));
  EXPECT_EQ(0, code);

  memset(message.data, 0, sizeof(message.data));
  code = GetResetChallenge(client.get(), &selector, &message.nonce, message.data, &len);
  ASSERT_NO_ERROR(code, "");
  // Expect, not assert, just in case something goes weird, we may still
  // exit cleanly.
  EXPECT_EQ(0, memcmp(message.data, kDeviceHash, sizeof(kDeviceHash)));
  ASSERT_EQ(0, SignChallenge(&message, signature, &siglen));

  // Try a bad challenge, wasting the nonce.
  uint8_t orig = signature[0];
  signature[0] += 1;
  code = Reset(client.get(), ResetRequest::PRODUCTION, signature, siglen);
  // TODO: expect the LINENO error
  EXPECT_NE(0, code);
  signature[0] = orig;
  // Re-lock since TEST_IMAGE will unlock on failure.
  code = SetProduction(client.get(), true, kDeviceHash, sizeof(kDeviceHash));
  EXPECT_EQ(0, code);

  // Now use a good one, but without getting a new nonce.
  code = Reset(client.get(), ResetRequest::PRODUCTION, signature, siglen);
  EXPECT_EQ(code, APP_ERROR_AVB_DENIED);

  // Now get the nonce and use a good signature.
  code = GetResetChallenge(client.get(), &selector, &message.nonce, message.data, &len);
  ASSERT_NO_ERROR(code, "");
  ASSERT_EQ(0, SignChallenge(&message, signature, &siglen));
  code = Reset(client.get(), ResetRequest::PRODUCTION, signature, siglen);
  ASSERT_NO_ERROR(code, "");
}

static const uint8_t kNullSig[] = {
  0x95, 0x35, 0x5a, 0xb6, 0xe3, 0x8e, 0x43, 0x03, 0xd9, 0xd9, 0xd5, 0x6e,
  0x99, 0x86, 0xff, 0x8e, 0x6a, 0xf1, 0x54, 0x6f, 0xa8, 0xff, 0x37, 0x38,
  0xc6, 0x9b, 0x4d, 0xc6, 0x99, 0x1f, 0x37, 0x5c, 0xec, 0xf4, 0x32, 0xd8,
  0xe6, 0x00, 0xcc, 0x74, 0xde, 0xa9, 0x68, 0x1a, 0xab, 0x6a, 0x6e, 0xe7,
  0xa7, 0xa1, 0x59, 0xe0, 0x7c, 0x86, 0x95, 0x28, 0x94, 0x18, 0x3f, 0x0f,
  0xb9, 0x0f, 0x05, 0x6c, 0x86, 0x5a, 0x6a, 0xe4, 0x6d, 0x36, 0x71, 0x86,
  0x38, 0xab, 0x7a, 0x2d, 0x9c, 0xa5, 0xfa, 0xc8, 0x7c, 0x48, 0x02, 0x8c,
  0x6b, 0x4d, 0xda, 0xa4, 0xb5, 0xa8, 0x17, 0x39, 0x5e, 0xe3, 0x1a, 0xd5,
  0xf8, 0x87, 0x6e, 0xd9, 0xc0, 0x0c, 0x29, 0x4d, 0x93, 0xa2, 0x3b, 0xfc,
  0x2d, 0x38, 0x8e, 0x2b, 0xc7, 0x49, 0x26, 0xd9, 0xcb, 0x47, 0x89, 0x4c,
  0x79, 0xd3, 0x60, 0x62, 0xf9, 0x71, 0xa7, 0x73, 0x6a, 0x03, 0x65, 0x1f,
  0x11, 0x0d, 0x9e, 0x27, 0x99, 0x6b, 0xa7, 0x46, 0x85, 0x75, 0xec, 0xff,
  0x5b, 0x1d, 0x8d, 0x1b, 0x34, 0xd8, 0xb9, 0x4f, 0x63, 0x88, 0x08, 0xa8,
  0x16, 0xba, 0xfc, 0xe7, 0x66, 0xa4, 0xe5, 0xde, 0x4e, 0x0b, 0x98, 0x80,
  0xd5, 0x16, 0x55, 0xfb, 0xdb, 0xe8, 0xa2, 0x90, 0x85, 0x4e, 0xa9, 0xb6,
  0x81, 0x55, 0xef, 0xbf, 0x12, 0xe3, 0xd2, 0xa9, 0xae, 0x2c, 0x43, 0x67,
  0x4c, 0x09, 0x6d, 0x95, 0xaf, 0x44, 0xc2, 0xb9, 0x9d, 0x7c, 0xb1, 0x88,
  0xf8, 0x6c, 0xa0, 0x13, 0x4c, 0xbf, 0x85, 0xa2, 0x8b, 0x9d, 0x06, 0xc8,
  0x11, 0xdb, 0x1f, 0xfb, 0x05, 0x15, 0xd6, 0x1f, 0xe5, 0x52, 0x9c, 0xd5,
  0xbd, 0xff, 0xb0, 0xce, 0x29, 0xec, 0xd8, 0x9e, 0xdb, 0x5b, 0xc9, 0x52,
  0x24, 0xaf, 0x22, 0xeb, 0xce, 0x15, 0x0d, 0xfd, 0x6c, 0x76, 0x90, 0x3e,
  0x4f, 0x63, 0xfd, 0xb1
};

const static unsigned int kNullSigLen = 256;

TEST_F(AvbTest, ProductionResetTestValid)
{
  static const uint8_t kDeviceHash[] = {
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8
  };
  struct ResetMessage message;
  int code;
  uint32_t selector = ResetToken::CURRENT;
  size_t len = sizeof(message.data);
  uint8_t signature[2048/8 + 1];
  size_t siglen = sizeof(signature);

  memcpy(message.data, kDeviceHash, sizeof(message.data));
  message.nonce = 123456;
  ASSERT_EQ(0, SignChallenge(&message, signature, &siglen));

  // Try a bad challenge, wasting the nonce.
  uint8_t orig = signature[0];
  signature[0] += 1;
  code = ProductionResetTest(selector, message.nonce, message.data, len,
                             signature, siglen);
  EXPECT_NE(0, code);
  signature[0] = orig;

  // Now use a good signature.
  code = ProductionResetTest(selector, message.nonce, message.data, len,
                             signature, siglen);
  ASSERT_NO_ERROR(code, "");

  // Note, testing nonce expiration is handled in the Reset(PRODUCTION)
  // test. This just checks a second signature over an app sourced nonce.
  code = GetResetChallenge(client.get(), &selector, &message.nonce, message.data, &len);
  ASSERT_NO_ERROR(code, "");
  ASSERT_EQ(0, SignChallenge(&message, signature, &siglen));
  code = Reset(client.get(), ResetRequest::PRODUCTION, signature, siglen);
  ASSERT_NO_ERROR(code, "");

  // Now test a null signature as we will for the real keys
  message.nonce = 0;
  memset(message.data, 0, sizeof(message.data));
  code = ProductionResetTest(selector, message.nonce, message.data, len,
                             kNullSig, kNullSigLen);
  ASSERT_NO_ERROR(code, "");
}

TEST_F(AvbTest, WipeUserDataDoesNotLockDeviceLock) {
  ASSERT_NO_ERROR(SetDeviceLock(0x00), "");
  ASSERT_TRUE(nugget_tools::WipeUserData(client.get()));

  uint8_t locks[4];
  GetState(client.get(), nullptr, nullptr, locks);
  EXPECT_EQ(locks[DEVICE], 0x00);
}

}  // namespace
