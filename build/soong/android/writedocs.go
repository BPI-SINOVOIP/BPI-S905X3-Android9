// Copyright 2015 Google Inc. All rights reserved.
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

package android

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/google/blueprint"
)

func init() {
	RegisterSingletonType("writedocs", DocsSingleton)
}

func DocsSingleton() Singleton {
	return &docsSingleton{}
}

type docsSingleton struct{}

func primaryBuilderPath(ctx SingletonContext) Path {
	primaryBuilder, err := filepath.Rel(ctx.Config().BuildDir(), os.Args[0])
	if err != nil {
		ctx.Errorf("path to primary builder %q is not in build dir %q",
			os.Args[0], ctx.Config().BuildDir())
	}

	return PathForOutput(ctx, primaryBuilder)
}

func (c *docsSingleton) GenerateBuildActions(ctx SingletonContext) {
	// Generate build system docs for the primary builder.  Generating docs reads the source
	// files used to build the primary builder, but that dependency will be picked up through
	// the dependency on the primary builder itself.  There are no dependencies on the
	// Blueprints files, as any relevant changes to the Blueprints files would have caused
	// a rebuild of the primary builder.
	docsFile := PathForOutput(ctx, "docs", "soong_build.html")
	primaryBuilder := primaryBuilderPath(ctx)
	soongDocs := ctx.Rule(pctx, "soongDocs",
		blueprint.RuleParams{
			Command: fmt.Sprintf("%s --soong_docs %s %s",
				primaryBuilder.String(), docsFile.String(), strings.Join(os.Args[1:], " ")),
			CommandDeps: []string{primaryBuilder.String()},
			Description: fmt.Sprintf("%s docs $out", primaryBuilder.Base()),
		})

	ctx.Build(pctx, BuildParams{
		Rule:   soongDocs,
		Output: docsFile,
	})

	// Add a phony target for building the documentation
	ctx.Build(pctx, BuildParams{
		Rule:   blueprint.Phony,
		Output: PathForPhony(ctx, "soong_docs"),
		Input:  docsFile,
	})
}
