// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>

#include "dbus_test.h"

namespace {

const char kServerAddress[] = "unix:abstract=/org/chromium/DBusTest";

}  // namespace

DBusMatch::DBusMatch()
    : message_type_(DBUS_MESSAGE_TYPE_INVALID),
      as_property_dictionary_(false),
      send_reply_(false),
      send_error_(false),
      expect_serial_(false),
      matched_(false) {

}

DBusMatch& DBusMatch::WithString(std::string value) {
  Arg arg;
  arg.type = DBUS_TYPE_STRING;
  arg.array = false;
  arg.string_value = value;

  if (send_reply_)
    reply_args_.push_back(arg);
  else
    args_.push_back(arg);
  return *this;
}

DBusMatch& DBusMatch::WithUnixFd(int value) {
  Arg arg;
  arg.type = DBUS_TYPE_UNIX_FD;
  arg.array = false;
  arg.int_value = value;

  if (send_reply_)
    reply_args_.push_back(arg);
  else
    args_.push_back(arg);
  return *this;
}

DBusMatch& DBusMatch::WithObjectPath(std::string value) {
  Arg arg;
  arg.type = DBUS_TYPE_OBJECT_PATH;
  arg.array = false;
  arg.string_value = value;

  if (send_reply_)
    reply_args_.push_back(arg);
  else
    args_.push_back(arg);
  return *this;
}

DBusMatch& DBusMatch::WithArrayOfStrings(std::vector<std::string> values) {
  Arg arg;
  arg.type = DBUS_TYPE_STRING;
  arg.array = true;
  arg.string_values = values;

  if (send_reply_)
    reply_args_.push_back(arg);
  else
    args_.push_back(arg);
  return *this;
}

DBusMatch& DBusMatch::WithArrayOfObjectPaths(std::vector<std::string> values) {
  Arg arg;
  arg.type = DBUS_TYPE_OBJECT_PATH;
  arg.array = true;
  arg.string_values = values;

  if (send_reply_)
    reply_args_.push_back(arg);
  else
    args_.push_back(arg);
  return *this;
}

DBusMatch& DBusMatch::WithNoMoreArgs() {
  Arg arg;
  arg.type = DBUS_TYPE_INVALID;

  args_.push_back(arg);
  return *this;
}

DBusMatch& DBusMatch::AsPropertyDictionary() {
  as_property_dictionary_ = true;
  return *this;
}


DBusMatch& DBusMatch::SendReply() {
  send_reply_ = true;
  expect_serial_ = true;
  return *this;
}

DBusMatch& DBusMatch::SendError(std::string error_name,
                                std::string error_message) {
  send_error_ = true;
  error_name_ = error_name;
  error_message_ = error_message;
  expect_serial_ = true;
  return *this;
}

DBusMatch& DBusMatch::SendReplyNoWait() {
  send_reply_ = true;
  expect_serial_ = false;
  return *this;
}


DBusMatch& DBusMatch::Send() {
  DBusMessage *message;
  if (message_type_ == DBUS_MESSAGE_TYPE_SIGNAL)
    message = dbus_message_new_signal(path_.c_str(),
                                      interface_.c_str(),
                                      member_.c_str());
  else if (message_type_ == DBUS_MESSAGE_TYPE_METHOD_CALL)
    message = dbus_message_new_method_call(NULL,
                                           path_.c_str(),
                                           interface_.c_str(),
                                           member_.c_str());
  else
    return *this;

  AppendArgsToMessage(message, &args_);
  SendMessage(conn_, message);

  dbus_message_unref(message);

  return *this;
}


void DBusMatch::ExpectMethodCall(std::string path,
                                 std::string interface,
                                 std::string method) {
  message_type_ = DBUS_MESSAGE_TYPE_METHOD_CALL;
  path_ = path;
  interface_ = interface;
  member_ = method;
}


void DBusMatch::CreateSignal(DBusConnection *conn,
                             std::string path,
                             std::string interface,
                             std::string signal_name) {
  message_type_ = DBUS_MESSAGE_TYPE_SIGNAL;
  path_ = path;
  interface_ = interface;
  member_ = signal_name;

  conn_ = conn;
  expect_serial_ = true;
  matched_ = true;
}

void DBusMatch::CreateMessageCall(DBusConnection *conn,
                                  std::string path,
                                  std::string interface,
                                  std::string method_name) {
  message_type_ = DBUS_MESSAGE_TYPE_METHOD_CALL;
  path_ = path;
  interface_ = interface;
  member_ = method_name;

  conn_ = conn;
  expect_serial_ = true;
  matched_ = true;
}


bool DBusMatch::MatchMessageArgs(DBusMessage *message,
                                 std::vector<Arg> *args)
{
  DBusMessageIter iter;
  dbus_message_iter_init(message, &iter);
  for (std::vector<Arg>::iterator it = args->begin(); it != args->end(); ++it) {
    Arg &arg = *it;

    int type = dbus_message_iter_get_arg_type(&iter);
    if (type != arg.type)
      return false;

    if (arg.type == DBUS_TYPE_STRING
      || arg.type == DBUS_TYPE_OBJECT_PATH) {
      const char *str_value;
      dbus_message_iter_get_basic(&iter, &str_value);
      if (strcmp(str_value, arg.string_value.c_str()) != 0)
        return false;
    }
    // TODO(keybuk): additional argument types

    dbus_message_iter_next(&iter);
  }

  return true;
}

void DBusMatch::AppendArgsToMessage(DBusMessage *message,
                                    std::vector<Arg> *args)
{
  DBusMessageIter message_iter;
  DBusMessageIter dict_array_iter;
  DBusMessageIter struct_iter;
  DBusMessageIter iter;

  if (as_property_dictionary_) {
    dbus_message_iter_init_append(message, &message_iter);
    dbus_message_iter_open_container(&message_iter,
                                     DBUS_TYPE_ARRAY, "{sv}",
                                     &dict_array_iter);
  } else {
    dbus_message_iter_init_append(message, &iter);
  }

  for (std::vector<Arg>::iterator it = args->begin(); it != args->end(); ++it) {
    Arg &arg = *it;

    if (as_property_dictionary_) {
      dbus_message_iter_open_container(&dict_array_iter,
                                       DBUS_TYPE_DICT_ENTRY, NULL,
                                       &struct_iter);

      const char *str_value = arg.string_value.c_str();
      dbus_message_iter_append_basic(&struct_iter, arg.type, &str_value);

      arg = *(++it);
    }

    const char *array_type, *element_type;
    switch (arg.type) {
      case DBUS_TYPE_STRING:
        array_type = DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING;
        element_type = DBUS_TYPE_STRING_AS_STRING;
        break;
      case DBUS_TYPE_OBJECT_PATH:
        array_type = DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_OBJECT_PATH_AS_STRING;
        element_type = DBUS_TYPE_OBJECT_PATH_AS_STRING;
        break;
      case DBUS_TYPE_UNIX_FD:
        array_type = DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UNIX_FD_AS_STRING;
        element_type = DBUS_TYPE_UNIX_FD_AS_STRING;
        break;
      default:
        abort();
        // TODO(keybuk): additional argument types
    }

    if (as_property_dictionary_) {
      dbus_message_iter_open_container(&struct_iter,
                                       DBUS_TYPE_VARIANT,
                                       arg.array ? array_type : element_type,
                                       &iter);
    }

    DBusMessageIter array_iter;
    if (arg.array) {
      dbus_message_iter_open_container(&iter,
                                       DBUS_TYPE_ARRAY, element_type,
                                       &array_iter);

      if (arg.type == DBUS_TYPE_STRING
          || arg.type == DBUS_TYPE_OBJECT_PATH) {
        for (std::vector<std::string>::const_iterator vit =
                 arg.string_values.begin(); vit != arg.string_values.end();
             ++vit) {
          const char *str_value = vit->c_str();
          dbus_message_iter_append_basic(&array_iter, arg.type, &str_value);
        }
      }
      // TODO(keybuk): additional element types

      dbus_message_iter_close_container(&iter, &array_iter);
    } else {
      if (arg.type == DBUS_TYPE_STRING
          || arg.type == DBUS_TYPE_OBJECT_PATH) {
        const char *str_value = arg.string_value.c_str();
        dbus_message_iter_append_basic(&iter, arg.type, &str_value);
      } else if (arg.type == DBUS_TYPE_UNIX_FD) {
        dbus_message_iter_append_basic(&iter, arg.type, &arg.int_value);
      }
      // TODO(keybuk): additional argument types
    }

    if (as_property_dictionary_) {
      dbus_message_iter_close_container(&struct_iter, &iter);
      dbus_message_iter_close_container(&dict_array_iter, &struct_iter);
    }
  }

  if (as_property_dictionary_)
    dbus_message_iter_close_container(&message_iter, &dict_array_iter);
}

void DBusMatch::SendMessage(DBusConnection *conn, DBusMessage *message) {
  dbus_bool_t success;
  dbus_uint32_t serial;
  success = dbus_connection_send(conn, message, &serial);

  if (success && expect_serial_)
    expected_serials_.push_back(serial);
}


bool DBusMatch::HandleServerMessage(DBusConnection *conn,
                                    DBusMessage *message) {
  // Make sure we're expecting a method call or signal of this name
  if (message_type_ == DBUS_MESSAGE_TYPE_METHOD_CALL &&
      !dbus_message_is_method_call(message,
                                   interface_.c_str(), member_.c_str()))
    return false;
  else if (message_type_ == DBUS_MESSAGE_TYPE_SIGNAL &&
           !dbus_message_is_signal(message,
                                   interface_.c_str(), member_.c_str()))
    return false;

  // Make sure the path is what we expected.
  if (path_.length() &&
      strcmp(path_.c_str(), dbus_message_get_path(message)) != 0)
    return false;

  // And the arguments.
  if (!MatchMessageArgs(message, &args_))
    return false;

  // Handle any actions.
  matched_ = true;
  if (send_reply_ || send_error_) {
    // Send out the reply
    DBusMessage *reply = NULL;
    if (send_reply_)
      reply = dbus_message_new_method_return(message);
    else if (send_error_)
      reply = dbus_message_new_error(message,
                                     error_name_.c_str(),
                                     error_message_.c_str());

    AppendArgsToMessage(reply, &reply_args_);
    SendMessage(conn, reply);

    dbus_message_unref(reply);
  }

  return true;
}

bool DBusMatch::HandleClientMessage(DBusConnection *conn,
                                    DBusMessage *message) {
  // From the client side we check whether the message has a serial number
  // we generated on our server side, and if so, remove it from the list of
  // those we're expecting to see.
  for (std::vector<dbus_uint32_t>::iterator it = expected_serials_.begin();
       it != expected_serials_.end(); ++it) {
    if (*it == dbus_message_get_serial(message)) {
      expected_serials_.erase(it);
      return true;
    }
  }

  return false;
}

bool DBusMatch::Complete() {
  return matched_ && expected_serials_.size() == 0;
}


DBusTest::DBusTest()
    : conn_(NULL),
      server_(NULL),
      server_conn_(NULL),
      dispatch_(false) {
}

DBusTest::~DBusTest() {
}


DBusMatch& DBusTest::ExpectMethodCall(std::string path,
                                      std::string interface,
                                      std::string method) {
  DBusMatch match;
  match.ExpectMethodCall(path, interface, method);
  pthread_mutex_lock(&mutex_);
  matches_.push_back(match);
  DBusMatch &ref = matches_.back();
  pthread_mutex_unlock(&mutex_);
  return ref;
}


DBusMatch& DBusTest::CreateSignal(std::string path,
                                  std::string interface,
                                  std::string signal_name) {
  DBusMatch match;
  match.CreateSignal(server_conn_, path, interface, signal_name);
  pthread_mutex_lock(&mutex_);
  matches_.push_back(match);
  DBusMatch &ref = matches_.back();
  pthread_mutex_unlock(&mutex_);
  return ref;
}

DBusMatch& DBusTest::CreateMessageCall(std::string path,
                                       std::string interface,
                                       std::string signal_name) {
  DBusMatch match;
  match.CreateMessageCall(server_conn_, path, interface, signal_name);
  pthread_mutex_lock(&mutex_);
  matches_.push_back(match);
  DBusMatch &ref = matches_.back();
  pthread_mutex_unlock(&mutex_);
  return ref;
}


void DBusTest::WaitForMatches() {
  for (;;) {
    pthread_mutex_lock(&mutex_);
    size_t incomplete_matches = 0;
    for (std::vector<DBusMatch>::iterator it = matches_.begin();
         it != matches_.end(); ++it) {
      DBusMatch &match = *it;
      if (!match.Complete())
        ++incomplete_matches;
    }
    pthread_mutex_unlock(&mutex_);

    if (!incomplete_matches)
      break;

    // Fish a message from the queue.
    DBusMessage *message;
    while ((message = dbus_connection_borrow_message(conn_)) == NULL)
      dbus_connection_read_write(conn_, -1);

    // Allow matches to verify the serial of the message.
    pthread_mutex_lock(&mutex_);
    for (std::vector<DBusMatch>::iterator it = matches_.begin();
         it != matches_.end(); ++it) {
      DBusMatch &match = *it;

      if (match.HandleClientMessage(conn_, message))
          break;
    }
    pthread_mutex_unlock(&mutex_);

    // Throw it back and dispatch.
    dbus_connection_return_message(conn_, message);
    dbus_connection_dispatch(conn_);
  }

  pthread_mutex_lock(&mutex_);
  matches_.erase(matches_.begin(), matches_.end());
  pthread_mutex_unlock(&mutex_);
}


void DBusTest::SetUp() {
  dbus_threads_init_default();

  // Create the D-Bus server that will accept a connection for us, since
  // there's no "just give me a socketpair" option in libdbus.
  server_ = dbus_server_listen(kServerAddress, NULL);
  ASSERT_TRUE(server_ != NULL);

  dbus_server_set_new_connection_function(server_, NewConnectionThunk,
                                          this, NULL);

  dbus_bool_t success;
  success = dbus_server_set_watch_functions(server_,
                                            AddWatchThunk,
                                            RemoveWatchThunk,
                                            WatchToggledThunk,
                                            this, NULL);
  ASSERT_TRUE(success);

  success = dbus_server_set_timeout_functions(server_,
                                              AddTimeoutThunk,
                                              RemoveTimeoutThunk,
                                              TimeoutToggledThunk,
                                              this, NULL);
  ASSERT_TRUE(success);

  // Open a connection to our server, this returns the "client" side of the
  // connection.
  conn_ = dbus_connection_open_private(kServerAddress, NULL);
  ASSERT_TRUE(conn_ != NULL);

  // The "server" side of the connection comes from the NewConnection method
  // we set above. Dispatch until we have it.
  while (!server_conn_)
    DispatchOnce();

  // Now we set off "main loop" in the background to dispatch until the
  // client is disconnected by the TearDown method.
  int r;
  r = pthread_mutex_init(&mutex_, NULL);
  ASSERT_EQ(0, r);

  dispatch_ = true;
  r = pthread_create(&thread_id_, NULL, DispatchLoopThunk, this);
  ASSERT_EQ(0, r);
}

void DBusTest::TearDown() {
  WaitForMatches();

  // Close the client end of the connection, this will result in a signal
  // within the dispatch loop of the server.
  if (conn_) {
    dbus_connection_flush(conn_);
    dbus_connection_close(conn_);
    dbus_connection_unref(conn_);
    conn_ = NULL;
  }

  // Join the thread and wait for it to finish dispatch.
  if (dispatch_)
    pthread_join(thread_id_, NULL);
  pthread_mutex_destroy(&mutex_);

  // Clean up the server end of the connection and the server itself.
  if (server_conn_) {
    dbus_connection_flush(server_conn_);
    dbus_connection_close(server_conn_);
    dbus_connection_unref(server_conn_);
    server_conn_ = NULL;
  }

  dbus_server_disconnect(server_);
  dbus_server_unref(server_);
  server_ = NULL;

  dbus_shutdown();
}

void DBusTest::NewConnectionThunk(DBusServer *server,
                                  DBusConnection *conn,
                                  void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  test->NewConnection(server, conn);
}

void DBusTest::NewConnection(DBusServer *server, DBusConnection *conn) {
  ASSERT_TRUE(server_conn_ == NULL);

  dbus_bool_t success;
  success = dbus_connection_set_watch_functions(conn,
                                                AddWatchThunk,
                                                RemoveWatchThunk,
                                                WatchToggledThunk,
                                                this, NULL);
  ASSERT_TRUE(success);

  success = dbus_connection_set_timeout_functions(conn,
                                                  AddTimeoutThunk,
                                                  RemoveTimeoutThunk,
                                                  TimeoutToggledThunk,
                                                  this, NULL);
  ASSERT_TRUE(success);

  success = dbus_connection_add_filter(conn,
                                       HandleMessageThunk,
                                       this, NULL);
  ASSERT_TRUE(success);

  server_conn_ = conn;
  dbus_connection_ref(server_conn_);
}

dbus_bool_t DBusTest::AddWatchThunk(DBusWatch *watch, void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  return test->AddWatch(watch);
}

dbus_bool_t DBusTest::AddWatch(DBusWatch *watch) {
  watches_.push_back(watch);
  return TRUE;
}

void DBusTest::RemoveWatchThunk(DBusWatch *watch, void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  test->RemoveWatch(watch);
}

void DBusTest::RemoveWatch(DBusWatch *watch) {
  std::vector<DBusWatch *>::iterator it =
      find(watches_.begin(), watches_.end(), watch);
  if (it != watches_.end())
    watches_.erase(it);
}

void DBusTest::WatchToggledThunk(DBusWatch *watch, void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  test->WatchToggled(watch);
}

void DBusTest::WatchToggled(DBusWatch *watch) {
}

dbus_bool_t DBusTest::AddTimeoutThunk(DBusTimeout *timeout, void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  return test->AddTimeout(timeout);
}

dbus_bool_t DBusTest::AddTimeout(DBusTimeout *timeout) {
  timeouts_.push_back(timeout);
  return TRUE;
}

void DBusTest::RemoveTimeoutThunk(DBusTimeout *timeout, void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  test->RemoveTimeout(timeout);
}

void DBusTest::RemoveTimeout(DBusTimeout *timeout) {
  std::vector<DBusTimeout *>::iterator it =
      find(timeouts_.begin(), timeouts_.end(), timeout);
  if (it != timeouts_.end())
    timeouts_.erase(it);
}

void DBusTest::TimeoutToggledThunk(DBusTimeout *timeout, void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  test->TimeoutToggled(timeout);
}

void DBusTest::TimeoutToggled(DBusTimeout *timeout) {
}

DBusHandlerResult DBusTest::HandleMessageThunk(DBusConnection *conn,
                                               DBusMessage *message,
                                               void *data) {
  DBusTest *test = static_cast<DBusTest *>(data);
  return test->HandleMessage(conn, message);
}

DBusHandlerResult DBusTest::HandleMessage(DBusConnection *conn,
                                          DBusMessage *message) {
  if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
    dispatch_ = false;
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  pthread_mutex_lock(&mutex_);
  for (std::vector<DBusMatch>::iterator it = matches_.begin();
       it != matches_.end(); ++it) {
    DBusMatch &match = *it;

    if (match.HandleServerMessage(conn, message)) {
      pthread_mutex_unlock(&mutex_);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
  }
  pthread_mutex_unlock(&mutex_);

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


void *DBusTest::DispatchLoopThunk(void *ptr) {
  DBusTest *test = static_cast<DBusTest *>(ptr);
  return test->DispatchLoop();
}

void *DBusTest::DispatchLoop() {
  while (dispatch_)
    DispatchOnce();

  return NULL;
}

void DBusTest::DispatchOnce() {
  fd_set readfds, writefds;
  int nfds = 0;
  int r;

  // Ideally we'd just use dbus_connection_read_write_dispatch() here, but
  // we have to deal with both the server itself and its connection, so we
  // have to do it all by hand.
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  for (std::vector<DBusWatch *>::iterator it = watches_.begin();
       it != watches_.end(); ++it) {
    DBusWatch *watch = *it;

    if (!dbus_watch_get_enabled(watch))
      continue;

    int fd = dbus_watch_get_unix_fd(watch);
    if (fd + 1 > nfds)
      nfds = fd + 1;

    unsigned int flags = dbus_watch_get_flags(watch);
    if (flags & DBUS_WATCH_READABLE)
      FD_SET(fd, &readfds);
    if (flags & DBUS_WATCH_WRITABLE)
      FD_SET(fd, &writefds);
  }

  // Only block in select for the interval of the smallest timeout; this
  // isn't quite right according to the D-Bus spec, since the interval is
  // supposed to be since the time the timeout was added or toggled, but
  // it's good enough for the purposes of testing.
  DBusTimeout *earliest_timeout = NULL;
  struct timeval timeval;

  for (std::vector<DBusTimeout *>::iterator it = timeouts_.begin();
       it != timeouts_.end(); ++it) {
    DBusTimeout *timeout = *it;

    if (!dbus_timeout_get_enabled(timeout))
      continue;

    if (!earliest_timeout || (dbus_timeout_get_interval(timeout) <
                              dbus_timeout_get_interval(earliest_timeout)))
      earliest_timeout = timeout;
  }

  if (earliest_timeout) {
    int interval = dbus_timeout_get_interval(earliest_timeout);
    timeval.tv_sec = interval / 1000;
    timeval.tv_usec = (interval % 1000) * 1000;

    r = select(nfds, &readfds, &writefds, NULL, &timeval);
  } else {
    r = select(nfds, &readfds, &writefds, NULL, NULL);
  }

  ASSERT_LE(0, r);

  // Handle the timeout if we didn't poll for anything else.
  if (r == 0 && earliest_timeout)
    dbus_timeout_handle(earliest_timeout);

  // Handle the watches, use a copy of the vector since a watch handler
  // might remove other watches in the vector.
  std::vector<DBusWatch *> immutable_watches = watches_;
  for (std::vector<DBusWatch *>::iterator it = immutable_watches.begin();
       it != immutable_watches.end(); ++it) {
    DBusWatch *watch = *it;

    int fd = dbus_watch_get_unix_fd(watch);
    unsigned int flags = 0;

    if (FD_ISSET(fd, &readfds))
      flags |= DBUS_WATCH_READABLE;
    if (FD_ISSET(fd, &writefds))
      flags |= DBUS_WATCH_WRITABLE;

    if (flags)
      dbus_watch_handle(watch, flags);
  }

  // Dispatch data on the server-side of the connection.
  while (server_conn_ &&
         dbus_connection_dispatch(server_conn_) == DBUS_DISPATCH_DATA_REMAINS)
    ;
}
