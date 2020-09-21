/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBTEXTCLASSIFIER_UTIL_BASE_LOGGING_H_
#define LIBTEXTCLASSIFIER_UTIL_BASE_LOGGING_H_

#include <cassert>
#include <string>

#include "util/base/logging_levels.h"
#include "util/base/port.h"


namespace libtextclassifier2 {
namespace logging {

// A tiny code footprint string stream for assembling log messages.
struct LoggingStringStream {
  LoggingStringStream() {}
  LoggingStringStream &stream() { return *this; }
  // Needed for invocation in TC_CHECK macro.
  explicit operator bool() const { return true; }

  std::string message;
};

template <typename T>
inline LoggingStringStream &operator<<(LoggingStringStream &stream,
                                       const T &entry) {
  stream.message.append(std::to_string(entry));
  return stream;
}

inline LoggingStringStream &operator<<(LoggingStringStream &stream,
                                       const char *message) {
  stream.message.append(message);
  return stream;
}

#if defined(HAS_GLOBAL_STRING)
inline LoggingStringStream &operator<<(LoggingStringStream &stream,
                                       const ::string &message) {
  stream.message.append(message);
  return stream;
}
#endif

inline LoggingStringStream &operator<<(LoggingStringStream &stream,
                                       const std::string &message) {
  stream.message.append(message);
  return stream;
}

// The class that does all the work behind our TC_LOG(severity) macros.  Each
// TC_LOG(severity) << obj1 << obj2 << ...; logging statement creates a
// LogMessage temporary object containing a stringstream.  Each operator<< adds
// info to that stringstream and the LogMessage destructor performs the actual
// logging.  The reason this works is that in C++, "all temporary objects are
// destroyed as the last step in evaluating the full-expression that (lexically)
// contains the point where they were created."  For more info, see
// http://en.cppreference.com/w/cpp/language/lifetime.  Hence, the destructor is
// invoked after the last << from that logging statement.
class LogMessage {
 public:
  LogMessage(LogSeverity severity, const char *file_name,
             int line_number) TC_ATTRIBUTE_NOINLINE;

  ~LogMessage() TC_ATTRIBUTE_NOINLINE;

  // Returns the stream associated with the logger object.
  LoggingStringStream &stream() { return stream_; }

 private:
  const LogSeverity severity_;

  // Stream that "prints" all info into a string (not to a file).  We construct
  // here the entire logging message and next print it in one operation.
  LoggingStringStream stream_;
};

// Pseudo-stream that "eats" the tokens <<-pumped into it, without printing
// anything.
class NullStream {
 public:
  NullStream() {}
  NullStream &stream() { return *this; }
};
template <typename T>
inline NullStream &operator<<(NullStream &str, const T &) {
  return str;
}

}  // namespace logging
}  // namespace libtextclassifier2

#define TC_LOG(severity)                                           \
  ::libtextclassifier2::logging::LogMessage(                       \
      ::libtextclassifier2::logging::severity, __FILE__, __LINE__) \
      .stream()

// If condition x is true, does nothing.  Otherwise, crashes the program (liek
// LOG(FATAL)) with an informative message.  Can be continued with extra
// messages, via <<, like any logging macro, e.g.,
//
// TC_CHECK(my_cond) << "I think we hit a problem";
#define TC_CHECK(x)                                                           \
  (x) || TC_LOG(FATAL) << __FILE__ << ":" << __LINE__ << ": check failed: \"" \
                       << #x

#define TC_CHECK_EQ(x, y) TC_CHECK((x) == (y))
#define TC_CHECK_LT(x, y) TC_CHECK((x) < (y))
#define TC_CHECK_GT(x, y) TC_CHECK((x) > (y))
#define TC_CHECK_LE(x, y) TC_CHECK((x) <= (y))
#define TC_CHECK_GE(x, y) TC_CHECK((x) >= (y))
#define TC_CHECK_NE(x, y) TC_CHECK((x) != (y))

#define TC_NULLSTREAM ::libtextclassifier2::logging::NullStream().stream()

// Debug checks: a TC_DCHECK<suffix> macro should behave like TC_CHECK<suffix>
// in debug mode an don't check / don't print anything in non-debug mode.
#ifdef NDEBUG

#define TC_DCHECK(x) TC_NULLSTREAM
#define TC_DCHECK_EQ(x, y) TC_NULLSTREAM
#define TC_DCHECK_LT(x, y) TC_NULLSTREAM
#define TC_DCHECK_GT(x, y) TC_NULLSTREAM
#define TC_DCHECK_LE(x, y) TC_NULLSTREAM
#define TC_DCHECK_GE(x, y) TC_NULLSTREAM
#define TC_DCHECK_NE(x, y) TC_NULLSTREAM

#else  // NDEBUG

// In debug mode, each TC_DCHECK<suffix> is equivalent to TC_CHECK<suffix>,
// i.e., a real check that crashes when the condition is not true.
#define TC_DCHECK(x) TC_CHECK(x)
#define TC_DCHECK_EQ(x, y) TC_CHECK_EQ(x, y)
#define TC_DCHECK_LT(x, y) TC_CHECK_LT(x, y)
#define TC_DCHECK_GT(x, y) TC_CHECK_GT(x, y)
#define TC_DCHECK_LE(x, y) TC_CHECK_LE(x, y)
#define TC_DCHECK_GE(x, y) TC_CHECK_GE(x, y)
#define TC_DCHECK_NE(x, y) TC_CHECK_NE(x, y)

#endif  // NDEBUG

#ifdef LIBTEXTCLASSIFIER_VLOG
#define TC_VLOG(severity)                                      \
  ::libtextclassifier2::logging::LogMessage(                   \
      ::libtextclassifier2::logging::INFO, __FILE__, __LINE__) \
      .stream()
#else
#define TC_VLOG(severity) TC_NULLSTREAM
#endif

#endif  // LIBTEXTCLASSIFIER_UTIL_BASE_LOGGING_H_
