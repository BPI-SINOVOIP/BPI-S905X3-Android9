// Copyright 2018 Google Inc. All rights reserved.
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

import (
	"android/soong/android"
	"android/soong/java/config"
	"fmt"
	"strings"

	"github.com/google/blueprint"
)

var (
	javadoc = pctx.AndroidStaticRule("javadoc",
		blueprint.RuleParams{
			Command: `rm -rf "$outDir" "$srcJarDir" "$stubsDir" && mkdir -p "$outDir" "$srcJarDir" "$stubsDir" && ` +
				`${config.ZipSyncCmd} -d $srcJarDir -l $srcJarDir/list -f "*.java" $srcJars && ` +
				`${config.JavadocCmd} -encoding UTF-8 @$out.rsp @$srcJarDir/list ` +
				`$opts $bootclasspathArgs $classpathArgs -sourcepath $sourcepath ` +
				`-d $outDir -quiet  && ` +
				`${config.SoongZipCmd} -write_if_changed -d -o $docZip -C $outDir -D $outDir && ` +
				`${config.SoongZipCmd} -write_if_changed -jar -o $out -C $stubsDir -D $stubsDir`,
			CommandDeps: []string{
				"${config.ZipSyncCmd}",
				"${config.JavadocCmd}",
				"${config.SoongZipCmd}",
				"$JsilverJar",
				"$DoclavaJar",
			},
			Rspfile:        "$out.rsp",
			RspfileContent: "$in",
			Restat:         true,
		},
		"outDir", "srcJarDir", "stubsDir", "srcJars", "opts",
		"bootclasspathArgs", "classpathArgs", "sourcepath", "docZip", "JsilverJar", "DoclavaJar")
)

func init() {
	android.RegisterModuleType("droiddoc", DroiddocFactory)
	android.RegisterModuleType("droiddoc_host", DroiddocHostFactory)
	android.RegisterModuleType("droiddoc_template", DroiddocTemplateFactory)
	android.RegisterModuleType("javadoc", JavadocFactory)
	android.RegisterModuleType("javadoc_host", JavadocHostFactory)
}

type JavadocProperties struct {
	// list of source files used to compile the Java module.  May be .java, .logtags, .proto,
	// or .aidl files.
	Srcs []string `android:"arch_variant"`

	// list of directories rooted at the Android.bp file that will
	// be added to the search paths for finding source files when passing package names.
	Local_sourcepaths []string `android:"arch_variant"`

	// list of source files that should not be used to build the Java module.
	// This is most useful in the arch/multilib variants to remove non-common files
	// filegroup or genrule can be included within this property.
	Exclude_srcs []string `android:"arch_variant"`

	// list of of java libraries that will be in the classpath.
	Libs []string `android:"arch_variant"`

	// If set to false, don't allow this module(-docs.zip) to be exported. Defaults to true.
	Installable *bool `android:"arch_variant"`

	// if not blank, set to the version of the sdk to compile against
	Sdk_version *string `android:"arch_variant"`
}

type DroiddocProperties struct {
	// directory relative to top of the source tree that contains doc templates files.
	Custom_template *string `android:"arch_variant"`

	// directories relative to top of the source tree which contains html/jd files.
	Html_dirs []string `android:"arch_variant"`

	// set a value in the Clearsilver hdf namespace.
	Hdf []string `android:"arch_variant"`

	// proofread file contains all of the text content of the javadocs concatenated into one file,
	// suitable for spell-checking and other goodness.
	Proofread_file *string `android:"arch_variant"`

	// a todo file lists the program elements that are missing documentation.
	// At some point, this might be improved to show more warnings.
	Todo_file *string `android:"arch_variant"`

	// local files that are used within user customized droiddoc options.
	Arg_files []string `android:"arch_variant"`

	// user customized droiddoc args.
	// Available variables for substitution:
	//
	//  $(location <label>): the path to the arg_files with name <label>
	Args *string `android:"arch_variant"`

	// names of the output files used in args that will be generated
	Out []string `android:"arch_variant"`

	// a list of files under current module source dir which contains known tags in Java sources.
	// filegroup or genrule can be included within this property.
	Knowntags []string `android:"arch_variant"`
}

type Javadoc struct {
	android.ModuleBase
	android.DefaultableModuleBase

	properties JavadocProperties

	srcJars     android.Paths
	srcFiles    android.Paths
	sourcepaths android.Paths

	docZip   android.WritablePath
	stubsJar android.WritablePath
}

type Droiddoc struct {
	Javadoc

	properties DroiddocProperties
}

func InitDroiddocModule(module android.DefaultableModule, hod android.HostOrDeviceSupported) {
	android.InitAndroidArchModule(module, hod, android.MultilibCommon)
	android.InitDefaultableModule(module)
}

func JavadocFactory() android.Module {
	module := &Javadoc{}

	module.AddProperties(&module.properties)

	InitDroiddocModule(module, android.HostAndDeviceSupported)
	return module
}

func JavadocHostFactory() android.Module {
	module := &Javadoc{}

	module.AddProperties(&module.properties)

	InitDroiddocModule(module, android.HostSupported)
	return module
}

func DroiddocFactory() android.Module {
	module := &Droiddoc{}

	module.AddProperties(&module.properties,
		&module.Javadoc.properties)

	InitDroiddocModule(module, android.HostAndDeviceSupported)
	return module
}

func DroiddocHostFactory() android.Module {
	module := &Droiddoc{}

	module.AddProperties(&module.properties,
		&module.Javadoc.properties)

	InitDroiddocModule(module, android.HostSupported)
	return module
}

func (j *Javadoc) addDeps(ctx android.BottomUpMutatorContext) {
	if ctx.Device() {
		sdkDep := decodeSdkDep(ctx, String(j.properties.Sdk_version))
		if sdkDep.useDefaultLibs {
			ctx.AddDependency(ctx.Module(), bootClasspathTag, config.DefaultBootclasspathLibraries...)
			ctx.AddDependency(ctx.Module(), libTag, []string{"ext", "framework"}...)
		} else if sdkDep.useModule {
			ctx.AddDependency(ctx.Module(), bootClasspathTag, sdkDep.module)
		}
	}

	ctx.AddDependency(ctx.Module(), libTag, j.properties.Libs...)

	android.ExtractSourcesDeps(ctx, j.properties.Srcs)

	// exclude_srcs may contain filegroup or genrule.
	android.ExtractSourcesDeps(ctx, j.properties.Exclude_srcs)
}

func (j *Javadoc) collectDeps(ctx android.ModuleContext) deps {
	var deps deps

	sdkDep := decodeSdkDep(ctx, String(j.properties.Sdk_version))
	if sdkDep.invalidVersion {
		ctx.AddMissingDependencies([]string{sdkDep.module})
	} else if sdkDep.useFiles {
		deps.bootClasspath = append(deps.bootClasspath, sdkDep.jar)
	}

	ctx.VisitDirectDeps(func(module android.Module) {
		otherName := ctx.OtherModuleName(module)
		tag := ctx.OtherModuleDependencyTag(module)

		switch dep := module.(type) {
		case Dependency:
			switch tag {
			case bootClasspathTag:
				deps.bootClasspath = append(deps.bootClasspath, dep.ImplementationJars()...)
			case libTag:
				deps.classpath = append(deps.classpath, dep.ImplementationJars()...)
			default:
				panic(fmt.Errorf("unknown dependency %q for %q", otherName, ctx.ModuleName()))
			}
		case android.SourceFileProducer:
			switch tag {
			case libTag:
				checkProducesJars(ctx, dep)
				deps.classpath = append(deps.classpath, dep.Srcs()...)
			case android.DefaultsDepTag, android.SourceDepTag:
				// Nothing to do
			default:
				ctx.ModuleErrorf("dependency on genrule %q may only be in srcs, libs", otherName)
			}
		default:
			switch tag {
			case android.DefaultsDepTag, android.SourceDepTag, droiddocTemplateTag:
				// Nothing to do
			default:
				ctx.ModuleErrorf("depends on non-java module %q", otherName)
			}
		}
	})
	// do not pass exclude_srcs directly when expanding srcFiles since exclude_srcs
	// may contain filegroup or genrule.
	srcFiles := ctx.ExpandSources(j.properties.Srcs, j.properties.Exclude_srcs)

	// srcs may depend on some genrule output.
	j.srcJars = srcFiles.FilterByExt(".srcjar")
	j.srcFiles = srcFiles.FilterOutByExt(".srcjar")

	j.docZip = android.PathForModuleOut(ctx, ctx.ModuleName()+"-"+"docs.zip")
	j.stubsJar = android.PathForModuleOut(ctx, ctx.ModuleName()+"-"+"stubs.srcjar")

	if j.properties.Local_sourcepaths == nil {
		j.properties.Local_sourcepaths = append(j.properties.Local_sourcepaths, ".")
	}
	j.sourcepaths = android.PathsForModuleSrc(ctx, j.properties.Local_sourcepaths)
	j.sourcepaths = append(j.sourcepaths, deps.bootClasspath...)
	j.sourcepaths = append(j.sourcepaths, deps.classpath...)

	return deps
}

func (j *Javadoc) DepsMutator(ctx android.BottomUpMutatorContext) {
	j.addDeps(ctx)
}

func (j *Javadoc) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	deps := j.collectDeps(ctx)

	var implicits android.Paths
	implicits = append(implicits, deps.bootClasspath...)
	implicits = append(implicits, deps.classpath...)

	var bootClasspathArgs, classpathArgs string
	if ctx.Config().UseOpenJDK9() {
		if len(deps.bootClasspath) > 0 {
			// For OpenJDK 9 we use --patch-module to define the core libraries code.
			// TODO(tobiast): Reorganize this when adding proper support for OpenJDK 9
			// modules. Here we treat all code in core libraries as being in java.base
			// to work around the OpenJDK 9 module system. http://b/62049770
			bootClasspathArgs = "--patch-module=java.base=" + strings.Join(deps.bootClasspath.Strings(), ":")
		}
	} else {
		if len(deps.bootClasspath.Strings()) > 0 {
			// For OpenJDK 8 we can use -bootclasspath to define the core libraries code.
			bootClasspathArgs = deps.bootClasspath.FormJavaClassPath("-bootclasspath")
		}
	}
	if len(deps.classpath.Strings()) > 0 {
		classpathArgs = "-classpath " + strings.Join(deps.classpath.Strings(), ":")
	}

	implicits = append(implicits, j.srcJars...)

	opts := "-J-Xmx1024m -XDignore.symbol.file -Xdoclint:none"

	ctx.Build(pctx, android.BuildParams{
		Rule:           javadoc,
		Description:    "Javadoc",
		Output:         j.stubsJar,
		ImplicitOutput: j.docZip,
		Inputs:         j.srcFiles,
		Implicits:      implicits,
		Args: map[string]string{
			"outDir":            android.PathForModuleOut(ctx, "docs", "out").String(),
			"srcJarDir":         android.PathForModuleOut(ctx, "docs", "srcjars").String(),
			"stubsDir":          android.PathForModuleOut(ctx, "docs", "stubsDir").String(),
			"srcJars":           strings.Join(j.srcJars.Strings(), " "),
			"opts":              opts,
			"bootClasspathArgs": bootClasspathArgs,
			"classpathArgs":     classpathArgs,
			"sourcepath":        strings.Join(j.sourcepaths.Strings(), ":"),
			"docZip":            j.docZip.String(),
		},
	})
}

func (d *Droiddoc) DepsMutator(ctx android.BottomUpMutatorContext) {
	d.Javadoc.addDeps(ctx)

	if String(d.properties.Custom_template) == "" {
		// TODO: This is almost always droiddoc-templates-sdk
		ctx.PropertyErrorf("custom_template", "must specify a template")
	} else {
		ctx.AddDependency(ctx.Module(), droiddocTemplateTag, String(d.properties.Custom_template))
	}

	// extra_arg_files may contains filegroup or genrule.
	android.ExtractSourcesDeps(ctx, d.properties.Arg_files)

	// knowntags may contain filegroup or genrule.
	android.ExtractSourcesDeps(ctx, d.properties.Knowntags)
}

func (d *Droiddoc) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	deps := d.Javadoc.collectDeps(ctx)

	var implicits android.Paths
	implicits = append(implicits, deps.bootClasspath...)
	implicits = append(implicits, deps.classpath...)

	argFiles := ctx.ExpandSources(d.properties.Arg_files, nil)
	argFilesMap := map[string]android.Path{}

	for _, f := range argFiles {
		implicits = append(implicits, f)
		if _, exists := argFilesMap[f.Rel()]; !exists {
			argFilesMap[f.Rel()] = f
		} else {
			ctx.ModuleErrorf("multiple arg_files for %q, %q and %q",
				f, argFilesMap[f.Rel()], f.Rel())
		}
	}

	args, err := android.Expand(String(d.properties.Args), func(name string) (string, error) {
		if strings.HasPrefix(name, "location ") {
			label := strings.TrimSpace(strings.TrimPrefix(name, "location "))
			if f, ok := argFilesMap[label]; ok {
				return f.String(), nil
			} else {
				return "", fmt.Errorf("unknown location label %q", label)
			}
		} else if name == "genDir" {
			return android.PathForModuleGen(ctx).String(), nil
		}
		return "", fmt.Errorf("unknown variable '$(%s)'", name)
	})

	if err != nil {
		ctx.PropertyErrorf("extra_args", "%s", err.Error())
		return
	}

	var bootClasspathArgs, classpathArgs string
	if len(deps.bootClasspath.Strings()) > 0 {
		bootClasspathArgs = "-bootclasspath " + strings.Join(deps.bootClasspath.Strings(), ":")
	}
	if len(deps.classpath.Strings()) > 0 {
		classpathArgs = "-classpath " + strings.Join(deps.classpath.Strings(), ":")
	}

	var templateDir string
	ctx.VisitDirectDepsWithTag(droiddocTemplateTag, func(m android.Module) {
		if t, ok := m.(*DroiddocTemplate); ok {
			implicits = append(implicits, t.deps...)
			templateDir = t.dir.String()
		} else {
			ctx.PropertyErrorf("custom_template", "module %q is not a droiddoc_template", ctx.OtherModuleName(m))
		}
	})

	var htmlDirArgs string
	if len(d.properties.Html_dirs) > 0 {
		htmlDir := android.PathForModuleSrc(ctx, d.properties.Html_dirs[0])
		implicits = append(implicits, ctx.Glob(htmlDir.Join(ctx, "**/*").String(), nil)...)
		htmlDirArgs = "-htmldir " + htmlDir.String()
	}

	var htmlDir2Args string
	if len(d.properties.Html_dirs) > 1 {
		htmlDir2 := android.PathForModuleSrc(ctx, d.properties.Html_dirs[1])
		implicits = append(implicits, ctx.Glob(htmlDir2.Join(ctx, "**/*").String(), nil)...)
		htmlDir2Args = "-htmldir2 " + htmlDir2.String()
	}

	if len(d.properties.Html_dirs) > 2 {
		ctx.PropertyErrorf("html_dirs", "Droiddoc only supports up to 2 html dirs")
	}

	knownTags := ctx.ExpandSources(d.properties.Knowntags, nil)
	implicits = append(implicits, knownTags...)

	for _, kt := range knownTags {
		args = args + " -knowntags " + kt.String()
	}
	for _, hdf := range d.properties.Hdf {
		args = args + " -hdf " + hdf
	}

	if String(d.properties.Proofread_file) != "" {
		proofreadFile := android.PathForModuleOut(ctx, String(d.properties.Proofread_file))
		args = args + " -proofread " + proofreadFile.String()
	}
	if String(d.properties.Todo_file) != "" {
		// tricky part:
		// we should not compute full path for todo_file through PathForModuleOut().
		// the non-standard doclet will get the full path relative to "-o".
		args = args + " -todo " + String(d.properties.Todo_file)
	}

	implicits = append(implicits, d.Javadoc.srcJars...)

	opts := "-source 1.8 -J-Xmx1600m -J-XX:-OmitStackTraceInFastThrow -XDignore.symbol.file " +
		"-doclet com.google.doclava.Doclava -docletpath ${config.JsilverJar}:${config.DoclavaJar} " +
		"-templatedir " + templateDir + " " + htmlDirArgs + " " + htmlDir2Args + " " +
		"-hdf page.build " + ctx.Config().BuildId() + "-" + ctx.Config().BuildNumberFromFile() + " " +
		"-hdf page.now " + `"$$(date -d @$$(cat ` + ctx.Config().Getenv("BUILD_DATETIME_FILE") + `) "+%d %b %Y %k:%M")"` + " " +
		args + " -stubs " + android.PathForModuleOut(ctx, "docs", "stubsDir").String()

	var implicitOutputs android.WritablePaths
	implicitOutputs = append(implicitOutputs, d.Javadoc.docZip)
	for _, o := range d.properties.Out {
		implicitOutputs = append(implicitOutputs, android.PathForModuleGen(ctx, o))
	}

	ctx.Build(pctx, android.BuildParams{
		Rule:            javadoc,
		Description:     "Droiddoc",
		Output:          d.Javadoc.stubsJar,
		Inputs:          d.Javadoc.srcFiles,
		Implicits:       implicits,
		ImplicitOutputs: implicitOutputs,
		Args: map[string]string{
			"outDir":            android.PathForModuleOut(ctx, "docs", "out").String(),
			"srcJarDir":         android.PathForModuleOut(ctx, "docs", "srcjars").String(),
			"stubsDir":          android.PathForModuleOut(ctx, "docs", "stubsDir").String(),
			"srcJars":           strings.Join(d.Javadoc.srcJars.Strings(), " "),
			"opts":              opts,
			"bootclasspathArgs": bootClasspathArgs,
			"classpathArgs":     classpathArgs,
			"sourcepath":        strings.Join(d.Javadoc.sourcepaths.Strings(), ":"),
			"docZip":            d.Javadoc.docZip.String(),
			"JsilverJar":        "${config.JsilverJar}",
			"DoclavaJar":        "${config.DoclavaJar}",
		},
	})
}

var droiddocTemplateTag = dependencyTag{name: "droiddoc-template"}

type DroiddocTemplateProperties struct {
	// path to the directory containing the droiddoc templates.
	Path *string
}

type DroiddocTemplate struct {
	android.ModuleBase

	properties DroiddocTemplateProperties

	deps android.Paths
	dir  android.Path
}

func DroiddocTemplateFactory() android.Module {
	module := &DroiddocTemplate{}
	module.AddProperties(&module.properties)
	android.InitAndroidModule(module)
	return module
}

func (d *DroiddocTemplate) DepsMutator(android.BottomUpMutatorContext) {}

func (d *DroiddocTemplate) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	path := android.PathForModuleSrc(ctx, String(d.properties.Path))
	d.dir = path
	d.deps = ctx.Glob(path.Join(ctx, "**/*").String(), nil)
}
