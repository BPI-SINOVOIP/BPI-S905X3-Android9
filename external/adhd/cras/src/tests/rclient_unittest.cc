// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <gtest/gtest.h>
#include <unistd.h>

extern "C" {
#include "audio_thread.h"
#include "cras_messages.h"
#include "cras_rclient.h"
#include "cras_rstream.h"
#include "cras_system_state.h"

// Access to data structures and static functions.
#include "cras_rclient.c"
}

//  Stub data.
static int cras_rstream_create_return;
static struct cras_rstream *cras_rstream_create_stream_out;
static int cras_iodev_attach_stream_retval;
static size_t cras_system_set_volume_value;
static int cras_system_set_volume_called;
static size_t cras_system_set_capture_gain_value;
static int cras_system_set_capture_gain_called;
static size_t cras_system_set_mute_value;
static int cras_system_set_mute_called;
static size_t cras_system_set_user_mute_value;
static int cras_system_set_user_mute_called;
static size_t cras_system_set_mute_locked_value;
static int cras_system_set_mute_locked_called;
static size_t cras_system_set_capture_mute_value;
static int cras_system_set_capture_mute_called;
static size_t cras_system_set_capture_mute_locked_value;
static int cras_system_set_capture_mute_locked_called;
static size_t cras_make_fd_nonblocking_called;
static audio_thread* iodev_get_thread_return;
static int stream_list_add_stream_return;
static unsigned int stream_list_add_stream_called;
static unsigned int stream_list_disconnect_stream_called;
static unsigned int cras_iodev_list_rm_input_called;
static unsigned int cras_iodev_list_rm_output_called;
static struct cras_rstream dummy_rstream;
static size_t cras_observer_num_ops_registered;
static size_t cras_observer_register_notify_called;
static size_t cras_observer_add_called;
static void *cras_observer_add_context_value;
static struct cras_observer_client *cras_observer_add_return_value;
static size_t cras_observer_get_ops_called;
static struct cras_observer_ops cras_observer_ops_value;
static size_t cras_observer_set_ops_called;
static size_t cras_observer_ops_are_empty_called;
static struct cras_observer_ops cras_observer_ops_are_empty_empty_ops;
static size_t cras_observer_remove_called;

void ResetStubData() {
  cras_rstream_create_return = 0;
  cras_rstream_create_stream_out = (struct cras_rstream *)NULL;
  cras_iodev_attach_stream_retval = 0;
  cras_system_set_volume_value = 0;
  cras_system_set_volume_called = 0;
  cras_system_set_capture_gain_value = 0;
  cras_system_set_capture_gain_called = 0;
  cras_system_set_mute_value = 0;
  cras_system_set_mute_called = 0;
  cras_system_set_user_mute_value = 0;
  cras_system_set_user_mute_called = 0;
  cras_system_set_mute_locked_value = 0;
  cras_system_set_mute_locked_called = 0;
  cras_system_set_capture_mute_value = 0;
  cras_system_set_capture_mute_called = 0;
  cras_system_set_capture_mute_locked_value = 0;
  cras_system_set_capture_mute_locked_called = 0;
  cras_make_fd_nonblocking_called = 0;
  iodev_get_thread_return = reinterpret_cast<audio_thread*>(0xad);
  stream_list_add_stream_return = 0;
  stream_list_add_stream_called = 0;
  stream_list_disconnect_stream_called = 0;
  cras_iodev_list_rm_output_called = 0;
  cras_iodev_list_rm_input_called = 0;
  cras_observer_num_ops_registered = 0;
  cras_observer_register_notify_called = 0;
  cras_observer_add_called = 0;
  cras_observer_add_return_value =
      reinterpret_cast<struct cras_observer_client *>(1);
  cras_observer_add_context_value = NULL;
  cras_observer_get_ops_called = 0;
  memset(&cras_observer_ops_value, 0, sizeof(cras_observer_ops_value));
  cras_observer_set_ops_called = 0;
  cras_observer_ops_are_empty_called = 0;
  memset(&cras_observer_ops_are_empty_empty_ops, 0,
         sizeof(cras_observer_ops_are_empty_empty_ops));
  cras_observer_remove_called = 0;
}

namespace {

TEST(RClientSuite, CreateSendMessage) {
  struct cras_rclient *rclient;
  int rc;
  struct cras_client_connected msg;
  int pipe_fds[2];

  ResetStubData();

  rc = pipe(pipe_fds);
  ASSERT_EQ(0, rc);

  rclient = cras_rclient_create(pipe_fds[1], 800);
  ASSERT_NE((void *)NULL, rclient);

  rc = read(pipe_fds[0], &msg, sizeof(msg));
  EXPECT_EQ(sizeof(msg), rc);
  EXPECT_EQ(CRAS_CLIENT_CONNECTED, msg.header.id);

  cras_rclient_destroy(rclient);
  close(pipe_fds[0]);
  close(pipe_fds[1]);
}

class RClientMessagesSuite : public testing::Test {
  protected:
    virtual void SetUp() {
      int rc;
      struct cras_client_connected msg;

      rc = pipe(pipe_fds_);
      if (rc < 0)
        return;
      rclient_ = cras_rclient_create(pipe_fds_[1], 800);
      rc = read(pipe_fds_[0], &msg, sizeof(msg));
      if (rc < 0)
        return;

      rstream_ = (struct cras_rstream *)calloc(1, sizeof(*rstream_));

      stream_id_ = 0x10002;
      connect_msg_.header.id = CRAS_SERVER_CONNECT_STREAM;
      connect_msg_.header.length = sizeof(connect_msg_);
      connect_msg_.stream_type = CRAS_STREAM_TYPE_DEFAULT;
      connect_msg_.direction = CRAS_STREAM_OUTPUT;
      connect_msg_.stream_id = stream_id_;
      connect_msg_.buffer_frames = 480;
      connect_msg_.cb_threshold = 240;
      connect_msg_.flags = 0;
      connect_msg_.format.num_channels = 2;
      connect_msg_.format.frame_rate = 48000;
      connect_msg_.format.format = SND_PCM_FORMAT_S16_LE;
      connect_msg_.dev_idx = NO_DEVICE;

      ResetStubData();
    }

    virtual void TearDown() {
      cras_rclient_destroy(rclient_);
      free(rstream_);
      close(pipe_fds_[0]);
      close(pipe_fds_[1]);
    }

    void RegisterNotification(enum CRAS_CLIENT_MESSAGE_ID msg_id,
                              void *callback, void **ops_address);

    struct cras_connect_message connect_msg_;
    struct cras_rclient *rclient_;
    struct cras_rstream *rstream_;
    size_t stream_id_;
    int pipe_fds_[2];
};

TEST_F(RClientMessagesSuite, AudThreadAttachFail) {
  struct cras_client_stream_connected out_msg;
  int rc;

  cras_rstream_create_stream_out = rstream_;
  stream_list_add_stream_return = -EINVAL;

  rc = cras_rclient_message_from_client(rclient_, &connect_msg_.header, 100);
  EXPECT_EQ(0, rc);

  rc = read(pipe_fds_[0], &out_msg, sizeof(out_msg));
  EXPECT_EQ(sizeof(out_msg), rc);
  EXPECT_EQ(stream_id_, out_msg.stream_id);
  EXPECT_NE(0, out_msg.err);
  EXPECT_EQ(0, cras_iodev_list_rm_output_called);
  EXPECT_EQ(1, stream_list_add_stream_called);
  EXPECT_EQ(0, stream_list_disconnect_stream_called);
}

TEST_F(RClientMessagesSuite, ConnectMsgWithBadFd) {
  struct cras_client_stream_connected out_msg;
  int rc;

  rc = cras_rclient_message_from_client(rclient_, &connect_msg_.header, -1);
  EXPECT_EQ(0, rc);

  rc = read(pipe_fds_[0], &out_msg, sizeof(out_msg));
  EXPECT_EQ(sizeof(out_msg), rc);
  EXPECT_EQ(stream_id_, out_msg.stream_id);
  EXPECT_NE(0, out_msg.err);
  EXPECT_EQ(stream_list_add_stream_called,
            stream_list_disconnect_stream_called);
}

TEST_F(RClientMessagesSuite, SuccessReply) {
  struct cras_client_stream_connected out_msg;
  int rc;

  cras_rstream_create_stream_out = rstream_;
  cras_iodev_attach_stream_retval = 0;

  rc = cras_rclient_message_from_client(rclient_, &connect_msg_.header, 100);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_make_fd_nonblocking_called);

  rc = read(pipe_fds_[0], &out_msg, sizeof(out_msg));
  EXPECT_EQ(sizeof(out_msg), rc);
  EXPECT_EQ(stream_id_, out_msg.stream_id);
  EXPECT_EQ(0, out_msg.err);
  EXPECT_EQ(1, stream_list_add_stream_called);
  EXPECT_EQ(0, stream_list_disconnect_stream_called);
}

TEST_F(RClientMessagesSuite, SuccessCreateThreadReply) {
  struct cras_client_stream_connected out_msg;
  int rc;

  cras_rstream_create_stream_out = rstream_;
  cras_iodev_attach_stream_retval = 0;

  rc = cras_rclient_message_from_client(rclient_, &connect_msg_.header, 100);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_make_fd_nonblocking_called);

  rc = read(pipe_fds_[0], &out_msg, sizeof(out_msg));
  EXPECT_EQ(sizeof(out_msg), rc);
  EXPECT_EQ(stream_id_, out_msg.stream_id);
  EXPECT_EQ(0, out_msg.err);
  EXPECT_EQ(1, stream_list_add_stream_called);
  EXPECT_EQ(0, stream_list_disconnect_stream_called);
}

TEST_F(RClientMessagesSuite, SetVolume) {
  struct cras_set_system_volume msg;
  int rc;

  msg.header.id = CRAS_SERVER_SET_SYSTEM_VOLUME;
  msg.header.length = sizeof(msg);
  msg.volume = 66;

  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_system_set_volume_called);
  EXPECT_EQ(66, cras_system_set_volume_value);
}

TEST_F(RClientMessagesSuite, SetCaptureVolume) {
  struct cras_set_system_volume msg;
  int rc;

  msg.header.id = CRAS_SERVER_SET_SYSTEM_CAPTURE_GAIN;
  msg.header.length = sizeof(msg);
  msg.volume = 66;

  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_system_set_capture_gain_called);
  EXPECT_EQ(66, cras_system_set_capture_gain_value);
}

TEST_F(RClientMessagesSuite, SetMute) {
  struct cras_set_system_mute msg;
  int rc;

  msg.header.id = CRAS_SERVER_SET_SYSTEM_MUTE;
  msg.header.length = sizeof(msg);
  msg.mute = 1;

  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_system_set_mute_called);
  EXPECT_EQ(1, cras_system_set_mute_value);

  msg.header.id = CRAS_SERVER_SET_SYSTEM_MUTE_LOCKED;
  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_system_set_mute_locked_called);
  EXPECT_EQ(1, cras_system_set_mute_locked_value);
}

TEST_F(RClientMessagesSuite, SetUserMute) {
  struct cras_set_system_mute msg;
  int rc;

  msg.header.id = CRAS_SERVER_SET_USER_MUTE;
  msg.header.length = sizeof(msg);
  msg.mute = 1;

  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_system_set_user_mute_called);
  EXPECT_EQ(1, cras_system_set_user_mute_value);
}

TEST_F(RClientMessagesSuite, SetCaptureMute) {
  struct cras_set_system_mute msg;
  int rc;

  msg.header.id = CRAS_SERVER_SET_SYSTEM_CAPTURE_MUTE;
  msg.header.length = sizeof(msg);
  msg.mute = 1;

  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_system_set_capture_mute_called);
  EXPECT_EQ(1, cras_system_set_capture_mute_value);

  msg.header.id = CRAS_SERVER_SET_SYSTEM_CAPTURE_MUTE_LOCKED;
  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(1, cras_system_set_capture_mute_locked_called);
  EXPECT_EQ(1, cras_system_set_capture_mute_locked_value);
}

void RClientMessagesSuite::RegisterNotification(
    enum CRAS_CLIENT_MESSAGE_ID msg_id,
    void *callback, void **ops_address) {
  struct cras_register_notification msg;
  int do_register = callback != NULL ? 1 : 0;
  int rc;

  cras_observer_register_notify_called++;

  cras_fill_register_notification_message(&msg, msg_id, do_register);
  EXPECT_EQ(msg.header.length, sizeof(msg));
  EXPECT_EQ(msg.header.id, CRAS_SERVER_REGISTER_NOTIFICATION);
  EXPECT_EQ(msg.do_register, do_register);
  EXPECT_EQ(msg.msg_id, msg_id);

  rc = cras_rclient_message_from_client(rclient_, &msg.header, -1);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(cras_observer_register_notify_called, cras_observer_get_ops_called);
  EXPECT_EQ(cras_observer_register_notify_called,\
            cras_observer_ops_are_empty_called);
  if (msg.do_register)
    cras_observer_num_ops_registered++;
  if (cras_observer_num_ops_registered == 1) {
    if (msg.do_register) {
      EXPECT_EQ(1, cras_observer_add_called);
      EXPECT_EQ(rclient_, cras_observer_add_context_value);
      EXPECT_EQ(rclient_->observer, cras_observer_add_return_value);
    } else {
      EXPECT_EQ(1, cras_observer_remove_called);
      EXPECT_EQ(rclient_->observer, (struct cras_observer_client *)NULL);
    }
  } else {
    EXPECT_EQ(cras_observer_register_notify_called - 1,\
              cras_observer_set_ops_called);
  }
  if (!msg.do_register)
    cras_observer_num_ops_registered--;
  if (cras_observer_num_ops_registered)
    EXPECT_EQ(callback, *ops_address);
}

TEST_F(RClientMessagesSuite, RegisterStatusNotification) {
  /* First registration for this client. */
  RegisterNotification(
      CRAS_CLIENT_OUTPUT_VOLUME_CHANGED,
      (void *)send_output_volume_changed,
      (void **)&cras_observer_ops_value.output_volume_changed);

  /* Second registration for this client. */
  RegisterNotification(
      CRAS_CLIENT_CAPTURE_GAIN_CHANGED,
      (void *)send_capture_gain_changed,
      (void **)&cras_observer_ops_value.capture_gain_changed);

  /* Deregister output_volume. */
  RegisterNotification(
      CRAS_CLIENT_OUTPUT_VOLUME_CHANGED, NULL,
      (void **)&cras_observer_ops_value.output_volume_changed);

  /* Register/deregister all msg_ids. */

  RegisterNotification(
      CRAS_CLIENT_OUTPUT_MUTE_CHANGED,
      (void *)send_output_mute_changed,
      (void **)&cras_observer_ops_value.output_mute_changed);
  RegisterNotification(
      CRAS_CLIENT_OUTPUT_MUTE_CHANGED, NULL,
      (void **)&cras_observer_ops_value.output_mute_changed);

  RegisterNotification(
      CRAS_CLIENT_CAPTURE_MUTE_CHANGED,
      (void *)send_capture_mute_changed,
      (void **)&cras_observer_ops_value.capture_mute_changed);
  RegisterNotification(
      CRAS_CLIENT_CAPTURE_MUTE_CHANGED, NULL,
      (void **)&cras_observer_ops_value.capture_mute_changed);

  RegisterNotification(
      CRAS_CLIENT_NODES_CHANGED,
      (void *)send_nodes_changed,
      (void **)&cras_observer_ops_value.nodes_changed);
  RegisterNotification(
      CRAS_CLIENT_NODES_CHANGED, NULL,
      (void **)&cras_observer_ops_value.nodes_changed);

  RegisterNotification(
      CRAS_CLIENT_ACTIVE_NODE_CHANGED,
      (void *)send_active_node_changed,
      (void **)&cras_observer_ops_value.active_node_changed);
  RegisterNotification(
      CRAS_CLIENT_ACTIVE_NODE_CHANGED, NULL,
      (void **)&cras_observer_ops_value.active_node_changed);

  RegisterNotification(
      CRAS_CLIENT_OUTPUT_NODE_VOLUME_CHANGED,
      (void *)send_output_node_volume_changed,
      (void **)&cras_observer_ops_value.output_node_volume_changed);
  RegisterNotification(
      CRAS_CLIENT_OUTPUT_NODE_VOLUME_CHANGED, NULL,
      (void **)&cras_observer_ops_value.output_node_volume_changed);

  RegisterNotification(
      CRAS_CLIENT_NODE_LEFT_RIGHT_SWAPPED_CHANGED,
      (void *)send_node_left_right_swapped_changed,
      (void **)&cras_observer_ops_value.node_left_right_swapped_changed);
  RegisterNotification(
      CRAS_CLIENT_NODE_LEFT_RIGHT_SWAPPED_CHANGED, NULL,
      (void **)&cras_observer_ops_value.node_left_right_swapped_changed);

  RegisterNotification(
      CRAS_CLIENT_INPUT_NODE_GAIN_CHANGED,
      (void *)send_input_node_gain_changed,
      (void **)&cras_observer_ops_value.input_node_gain_changed);
  RegisterNotification(
      CRAS_CLIENT_INPUT_NODE_GAIN_CHANGED, NULL,
      (void **)&cras_observer_ops_value.input_node_gain_changed);

  RegisterNotification(
      CRAS_CLIENT_NUM_ACTIVE_STREAMS_CHANGED,
      (void *)send_num_active_streams_changed,
      (void **)&cras_observer_ops_value.num_active_streams_changed);
  RegisterNotification(
      CRAS_CLIENT_NUM_ACTIVE_STREAMS_CHANGED, NULL,
      (void **)&cras_observer_ops_value.num_active_streams_changed);

  /* Deregister last. */
  RegisterNotification(
      CRAS_CLIENT_CAPTURE_GAIN_CHANGED, NULL,
      (void **)&cras_observer_ops_value.capture_gain_changed);
}

TEST_F(RClientMessagesSuite, SendOutputVolumeChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_volume_changed *msg =
      (struct cras_client_volume_changed *)buf;
  const int32_t volume = 90;

  send_output_volume_changed(void_client, volume);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_OUTPUT_VOLUME_CHANGED);
  EXPECT_EQ(msg->volume, volume);
}

TEST_F(RClientMessagesSuite, SendOutputMuteChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_mute_changed *msg =
      (struct cras_client_mute_changed *)buf;
  const int muted = 1;
  const int user_muted = 0;
  const int mute_locked = 1;

  send_output_mute_changed(void_client, muted, user_muted, mute_locked);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_OUTPUT_MUTE_CHANGED);
  EXPECT_EQ(msg->muted, muted);
  EXPECT_EQ(msg->user_muted, user_muted);
  EXPECT_EQ(msg->mute_locked, mute_locked);
}

TEST_F(RClientMessagesSuite, SendCaptureGainChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_volume_changed *msg =
      (struct cras_client_volume_changed *)buf;
  const int32_t gain = 90;

  send_capture_gain_changed(void_client, gain);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_CAPTURE_GAIN_CHANGED);
  EXPECT_EQ(msg->volume, gain);
}

TEST_F(RClientMessagesSuite, SendCaptureMuteChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_mute_changed *msg =
      (struct cras_client_mute_changed *)buf;
  const int muted = 1;
  const int mute_locked = 0;

  send_capture_mute_changed(void_client, muted, mute_locked);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_CAPTURE_MUTE_CHANGED);
  EXPECT_EQ(msg->muted, muted);
  EXPECT_EQ(msg->mute_locked, mute_locked);
}

TEST_F(RClientMessagesSuite, SendNodesChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_nodes_changed *msg =
      (struct cras_client_nodes_changed *)buf;

  send_nodes_changed(void_client);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_NODES_CHANGED);
}

TEST_F(RClientMessagesSuite, SendActiveNodeChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_active_node_changed *msg =
      (struct cras_client_active_node_changed *)buf;
  const enum CRAS_STREAM_DIRECTION dir = CRAS_STREAM_INPUT;
  const cras_node_id_t node_id = 0x0001000200030004;

  send_active_node_changed(void_client, dir, node_id);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_ACTIVE_NODE_CHANGED);
  EXPECT_EQ(msg->direction, (int32_t)dir);
  EXPECT_EQ(msg->node_id, node_id);
}

TEST_F(RClientMessagesSuite, SendOutputNodeVolumeChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_node_value_changed *msg =
      (struct cras_client_node_value_changed *)buf;
  const cras_node_id_t node_id = 0x0001000200030004;
  const int32_t value = 90;

  send_output_node_volume_changed(void_client, node_id, value);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_OUTPUT_NODE_VOLUME_CHANGED);
  EXPECT_EQ(msg->node_id, node_id);
  EXPECT_EQ(msg->value, value);
}

TEST_F(RClientMessagesSuite, SendNodeLeftRightSwappedChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_node_value_changed *msg =
      (struct cras_client_node_value_changed *)buf;
  const cras_node_id_t node_id = 0x0001000200030004;
  const int32_t value = 0;

  send_node_left_right_swapped_changed(void_client, node_id, value);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_NODE_LEFT_RIGHT_SWAPPED_CHANGED);
  EXPECT_EQ(msg->node_id, node_id);
  EXPECT_EQ(msg->value, value);
}

TEST_F(RClientMessagesSuite, SendNodeInputNodeGainChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_node_value_changed *msg =
      (struct cras_client_node_value_changed *)buf;
  const cras_node_id_t node_id = 0x0001000200030004;
  const int32_t value = -19;

  send_input_node_gain_changed(void_client, node_id, value);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_INPUT_NODE_GAIN_CHANGED);
  EXPECT_EQ(msg->node_id, node_id);
  EXPECT_EQ(msg->value, value);
}

TEST_F(RClientMessagesSuite, SendNumActiveStreamsChanged) {
  void *void_client = reinterpret_cast<void *>(rclient_);
  char buf[1024];
  ssize_t rc;
  struct cras_client_num_active_streams_changed *msg =
      (struct cras_client_num_active_streams_changed *)buf;
  const enum CRAS_STREAM_DIRECTION dir = CRAS_STREAM_INPUT;
  const uint32_t num_active_streams = 3;

  send_num_active_streams_changed(void_client, dir, num_active_streams);
  rc = read(pipe_fds_[0], buf, sizeof(buf));
  ASSERT_EQ(rc, (ssize_t)sizeof(*msg));
  EXPECT_EQ(msg->header.id, CRAS_CLIENT_NUM_ACTIVE_STREAMS_CHANGED);
  EXPECT_EQ(msg->direction, (int32_t)dir);
  EXPECT_EQ(msg->num_active_streams, num_active_streams);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

/* stubs */
extern "C" {

struct audio_thread* cras_iodev_list_get_audio_thread() {
  return iodev_get_thread_return;
}

void cras_iodev_list_add_active_node(enum CRAS_STREAM_DIRECTION dir,
                                     cras_node_id_t node_id)
{
}

void cras_iodev_list_rm_active_node(enum CRAS_STREAM_DIRECTION dir,
                                    cras_node_id_t node_id)
{
}

int audio_thread_rm_stream(audio_thread* thread,
			   cras_rstream* stream) {
  return 0;
}

void audio_thread_add_output_dev(struct audio_thread *thread,
				 struct cras_iodev *odev)
{
}

int audio_thread_dump_thread_info(struct audio_thread *thread,
				  struct audio_debug_info *info)
{
  return 0;
}

int audio_thread_suspend(struct audio_thread *thread)
{
  return 0;
}

int audio_thread_resume(struct audio_thread *thread)
{
  return 0;
}

int audio_thread_config_global_remix(struct audio_thread *thread,
    unsigned int num_channels,
    const float *coefficient)
{
  return 0;
}

const char *cras_config_get_socket_file_dir()
{
  return CRAS_UT_TMPDIR;
}

int cras_rstream_create(struct cras_rstream_config *stream_config,
			struct cras_rstream **stream_out)
{
  *stream_out = cras_rstream_create_stream_out;
  return cras_rstream_create_return;
}

int cras_iodev_move_stream_type(uint32_t type, uint32_t index)
{
  return 0;
}

int cras_iodev_list_rm_output(struct cras_iodev *output) {
  cras_iodev_list_rm_output_called++;
  return 0;
}

int cras_iodev_list_rm_input(struct cras_iodev *input) {
  cras_iodev_list_rm_input_called++;
  return 0;
}

int cras_server_disconnect_from_client_socket(int socket_fd) {
  return 0;
}

int cras_make_fd_nonblocking(int fd)
{
  cras_make_fd_nonblocking_called++;
  return 0;
}

void cras_system_set_volume(size_t volume)
{
  cras_system_set_volume_value = volume;
  cras_system_set_volume_called++;
}

void cras_system_set_capture_gain(long gain)
{
  cras_system_set_capture_gain_value = gain;
  cras_system_set_capture_gain_called++;
}

//  From system_state.
void cras_system_set_mute(int mute)
{
  cras_system_set_mute_value = mute;
  cras_system_set_mute_called++;
}
void cras_system_set_user_mute(int mute)
{
  cras_system_set_user_mute_value = mute;
  cras_system_set_user_mute_called++;
}
void cras_system_set_mute_locked(int mute)
{
  cras_system_set_mute_locked_value = mute;
  cras_system_set_mute_locked_called++;
}
void cras_system_set_capture_mute(int mute)
{
  cras_system_set_capture_mute_value = mute;
  cras_system_set_capture_mute_called++;
}
void cras_system_set_capture_mute_locked(int mute)
{
  cras_system_set_capture_mute_locked_value = mute;
  cras_system_set_capture_mute_locked_called++;
}

int cras_system_remove_alsa_card(size_t alsa_card_index)
{
	return -1;
}

void cras_system_set_suspended(int suspended) {
}

struct cras_server_state *cras_system_state_get_no_lock()
{
  return NULL;
}

key_t cras_sys_state_shm_fd()
{
  return 1;
}

void cras_dsp_reload_ini()
{
}

void cras_dsp_dump_info()
{
}

int cras_iodev_list_set_node_attr(cras_node_id_t id,
				  enum ionode_attr attr, int value)
{
  return 0;
}

void cras_iodev_list_select_node(enum CRAS_STREAM_DIRECTION direction,
				 cras_node_id_t node_id)
{
}

void cras_iodev_list_add_test_dev(enum TEST_IODEV_TYPE type) {
}

struct stream_list *cras_iodev_list_get_stream_list()
{
  return NULL;
}

/* Handles sending a command to a test iodev. */
void cras_iodev_list_test_dev_command(unsigned int iodev_idx,
                                      enum CRAS_TEST_IODEV_CMD command,
                                      unsigned int data_len,
                                      const uint8_t *data) {
}

void cras_iodev_list_configure_global_remix_converter(
    unsigned int num_channels,
    const float *coefficient)
{
}

int stream_list_add(struct stream_list *list,
                    struct cras_rstream_config *config,
		    struct cras_rstream **stream)
{
  int ret;

  *stream = &dummy_rstream;

  stream_list_add_stream_called++;
  ret = stream_list_add_stream_return;
  if (ret)
    stream_list_add_stream_return = -EINVAL;

  dummy_rstream.direction = config->direction;
  dummy_rstream.stream_id = config->stream_id;

  return ret;
}

int stream_list_rm(struct stream_list *list, cras_stream_id_t id)
{
  stream_list_disconnect_stream_called++;
  return 0;
}

int stream_list_rm_all_client_streams(struct stream_list *list,
				      struct cras_rclient *rclient)
{
  return 0;
}

int cras_send_with_fds(int sockfd, const void *buf, size_t len, int *fd,
                       unsigned int num_fds)
{
  return write(sockfd, buf, len);
}

char *cras_iodev_list_get_hotword_models(cras_node_id_t node_id)
{
  return NULL;
}

int cras_iodev_list_set_hotword_model(cras_node_id_t id,
              const char *model_name)
{
  return 0;
}

struct cras_observer_client *cras_observer_add(
			const struct cras_observer_ops *ops,
			void *context)
{
  cras_observer_add_called++;
  cras_observer_add_context_value = context;
  memcpy(&cras_observer_ops_value, ops, sizeof(cras_observer_ops_value));
  return cras_observer_add_return_value;
}

void cras_observer_get_ops(const struct cras_observer_client *client,
			   struct cras_observer_ops *ops)
{
  cras_observer_get_ops_called++;
  memcpy(ops, &cras_observer_ops_value, sizeof(*ops));
}

void cras_observer_set_ops(struct cras_observer_client *client,
			   const struct cras_observer_ops *ops)
{
  cras_observer_set_ops_called++;
  memcpy(&cras_observer_ops_value, ops, sizeof(cras_observer_ops_value));
}

int cras_observer_ops_are_empty(const struct cras_observer_ops *ops)
{
  cras_observer_ops_are_empty_called++;
  return memcmp(&cras_observer_ops_are_empty_empty_ops, ops,
                sizeof(cras_observer_ops_are_empty_empty_ops)) == 0;
}

void cras_observer_remove(struct cras_observer_client *client)
{
  cras_observer_remove_called++;
}

}  // extern "C"
