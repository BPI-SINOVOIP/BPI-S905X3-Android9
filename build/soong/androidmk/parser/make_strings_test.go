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

package parser

import (
	"strings"
	"testing"
)

var splitNTestCases = []struct {
	in       *MakeString
	expected []*MakeString
	sep      string
	n        int
}{
	{
		in: &MakeString{
			Strings: []string{
				"a b c",
				"d e f",
				" h i j",
			},
			Variables: []Variable{
				Variable{Name: SimpleMakeString("var1", NoPos)},
				Variable{Name: SimpleMakeString("var2", NoPos)},
			},
		},
		sep: " ",
		n:   -1,
		expected: []*MakeString{
			SimpleMakeString("a", NoPos),
			SimpleMakeString("b", NoPos),
			&MakeString{
				Strings: []string{"c", "d"},
				Variables: []Variable{
					Variable{Name: SimpleMakeString("var1", NoPos)},
				},
			},
			SimpleMakeString("e", NoPos),
			&MakeString{
				Strings: []string{"f", ""},
				Variables: []Variable{
					Variable{Name: SimpleMakeString("var2", NoPos)},
				},
			},
			SimpleMakeString("h", NoPos),
			SimpleMakeString("i", NoPos),
			SimpleMakeString("j", NoPos),
		},
	},
	{
		in: &MakeString{
			Strings: []string{
				"a b c",
				"d e f",
				" h i j",
			},
			Variables: []Variable{
				Variable{Name: SimpleMakeString("var1", NoPos)},
				Variable{Name: SimpleMakeString("var2", NoPos)},
			},
		},
		sep: " ",
		n:   3,
		expected: []*MakeString{
			SimpleMakeString("a", NoPos),
			SimpleMakeString("b", NoPos),
			&MakeString{
				Strings: []string{"c", "d e f", " h i j"},
				Variables: []Variable{
					Variable{Name: SimpleMakeString("var1", NoPos)},
					Variable{Name: SimpleMakeString("var2", NoPos)},
				},
			},
		},
	},
}

func TestMakeStringSplitN(t *testing.T) {
	for _, test := range splitNTestCases {
		got := test.in.SplitN(test.sep, test.n)
		gotString := dumpArray(got)
		expectedString := dumpArray(test.expected)
		if gotString != expectedString {
			t.Errorf("expected:\n%s\ngot:\n%s", expectedString, gotString)
		}
	}
}

func dumpArray(a []*MakeString) string {
	ret := make([]string, len(a))

	for i, s := range a {
		ret[i] = s.Dump()
	}

	return strings.Join(ret, "|||")
}
