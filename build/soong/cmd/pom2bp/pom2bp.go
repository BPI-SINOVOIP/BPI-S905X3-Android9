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

package main

import (
	"bufio"
	"bytes"
	"encoding/xml"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"text/template"

	"github.com/google/blueprint/proptools"

	"android/soong/bpfix/bpfix"
)

type RewriteNames []RewriteName
type RewriteName struct {
	regexp *regexp.Regexp
	repl   string
}

func (r *RewriteNames) String() string {
	return ""
}

func (r *RewriteNames) Set(v string) error {
	split := strings.SplitN(v, "=", 2)
	if len(split) != 2 {
		return fmt.Errorf("Must be in the form of <regex>=<replace>")
	}
	regex, err := regexp.Compile(split[0])
	if err != nil {
		return nil
	}
	*r = append(*r, RewriteName{
		regexp: regex,
		repl:   split[1],
	})
	return nil
}

func (r *RewriteNames) MavenToBp(groupId string, artifactId string) string {
	for _, r := range *r {
		if r.regexp.MatchString(groupId + ":" + artifactId) {
			return r.regexp.ReplaceAllString(groupId+":"+artifactId, r.repl)
		} else if r.regexp.MatchString(artifactId) {
			return r.regexp.ReplaceAllString(artifactId, r.repl)
		}
	}
	return artifactId
}

var rewriteNames = RewriteNames{}

type ExtraDeps map[string][]string

func (d ExtraDeps) String() string {
	return ""
}

func (d ExtraDeps) Set(v string) error {
	split := strings.SplitN(v, "=", 2)
	if len(split) != 2 {
		return fmt.Errorf("Must be in the form of <module>=<module>[,<module>]")
	}
	d[split[0]] = strings.Split(split[1], ",")
	return nil
}

var extraDeps = make(ExtraDeps)

type Exclude map[string]bool

func (e Exclude) String() string {
	return ""
}

func (e Exclude) Set(v string) error {
	e[v] = true
	return nil
}

var excludes = make(Exclude)

var sdkVersion string
var useVersion string

func InList(s string, list []string) bool {
	for _, l := range list {
		if l == s {
			return true
		}
	}

	return false
}

type Dependency struct {
	XMLName xml.Name `xml:"dependency"`

	BpTarget string `xml:"-"`

	GroupId    string `xml:"groupId"`
	ArtifactId string `xml:"artifactId"`
	Version    string `xml:"version"`
	Type       string `xml:"type"`
	Scope      string `xml:"scope"`
}

func (d Dependency) BpName() string {
	if d.BpTarget == "" {
		d.BpTarget = rewriteNames.MavenToBp(d.GroupId, d.ArtifactId)
	}
	return d.BpTarget
}

type Pom struct {
	XMLName xml.Name `xml:"http://maven.apache.org/POM/4.0.0 project"`

	PomFile      string `xml:"-"`
	ArtifactFile string `xml:"-"`
	BpTarget     string `xml:"-"`

	GroupId    string `xml:"groupId"`
	ArtifactId string `xml:"artifactId"`
	Version    string `xml:"version"`
	Packaging  string `xml:"packaging"`

	Dependencies []*Dependency `xml:"dependencies>dependency"`
}

func (p Pom) IsAar() bool {
	return p.Packaging == "aar"
}

func (p Pom) IsJar() bool {
	return p.Packaging == "jar"
}

func (p Pom) BpName() string {
	if p.BpTarget == "" {
		p.BpTarget = rewriteNames.MavenToBp(p.GroupId, p.ArtifactId)
	}
	return p.BpTarget
}

func (p Pom) BpJarDeps() []string {
	return p.BpDeps("jar", []string{"compile", "runtime"})
}

func (p Pom) BpAarDeps() []string {
	return p.BpDeps("aar", []string{"compile", "runtime"})
}

func (p Pom) BpExtraDeps() []string {
	return extraDeps[p.BpName()]
}

// BpDeps obtains dependencies filtered by type and scope. The results of this
// method are formatted as Android.bp targets, e.g. run through MavenToBp rules.
func (p Pom) BpDeps(typeExt string, scopes []string) []string {
	var ret []string
	for _, d := range p.Dependencies {
		if d.Type != typeExt || !InList(d.Scope, scopes) {
			continue
		}
		name := rewriteNames.MavenToBp(d.GroupId, d.ArtifactId)
		ret = append(ret, name)
	}
	return ret
}

func (p Pom) SdkVersion() string {
	return sdkVersion
}

func (p *Pom) FixDeps(modules map[string]*Pom) {
	for _, d := range p.Dependencies {
		if d.Type == "" {
			if depPom, ok := modules[d.BpName()]; ok {
				// We've seen the POM for this dependency, use its packaging
				// as the dependency type rather than Maven spec default.
				d.Type = depPom.Packaging
			} else {
				// Dependency type was not specified and we don't have the POM
				// for this artifact, use the default from Maven spec.
				d.Type = "jar"
			}
		}
		if d.Scope == "" {
			// Scope was not specified, use the default from Maven spec.
			d.Scope = "compile"
		}
	}
}

var bpTemplate = template.Must(template.New("bp").Parse(`
{{if .IsAar}}android_library_import{{else}}java_import{{end}} {
    name: "{{.BpName}}-nodeps",
    {{if .IsAar}}aars{{else}}jars{{end}}: ["{{.ArtifactFile}}"],
    sdk_version: "{{.SdkVersion}}",{{if .IsAar}}
    static_libs: [{{range .BpAarDeps}}
        "{{.}}",{{end}}{{range .BpExtraDeps}}
        "{{.}}",{{end}}
    ],{{end}}
}

{{if .IsAar}}android_library{{else}}java_library_static{{end}} {
    name: "{{.BpName}}",
    sdk_version: "{{.SdkVersion}}",{{if .IsAar}}
    manifest: "manifests/{{.BpName}}/AndroidManifest.xml",{{end}}
    static_libs: [
        "{{.BpName}}-nodeps",{{range .BpJarDeps}}
        "{{.}}",{{end}}{{range .BpAarDeps}}
        "{{.}}",{{end}}{{range .BpExtraDeps}}
        "{{.}}",{{end}}
    ],
    java_version: "1.7",
}
`))

func parse(filename string) (*Pom, error) {
	data, err := ioutil.ReadFile(filename)
	if err != nil {
		return nil, err
	}

	var pom Pom
	err = xml.Unmarshal(data, &pom)
	if err != nil {
		return nil, err
	}

	if useVersion != "" && pom.Version != useVersion {
		return nil, nil
	}

	if pom.Packaging == "" {
		pom.Packaging = "jar"
	}

	pom.PomFile = filename
	pom.ArtifactFile = strings.TrimSuffix(filename, ".pom") + "." + pom.Packaging

	return &pom, nil
}

func rerunForRegen(filename string) error {
	buf, err := ioutil.ReadFile(filename)
	if err != nil {
		return err
	}

	scanner := bufio.NewScanner(bytes.NewBuffer(buf))

	// Skip the first line in the file
	for i := 0; i < 2; i++ {
		if !scanner.Scan() {
			if scanner.Err() != nil {
				return scanner.Err()
			} else {
				return fmt.Errorf("unexpected EOF")
			}
		}
	}

	// Extract the old args from the file
	line := scanner.Text()
	if strings.HasPrefix(line, "// pom2bp ") {
		line = strings.TrimPrefix(line, "// pom2bp ")
	} else if strings.HasPrefix(line, "// pom2mk ") {
		line = strings.TrimPrefix(line, "// pom2mk ")
	} else if strings.HasPrefix(line, "# pom2mk ") {
		line = strings.TrimPrefix(line, "# pom2mk ")
	} else {
		return fmt.Errorf("unexpected second line: %q", line)
	}
	args := strings.Split(line, " ")
	lastArg := args[len(args)-1]
	args = args[:len(args)-1]

	// Append all current command line args except -regen <file> to the ones from the file
	for i := 1; i < len(os.Args); i++ {
		if os.Args[i] == "-regen" {
			i++
		} else {
			args = append(args, os.Args[i])
		}
	}
	args = append(args, lastArg)

	cmd := os.Args[0] + " " + strings.Join(args, " ")
	// Re-exec pom2bp with the new arguments
	output, err := exec.Command("/bin/sh", "-c", cmd).Output()
	if exitErr, _ := err.(*exec.ExitError); exitErr != nil {
		return fmt.Errorf("failed to run %s\n%s", cmd, string(exitErr.Stderr))
	} else if err != nil {
		return err
	}

	// If the old file was a .mk file, replace it with a .bp file
	if filepath.Ext(filename) == ".mk" {
		os.Remove(filename)
		filename = strings.TrimSuffix(filename, ".mk") + ".bp"
	}

	return ioutil.WriteFile(filename, output, 0666)
}

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, `pom2bp, a tool to create Android.bp files from maven repos

The tool will extract the necessary information from *.pom files to create an Android.bp whose
aar libraries can be linked against when using AAPT2.

Usage: %s [--rewrite <regex>=<replace>] [-exclude <module>] [--extra-deps <module>=<module>[,<module>]] [<dir>] [-regen <file>]

  -rewrite <regex>=<replace>
     rewrite can be used to specify mappings between Maven projects and Android.bp modules. The -rewrite
     option can be specified multiple times. When determining the Android.bp module for a given Maven
     project, mappings are searched in the order they were specified. The first <regex> matching
     either the Maven project's <groupId>:<artifactId> or <artifactId> will be used to generate
     the Android.bp module name using <replace>. If no matches are found, <artifactId> is used.
  -exclude <module>
     Don't put the specified module in the Android.bp file.
  -extra-deps <module>=<module>[,<module>]
     Some Android.bp modules have transitive dependencies that must be specified when they are
     depended upon (like android-support-v7-mediarouter requires android-support-v7-appcompat).
     This may be specified multiple times to declare these dependencies.
  -sdk-version <version>
     Sets LOCAL_SDK_VERSION := <version> for all modules.
  -use-version <version>
     If the maven directory contains multiple versions of artifacts and their pom files,
     -use-version can be used to only write Android.bp files for a specific version of those artifacts.
  <dir>
     The directory to search for *.pom files under.
     The contents are written to stdout, to be put in the current directory (often as Android.bp)
  -regen <file>
     Read arguments from <file> and overwrite it (if it ends with .bp) or move it to .bp (if it
     ends with .mk).

`, os.Args[0])
	}

	var regen string

	flag.Var(&excludes, "exclude", "Exclude module")
	flag.Var(&extraDeps, "extra-deps", "Extra dependencies needed when depending on a module")
	flag.Var(&rewriteNames, "rewrite", "Regex(es) to rewrite artifact names")
	flag.StringVar(&sdkVersion, "sdk-version", "", "What to write to LOCAL_SDK_VERSION")
	flag.StringVar(&useVersion, "use-version", "", "Only read artifacts of a specific version")
	flag.Bool("static-deps", false, "Ignored")
	flag.StringVar(&regen, "regen", "", "Rewrite specified file")
	flag.Parse()

	if regen != "" {
		err := rerunForRegen(regen)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
		os.Exit(0)
	}

	if flag.NArg() == 0 {
		fmt.Fprintln(os.Stderr, "Directory argument is required")
		os.Exit(1)
	} else if flag.NArg() > 1 {
		fmt.Fprintln(os.Stderr, "Multiple directories provided:", strings.Join(flag.Args(), " "))
		os.Exit(1)
	}

	dir := flag.Arg(0)
	absDir, err := filepath.Abs(dir)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to get absolute directory:", err)
		os.Exit(1)
	}

	var filenames []string
	err = filepath.Walk(absDir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		name := info.Name()
		if info.IsDir() {
			if strings.HasPrefix(name, ".") {
				return filepath.SkipDir
			}
			return nil
		}

		if strings.HasPrefix(name, ".") {
			return nil
		}

		if strings.HasSuffix(name, ".pom") {
			path, err = filepath.Rel(absDir, path)
			if err != nil {
				return err
			}
			filenames = append(filenames, filepath.Join(dir, path))
		}
		return nil
	})
	if err != nil {
		fmt.Fprintln(os.Stderr, "Error walking files:", err)
		os.Exit(1)
	}

	if len(filenames) == 0 {
		fmt.Fprintln(os.Stderr, "Error: no *.pom files found under", dir)
		os.Exit(1)
	}

	sort.Strings(filenames)

	poms := []*Pom{}
	modules := make(map[string]*Pom)
	duplicate := false
	for _, filename := range filenames {
		pom, err := parse(filename)
		if err != nil {
			fmt.Fprintln(os.Stderr, "Error converting", filename, err)
			os.Exit(1)
		}

		if pom != nil {
			key := pom.BpName()
			if excludes[key] {
				continue
			}

			if old, ok := modules[key]; ok {
				fmt.Fprintln(os.Stderr, "Module", key, "defined twice:", old.PomFile, pom.PomFile)
				duplicate = true
			}

			poms = append(poms, pom)
			modules[key] = pom
		}
	}
	if duplicate {
		os.Exit(1)
	}

	for _, pom := range poms {
		pom.FixDeps(modules)
	}

	buf := &bytes.Buffer{}

	fmt.Fprintln(buf, "// Automatically generated with:")
	fmt.Fprintln(buf, "// pom2bp", strings.Join(proptools.ShellEscape(os.Args[1:]), " "))

	for _, pom := range poms {
		var err error
		err = bpTemplate.Execute(buf, pom)
		if err != nil {
			fmt.Fprintln(os.Stderr, "Error writing", pom.PomFile, pom.BpName(), err)
			os.Exit(1)
		}
	}

	out, err := bpfix.Reformat(buf.String())
	if err != nil {
		fmt.Fprintln(os.Stderr, "Error formatting output", err)
		os.Exit(1)
	}

	os.Stdout.WriteString(out)
}
