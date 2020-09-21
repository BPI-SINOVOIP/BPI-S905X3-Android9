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

// ---------------------------------------------------------------------------

// Package wayland_protcool defines extension modules for the Soong build system
// to make it easier to generate code from a list of Wayland protocol files.
//
// The primary extension module is "wayland_protocol_codegen", which applies a
// code generation tool to a list of source protocol files.
//
// Note that the code generation done here is similar to what is done by the
// base Soong "gensrcs" module, but there are two functional differences:
//
//     1) The output filenames are computed from the input filenames, rather
//        than needing to be specified explicitly. An optional prefix as well
//        as a suffix can be added to the protocol filename (without extension).
//
//     2) Code generation is done for each file independently by emitting
//        multiple Ninja build commands, rather than one build command which
//        does it all.
package wayland_protocol

import (
	"fmt"
	"strings"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"

	"android/soong/android"
	"android/soong/genrule"
)

func init() {
	// Register out extension module type name with Soong.
	android.RegisterModuleType(
		"wayland_protocol_codegen", waylandCodegenModuleFactory)
}

var (
	// Create a context for build rule output from this package
	pctx = android.NewPackageContext("android/soong/external/wayland-protocol")
)

type hostToolDependencyTag struct {
	blueprint.BaseDependencyTag
}

var hostToolDepTag hostToolDependencyTag

// waylandCodegenProperties defines the properties that will be read in from the
// Android.bp file for each instantiation of the module.
type waylandCodegenProperties struct {
	// This string gives the command line template to run on each protocol file
	// to wayland_protocol_codegen.
	//
	// The string can contain one or more "$" prefixed variable names for
	// values that can vary. At a minimum you need to use ${location}, ${out}
	// and ${in}
	//
	//  $(location): the path to the first entry in tools or tool_files
	//  $(location <label>): the path to the tool or tool_file with name <label>
	//  $(in): A protocol file from srcs
	//  $(out): The constructed output filename from the protocol filename.
	//  $$: a literal $
	Cmd *string

	// The string to prepend to every protcol filename to generate the
	// corresponding output filename. The empty string by default.
	Prefix *string

	// The suffix to append to every protocol filename to generate the
	// corresponding output filename. The empty string by default.
	Suffix *string

	// The list of protocol files to process.
	Srcs []string

	// The names of any built host executables to use for code generation. Can
	// be left empty if a local script is used instead (specified in
	// tool_files).
	Tools []string

	// Local files that are used for code generation. Can be scripts to run, but
	// should also include any other files that the code generation step should
	// depend on that might be used by the code gen tool.
	Tool_files []string
}

// waylandGenModule defines the Soong module for each instance.
type waylandGenModule struct {
	android.ModuleBase

	// Store a copy of the parsed properties for easy reference.
	properties waylandCodegenProperties

	// Each module emits its own blueprint (Ninja) rule. Store a reference
	// to the one created for this instance.
	rule blueprint.Rule

	// Each module exports one or more include directories. Store the paths here
	// here for easy retrieval.
	exportedIncludeDirs android.Paths

	// Each module has a list of files it outputs, that can be used by other
	// modules. Store the list of paths here for easy reference.
	outputFiles android.Paths
}

// For the uninitiated, this is an idiom to check that a given object implements
// an interface. In this case we want to be sure that waylandGenModule
// implements genrule.SourceFileGenerator
var _ genrule.SourceFileGenerator = (*waylandGenModule)(nil)

// Check that we implement android.SourceFileProducer
var _ android.SourceFileProducer = (*waylandGenModule)(nil)

// GeneratedSourceFiles implements the genrule.SourceFileGenerator
// GeneratedSourceFiles method to return the list of generated source files.
func (g *waylandGenModule) GeneratedSourceFiles() android.Paths {
	return g.outputFiles
}

// GeneratedHeaderDirs implements the genrule.SourceFileGenerator
// GeneratedHeaderDirs method to return the list of exported include
// directories.
func (g *waylandGenModule) GeneratedHeaderDirs() android.Paths {
	return g.exportedIncludeDirs
}

// GeneratedDeps implements the genrule.SourceFileGenerator GeneratedDeps
// method to return the list of files to be used as dependencies when using
// GeneratedHeaderDirs.
func (g *waylandGenModule) GeneratedDeps() android.Paths {
	return g.outputFiles
}

// Srcs implements the android.SourceFileProducer Srcs method to return the list
// of source files.
func (g *waylandGenModule) Srcs() android.Paths {
	return g.outputFiles
}

// DepsMutator implements the android.Module DepsMutator method to apply a
// mutator context to the build graph.
func (g *waylandGenModule) DepsMutator(ctx android.BottomUpMutatorContext) {
	// This implementatoin duplicates the one from genrule.go, where gensrcs is
	// defined.
	android.ExtractSourcesDeps(ctx, g.properties.Srcs)
	if g, ok := ctx.Module().(*waylandGenModule); ok {
		if len(g.properties.Tools) > 0 {
			ctx.AddFarVariationDependencies([]blueprint.Variation{
				{"arch", ctx.AConfig().BuildOsVariant},
			}, hostToolDepTag, g.properties.Tools...)
		}
	}
}

// GenerateAndroidBuildActions implements the android.Module
// GenerateAndroidBuildActions method, which generates all the rules and builds
// commands used by this module instance.
func (g *waylandGenModule) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	if len(g.properties.Tools) == 0 && len(g.properties.Tool_files) == 0 {
		ctx.ModuleErrorf("at least one `tools` or `tool_files` is required")
		return
	}

	// Prepare the list of tools that were defined for codegen purposes.
	tools, implicitDeps := g.prepareTools(ctx)

	if ctx.Failed() {
		return
	}

	// Emit the rule for generating for processing each source file
	g.emitRule(ctx, tools)

	if ctx.Failed() {
		return
	}

	generatedFilenamePrefix := proptools.String(g.properties.Prefix)
	generatedFilenameSuffix := proptools.String(g.properties.Suffix)
	for _, src := range ctx.ExpandSources(g.properties.Srcs, nil) {
		out := g.generateOutputPath(ctx, src, generatedFilenamePrefix, generatedFilenameSuffix)
		if out == nil {
			continue
		}

		g.emitBuild(ctx, src, out, implicitDeps)
		g.outputFiles = append(g.outputFiles, out)
	}

	g.exportedIncludeDirs = append(g.exportedIncludeDirs, android.PathForModuleGen(ctx))
}

// genrateOutputPath takes an source path, a prefix, and a suffix, and use them
// to generate and return corresponding an output file path.
func (g *waylandGenModule) generateOutputPath(ctx android.ModuleContext, src android.Path, prefix string, suffix string) android.WritablePath {
	// Construct a new filename by adding the requested prefix and suffix for this
	// code generator instance. If the input file name is "wayland.xml", and the
	// properties specify a prefix of "test-" and a suffix of "-client.cpp", we
	// will end up with a fulename of "test-wayland-client.cpp"
	protocolFilename, protocolExt := splitExt(src.Base())
	if protocolExt != ".xml" {
		ctx.ModuleErrorf("Source file %q does not end with .xml", src)
		return nil
	}
	return android.PathForModuleGen(ctx, prefix+protocolFilename+suffix)
}

// emitRule is an internal function to emit each Ninja rule.
func (g *waylandGenModule) emitRule(ctx android.ModuleContext, tools map[string]android.Path) {
	// Get the command to run to process each protocol file. Since everything
	// should be templated, we generate a Ninja rule that uses the command,
	// and invoke it from each Ninja build command we emit.
	g.rule = ctx.Rule(pctx, "generator", blueprint.RuleParams{
		Command: g.expandCmd(ctx, tools),
	})
}

// emitBuild is an internal function to emit each Build command.
func (g *waylandGenModule) emitBuild(ctx android.ModuleContext, src android.Path, out android.WritablePath, implicitDeps android.Paths) android.Path {
	ctx.Build(pctx, android.BuildParams{
		Rule:        g.rule,
		Description: "generate " + out.Base(),
		Output:      out,
		Inputs:      android.Paths{src},
		Implicits:   implicitDeps,
	})

	return out
}

// prepareTools is an internal function to prepare a list of tools.
func (g *waylandGenModule) prepareTools(ctx android.ModuleContext) (tools map[string]android.Path, implicitDeps android.Paths) {
	tools = map[string]android.Path{}

	// This was extracted and slightly simplifed from equivalent code in
	// genrule.go.

	// For each entry in "tool", walk the dependency graph to get more
	// information about it.
	if len(g.properties.Tools) > 0 {
		ctx.VisitDirectDepsBlueprint(func(module blueprint.Module) {
			switch ctx.OtherModuleDependencyTag(module) {
			case android.SourceDepTag:
				// Nothing to do
			case hostToolDepTag:
				tool := ctx.OtherModuleName(module)
				var path android.OptionalPath

				if t, ok := module.(genrule.HostToolProvider); ok {
					if !t.(android.Module).Enabled() {
						if ctx.AConfig().AllowMissingDependencies() {
							ctx.AddMissingDependencies([]string{tool})
						} else {
							ctx.ModuleErrorf("depends on disabled module %q", tool)
						}
						break
					}
					path = t.HostToolPath()
				} else {
					ctx.ModuleErrorf("%q is not a host tool provider", tool)
					break
				}

				if path.Valid() {
					implicitDeps = append(implicitDeps, path.Path())
					if _, exists := tools[tool]; !exists {
						tools[tool] = path.Path()
					} else {
						ctx.ModuleErrorf("multiple tools for %q, %q and %q", tool, tools[tool], path.Path().String())
					}
				} else {
					ctx.ModuleErrorf("host tool %q missing output file", tool)
				}
			default:
				ctx.ModuleErrorf("unknown dependency on %q", ctx.OtherModuleName(module))
			}
		})
	}

	// Get more information about each entry in "tool_files".
	for _, tool := range g.properties.Tool_files {
		toolPath := android.PathForModuleSrc(ctx, tool)
		implicitDeps = append(implicitDeps, toolPath)
		if _, exists := tools[tool]; !exists {
			tools[tool] = toolPath
		} else {
			ctx.ModuleErrorf("multiple tools for %q, %q and %q", tool, tools[tool], toolPath.String())
		}
	}
	return
}

// expandCmd is an internal function to do some expansion and any additional
// wrapping of the generator command line. Returns the command line to use and
// an error value.
func (g *waylandGenModule) expandCmd(ctx android.ModuleContext, tools map[string]android.Path) (cmd string) {
	cmd, err := android.Expand(proptools.String(g.properties.Cmd), func(name string) (string, error) {
		switch name {
		case "in":
			return "$in", nil
		case "out":
			// We need to use the sandbox out path instead
			//return "$sandboxOut", nil
			return "$out", nil
		case "location":
			if len(g.properties.Tools) > 0 {
				return tools[g.properties.Tools[0]].String(), nil
			} else {
				return tools[g.properties.Tool_files[0]].String(), nil
			}
		default:
			if strings.HasPrefix(name, "location ") {
				label := strings.TrimSpace(strings.TrimPrefix(name, "location "))
				if tool, ok := tools[label]; ok {
					return tool.String(), nil
				} else {
					return "", fmt.Errorf("unknown location label %q", label)
				}
			}
			return "", fmt.Errorf("unknown variable '$(%s)'", name)
		}
	})
	if err != nil {
		ctx.PropertyErrorf("cmd", "%s", err.Error())
	}
	return
}

// waylandCodegenModuleFactory creates an extension module instance.
func waylandCodegenModuleFactory() android.Module {
	m := &waylandGenModule{}
	m.AddProperties(&m.properties)
	android.InitAndroidModule(m)
	return m
}

// splitExt splits a base filename into (filename, ext) components, such that
// input == filename + ext
func splitExt(input string) (filename string, ext string) {
	// There is no filepath.SplitExt() or equivalent.
	dot := strings.LastIndex(input, ".")
	if dot != -1 {
		ext = input[dot:]
		filename = input[:dot]
	} else {
		ext = ""
		filename = input
	}
	return
}
