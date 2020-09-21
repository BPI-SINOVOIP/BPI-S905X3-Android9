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

#ifndef LIBTEXTCLASSIFIER_UTIL_UTF8_UNICODETEXT_H_
#define LIBTEXTCLASSIFIER_UTIL_UTF8_UNICODETEXT_H_

#include <iterator>
#include <string>
#include <utility>

#include "util/base/integral_types.h"

namespace libtextclassifier2 {

// ***************************** UnicodeText **************************
//
// A UnicodeText object is a wrapper around a sequence of Unicode
// codepoint values that allows iteration over these values.
//
// The internal representation of the text is UTF-8. Since UTF-8 is a
// variable-width format, UnicodeText does not provide random access
// to the text, and changes to the text are permitted only at the end.
//
// The UnicodeText class defines a const_iterator. The dereferencing
// operator (*) returns a codepoint (int32). The iterator is a
// read-only iterator. It becomes invalid if the text is changed.
//
// Codepoints are integers in the range [0, 0xD7FF] or [0xE000,
// 0x10FFFF], but UnicodeText has the additional restriction that it
// can contain only those characters that are valid for interchange on
// the Web. This excludes all of the control codes except for carriage
// return, line feed, and horizontal tab.  It also excludes
// non-characters, but codepoints that are in the Private Use regions
// are allowed, as are codepoints that are unassigned. (See the
// Unicode reference for details.)
//
// MEMORY MANAGEMENT:
//
// PointToUTF8(buffer, size) creates an alias pointing to buffer.
//
// The purpose of an alias is to avoid making an unnecessary copy of a
// UTF-8 buffer while still providing access to the Unicode values
// within that text through iterators. The lifetime of an alias must not
// exceed the lifetime of the buffer from which it was constructed.
//
// Aliases should be used with care. If the source from which an alias
// was created is freed, or if the contents are changed, while the
// alias is still in use, fatal errors could result. But it can be
// quite useful to have a UnicodeText "window" through which to see a
// UTF-8 buffer without having to pay the price of making a copy.

class UnicodeText {
 public:
  class const_iterator;

  UnicodeText();  // Create an empty text.
  UnicodeText(const UnicodeText& src);
  UnicodeText& operator=(UnicodeText&& src);
  ~UnicodeText();

  class const_iterator {
    typedef const_iterator CI;

   public:
    typedef std::input_iterator_tag iterator_category;
    typedef char32 value_type;
    typedef int difference_type;
    typedef void pointer;            // (Not needed.)
    typedef const char32 reference;  // (Needed for const_reverse_iterator)

    // Iterators are default-constructible.
    const_iterator();

    // It's safe to make multiple passes over a UnicodeText.
    const_iterator& operator=(const const_iterator& other);

    char32 operator*() const;  // Dereference

    const_iterator& operator++();     // Advance (++iter)
    const_iterator operator++(int) {  // (iter++)
      const_iterator result(*this);
      ++*this;
      return result;
    }

    const_iterator& operator--();     // Retreat (--iter)
    const_iterator operator--(int) {  // (iter--)
      const_iterator result(*this);
      --*this;
      return result;
    }

    friend bool operator==(const CI& lhs, const CI& rhs) {
      return lhs.it_ == rhs.it_;
    }
    friend bool operator!=(const CI& lhs, const CI& rhs) {
      return !(lhs == rhs);
    }
    friend bool operator<(const CI& lhs, const CI& rhs);
    friend bool operator>(const CI& lhs, const CI& rhs) { return rhs < lhs; }
    friend bool operator<=(const CI& lhs, const CI& rhs) {
      return !(rhs < lhs);
    }
    friend bool operator>=(const CI& lhs, const CI& rhs) {
      return !(lhs < rhs);
    }

    int utf8_length() const {
      if (it_[0] < 0x80) {
        return 1;
      } else if (it_[0] < 0xE0) {
        return 2;
      } else if (it_[0] < 0xF0) {
        return 3;
      } else {
        return 4;
      }
    }
    const char* utf8_data() const { return it_; }

   private:
    friend class UnicodeText;
    explicit const_iterator(const char* it) : it_(it) {}

    const char* it_;
  };

  const_iterator begin() const;
  const_iterator end() const;

  // Gets pointer to the underlying utf8 data.
  const char* data() const;

  // Gets length (in bytes) of the underlying utf8 data.
  int size_bytes() const;

  // Computes length (in number of Unicode codepoints) of the underlying utf8
  // data.
  // NOTE: Complexity O(n).
  int size_codepoints() const;

  bool empty() const;

  // Checks whether the underlying data is valid utf8 data.
  bool is_valid() const;

  bool operator==(const UnicodeText& other) const;

  // x.PointToUTF8(buf,len) changes x so that it points to buf
  // ("becomes an alias"). It does not take ownership or copy buf.
  // This function assumes that the input is interchange valid UTF8.
  UnicodeText& Copy(const UnicodeText& src);
  UnicodeText& PointToUTF8(const char* utf8_buffer, int byte_length);
  UnicodeText& CopyUTF8(const char* utf8_buffer, int byte_length);

  // Calling this may invalidate pointers to underlying data.
  UnicodeText& AppendUTF8(const char* utf8, int len);
  UnicodeText& AppendCodepoint(char32 ch);
  void clear();

  std::string ToUTF8String() const;
  static std::string UTF8Substring(const const_iterator& first,
                                   const const_iterator& last);

 private:
  friend class const_iterator;

  class Repr {  // A byte-string.
   public:
    char* data_;
    int size_;
    int capacity_;
    bool ours_;  // Do we own data_?

    Repr() : data_(nullptr), size_(0), capacity_(0), ours_(true) {}
    Repr& operator=(Repr&& src);
    ~Repr() {
      if (ours_) delete[] data_;
    }

    void clear();
    void reserve(int capacity);
    void resize(int size);

    void append(const char* bytes, int byte_length);
    void Copy(const char* data, int size);
    void PointTo(const char* data, int size);

   private:
    Repr& operator=(const Repr&);
    Repr(const Repr& other);
  };

  Repr repr_;
};

typedef std::pair<UnicodeText::const_iterator, UnicodeText::const_iterator>
    UnicodeTextRange;

// NOTE: The following are needed to avoid implicit conversion from char* to
// std::string, or from ::string to std::string, because if this happens it
// often results in invalid memory access to a temporary object created during
// such conversion (if do_copy == false).
UnicodeText UTF8ToUnicodeText(const char* utf8_buf, int len, bool do_copy);
UnicodeText UTF8ToUnicodeText(const char* utf8_buf, bool do_copy);
UnicodeText UTF8ToUnicodeText(const std::string& str, bool do_copy);
UnicodeText UTF8ToUnicodeText(const std::string& str);

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_UTF8_UNICODETEXT_H_
