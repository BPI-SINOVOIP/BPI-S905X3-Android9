// Copyright (C) 2017 The Android Open Source Project
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

package hal2vts

import (
	"path/filepath"
	"sort"
	"strings"
	"sync"

	"android/soong/android"
	"android/soong/genrule"

	"github.com/google/blueprint"
)

func init() {
	android.RegisterModuleType("hal2vts", hal2vtsFactory)
}

var (
	pctx = android.NewPackageContext("android/soong/hal2vts")

	hidlGen = pctx.HostBinToolVariable("hidlGen", "hidl-gen")

	hidlGenCmd = "${hidlGen} -o ${genDir} -L vts ${args} " +
		"${pckg}@${ver}::${halFile}"

	hal2vtsRule = pctx.StaticRule("hal2vtsRule", blueprint.RuleParams{
		Command:     hidlGenCmd,
		CommandDeps: []string{"${hidlGen}"},
		Description: "hidl-gen -l vts $in => $out",
	}, "genDir", "args", "pckg", "ver", "halFile")
)

type hal2vtsProperties struct {
	Hidl_gen_args string
	Srcs []string
	Out  []string
}

type hal2vts struct {
	android.ModuleBase

	properties hal2vtsProperties

	exportedHeaderDirs android.Paths
	generatedHeaders   android.Paths
}

var _ genrule.SourceFileGenerator = (*hal2vts)(nil)

func (h *hal2vts) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	// Sort by name to match .hal to .vts file name.
	sort.Strings(h.properties.Out)
	srcFiles := ctx.ExpandSources(h.properties.Srcs, nil)
	sort.SliceStable(srcFiles, func(i, j int) bool {
		return strings.Compare(srcFiles[j].String(), srcFiles[i].String()) == 1
	})

	if len(srcFiles) != len(h.properties.Out) {
		ctx.ModuleErrorf("Number of inputs must be equal to number of outputs.")
	}

	args := h.properties.Hidl_gen_args

	genDir := android.PathForModuleGen(ctx, "").String()

	vtsList := vtsList(ctx.AConfig())

	vtsListMutex.Lock()
	defer vtsListMutex.Unlock()

	for i, src := range srcFiles {
		out := android.PathForModuleGen(ctx, h.properties.Out[i])
		if src.Ext() != ".hal" {
			ctx.ModuleErrorf("Source file has to be a .hal file.")
		}

		relOut, err := filepath.Rel(genDir, out.String())
		if err != nil {
			ctx.ModuleErrorf("Cannot get relative output path.")
		}
		halDir := filepath.Dir(relOut)
		halFile := strings.TrimSuffix(src.Base(), ".hal")

		ver := filepath.Base(halDir)
		// Need this to transform directory path to hal name.
		// For example, audio/effect -> audio.effect
		pckg := strings.Replace(filepath.Dir(halDir), "/", ".", -1)

		ctx.ModuleBuild(pctx, android.ModuleBuildParams{
			Rule:   hal2vtsRule,
			Input:  src,
			Output: out,
			Args: map[string]string{
				"genDir":  genDir,
				"args":    args,
				"pckg":    pckg,
				"ver":     ver,
				"halFile": halFile,
			},
		})
		h.generatedHeaders = append(h.generatedHeaders, out)
		*vtsList = append(*vtsList, out)
	}
	h.exportedHeaderDirs = append(h.exportedHeaderDirs, android.PathForModuleGen(ctx, ""))
}

func (h *hal2vts) DepsMutator(ctx android.BottomUpMutatorContext) {
	android.ExtractSourcesDeps(ctx, h.properties.Srcs)
}

func (h *hal2vts) GeneratedHeaderDirs() android.Paths {
	return h.exportedHeaderDirs
}

func (h *hal2vts) GeneratedSourceFiles() android.Paths {
	return nil
}

func (h *hal2vts) GeneratedDeps() android.Paths {
	return h.generatedHeaders
}

func hal2vtsFactory() android.Module {
	h := &hal2vts{}
	h.AddProperties(&h.properties)
	android.InitAndroidModule(h)
	return h
}

func vtsList(config android.Config) *android.Paths {
	return config.Once("vtsList", func() interface{} {
		return &android.Paths{}
	}).(*android.Paths)
}

var vtsListMutex sync.Mutex

func init() {
	android.RegisterMakeVarsProvider(pctx, makeVarsProvider)
}

func makeVarsProvider(ctx android.MakeVarsContext) {
	vtsList := vtsList(ctx.Config()).Strings()
	sort.Strings(vtsList)

	ctx.Strict("VTS_SPEC_FILE_LIST", strings.Join(vtsList, " "))
}
