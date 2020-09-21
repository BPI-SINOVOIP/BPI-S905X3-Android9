#include "gtest/gtest.h"
#include "avb_tools.h"
#include "keymaster_tools.h"
#include "nugget_tools.h"
#include "nugget/app/keymaster/keymaster.pb.h"
#include "nugget/app/keymaster/keymaster_defs.pb.h"
#include "nugget/app/keymaster/keymaster_types.pb.h"
#include "Keymaster.client.h"
#include "util.h"

#include "src/blob.h"
#include "src/macros.h"
#include "src/test-data/test-keys/rsa.h"

#include "openssl/bn.h"
#include "openssl/ec_key.h"
#include "openssl/nid.h"

#include <sstream>

using std::cout;
using std::string;
using std::stringstream;
using std::unique_ptr;

using namespace nugget::app::keymaster;

using namespace test_data;

namespace {

class ImportKeyTest: public testing::Test {
 protected:
  static unique_ptr<nos::NuggetClientInterface> client;
  static unique_ptr<Keymaster> service;
  static unique_ptr<test_harness::TestHarness> uart_printer;

  static void SetUpTestCase();
  static void TearDownTestCase();

  void initRSARequest(ImportKeyRequest *request, Algorithm alg) {
    KeyParameters *params = request->mutable_params();
    KeyParameter *param = params->add_params();
    param->set_tag(Tag::ALGORITHM);
    param->set_integer((uint32_t)alg);
  }

  void initRSARequest(ImportKeyRequest *request, Algorithm alg, int key_size) {
    initRSARequest(request, alg);

    if (key_size >= 0) {
      KeyParameters *params = request->mutable_params();
      KeyParameter *param = params->add_params();
      param->set_tag(Tag::KEY_SIZE);
      param->set_integer(key_size);
    }
  }

  void initRSARequest(ImportKeyRequest *request, Algorithm alg, int key_size,
                   int public_exponent_tag) {
    initRSARequest(request, alg, key_size);

    if (public_exponent_tag >= 0) {
      KeyParameters *params = request->mutable_params();
      KeyParameter *param = params->add_params();
      param->set_tag(Tag::RSA_PUBLIC_EXPONENT);
      param->set_long_integer(public_exponent_tag);
    }
  }

  void initRSARequest(ImportKeyRequest *request, Algorithm alg, int key_size,
                   int public_exponent_tag, uint32_t public_exponent,
                   const string& d, const string& n) {
    initRSARequest(request, alg, key_size, public_exponent_tag);

    request->mutable_rsa()->set_e(public_exponent);
    request->mutable_rsa()->set_d(d);
    request->mutable_rsa()->set_n(n);
  }
};

unique_ptr<nos::NuggetClientInterface> ImportKeyTest::client;
unique_ptr<Keymaster> ImportKeyTest::service;
unique_ptr<test_harness::TestHarness> ImportKeyTest::uart_printer;

void ImportKeyTest::SetUpTestCase() {
  uart_printer = test_harness::TestHarness::MakeUnique();

  client = nugget_tools::MakeNuggetClient();
  client->Open();
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";

  service.reset(new Keymaster(*client));

  // Do setup that is normally done by the bootloader.
  keymaster_tools::SetRootOfTrust(client.get());
}

void ImportKeyTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();

  uart_printer = nullptr;
}

// TODO: refactor into import key tests.

// Failure cases.
TEST_F(ImportKeyTest, AlgorithmMissingFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();

  /* Algorithm tag is unspecified, import should fail. */
  KeyParameter *param = params->add_params();
  param->set_tag(Tag::KEY_SIZE);
  param->set_integer(512);

  param = params->add_params();
  param->set_tag(Tag::RSA_PUBLIC_EXPONENT);
  param->set_long_integer(3);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

// RSA

TEST_F(ImportKeyTest, RSAInvalidKeySizeFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRSARequest(&request, Algorithm::RSA, 256, 3);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::UNSUPPORTED_KEY_SIZE);
}

TEST_F(ImportKeyTest, RSAInvalidPublicExponentFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // Unsupported exponent
  initRSARequest(&request, Algorithm::RSA, 512, 2, 2,
                 string(64, '\0'), string(64, '\0'));

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::UNSUPPORTED_KEY_SIZE);
}

TEST_F(ImportKeyTest, RSAKeySizeTagMisatchNFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // N does not match KEY_SIZE.
  initRSARequest(&request, Algorithm::RSA, 512, 3, 3,
                 string(64, '\0'), string(63, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(ImportKeyTest, RSAKeySizeTagMisatchDFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // D does not match KEY_SIZE.
  initRSARequest(&request, Algorithm::RSA, 512, 3, 3,
                 string(63, '\0'), string(64, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(ImportKeyTest, RSAPublicExponentTagMisatchFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // e does not match PUBLIC_EXPONENT tag.
  initRSARequest(&request, Algorithm::RSA, 512, 3, 2,
                 string(64, '\0'), string(64, '\0'));
  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(),
            ErrorCode::IMPORT_PARAMETER_MISMATCH);
}

TEST_F(ImportKeyTest, RSA1024BadEFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  // Mis-matched e.
  const string d((const char *)RSA_1024_D, sizeof(RSA_1024_D));
  const string N((const char *)RSA_1024_N, sizeof(RSA_1024_N));
  initRSARequest(&request, Algorithm::RSA, 1024, 3, 3, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ImportKeyTest, RSA1024BadDFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  const string d(string("\x01") +  /* Twiddle LSB of D. */
                 string((const char *)RSA_1024_D, sizeof(RSA_1024_D) - 1));
  const string N((const char *)RSA_1024_N, sizeof(RSA_1024_N));
  initRSARequest(&request, Algorithm::RSA, 1024, 65537, 65537, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ImportKeyTest, RSA1024BadNFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  const string d((const char *)RSA_1024_D, sizeof(RSA_1024_D));
  const string N(string("\x01") +  /* Twiddle LSB of N. */
                 string((const char *)RSA_1024_N, sizeof(RSA_1024_N) - 1));
  initRSARequest(&request, Algorithm::RSA, 1024, 65537, 65537, d, N);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ImportKeyTest, RSASuccess) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRSARequest(&request, Algorithm::RSA);
  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  for (size_t i = 0; i < ARRAYSIZE(TEST_RSA_KEYS); i++) {
    param->set_tag(Tag::RSA_PUBLIC_EXPONENT);
    param->set_long_integer(TEST_RSA_KEYS[i].e);

    request.mutable_rsa()->set_e(TEST_RSA_KEYS[i].e);
    request.mutable_rsa()->set_d(TEST_RSA_KEYS[i].d, TEST_RSA_KEYS[i].size);
    request.mutable_rsa()->set_n(TEST_RSA_KEYS[i].n, TEST_RSA_KEYS[i].size);

    stringstream ss;
    ss << "Failed at TEST_RSA_KEYS[" << i << "]";
    ASSERT_NO_ERROR(service->ImportKey(request, &response), ss.str());
    EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK)
        << "Failed at TEST_RSA_KEYS[" << i << "]";

    /* TODO: add separate tests for blobs! */
    EXPECT_EQ(sizeof(struct km_blob), response.blob().blob().size());
    const struct km_blob *blob =
        (const struct km_blob *)response.blob().blob().data();
    EXPECT_EQ(memcmp(blob->b.key.rsa.N_bytes, TEST_RSA_KEYS[i].n,
                     TEST_RSA_KEYS[i].size), 0);
    EXPECT_EQ(memcmp(blob->b.key.rsa.d_bytes,
                     TEST_RSA_KEYS[i].d, TEST_RSA_KEYS[i].size), 0);
  }
}

TEST_F(ImportKeyTest, RSA1024OptionalParamsAbsentSuccess) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  initRSARequest(&request, Algorithm::RSA, -1, -1, 65537,
                 string((const char *)RSA_1024_D, sizeof(RSA_1024_D)),
                 string((const char *)RSA_1024_N, sizeof(RSA_1024_N)));

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);

  EXPECT_EQ(sizeof(struct km_blob), response.blob().blob().size());
  const struct km_blob *blob =
      (const struct km_blob *)response.blob().blob().data();
  EXPECT_EQ(memcmp(blob->b.key.rsa.N_bytes, RSA_1024_N,
                   sizeof(RSA_1024_N)), 0);
  EXPECT_EQ(memcmp(blob->b.key.rsa.d_bytes,
                   RSA_1024_D, sizeof(RSA_1024_D)), 0);
}

// EC

TEST_F(ImportKeyTest, ECMissingCurveIdTagFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag(Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ImportKeyTest, ECMisMatchedCurveIdTagFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag(Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag(Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  request.mutable_ec()->set_curve_id(((uint32_t)EcCurve::P_256) + 1);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ImportKeyTest, ECMisMatchedKeySizeTagCurveTagFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag(Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag(Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  param = params->add_params();
  param->set_tag(Tag::KEY_SIZE);
  param->set_integer((uint32_t)384);  /* Should be 256 */

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ImportKeyTest, ECMisMatchedP256KeySizeFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag(Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag(Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);
  request.mutable_ec()->set_d(string((256 >> 3) - 1, '\0'));
  request.mutable_ec()->set_x(string((256 >> 3), '\0'));
  request.mutable_ec()->set_y(string((256 >> 3), '\0'));

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

// TODO: bad key tests.  invalid d, {x,y} not on curve, d, xy mismatched.
TEST_F(ImportKeyTest, ECP256BadKeyFails) {
  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag(Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag(Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);
  request.mutable_ec()->set_d(string((256 >> 3), '\0'));
  request.mutable_ec()->set_x(string((256 >> 3), '\0'));
  request.mutable_ec()->set_y(string((256 >> 3), '\0'));

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::INVALID_ARGUMENT);
}

TEST_F (ImportKeyTest, ImportECP256KeySuccess) {
  // Generate an EC key.
  // TODO: just hardcode a test key.
  bssl::UniquePtr<EC_KEY> ec(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
  EXPECT_EQ(EC_KEY_generate_key(ec.get()), 1);
  const EC_GROUP *group = EC_KEY_get0_group(ec.get());
  const BIGNUM *d = EC_KEY_get0_private_key(ec.get());
  const EC_POINT *point = EC_KEY_get0_public_key(ec.get());
  bssl::UniquePtr<BIGNUM> x(BN_new());
  bssl::UniquePtr<BIGNUM> y(BN_new());
  EXPECT_EQ(EC_POINT_get_affine_coordinates_GFp(
      group, point, x.get(), y.get(), NULL), 1);

  // Turn d, x, y into binary strings.
  const size_t field_size = (EC_GROUP_get_degree(group) + 7) >> 3;
  std::unique_ptr<uint8_t []> dstr(new uint8_t[field_size]);
  std::unique_ptr<uint8_t []> xstr(new uint8_t[field_size]);
  std::unique_ptr<uint8_t []> ystr(new uint8_t[field_size]);

  EXPECT_EQ(BN_bn2le_padded(dstr.get(), field_size, d), 1);
  EXPECT_EQ(BN_bn2le_padded(xstr.get(), field_size, x.get()), 1);
  EXPECT_EQ(BN_bn2le_padded(ystr.get(), field_size, y.get()), 1);

  ImportKeyRequest request;
  ImportKeyResponse response;

  KeyParameters *params = request.mutable_params();
  KeyParameter *param = params->add_params();
  param->set_tag(Tag::ALGORITHM);
  param->set_integer((uint32_t)Algorithm::EC);

  param = params->add_params();
  param->set_tag(Tag::EC_CURVE);
  param->set_integer((uint32_t)EcCurve::P_256);

  param = params->add_params();
  param->set_tag(Tag::KEY_SIZE);
  param->set_integer((uint32_t)256);

  request.mutable_ec()->set_curve_id((uint32_t)EcCurve::P_256);
  request.mutable_ec()->set_d(dstr.get(), field_size);
  request.mutable_ec()->set_x(xstr.get(), field_size);
  request.mutable_ec()->set_y(ystr.get(), field_size);

  ASSERT_NO_ERROR(service->ImportKey(request, &response), "");
  EXPECT_EQ((ErrorCode)response.error_code(), ErrorCode::OK);
}

// TODO: add tests for symmetric key import.

}  // namespace
