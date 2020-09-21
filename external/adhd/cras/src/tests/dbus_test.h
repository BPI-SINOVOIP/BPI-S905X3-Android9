/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_DBUS_TEST_H_
#define CRAS_DBUS_TEST_H_

#include <string>
#include <vector>

#include <stdint.h>
#include <dbus/dbus.h>
#include <gtest/gtest.h>
#include <pthread.h>

/* DBusTest, and the related DBusMatch class, are used to provide a
 * GMock-like experience for testing D-Bus code within cras.
 *
 * It works by providing a connection to a private D-Bus Server for use
 * by code you intend to test. Before making calls, you set expectations
 * of method calls that the server should receive and reply to, or
 * instructions for the server to send signals that your connection
 * should receive and handle.
 *
 * The code style is similar to GMock for purposes of familiarity.
 *
 * Examples
 * --------
 *
 * To create a test suite class implementing a SetUp and TearDown method,
 * be sure to call the base-class methods at the appropriate time.
 *
 *   class ExampleTestSuite : public DBusTest {
 *     virtual void SetUp() {
 *       DBusTest::SetUp();
 *       // your setup code here
 *     }
 *
 *     virtual void TearDown() {
 *       // your teardown code here
 *       DBusTest::TearDown();
 *     }
 *   };
 *
 * To expect a method call to be made against the server; matching the
 * object path, interface and method name and then generating an empty
 * reply. The test code ensures that the reply is received during the
 * TearDown method.
 *
 *   TEST_F(ExampleTestSuite, ExampleTest) {
 *     ExpectMethodCall("/object/path", "object.Interface", "MethodName")
 *         .SendReply();
 *
 *     // code to generate the method call here
 *   }
 *
 * Due to the asynchronous nature of D-Bus, if you need to check some
 * state, it's not enough to immediately follow the code that generates
 * the method call. You must instead ensure that all expectations up to
 * that point have been met:
 *
 *   TEST_F(ExampleTestSuite, ExampleTest) {
 *     ExpectMethodCall("/object/path", "object.Interface", "MethodName")
 *         .SendReply();
 *
 *     // code to generate the method call here
 *
 *     WaitForMatches();
 *
 *     // code to examine state here
 *   }
 *
 * To verify the arguments to method calls, place .With*() calls before
 * sending the reply:
 *
 *   ExpectMethodCall("/object/path", "object.Interface", "MethodName")
 *       .WithObjectPath("/arg0/object/path")
 *       .WithString("arg1")
 *       .WithString("arg2")
 *       .SendReply();
 *
 * Normally additional arguments are permitted, since most D-Bus services
 * don't go out of their way to check they aren't provided; to verify
 * there are no more arguments use .WithNoMoreArgs():
 *
 *   ExpectMethodCall("/object/path", "object.Interface", "MethodName")
 *       .WithString("arg0")
 *       .WithNoMoreArgs()
 *       .SendReply();
 *
 * To append arguments to the reply, place .With*() calls after the
 * instruction to send the reply:
 *
 *   ExpectMethodCall("/object/path", "object.Interface", "MethodName")
 *       .SendReply()
 *       .WithString("arg0")
 *       .WithObjectPath("/arg1/object/path");
 *
 * Property dictionaries are sufficiently difficult to deal with that
 * there is special handling for them; to append one to the reply use
 * .AsPropertyDictionary() and follow with alternate .WithString() and
 * other .With*() calls for each property:
 *
 *  ExpectMethodCall("/object/path", "object.Interface", "GetProperties")
 *       .SendReply()
 *       .AsPropertyDictionary()
 *       .WithString("Keyword")
 *       .WithObjectPath("/value/of/keyword");
 *
 * To reply with an error use .SendError() instead of .SendReply(),
 * passing the error name and message
 *
 *   ExpectMethodCall("/object/path", "object.Interface", "MethodName")
 *       .SendError("some.error.Name", "Message for error");
 *
 * In some cases (notably "AddMatch" method calls) the method call will
 * be handled by libdbus itself and the mechanism DBusTest uses to verify
 * that the reply is recieved does not work. In which case you need to use
 * .SendReplyNoWait() instead.
 *
 *    ExpectMethodCall("", DBUS_INTERFACE_DBUS, "AddMatch")
 *        .SendReplyNoWait();
 *
 * Sending signals from the server side is very similar:
 *
 *    CreateSignal("/object/path", "object.Interface", "SignalName")
 *        .WithString("arg0")
 *        .WithObjectPat("/arg1/object/path")
 *        .Send();
 *
 * Create messages from server side:
 *    CreateMessageCall("/object/path". "object.Interface", "MethodName")
 *        .WithString("arg0")
 *        .WithUnixFd(arg1)
 *        .Send();
 *
 * The TearDown() method will verify that it is received by the client,
 * use WaitForMatches() to force verification earlier in order to check
 * state.
 */

class DBusTest;

class DBusMatch {
 public:
  DBusMatch();

  struct Arg {
    int type;
    bool array;
    std::string string_value;
    int int_value;
    std::vector<std::string> string_values;
  };

  // Append arguments to a match.
  DBusMatch& WithString(std::string value);
  DBusMatch& WithUnixFd(int value);
  DBusMatch& WithObjectPath(std::string value);
  DBusMatch& WithArrayOfStrings(std::vector<std::string> values);
  DBusMatch& WithArrayOfObjectPaths(std::vector<std::string> values);
  DBusMatch& WithNoMoreArgs();

  // Indicates that all arguments in either the method call or reply
  // should be wrapped into a property dictionary with a string for keys
  // and a variant for the data.
  DBusMatch& AsPropertyDictionary();

  // Send a reply to a method call and wait for it to be received by the
  // client; may be followed by methods to append arguments.
  DBusMatch& SendReply();

  // Send an error in reply to a method call and wait for it to be received
  // by the client; may also be followed by methods to append arguments.
  DBusMatch& SendError(std::string error_name, std::string error_message);

  // Send a reply to a method call but do not wait for it to be received;
  // mostly needed for internal D-Bus messages.
  DBusMatch& SendReplyNoWait();

  // Send a created signal.
  DBusMatch& Send();

 private:
  friend class DBusTest;

  // Methods used by DBusTest after constructing the DBusMatch instance
  // to set the type of match.
  void ExpectMethodCall(std::string path, std::string interface,
                        std::string method);

  void CreateSignal(DBusConnection *conn,
                    std::string path, std::string interface,
                    std::string signal_name);

  void CreateMessageCall(DBusConnection *conn,
                         std::string path, std::string interface,
                         std::string signal_name);

  // Determine whether a message matches a set of arguments.
  bool MatchMessageArgs(DBusMessage *message, std::vector<Arg> *args);

  // Append a set of arguments to a message.
  void AppendArgsToMessage(DBusMessage *message, std::vector<Arg> *args);

  // Send a message on a connection.
  void SendMessage(DBusConnection *conn, DBusMessage *message);

  // Handle a message received by the server connection.
  bool HandleServerMessage(DBusConnection *conn, DBusMessage *message);

  // Handle a message received by the client connection.
  bool HandleClientMessage(DBusConnection *conn, DBusMessage *message);

  // Verify whether the match is complete.
  bool Complete();

  int message_type_;
  std::string path_;
  std::string interface_;
  std::string member_;

  bool as_property_dictionary_;
  std::vector<Arg> args_;

  DBusConnection *conn_;

  bool send_reply_;
  std::vector<Arg> reply_args_;

  bool send_error_;
  std::string error_name_;
  std::string error_message_;

  bool expect_serial_;
  std::vector<dbus_uint32_t> expected_serials_;

  bool matched_;
};

class DBusTest : public ::testing::Test {
 public:
  DBusTest();
  virtual ~DBusTest();

 protected:
  // Connection to the D-Bus server, this may be used during tests as the
  // "bus" connection, all messages go to and from the internal D-Bus server.
  DBusConnection *conn_;

  // Expect a method call to be received by the server.
  DBusMatch& ExpectMethodCall(std::string path, std::string interface,
                              std::string method);

  // Send a signal from the client to the server.
  DBusMatch& CreateSignal(std::string path, std::string interface,
                          std::string signal_name);

  // Send a message from the client to the server.
  DBusMatch& CreateMessageCall(std::string path, std::string interface,
                               std::string signal_name);

  // Wait for all matches created by Expect*() or Create*() methods to
  // be complete.
  void WaitForMatches();

  // When overriding be sure to call these parent methods to allow the
  // D-Bus server thread to be cleanly initialized and shut down.
  virtual void SetUp();
  virtual void TearDown();

 private:
  DBusServer *server_;
  DBusConnection *server_conn_;

  std::vector<DBusWatch *> watches_;
  std::vector<DBusTimeout *> timeouts_;

  pthread_t thread_id_;
  pthread_mutex_t mutex_;
  bool dispatch_;

  std::vector<DBusMatch> matches_;

  static void NewConnectionThunk(DBusServer *server, DBusConnection *conn,
                                 void *data);
  void NewConnection(DBusServer *server, DBusConnection *conn);

  static dbus_bool_t AddWatchThunk(DBusWatch *watch, void *data);
  dbus_bool_t AddWatch(DBusWatch *watch);

  static void RemoveWatchThunk(DBusWatch *watch, void *data);
  void RemoveWatch(DBusWatch *watch);

  static void WatchToggledThunk(DBusWatch *watch, void *data);
  void WatchToggled(DBusWatch *watch);

  static dbus_bool_t AddTimeoutThunk(DBusTimeout *timeout, void *data);
  dbus_bool_t AddTimeout(DBusTimeout *timeout);

  static void RemoveTimeoutThunk(DBusTimeout *timeout, void *data);
  void RemoveTimeout(DBusTimeout *timeout);

  static void TimeoutToggledThunk(DBusTimeout *timeout, void *data);
  void TimeoutToggled(DBusTimeout *timeout);

  static DBusHandlerResult HandleMessageThunk(DBusConnection *conn,
                                              DBusMessage *message, void *data);
  DBusHandlerResult HandleMessage(DBusConnection *conn, DBusMessage *message);

  static void *DispatchLoopThunk(void *ptr);
  void *DispatchLoop();
  void DispatchOnce();
};


#endif /* CRAS_DBUS_TEST_H_ */
