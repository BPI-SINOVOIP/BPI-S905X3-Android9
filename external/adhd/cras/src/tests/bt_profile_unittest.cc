/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <dbus/dbus.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <sys/socket.h>

#include "dbus_test.h"

extern "C" {
#include "cras_bt_constants.h"
#include "cras_bt_profile.h"
}

namespace {

static struct cras_bt_profile fake_profile;
static struct cras_bt_transport *fake_transport;
static int profile_release_called;
static struct cras_bt_profile *profile_release_arg_value;
static int profile_new_connection_called;
static struct cras_bt_transport *profile_new_connection_arg_value;
static int profile_request_disconnection_called;
static struct cras_bt_transport *profile_request_disconnection_arg_value;
static int profile_cancel_called;
static struct cras_bt_profile *profile_cancel_arg_value;
static int cras_bt_transport_get_called;
static const char *cras_bt_transport_get_arg_value;

void fake_profile_release(struct cras_bt_profile *profile);
void fake_profile_new_connection(struct cras_bt_profile *profile,
				 struct cras_bt_transport *transport);
void fake_profile_request_disconnection(struct cras_bt_profile *profile,
					struct cras_bt_transport *transport);
void fake_profile_cancel(struct cras_bt_profile *profile);

class BtProfileTestSuite : public DBusTest {
  virtual void SetUp() {
    DBusTest::SetUp();

    profile_release_called = 0;
    profile_new_connection_called = 0;
    profile_request_disconnection_called = 0;
    profile_cancel_called = 0;

    fake_profile.name = "fake";
    fake_profile.object_path = "/fake";
    fake_profile.uuid = "0";
    fake_profile.version = 0;
    fake_profile.role = NULL;
    fake_profile.features = 0;
    fake_profile.release = fake_profile_release;
    fake_profile.new_connection = fake_profile_new_connection;
    fake_profile.request_disconnection = fake_profile_request_disconnection;
    fake_profile.cancel = fake_profile_cancel;

    fake_transport = reinterpret_cast<struct cras_bt_transport*>(0x321);
    cras_bt_transport_get_called = 0;
  }
};

TEST_F(BtProfileTestSuite, RegisterProfile) {
  struct cras_bt_profile *profile;

  ExpectMethodCall(PROFILE_MANAGER_OBJ_PATH, BLUEZ_PROFILE_MGMT_INTERFACE,
                   "RegisterProfile")
      .WithObjectPath("/fake")
      .SendReply();

  cras_bt_add_profile(conn_, &fake_profile);
  cras_bt_register_profiles(conn_);

  WaitForMatches();
  profile = cras_bt_profile_get("/fake");

  EXPECT_TRUE(profile == &fake_profile);
}

TEST_F(BtProfileTestSuite, ResetProfile) {
  cras_bt_add_profile(conn_, &fake_profile);
  cras_bt_profile_reset();

  ASSERT_EQ(1, profile_release_called);
}

TEST_F(BtProfileTestSuite, HandleMessage) {
  ExpectMethodCall(PROFILE_MANAGER_OBJ_PATH, BLUEZ_PROFILE_MGMT_INTERFACE,
		   "RegisterProfile")
      .WithObjectPath("/fake")
      .SendReply();

  cras_bt_add_profile(conn_, &fake_profile);
  cras_bt_register_profiles(conn_);

  WaitForMatches();

  /* Use stdin as mock fd */
  CreateMessageCall("/fake", "org.bluez.Profile1", "NewConnection")
      .WithString("device")
      .WithUnixFd(0)
      .Send();

  WaitForMatches();
  ASSERT_EQ(1, profile_new_connection_called);
  ASSERT_STREQ("device", cras_bt_transport_get_arg_value);
  ASSERT_EQ(1, cras_bt_transport_get_called);
  ASSERT_EQ(fake_transport, profile_new_connection_arg_value);

  CreateMessageCall("/fake", "org.bluez.Profile1", "RequestDisconnection")
      .WithString("device")
      .Send();
  WaitForMatches();
  ASSERT_EQ(2, cras_bt_transport_get_called);
  ASSERT_EQ(1, profile_request_disconnection_called);
  ASSERT_EQ(fake_transport, profile_request_disconnection_arg_value);

  CreateMessageCall("/fake", "org.bluez.Profile1", "Release")
      .Send();
  WaitForMatches();
  ASSERT_EQ(1, profile_release_called);
  ASSERT_EQ(&fake_profile, profile_release_arg_value);

  CreateMessageCall("/fake", "org.bluez.Profile1", "Cancel")
      .Send();
  WaitForMatches();
  ASSERT_EQ(1, profile_cancel_called);
  ASSERT_EQ(&fake_profile, profile_cancel_arg_value);
}

void fake_profile_release(struct cras_bt_profile *profile)
{
  profile_release_arg_value = profile;
  profile_release_called++;
}

void fake_profile_new_connection(struct cras_bt_profile *profile,
				 struct cras_bt_transport *transport)
{
  profile_new_connection_arg_value = transport;
  profile_new_connection_called++;
}

void fake_profile_request_disconnection(struct cras_bt_profile *profile,
					struct cras_bt_transport *transport)
{
  profile_request_disconnection_arg_value = transport;
  profile_request_disconnection_called++;
}

void fake_profile_cancel(struct cras_bt_profile *profile)
{
  profile_cancel_arg_value = profile;
  profile_cancel_called++;
}

} // namespace

extern "C" {
dbus_bool_t append_key_value(DBusMessageIter *iter, const char *key,
                             int type, const char *type_string,
                             void *value)
{
  return TRUE;
}

struct cras_bt_transport *cras_bt_transport_get(const char *object_path)
{
  cras_bt_transport_get_called++;
  cras_bt_transport_get_arg_value = object_path;
  return fake_transport;
}

void cras_bt_transport_destroy(struct cras_bt_transport *transport)
{
}

struct cras_bt_transport *cras_bt_transport_create(DBusConnection *conn,
						   const char *object_path)
{
  return fake_transport;
}

} // extern "C"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
