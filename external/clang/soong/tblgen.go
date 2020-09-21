// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package clang

import (
	"path/filepath"
	"strings"

	"android/soong/android"
	"android/soong/genrule"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"
)

func init() {
	android.RegisterModuleType("clang_tblgen", clangTblgenFactory)
}

var (
	pctx = android.NewPackageContext("android/soong/clang")

	clangTblgen = pctx.HostBinToolVariable("clangTblgen", "clang-tblgen")

	tblgenRule = pctx.StaticRule("tblgenRule", blueprint.RuleParams{
		Depfile:     "${out}.d",
		Deps:        blueprint.DepsGCC,
		Command:     "${clangTblgen} ${includes} ${genopt} -d ${depfile} -o ${out} ${in}",
		CommandDeps: []string{"${clangTblgen}"},
		Description: "Clang TableGen $in => $out",
	}, "includes", "depfile", "genopt")
)

type tblgenProperties struct {
	In   *string
	Outs []string
}

type tblgen struct {
	android.ModuleBase

	properties tblgenProperties

	exportedHeaderDirs android.Paths
	generatedHeaders   android.Paths
}

var _ genrule.SourceFileGenerator = (*tblgen)(nil)

func (t *tblgen) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	in := android.PathForModuleSrc(ctx, proptools.String(t.properties.In))

	includes := []string{
		"-I " + ctx.ModuleDir(),
		"-I external/llvm/include",
		"-I external/llvm/lib/Target",
		"-I external/clang/include",
		"-I " + filepath.Dir(in.String()),
	}

	for _, o := range t.properties.Outs {
		out := android.PathForModuleGen(ctx, o)
		generator := outToGenerator(ctx, o)

		ctx.ModuleBuild(pctx, android.ModuleBuildParams{
			Rule:   tblgenRule,
			Input:  in,
			Output: out,
			Args: map[string]string{
				"includes": strings.Join(includes, " "),
				"genopt":   generator,
			},
		})
		t.generatedHeaders = append(t.generatedHeaders, out)
	}

	t.exportedHeaderDirs = append(t.exportedHeaderDirs, android.PathForModuleGen(ctx, ""))
}

func outToGenerator(ctx android.ModuleContext, out string) string {
	out = filepath.Base(out)
	switch {
	case out == "AttrDump.inc":
		return "-gen-clang-attr-dump"
	case out == "AttrImpl.inc":
		return "-gen-clang-attr-impl"
	case out == "AttrHasAttributeImpl.inc":
		return "-gen-clang-attr-has-attribute-impl"
	case out == "AttrList.inc":
		return "-gen-clang-attr-list"
	case out == "AttrSpellingListIndex.inc":
		return "-gen-clang-attr-spelling-index"
	case out == "AttrPCHRead.inc":
		return "-gen-clang-attr-pch-read"
	case out == "AttrPCHWrite.inc":
		return "-gen-clang-attr-pch-write"
	case out == "Attrs.inc":
		return "-gen-clang-attr-classes"
	case out == "AttrParserStringSwitches.inc":
		return "-gen-clang-attr-parser-string-switches"
	case out == "AttrVisitor.inc":
		return "-gen-clang-attr-ast-visitor"
	case out == "AttrParsedAttrKinds.inc":
		return "-gen-clang-attr-parsed-attr-kinds"
	case out == "AttrParsedAttrImpl.inc":
		return "-gen-clang-attr-parsed-attr-impl"
	case out == "AttrParsedAttrList.inc":
		return "-gen-clang-attr-parsed-attr-list"
	case out == "AttrTemplateInstantiate.inc":
		return "-gen-clang-attr-template-instantiate"
	case out == "Checkers.inc":
		return "-gen-clang-sa-checkers"
	case out == "CommentCommandInfo.inc":
		return "-gen-clang-comment-command-info"
	case out == "CommentCommandList.inc":
		return "-gen-clang-comment-command-list"
	case out == "CommentHTMLNamedCharacterReferences.inc":
		return "-gen-clang-comment-html-named-character-references"
	case out == "CommentHTMLTagsProperties.inc":
		return "-gen-clang-comment-html-tags-properties"
	case out == "CommentHTMLTags.inc":
		return "-gen-clang-comment-html-tags"
	case out == "CommentNodes.inc":
		return "-gen-clang-comment-nodes"
	case strings.HasPrefix(out, "Diagnostic") && strings.HasSuffix(out, "Kinds.inc"):
		component := strings.TrimPrefix(strings.TrimSuffix(out, "Kinds.inc"), "Diagnostic")
		return "-gen-clang-diags-defs -clang-component=" + component
	case out == "DiagnosticGroups.inc":
		return "-gen-clang-diag-groups"
	case out == "DiagnosticIndexName.inc":
		return "-gen-clang-diag-groups"
	case out == "DeclNodes.inc":
		return "-gen-clang-decl-nodes"
	case out == "StmtNodes.inc":
		return "-gen-clang-stmt-nodes"
	case out == "arm_neon.inc":
		return "-gen-arm-neon-sema"
	case out == "arm_neon.h":
		return "-gen-arm-neon"
	}

	ctx.ModuleErrorf("couldn't map output file %q to a generator", out)
	return ""
}

func (t *tblgen) DepsMutator(ctx android.BottomUpMutatorContext) {
}

func (t *tblgen) GeneratedHeaderDirs() android.Paths {
	return t.exportedHeaderDirs
}

func (t *tblgen) GeneratedSourceFiles() android.Paths {
	return nil
}

func (t *tblgen) GeneratedDeps() android.Paths {
	return t.generatedHeaders
}

func clangTblgenFactory() android.Module {
	t := &tblgen{}

	t.AddProperties(&t.properties)
	android.InitAndroidModule(t)
	return t
}
