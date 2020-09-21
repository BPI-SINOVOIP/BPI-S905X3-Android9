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

#ifndef AST_PROCESSING_H_
#define AST_PROCESSING_H_

#include "ast_util.h"
#include <ir_representation.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Mangle.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PPCallbacks.h>

#include <set>

class HeaderASTVisitor
    : public clang::RecursiveASTVisitor<HeaderASTVisitor> {
 public:
  HeaderASTVisitor(clang::MangleContext *mangle_contextp,
                   clang::ASTContext *ast_contextp,
                   const clang::CompilerInstance *compiler_instance_p,
                   const std::set<std::string> &exported_headers,
                   const clang::Decl *tu_decl,
                   abi_util::IRDumper *ir_dumper,
                   ast_util::ASTCaches *ast_caches);

  bool VisitRecordDecl(const clang::RecordDecl *decl);

  bool VisitFunctionDecl(const clang::FunctionDecl *decl);

  bool VisitEnumDecl(const clang::EnumDecl *decl);

  bool VisitVarDecl(const clang::VarDecl *decl);

  bool TraverseDecl(clang::Decl *decl);

  // Enable recursive traversal of template instantiations.
  bool shouldVisitTemplateInstantiations() const {
    return true;
  }

 private:
  clang::MangleContext *mangle_contextp_;
  clang::ASTContext *ast_contextp_;
  const clang::CompilerInstance *cip_;
  const std::set<std::string> &exported_headers_;
  // To optimize recursion into only exported abi.
  const clang::Decl *tu_decl_;
  abi_util::IRDumper *ir_dumper_;
  // We cache the source file an AST node corresponds to, to avoid repeated
  // calls to "realpath".
  ast_util::ASTCaches *ast_caches_;
};

class HeaderASTConsumer : public clang::ASTConsumer {
 public:
  HeaderASTConsumer(clang::CompilerInstance *compiler_instancep,
                    const std::string &out_dump_name,
                    std::set<std::string> &exported_headers,
                    abi_util::TextFormatIR text_format);

  void HandleTranslationUnit(clang::ASTContext &ctx) override;

 private:
  clang::CompilerInstance *cip_;
  const std::string &out_dump_name_;
  std::set<std::string> &exported_headers_;
  abi_util::TextFormatIR text_format_;
};

#endif // AST_PROCESSING_H_
