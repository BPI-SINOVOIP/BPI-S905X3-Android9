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

package hidl

import (
	"strings"
	"sync"

	"github.com/google/blueprint/proptools"

	"android/soong/android"
	"android/soong/cc"
	"android/soong/genrule"
	"android/soong/java"
)

var (
	hidlInterfaceSuffix = "_interface"
)

func init() {
	android.RegisterModuleType("hidl_interface", hidlInterfaceFactory)
}

type hidlInterfaceProperties struct {
	// Vndk properties for interface library only.
	cc.VndkProperties

	// The owner of the module
	Owner *string

	// List of .hal files which compose this interface.
	Srcs []string

	// List of hal interface packages that this library depends on.
	Interfaces []string

	// Package root for this package, must be a prefix of name
	Root string

	// List of non-TypeDef types declared in types.hal.
	Types []string

	// Whether to generate the Java library stubs.
	// Default: true
	Gen_java *bool

	// Whether to generate a Java library containing constants
	// expressed by @export annotations in the hal files.
	Gen_java_constants bool

	// Don't generate "android.hidl.foo@1.0" C library. Instead
	// only generate the genrules so that this package can be
	// included in libhidltransport.
	Core_interface bool
}

type hidlInterface struct {
	android.ModuleBase

	properties hidlInterfaceProperties
}

func processSources(mctx android.LoadHookContext, srcs []string) ([]string, []string, bool) {
	var interfaces []string
	var types []string // hidl-gen only supports types.hal, but don't assume that here

	hasError := false

	for _, v := range srcs {
		if !strings.HasSuffix(v, ".hal") {
			mctx.PropertyErrorf("srcs", "Source must be a .hal file: "+v)
			hasError = true
			continue
		}

		name := strings.TrimSuffix(v, ".hal")

		if strings.HasPrefix(name, "I") {
			baseName := strings.TrimPrefix(name, "I")
			interfaces = append(interfaces, baseName)
		} else {
			types = append(types, name)
		}
	}

	return interfaces, types, !hasError
}

func processDependencies(mctx android.LoadHookContext, interfaces []string) ([]string, []string, bool) {
	var dependencies []string
	var javaDependencies []string

	hasError := false

	for _, v := range interfaces {
		name, err := parseFqName(v)
		if err != nil {
			mctx.PropertyErrorf("interfaces", err.Error())
			hasError = true
			continue
		}
		dependencies = append(dependencies, name.string())
		javaDependencies = append(javaDependencies, name.javaName())
	}

	return dependencies, javaDependencies, !hasError
}

func getRootList(mctx android.LoadHookContext, interfaces []string) ([]string, bool) {
	var roots []string
	hasError := false

	for _, i := range interfaces {
		interfaceObject := lookupInterface(i)
		if interfaceObject == nil {
			mctx.PropertyErrorf("interfaces", "Cannot find interface "+i)
			hasError = true
			continue
		}
		root := interfaceObject.properties.Root
		rootObject := lookupPackageRoot(root)
		if rootObject == nil {
			mctx.PropertyErrorf("interfaces", `Cannot find package root specification for package `+
				`root '%s' needed for module '%s'. Either this is a mispelling of the package `+
				`root, or a new hidl_package_root module needs to be added. For example, you can `+
				`fix this error by adding the following to <some path>/Android.bp:

hidl_package_root {
    name: "%s",
    path: "<some path>",
}

This corresponds to the "-r%s:<some path>" option that would be passed into hidl-gen.`, root, i, root, root)
			hasError = true
			continue
		}

		roots = append(roots, root+":"+rootObject.properties.Path)
	}

	return android.FirstUniqueStrings(roots), !hasError
}

func removeCoreDependencies(mctx android.LoadHookContext, dependencies []string) ([]string, bool) {
	var ret []string
	hasError := false

	for _, i := range dependencies {
		interfaceObject := lookupInterface(i)
		if interfaceObject == nil {
			mctx.PropertyErrorf("interfaces", "Cannot find interface "+i)
			hasError = true
			continue
		}

		if !interfaceObject.properties.Core_interface {
			ret = append(ret, i)
		}
	}

	return ret, !hasError
}

func hidlGenCommand(lang string, roots []string, name *fqName) *string {
	cmd := "$(location hidl-gen) -d $(depfile) -o $(genDir)"
	cmd += " -L" + lang
	cmd += " " + strings.Join(wrap("-r", roots, ""), " ")
	cmd += " " + name.string()
	return &cmd
}

func hidlInterfaceMutator(mctx android.LoadHookContext, i *hidlInterface) {
	name, err := parseFqName(i.ModuleBase.Name())
	if err != nil {
		mctx.PropertyErrorf("name", err.Error())
	}

	if !name.inPackage(i.properties.Root) {
		mctx.PropertyErrorf("root", "Root, "+i.properties.Root+", for "+name.string()+" must be a prefix.")
	}

	interfaces, types, _ := processSources(mctx, i.properties.Srcs)

	if len(interfaces) == 0 && len(types) == 0 {
		mctx.PropertyErrorf("srcs", "No sources provided.")
	}

	dependencies, javaDependencies, _ := processDependencies(mctx, i.properties.Interfaces)
	roots, _ := getRootList(mctx, append(dependencies, name.string()))
	cppDependencies, _ := removeCoreDependencies(mctx, dependencies)

	if mctx.Failed() {
		return
	}

	shouldGenerateLibrary := !i.properties.Core_interface
	// explicitly true if not specified to give early warning to devs
	shouldGenerateJava := i.properties.Gen_java == nil || *i.properties.Gen_java
	shouldGenerateJavaConstants := i.properties.Gen_java_constants

	var libraryIfExists []string
	if shouldGenerateLibrary {
		libraryIfExists = []string{name.string()}
	}

	// TODO(b/69002743): remove filegroups
	mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.FileGroupFactory), &fileGroupProperties{
		Name:  proptools.StringPtr(name.fileGroupName()),
		Owner: i.properties.Owner,
		Srcs:  i.properties.Srcs,
	})

	mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.GenRuleFactory), &genruleProperties{
		Name:    proptools.StringPtr(name.sourcesName()),
		Depfile: proptools.BoolPtr(true),
		Owner:   i.properties.Owner,
		Tools:   []string{"hidl-gen"},
		Cmd:     hidlGenCommand("c++-sources", roots, name),
		Srcs:    i.properties.Srcs,
		Out: concat(wrap(name.dir(), interfaces, "All.cpp"),
			wrap(name.dir(), types, ".cpp")),
	})
	mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.GenRuleFactory), &genruleProperties{
		Name:    proptools.StringPtr(name.headersName()),
		Depfile: proptools.BoolPtr(true),
		Owner:   i.properties.Owner,
		Tools:   []string{"hidl-gen"},
		Cmd:     hidlGenCommand("c++-headers", roots, name),
		Srcs:    i.properties.Srcs,
		Out: concat(wrap(name.dir()+"I", interfaces, ".h"),
			wrap(name.dir()+"Bs", interfaces, ".h"),
			wrap(name.dir()+"BnHw", interfaces, ".h"),
			wrap(name.dir()+"BpHw", interfaces, ".h"),
			wrap(name.dir()+"IHw", interfaces, ".h"),
			wrap(name.dir(), types, ".h"),
			wrap(name.dir()+"hw", types, ".h")),
	})

	if shouldGenerateLibrary {
		mctx.CreateModule(android.ModuleFactoryAdaptor(cc.LibraryFactory), &ccProperties{
			Name:              proptools.StringPtr(name.string()),
			Owner:             i.properties.Owner,
			Vendor_available:  proptools.BoolPtr(true),
			Defaults:          []string{"hidl-module-defaults"},
			Generated_sources: []string{name.sourcesName()},
			Generated_headers: []string{name.headersName()},
			Shared_libs: concat(cppDependencies, []string{
				"libhidlbase",
				"libhidltransport",
				"libhwbinder",
				"liblog",
				"libutils",
				"libcutils",
			}),
			Export_shared_lib_headers: concat(cppDependencies, []string{
				"libhidlbase",
				"libhidltransport",
				"libhwbinder",
				"libutils",
			}),
			Export_generated_headers: []string{name.headersName()},
		}, &i.properties.VndkProperties)
	}

	if shouldGenerateJava {
		mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.GenRuleFactory), &genruleProperties{
			Name:    proptools.StringPtr(name.javaSourcesName()),
			Depfile: proptools.BoolPtr(true),
			Owner:   i.properties.Owner,
			Tools:   []string{"hidl-gen"},
			Cmd:     hidlGenCommand("java", roots, name),
			Srcs:    i.properties.Srcs,
			Out: concat(wrap(name.sanitizedDir()+"I", interfaces, ".java"),
				wrap(name.sanitizedDir(), i.properties.Types, ".java")),
		})
		mctx.CreateModule(android.ModuleFactoryAdaptor(java.LibraryFactory(true)), &javaProperties{
			Name:              proptools.StringPtr(name.javaName()),
			Owner:             i.properties.Owner,
			Sdk_version:       proptools.StringPtr("system_current"),
			Defaults:          []string{"hidl-java-module-defaults"},
			No_framework_libs: proptools.BoolPtr(true),
			Srcs:              []string{":" + name.javaSourcesName()},
			Static_libs:       append(javaDependencies, "hwbinder"),
		})
	}

	if shouldGenerateJavaConstants {
		mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.GenRuleFactory), &genruleProperties{
			Name:    proptools.StringPtr(name.javaConstantsSourcesName()),
			Depfile: proptools.BoolPtr(true),
			Owner:   i.properties.Owner,
			Tools:   []string{"hidl-gen"},
			Cmd:     hidlGenCommand("java-constants", roots, name),
			Srcs:    i.properties.Srcs,
			Out:     []string{name.sanitizedDir() + "Constants.java"},
		})
		mctx.CreateModule(android.ModuleFactoryAdaptor(java.LibraryFactory(true)), &javaProperties{
			Name:              proptools.StringPtr(name.javaConstantsName()),
			Owner:             i.properties.Owner,
			Defaults:          []string{"hidl-java-module-defaults"},
			No_framework_libs: proptools.BoolPtr(true),
			Srcs:              []string{":" + name.javaConstantsSourcesName()},
		})
	}

	mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.GenRuleFactory), &genruleProperties{
		Name:    proptools.StringPtr(name.adapterHelperSourcesName()),
		Depfile: proptools.BoolPtr(true),
		Owner:   i.properties.Owner,
		Tools:   []string{"hidl-gen"},
		Cmd:     hidlGenCommand("c++-adapter-sources", roots, name),
		Srcs:    i.properties.Srcs,
		Out:     wrap(name.dir()+"A", concat(interfaces, types), ".cpp"),
	})
	mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.GenRuleFactory), &genruleProperties{
		Name:    proptools.StringPtr(name.adapterHelperHeadersName()),
		Depfile: proptools.BoolPtr(true),
		Owner:   i.properties.Owner,
		Tools:   []string{"hidl-gen"},
		Cmd:     hidlGenCommand("c++-adapter-headers", roots, name),
		Srcs:    i.properties.Srcs,
		Out:     wrap(name.dir()+"A", concat(interfaces, types), ".h"),
	})

	mctx.CreateModule(android.ModuleFactoryAdaptor(cc.LibraryFactory), &ccProperties{
		Name:              proptools.StringPtr(name.adapterHelperName()),
		Owner:             i.properties.Owner,
		Vendor_available:  proptools.BoolPtr(true),
		Defaults:          []string{"hidl-module-defaults"},
		Generated_sources: []string{name.adapterHelperSourcesName()},
		Generated_headers: []string{name.adapterHelperHeadersName()},
		Shared_libs: []string{
			"libbase",
			"libcutils",
			"libhidlbase",
			"libhidltransport",
			"libhwbinder",
			"liblog",
			"libutils",
		},
		Static_libs: concat([]string{
			"libhidladapter",
		}, wrap("", dependencies, "-adapter-helper"), cppDependencies, libraryIfExists),
		Export_shared_lib_headers: []string{
			"libhidlbase",
			"libhidltransport",
		},
		Export_static_lib_headers: concat([]string{
			"libhidladapter",
		}, wrap("", dependencies, "-adapter-helper"), cppDependencies, libraryIfExists),
		Export_generated_headers: []string{name.adapterHelperHeadersName()},
		Group_static_libs:        proptools.BoolPtr(true),
	})
	mctx.CreateModule(android.ModuleFactoryAdaptor(genrule.GenRuleFactory), &genruleProperties{
		Name:    proptools.StringPtr(name.adapterSourcesName()),
		Depfile: proptools.BoolPtr(true),
		Owner:   i.properties.Owner,
		Tools:   []string{"hidl-gen"},
		Cmd:     hidlGenCommand("c++-adapter-main", roots, name),
		Srcs:    i.properties.Srcs,
		Out:     []string{"main.cpp"},
	})
	mctx.CreateModule(android.ModuleFactoryAdaptor(cc.TestFactory), &ccProperties{
		Name:              proptools.StringPtr(name.adapterName()),
		Owner:             i.properties.Owner,
		Generated_sources: []string{name.adapterSourcesName()},
		Shared_libs: []string{
			"libbase",
			"libcutils",
			"libhidlbase",
			"libhidltransport",
			"libhwbinder",
			"liblog",
			"libutils",
		},
		Static_libs: concat([]string{
			"libhidladapter",
			name.adapterHelperName(),
		}, wrap("", dependencies, "-adapter-helper"), cppDependencies, libraryIfExists),
		Group_static_libs: proptools.BoolPtr(true),
	})
}

func (h *hidlInterface) Name() string {
	return h.ModuleBase.Name() + hidlInterfaceSuffix
}
func (h *hidlInterface) GenerateAndroidBuildActions(ctx android.ModuleContext) {
}
func (h *hidlInterface) DepsMutator(ctx android.BottomUpMutatorContext) {
}

var hidlInterfaceMutex sync.Mutex
var hidlInterfaces []*hidlInterface

func hidlInterfaceFactory() android.Module {
	i := &hidlInterface{}
	i.AddProperties(&i.properties)
	android.InitAndroidModule(i)
	android.AddLoadHook(i, func(ctx android.LoadHookContext) { hidlInterfaceMutator(ctx, i) })

	hidlInterfaceMutex.Lock()
	hidlInterfaces = append(hidlInterfaces, i)
	hidlInterfaceMutex.Unlock()

	return i
}

func lookupInterface(name string) *hidlInterface {
	for _, i := range hidlInterfaces {
		if i.ModuleBase.Name() == name {
			return i
		}
	}
	return nil
}
