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

package build

import (
	"bufio"
	"path/filepath"
	"runtime"
	"strings"
)

// Checks for files in the out directory that have a rule that depends on them but no rule to
// create them. This catches a common set of build failures where a rule to generate a file is
// deleted (either by deleting a module in an Android.mk file, or by modifying the build system
// incorrectly).  These failures are often not caught by a local incremental build because the
// previously built files are still present in the output directory.
func testForDanglingRules(ctx Context, config Config) {
	// Many modules are disabled on mac.  Checking for dangling rules would cause lots of build
	// breakages, and presubmit wouldn't catch them, so just disable the check.
	if runtime.GOOS != "linux" {
		return
	}

	ctx.BeginTrace("test for dangling rules")
	defer ctx.EndTrace()

	// Get a list of leaf nodes in the dependency graph from ninja
	executable := config.PrebuiltBuildTool("ninja")

	args := []string{}
	args = append(args, config.NinjaArgs()...)
	args = append(args, "-f", config.CombinedNinjaFile())
	args = append(args, "-t", "targets", "rule")

	cmd := Command(ctx, config, "ninja", executable, args...)
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		ctx.Fatal(err)
	}

	cmd.StartOrFatal()

	outDir := config.OutDir()
	bootstrapDir := filepath.Join(outDir, "soong", ".bootstrap")
	miniBootstrapDir := filepath.Join(outDir, "soong", ".minibootstrap")

	var danglingRules []string

	scanner := bufio.NewScanner(stdout)
	for scanner.Scan() {
		line := scanner.Text()
		if !strings.HasPrefix(line, outDir) {
			// Leaf node is not in the out directory.
			continue
		}
		if strings.HasPrefix(line, bootstrapDir) || strings.HasPrefix(line, miniBootstrapDir) {
			// Leaf node is in one of Soong's bootstrap directories, which do not have
			// full build rules in the primary build.ninja file.
			continue
		}
		danglingRules = append(danglingRules, line)
	}

	cmd.WaitOrFatal()

	if len(danglingRules) > 0 {
		ctx.Println("Dependencies in out found with no rule to create them:")
		for _, dep := range danglingRules {
			ctx.Println(dep)
		}
		ctx.Fatal("")
	}
}
