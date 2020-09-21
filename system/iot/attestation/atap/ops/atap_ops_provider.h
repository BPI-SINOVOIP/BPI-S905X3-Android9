/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ATAP_OPS_PROVIDER_H_
#define ATAP_OPS_PROVIDER_H_

#include "atap_ops_delegate.h"

namespace atap {

// An implementation of ops callbacks that forwards to a given delegate. All
// instances of this class must be created on the same thread. A delegate must
// be provided either in the constructor or set_delegate() before using the
// AtapOps returned by atap_ops().
class AtapOpsProvider {
 public:
  AtapOpsProvider();
  AtapOpsProvider(AtapOpsDelegate* delegate);
  virtual ~AtapOpsProvider();

  static AtapOpsProvider* GetInstanceFromAtapOps(AtapOps* ops) {
    return reinterpret_cast<AtapOpsProvider*>(ops->user_data);
  }

  AtapOps* atap_ops() {
    atap_assert(delegate_);
    return &atap_ops_;
  }

  AtapOpsDelegate* delegate() {
    return delegate_;
  }

  // Does not take ownership of |delegate|.
  void set_delegate(AtapOpsDelegate* delegate) {
    delegate_ = delegate;
  }

 private:
  void setup_ops();

  AtapOps atap_ops_;
  AtapOpsDelegate* delegate_{nullptr};
};

}  // namespace atap

#endif /* ATAP_OPS_PROVIDER_H_ */
