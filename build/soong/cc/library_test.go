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

package cc

import (
	"reflect"
	"testing"
)

func TestLibraryReuse(t *testing.T) {
	t.Run("simple", func(t *testing.T) {
		ctx := testCc(t, `
		cc_library {
			name: "libfoo",
			srcs: ["foo.c"],
		}`)

		libfooShared := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_shared").Rule("ld")
		libfooStatic := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_static").Output("libfoo.a")

		if len(libfooShared.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo shared: %#v", libfooShared.Inputs.Strings())
		}

		if len(libfooStatic.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo static: %#v", libfooStatic.Inputs.Strings())
		}

		if libfooShared.Inputs[0] != libfooStatic.Inputs[0] {
			t.Errorf("static object not reused for shared library")
		}
	})

	t.Run("extra static source", func(t *testing.T) {
		ctx := testCc(t, `
		cc_library {
			name: "libfoo",
			srcs: ["foo.c"],
			static: {
				srcs: ["bar.c"]
			},
		}`)

		libfooShared := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_shared").Rule("ld")
		libfooStatic := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_static").Output("libfoo.a")

		if len(libfooShared.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo shared: %#v", libfooShared.Inputs.Strings())
		}

		if len(libfooStatic.Inputs) != 2 {
			t.Fatalf("unexpected inputs to libfoo static: %#v", libfooStatic.Inputs.Strings())
		}

		if libfooShared.Inputs[0] != libfooStatic.Inputs[0] {
			t.Errorf("static object not reused for shared library")
		}
	})

	t.Run("extra shared source", func(t *testing.T) {
		ctx := testCc(t, `
		cc_library {
			name: "libfoo",
			srcs: ["foo.c"],
			shared: {
				srcs: ["bar.c"]
			},
		}`)

		libfooShared := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_shared").Rule("ld")
		libfooStatic := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_static").Output("libfoo.a")

		if len(libfooShared.Inputs) != 2 {
			t.Fatalf("unexpected inputs to libfoo shared: %#v", libfooShared.Inputs.Strings())
		}

		if len(libfooStatic.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo static: %#v", libfooStatic.Inputs.Strings())
		}

		if libfooShared.Inputs[0] != libfooStatic.Inputs[0] {
			t.Errorf("static object not reused for shared library")
		}
	})

	t.Run("extra static cflags", func(t *testing.T) {
		ctx := testCc(t, `
		cc_library {
			name: "libfoo",
			srcs: ["foo.c"],
			static: {
				cflags: ["-DFOO"],
			},
		}`)

		libfooShared := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_shared").Rule("ld")
		libfooStatic := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_static").Output("libfoo.a")

		if len(libfooShared.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo shared: %#v", libfooShared.Inputs.Strings())
		}

		if len(libfooStatic.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo static: %#v", libfooStatic.Inputs.Strings())
		}

		if libfooShared.Inputs[0] == libfooStatic.Inputs[0] {
			t.Errorf("static object reused for shared library when it shouldn't be")
		}
	})

	t.Run("extra shared cflags", func(t *testing.T) {
		ctx := testCc(t, `
		cc_library {
			name: "libfoo",
			srcs: ["foo.c"],
			shared: {
				cflags: ["-DFOO"],
			},
		}`)

		libfooShared := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_shared").Rule("ld")
		libfooStatic := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_static").Output("libfoo.a")

		if len(libfooShared.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo shared: %#v", libfooShared.Inputs.Strings())
		}

		if len(libfooStatic.Inputs) != 1 {
			t.Fatalf("unexpected inputs to libfoo static: %#v", libfooStatic.Inputs.Strings())
		}

		if libfooShared.Inputs[0] == libfooStatic.Inputs[0] {
			t.Errorf("static object reused for shared library when it shouldn't be")
		}
	})

	t.Run("global cflags for reused generated sources", func(t *testing.T) {
		ctx := testCc(t, `
		cc_library {
			name: "libfoo",
			srcs: [
				"foo.c",
				"a.proto",
			],
			shared: {
				srcs: [
					"bar.c",
				],
			},
		}`)

		libfooShared := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_shared").Rule("ld")
		libfooStatic := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_static").Output("libfoo.a")

		if len(libfooShared.Inputs) != 3 {
			t.Fatalf("unexpected inputs to libfoo shared: %#v", libfooShared.Inputs.Strings())
		}

		if len(libfooStatic.Inputs) != 2 {
			t.Fatalf("unexpected inputs to libfoo static: %#v", libfooStatic.Inputs.Strings())
		}

		if !reflect.DeepEqual(libfooShared.Inputs[0:2].Strings(), libfooStatic.Inputs.Strings()) {
			t.Errorf("static objects not reused for shared library")
		}

		libfoo := ctx.ModuleForTests("libfoo", "android_arm_armv7-a-neon_core_shared").Module().(*Module)
		if !inList("-DGOOGLE_PROTOBUF_NO_RTTI", libfoo.flags.CFlags) {
			t.Errorf("missing protobuf cflags")
		}
	})
}
