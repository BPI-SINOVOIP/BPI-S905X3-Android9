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

package config

import (
	"sort"
	"strings"
)

// Cflags that should be filtered out when compiling with clang
var ClangUnknownCflags = sorted([]string{
	"-finline-functions",
	"-finline-limit=64",
	"-fno-canonical-system-headers",
	"-Wno-clobbered",
	"-fno-devirtualize",
	"-fno-tree-sra",
	"-fprefetch-loop-arrays",
	"-funswitch-loops",
	"-Werror=unused-but-set-parameter",
	"-Werror=unused-but-set-variable",
	"-Wmaybe-uninitialized",
	"-Wno-error=clobbered",
	"-Wno-error=maybe-uninitialized",
	"-Wno-error=unused-but-set-parameter",
	"-Wno-error=unused-but-set-variable",
	"-Wno-free-nonheap-object",
	"-Wno-literal-suffix",
	"-Wno-maybe-uninitialized",
	"-Wno-old-style-declaration",
	"-Wno-psabi",
	"-Wno-unused-but-set-parameter",
	"-Wno-unused-but-set-variable",
	"-Wno-unused-local-typedefs",
	"-Wunused-but-set-parameter",
	"-Wunused-but-set-variable",
	"-fdiagnostics-color",

	// arm + arm64 + mips + mips64
	"-fgcse-after-reload",
	"-frerun-cse-after-loop",
	"-frename-registers",
	"-fno-strict-volatile-bitfields",

	// arm + arm64
	"-fno-align-jumps",

	// arm
	"-mthumb-interwork",
	"-fno-builtin-sin",
	"-fno-caller-saves",
	"-fno-early-inlining",
	"-fno-move-loop-invariants",
	"-fno-partial-inlining",
	"-fno-tree-copy-prop",
	"-fno-tree-loop-optimize",

	// mips + mips64
	"-msynci",
	"-mno-synci",
	"-mno-fused-madd",

	// x86 + x86_64
	"-finline-limit=300",
	"-fno-inline-functions-called-once",
	"-mfpmath=sse",
	"-mbionic",

	// windows
	"--enable-stdcall-fixup",
})

var ClangLibToolingUnknownCflags = []string{
	"-flto*",
	"-fsanitize*",
}

func init() {
	pctx.StaticVariable("ClangExtraCflags", strings.Join([]string{
		"-D__compiler_offsetof=__builtin_offsetof",

		// Help catch common 32/64-bit errors.
		"-Werror=int-conversion",

		// Disable overly aggressive warning for macros defined with a leading underscore
		// This happens in AndroidConfig.h, which is included nearly everywhere.
		// TODO: can we remove this now?
		"-Wno-reserved-id-macro",

		// Disable overly aggressive warning for format strings.
		// Bug: 20148343
		"-Wno-format-pedantic",

		// Workaround for ccache with clang.
		// See http://petereisentraut.blogspot.com/2011/05/ccache-and-clang.html.
		"-Wno-unused-command-line-argument",

		// Force clang to always output color diagnostics. Ninja will strip the ANSI
		// color codes if it is not running in a terminal.
		"-fcolor-diagnostics",

		// http://b/29823425 Disable -Wexpansion-to-defined for Clang update to r271374
		"-Wno-expansion-to-defined",

		// http://b/68236239 Allow 0/NULL instead of using nullptr everywhere.
		"-Wno-zero-as-null-pointer-constant",

		// http://b/36463318 Clang executes with an absolute path, so clang-provided
		// headers are now absolute.
		"-fdebug-prefix-map=$$PWD/=",
	}, " "))

	pctx.StaticVariable("ClangExtraCppflags", strings.Join([]string{
		// Disable -Winconsistent-missing-override until we can clean up the existing
		// codebase for it.
		"-Wno-inconsistent-missing-override",

		// Bug: http://b/29823425 Disable -Wnull-dereference until the
		// new instances detected by this warning are fixed.
		"-Wno-null-dereference",

		// Enable clang's thread-safety annotations in libcxx.
		// Turn off -Wthread-safety-negative, to avoid breaking projects that use -Weverything.
		"-D_LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS",
		"-Wno-thread-safety-negative",

		// libc++'s math.h has an #include_next outside of system_headers.
		"-Wno-gnu-include-next",
	}, " "))

	pctx.StaticVariable("ClangExtraTargetCflags", strings.Join([]string{
		"-nostdlibinc",
	}, " "))

	pctx.StaticVariable("ClangExtraNoOverrideCflags", strings.Join([]string{
		"-Werror=address-of-temporary",
		// Bug: http://b/29823425 Disable -Wnull-dereference until the
		// new cases detected by this warning in Clang r271374 are
		// fixed.
		//"-Werror=null-dereference",
		"-Werror=return-type",

		// http://b/72331526 Disable -Wtautological-* until the instances detected by these
		// new warnings are fixed.
		"-Wno-tautological-constant-compare",

		// http://b/72331524 Allow null pointer arithmetic until the instances detected by
		// this new warning are fixed.
		"-Wno-null-pointer-arithmetic",

		// http://b/72330874 Disable -Wenum-compare until the instances detected by this new
		// warning are fixed.
		"-Wno-enum-compare",
		"-Wno-enum-compare-switch",
	}, " "))
}

func ClangFilterUnknownCflags(cflags []string) []string {
	ret := make([]string, 0, len(cflags))
	for _, f := range cflags {
		if !inListSorted(f, ClangUnknownCflags) {
			ret = append(ret, f)
		}
	}

	return ret
}

func inListSorted(s string, list []string) bool {
	for _, l := range list {
		if s == l {
			return true
		} else if s < l {
			return false
		}
	}
	return false
}

func sorted(list []string) []string {
	sort.Strings(list)
	return list
}
