// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FRONTEND_ACTION_H_
#define FRONTEND_ACTION_H_

#include <ir_representation.h>

#include <clang/Frontend/FrontendAction.h>
#include <llvm/ADT/StringRef.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace clang {
  class ASTConsumer;
  class CompilerInstance;
}  // namespace clang

class HeaderCheckerFrontendAction : public clang::ASTFrontendAction {
 private:
  const std::string &dump_name_;
  std::set<std::string> &exported_headers_;
  abi_util::TextFormatIR text_format_;

 public:
  HeaderCheckerFrontendAction(
      const std::string &dump_name,
      std::set<std::string> &exported_headers,
      abi_util::TextFormatIR text_format);

 protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &ci, llvm::StringRef header_file) override;
};

#endif  // FRONTEND_ACTION_H_
