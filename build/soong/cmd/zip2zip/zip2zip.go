// Copyright 2016 Google Inc. All rights reserved.
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
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"github.com/google/blueprint/pathtools"

	"android/soong/jar"
	"android/soong/third_party/zip"
)

var (
	input     = flag.String("i", "", "zip file to read from")
	output    = flag.String("o", "", "output file")
	sortGlobs = flag.Bool("s", false, "sort matches from each glob (defaults to the order from the input zip file)")
	sortJava  = flag.Bool("j", false, "sort using jar ordering within each glob (META-INF/MANIFEST.MF first)")
	setTime   = flag.Bool("t", false, "set timestamps to 2009-01-01 00:00:00")

	staticTime = time.Date(2009, 1, 1, 0, 0, 0, 0, time.UTC)

	excludes excludeArgs
)

func init() {
	flag.Var(&excludes, "x", "exclude a filespec from the output")
}

func main() {
	flag.Usage = func() {
		fmt.Fprintln(os.Stderr, "usage: zip2zip -i zipfile -o zipfile [-s|-j] [-t] [filespec]...")
		flag.PrintDefaults()
		fmt.Fprintln(os.Stderr, "  filespec:")
		fmt.Fprintln(os.Stderr, "    <name>")
		fmt.Fprintln(os.Stderr, "    <in_name>:<out_name>")
		fmt.Fprintln(os.Stderr, "    <glob>[:<out_dir>]")
		fmt.Fprintln(os.Stderr, "")
		fmt.Fprintln(os.Stderr, "<glob> uses the rules at https://godoc.org/github.com/google/blueprint/pathtools/#Match")
		fmt.Fprintln(os.Stderr, "")
		fmt.Fprintln(os.Stderr, "Files will be copied with their existing compression from the input zipfile to")
		fmt.Fprintln(os.Stderr, "the output zipfile, in the order of filespec arguments.")
		fmt.Fprintln(os.Stderr, "")
		fmt.Fprintln(os.Stderr, "If no filepsec is provided all files and directories are copied.")
	}

	flag.Parse()

	if *input == "" || *output == "" {
		flag.Usage()
		os.Exit(1)
	}

	log.SetFlags(log.Lshortfile)

	reader, err := zip.OpenReader(*input)
	if err != nil {
		log.Fatal(err)
	}
	defer reader.Close()

	output, err := os.Create(*output)
	if err != nil {
		log.Fatal(err)
	}
	defer output.Close()

	writer := zip.NewWriter(output)
	defer func() {
		err := writer.Close()
		if err != nil {
			log.Fatal(err)
		}
	}()

	if err := zip2zip(&reader.Reader, writer, *sortGlobs, *sortJava, *setTime,
		flag.Args(), excludes); err != nil {

		log.Fatal(err)
	}
}

type pair struct {
	*zip.File
	newName string
}

func zip2zip(reader *zip.Reader, writer *zip.Writer, sortOutput, sortJava, setTime bool,
	includes []string, excludes []string) error {

	matches := []pair{}

	sortMatches := func(matches []pair) {
		if sortJava {
			sort.SliceStable(matches, func(i, j int) bool {
				return jar.EntryNamesLess(matches[i].newName, matches[j].newName)
			})
		} else if sortOutput {
			sort.SliceStable(matches, func(i, j int) bool {
				return matches[i].newName < matches[j].newName
			})
		}
	}

	for _, include := range includes {
		// Reserve escaping for future implementation, so make sure no
		// one is using \ and expecting a certain behavior.
		if strings.Contains(include, "\\") {
			return fmt.Errorf("\\ characters are not currently supported")
		}

		input, output := includeSplit(include)

		var includeMatches []pair

		for _, file := range reader.File {
			var newName string
			if match, err := pathtools.Match(input, file.Name); err != nil {
				return err
			} else if match {
				if output == "" {
					newName = file.Name
				} else {
					if pathtools.IsGlob(input) {
						// If the input is a glob then the output is a directory.
						_, name := filepath.Split(file.Name)
						newName = filepath.Join(output, name)
					} else {
						// Otherwise it is a file.
						newName = output
					}
				}
				includeMatches = append(includeMatches, pair{file, newName})
			}
		}

		sortMatches(includeMatches)
		matches = append(matches, includeMatches...)
	}

	if len(includes) == 0 {
		// implicitly match everything
		for _, file := range reader.File {
			matches = append(matches, pair{file, file.Name})
		}
		sortMatches(matches)
	}

	var matchesAfterExcludes []pair
	seen := make(map[string]*zip.File)

	for _, match := range matches {
		// Filter out matches whose original file name matches an exclude filter
		excluded := false
		for _, exclude := range excludes {
			if excludeMatch, err := pathtools.Match(exclude, match.File.Name); err != nil {
				return err
			} else if excludeMatch {
				excluded = true
				break
			}
		}

		if excluded {
			continue
		}

		// Check for duplicate output names, ignoring ones that come from the same input zip entry.
		if prev, exists := seen[match.newName]; exists {
			if prev != match.File {
				return fmt.Errorf("multiple entries for %q with different contents", match.newName)
			}
			continue
		}
		seen[match.newName] = match.File

		matchesAfterExcludes = append(matchesAfterExcludes, match)
	}

	for _, match := range matchesAfterExcludes {
		if setTime {
			match.File.SetModTime(staticTime)
		}
		if err := writer.CopyFrom(match.File, match.newName); err != nil {
			return err
		}
	}

	return nil
}

func includeSplit(s string) (string, string) {
	split := strings.SplitN(s, ":", 2)
	if len(split) == 2 {
		return split[0], split[1]
	} else {
		return split[0], ""
	}
}

type excludeArgs []string

func (e *excludeArgs) String() string {
	return strings.Join(*e, " ")
}

func (e *excludeArgs) Set(s string) error {
	*e = append(*e, s)
	return nil
}
