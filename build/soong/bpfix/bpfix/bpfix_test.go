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

// This file implements the logic of bpfix and also provides a programmatic interface

package bpfix

import (
	"bytes"
	"fmt"
	"strings"
	"testing"

	"reflect"

	"github.com/google/blueprint/parser"
)

// TODO(jeffrygaston) remove this when position is removed from ParseNode (in b/38325146) and we can directly do reflect.DeepEqual
func printListOfStrings(items []string) (text string) {
	if len(items) == 0 {
		return "[]"
	}
	return fmt.Sprintf("[\"%s\"]", strings.Join(items, "\", \""))

}

func buildTree(local_include_dirs []string, export_include_dirs []string) (file *parser.File, errs []error) {
	// TODO(jeffrygaston) use the builder class when b/38325146 is done
	input := fmt.Sprintf(`cc_library_shared {
	    name: "iAmAModule",
	    local_include_dirs: %s,
	    export_include_dirs: %s,
	}
	`,
		printListOfStrings(local_include_dirs), printListOfStrings(export_include_dirs))
	tree, errs := parser.Parse("", strings.NewReader(input), parser.NewScope(nil))
	if len(errs) > 0 {
		errs = append([]error{fmt.Errorf("failed to parse:\n%s", input)}, errs...)
	}
	return tree, errs
}

func implFilterListTest(t *testing.T, local_include_dirs []string, export_include_dirs []string, expectedResult []string) {
	// build tree
	tree, errs := buildTree(local_include_dirs, export_include_dirs)
	if len(errs) > 0 {
		t.Error("failed to build tree")
		for _, err := range errs {
			t.Error(err)
		}
		t.Fatalf("%d parse errors", len(errs))
	}

	fixer := NewFixer(tree)

	// apply simplifications
	err := fixer.simplifyKnownPropertiesDuplicatingEachOther()
	if len(errs) > 0 {
		t.Fatal(err)
	}

	// lookup legacy property
	mod := fixer.tree.Defs[0].(*parser.Module)
	_, found := mod.GetProperty("local_include_dirs")
	if !found {
		t.Fatalf("failed to include key local_include_dirs in parse tree")
	}

	// check that the value for the legacy property was updated to the correct value
	errorHeader := fmt.Sprintf("\nFailed to correctly simplify key 'local_include_dirs' in the presence of 'export_include_dirs.'\n"+
		"original local_include_dirs: %q\n"+
		"original export_include_dirs: %q\n"+
		"expected result: %q\n"+
		"actual result: ",
		local_include_dirs, export_include_dirs, expectedResult)
	result, ok := mod.GetProperty("local_include_dirs")
	if !ok {
		t.Fatal(errorHeader + "property not found")
	}

	listResult, ok := result.Value.(*parser.List)
	if !ok {
		t.Fatalf("%sproperty is not a list: %v", errorHeader, listResult)
	}

	actualExpressions := listResult.Values
	actualValues := make([]string, 0)
	for _, expr := range actualExpressions {
		str := expr.(*parser.String)
		actualValues = append(actualValues, str.Value)
	}

	if !reflect.DeepEqual(actualValues, expectedResult) {
		t.Fatalf("%s%q\nlists are different", errorHeader, actualValues)
	}
}

func TestSimplifyKnownVariablesDuplicatingEachOther(t *testing.T) {
	// TODO use []Expression{} once buildTree above can support it (which is after b/38325146 is done)
	implFilterListTest(t, []string{"include"}, []string{"include"}, []string{})
	implFilterListTest(t, []string{"include1"}, []string{"include2"}, []string{"include1"})
	implFilterListTest(t, []string{"include1", "include2", "include3", "include4"}, []string{"include2"},
		[]string{"include1", "include3", "include4"})
	implFilterListTest(t, []string{}, []string{"include"}, []string{})
	implFilterListTest(t, []string{}, []string{}, []string{})
}

func TestMergeMatchingProperties(t *testing.T) {
	tests := []struct {
		name string
		in   string
		out  string
	}{
		{
			name: "empty",
			in: `
				java_library {
					name: "foo",
					static_libs: [],
					static_libs: [],
				}
			`,
			out: `
				java_library {
					name: "foo",
					static_libs: [],
				}
			`,
		},
		{
			name: "single line into multiline",
			in: `
				java_library {
					name: "foo",
					static_libs: [
						"a",
						"b",
					],
					//c1
					static_libs: ["c" /*c2*/],
				}
			`,
			out: `
				java_library {
					name: "foo",
					static_libs: [
						"a",
						"b",
						"c", /*c2*/
					],
					//c1
				}
			`,
		},
		{
			name: "multiline into multiline",
			in: `
				java_library {
					name: "foo",
					static_libs: [
						"a",
						"b",
					],
					//c1
					static_libs: [
						//c2
						"c", //c3
						"d",
					],
				}
			`,
			out: `
				java_library {
					name: "foo",
					static_libs: [
						"a",
						"b",
						//c2
						"c", //c3
						"d",
					],
					//c1
				}
			`,
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			expected, err := Reformat(test.out)
			if err != nil {
				t.Error(err)
			}

			in, err := Reformat(test.in)
			if err != nil {
				t.Error(err)
			}

			tree, errs := parser.Parse("<testcase>", bytes.NewBufferString(in), parser.NewScope(nil))
			if errs != nil {
				t.Fatal(errs)
			}

			fixer := NewFixer(tree)

			got := ""
			prev := "foo"
			passes := 0
			for got != prev && passes < 10 {
				err := fixer.mergeMatchingModuleProperties()
				if err != nil {
					t.Fatal(err)
				}

				out, err := parser.Print(fixer.tree)
				if err != nil {
					t.Fatal(err)
				}

				prev = got
				got = string(out)
				passes++
			}

			if got != expected {
				t.Errorf("failed testcase '%s'\ninput:\n%s\n\nexpected:\n%s\ngot:\n%s\n",
					test.name, in, expected, got)
			}

		})
	}
}
