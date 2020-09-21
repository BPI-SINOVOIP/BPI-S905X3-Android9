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

package llvm

import (
	"android/soong/android"
	"android/soong/cc"

	"github.com/google/blueprint/proptools"
)

func globalFlags(ctx android.BaseContext) []string {
	var cflags []string

	if ctx.AConfig().IsEnvTrue("FORCE_BUILD_LLVM_DISABLE_NDEBUG") {
		cflags = append(cflags, "-D_DEBUG", "-UNDEBUG")
	}

	return cflags
}

func deviceFlags(ctx android.BaseContext) []string {
	var cflags []string

	return cflags
}

func hostFlags(ctx android.BaseContext) []string {
	var cflags []string

	if ctx.AConfig().IsEnvTrue("FORCE_BUILD_LLVM_DEBUG") {
		cflags = append(cflags, "-O0", "-g")
	}

	return cflags
}

func llvmDefaults(ctx android.LoadHookContext) {
	type props struct {
		Target struct {
			Android struct {
				Cflags  []string
				Enabled *bool
			}
			Host struct {
				Enabled *bool
			}
			Not_windows struct {
				Cflags []string
			}
		}
		Cflags []string
	}

	p := &props{}
	p.Cflags = globalFlags(ctx)
	p.Target.Android.Cflags = deviceFlags(ctx)
	// Mingw fails to link binaries with lots of debug information
	p.Target.Not_windows.Cflags = hostFlags(ctx)

	if ctx.AConfig().IsEnvTrue("DISABLE_LLVM_DEVICE_BUILDS") {
		p.Target.Android.Enabled = proptools.BoolPtr(false)
	}

	ctx.AppendProperties(p)
}

func forceBuildLlvmComponents(ctx android.LoadHookContext) {
	if !ctx.AConfig().IsEnvTrue("FORCE_BUILD_LLVM_COMPONENTS") {
		type props struct {
			Target struct {
				Host struct {
					Enabled *bool
				}
			}
		}
		p := &props{}
		p.Target.Host.Enabled = proptools.BoolPtr(false)
		ctx.AppendProperties(p)
	}
}

func init() {
	android.RegisterModuleType("llvm_defaults", llvmDefaultsFactory)
	android.RegisterModuleType("force_build_llvm_components_defaults", forceBuildLlvmComponentsDefaultsFactory)
}

func llvmDefaultsFactory() android.Module {
	module := cc.DefaultsFactory()
	android.AddLoadHook(module, llvmDefaults)

	return module
}

func forceBuildLlvmComponentsDefaultsFactory() android.Module {
	module := cc.DefaultsFactory()
	android.AddLoadHook(module, forceBuildLlvmComponents)
	return module
}
