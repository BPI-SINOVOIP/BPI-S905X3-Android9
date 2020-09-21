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

package android

import (
	"github.com/google/blueprint"
	"github.com/google/blueprint/pathtools"
)

// SingletonContext
type SingletonContext interface {
	Config() Config

	ModuleName(module blueprint.Module) string
	ModuleDir(module blueprint.Module) string
	ModuleSubDir(module blueprint.Module) string
	ModuleType(module blueprint.Module) string
	BlueprintFile(module blueprint.Module) string

	ModuleErrorf(module blueprint.Module, format string, args ...interface{})
	Errorf(format string, args ...interface{})
	Failed() bool

	Variable(pctx PackageContext, name, value string)
	Rule(pctx PackageContext, name string, params blueprint.RuleParams, argNames ...string) blueprint.Rule
	Build(pctx PackageContext, params BuildParams)
	RequireNinjaVersion(major, minor, micro int)

	// SetNinjaBuildDir sets the value of the top-level "builddir" Ninja variable
	// that controls where Ninja stores its build log files.  This value can be
	// set at most one time for a single build, later calls are ignored.
	SetNinjaBuildDir(pctx PackageContext, value string)

	// Eval takes a string with embedded ninja variables, and returns a string
	// with all of the variables recursively expanded. Any variables references
	// are expanded in the scope of the PackageContext.
	Eval(pctx PackageContext, ninjaStr string) (string, error)

	VisitAllModules(visit func(Module))
	VisitAllModulesIf(pred func(Module) bool, visit func(Module))
	VisitDepsDepthFirst(module Module, visit func(Module))
	VisitDepsDepthFirstIf(module Module, pred func(Module) bool,
		visit func(Module))

	VisitAllModuleVariants(module Module, visit func(Module))

	PrimaryModule(module Module) Module
	FinalModule(module Module) Module

	AddNinjaFileDeps(deps ...string)

	// GlobWithDeps returns a list of files that match the specified pattern but do not match any
	// of the patterns in excludes.  It also adds efficient dependencies to rerun the primary
	// builder whenever a file matching the pattern as added or removed, without rerunning if a
	// file that does not match the pattern is added to a searched directory.
	GlobWithDeps(pattern string, excludes []string) ([]string, error)

	Fs() pathtools.FileSystem
}

type singletonAdaptor struct {
	Singleton
}

func (s singletonAdaptor) GenerateBuildActions(ctx blueprint.SingletonContext) {
	s.Singleton.GenerateBuildActions(singletonContextAdaptor{ctx})
}

type Singleton interface {
	GenerateBuildActions(SingletonContext)
}

type singletonContextAdaptor struct {
	blueprint.SingletonContext
}

func (s singletonContextAdaptor) Config() Config {
	return s.SingletonContext.Config().(Config)
}

func (s singletonContextAdaptor) Variable(pctx PackageContext, name, value string) {
	s.SingletonContext.Variable(pctx.PackageContext, name, value)
}

func (s singletonContextAdaptor) Rule(pctx PackageContext, name string, params blueprint.RuleParams, argNames ...string) blueprint.Rule {
	return s.SingletonContext.Rule(pctx.PackageContext, name, params, argNames...)
}

func (s singletonContextAdaptor) Build(pctx PackageContext, params BuildParams) {
	bparams := convertBuildParams(params)
	s.SingletonContext.Build(pctx.PackageContext, bparams)

}

func (s singletonContextAdaptor) SetNinjaBuildDir(pctx PackageContext, value string) {
	s.SingletonContext.SetNinjaBuildDir(pctx.PackageContext, value)
}

func (s singletonContextAdaptor) Eval(pctx PackageContext, ninjaStr string) (string, error) {
	return s.SingletonContext.Eval(pctx.PackageContext, ninjaStr)
}

// visitAdaptor wraps a visit function that takes an android.Module parameter into
// a function that takes an blueprint.Module parameter and only calls the visit function if the
// blueprint.Module is an android.Module.
func visitAdaptor(visit func(Module)) func(blueprint.Module) {
	return func(module blueprint.Module) {
		if aModule, ok := module.(Module); ok {
			visit(aModule)
		}
	}
}

// predAdaptor wraps a pred function that takes an android.Module parameter
// into a function that takes an blueprint.Module parameter and only calls the visit function if the
// blueprint.Module is an android.Module, otherwise returns false.
func predAdaptor(pred func(Module) bool) func(blueprint.Module) bool {
	return func(module blueprint.Module) bool {
		if aModule, ok := module.(Module); ok {
			return pred(aModule)
		} else {
			return false
		}
	}
}

func (s singletonContextAdaptor) VisitAllModules(visit func(Module)) {
	s.SingletonContext.VisitAllModules(visitAdaptor(visit))
}

func (s singletonContextAdaptor) VisitAllModulesIf(pred func(Module) bool, visit func(Module)) {
	s.SingletonContext.VisitAllModulesIf(predAdaptor(pred), visitAdaptor(visit))
}

func (s singletonContextAdaptor) VisitDepsDepthFirst(module Module, visit func(Module)) {
	s.SingletonContext.VisitDepsDepthFirst(module, visitAdaptor(visit))
}

func (s singletonContextAdaptor) VisitDepsDepthFirstIf(module Module, pred func(Module) bool, visit func(Module)) {
	s.SingletonContext.VisitDepsDepthFirstIf(module, predAdaptor(pred), visitAdaptor(visit))
}

func (s singletonContextAdaptor) VisitAllModuleVariants(module Module, visit func(Module)) {
	s.SingletonContext.VisitAllModuleVariants(module, visitAdaptor(visit))
}

func (s singletonContextAdaptor) PrimaryModule(module Module) Module {
	return s.SingletonContext.PrimaryModule(module).(Module)
}

func (s singletonContextAdaptor) FinalModule(module Module) Module {
	return s.SingletonContext.FinalModule(module).(Module)
}
