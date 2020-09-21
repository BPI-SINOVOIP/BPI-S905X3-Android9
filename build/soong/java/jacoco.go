// Copyright 2017 Google Inc. All rights reserved.
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

package java

// Rules for instrumenting classes using jacoco

import (
	"fmt"
	"path/filepath"
	"strings"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"

	"android/soong/android"
)

var (
	jacoco = pctx.AndroidStaticRule("jacoco", blueprint.RuleParams{
		Command: `rm -rf $tmpDir && mkdir -p $tmpDir && ` +
			`${config.Zip2ZipCmd} -i $in -o $strippedJar $stripSpec && ` +
			`${config.JavaCmd} -jar ${config.JacocoCLIJar} ` +
			`  instrument --quiet --dest $tmpDir $strippedJar && ` +
			`${config.Ziptime} $tmpJar && ` +
			`${config.MergeZipsCmd} --ignore-duplicates -j $out $tmpJar $in`,
		CommandDeps: []string{
			"${config.Zip2ZipCmd}",
			"${config.JavaCmd}",
			"${config.JacocoCLIJar}",
			"${config.Ziptime}",
			"${config.MergeZipsCmd}",
		},
	},
		"strippedJar", "stripSpec", "tmpDir", "tmpJar")
)

// Instruments a jar using the Jacoco command line interface.  Uses stripSpec to extract a subset
// of the classes in inputJar into strippedJar, instruments strippedJar into tmpJar, and then
// combines the classes in tmpJar with inputJar (preferring the instrumented classes in tmpJar)
// to produce instrumentedJar.
func jacocoInstrumentJar(ctx android.ModuleContext, instrumentedJar, strippedJar android.WritablePath,
	inputJar android.Path, stripSpec string) {

	// The basename of tmpJar has to be the same as the basename of strippedJar
	tmpJar := android.PathForModuleOut(ctx, "jacoco", "tmp", strippedJar.Base())

	ctx.Build(pctx, android.BuildParams{
		Rule:           jacoco,
		Description:    "jacoco",
		Output:         instrumentedJar,
		ImplicitOutput: strippedJar,
		Input:          inputJar,
		Args: map[string]string{
			"strippedJar": strippedJar.String(),
			"stripSpec":   stripSpec,
			"tmpDir":      filepath.Dir(tmpJar.String()),
			"tmpJar":      tmpJar.String(),
		},
	})
}

func (j *Module) jacocoModuleToZipCommand(ctx android.ModuleContext) string {
	includes, err := jacocoFiltersToSpecs(j.properties.Jacoco.Include_filter)
	if err != nil {
		ctx.PropertyErrorf("jacoco.include_filter", "%s", err.Error())
	}
	excludes, err := jacocoFiltersToSpecs(j.properties.Jacoco.Exclude_filter)
	if err != nil {
		ctx.PropertyErrorf("jacoco.exclude_filter", "%s", err.Error())
	}

	return jacocoFiltersToZipCommand(includes, excludes)
}

func jacocoFiltersToZipCommand(includes, excludes []string) string {
	specs := ""
	if len(excludes) > 0 {
		specs += android.JoinWithPrefix(excludes, "-x ") + " "
	}
	if len(includes) > 0 {
		specs += strings.Join(includes, " ")
	} else {
		specs += "**/*.class"
	}
	return specs
}

func jacocoFiltersToSpecs(filters []string) ([]string, error) {
	specs := make([]string, len(filters))
	var err error
	for i, f := range filters {
		specs[i], err = jacocoFilterToSpec(f)
		if err != nil {
			return nil, err
		}
	}
	return proptools.NinjaAndShellEscape(specs), nil
}

func jacocoFilterToSpec(filter string) (string, error) {
	recursiveWildcard := strings.HasSuffix(filter, "**")
	nonRecursiveWildcard := false
	if !recursiveWildcard {
		nonRecursiveWildcard = strings.HasSuffix(filter, "*")
		filter = strings.TrimSuffix(filter, "*")
	} else {
		filter = strings.TrimSuffix(filter, "**")
	}

	if recursiveWildcard && !(strings.HasSuffix(filter, ".") || filter == "") {
		return "", fmt.Errorf("only '**' or '.**' is supported as recursive wildcard in a filter")
	}

	if strings.ContainsRune(filter, '*') {
		return "", fmt.Errorf("'*' is only supported as the last character in a filter")
	}

	spec := strings.Replace(filter, ".", "/", -1)

	if recursiveWildcard {
		spec += "**/*.class"
	} else if nonRecursiveWildcard {
		spec += "*.class"
	} else {
		spec += ".class"
	}

	return spec, nil
}
