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

import "android/soong/android"

var (
	// These will be filled out by external/error_prone/soong/error_prone.go if it is available
	ErrorProneJavacJar    string
	ErrorProneJar         string
	ErrorProneClasspath   string
	ErrorProneChecksError string
	ErrorProneFlags       string
)

// Wrapper that grabs value of val late so it can be initialized by a later module's init function
func errorProneVar(name string, val *string) {
	pctx.VariableFunc(name, func(android.PackageVarContext) string {
		return *val
	})
}

func init() {
	errorProneVar("ErrorProneJar", &ErrorProneJar)
	errorProneVar("ErrorProneJavacJar", &ErrorProneJavacJar)
	errorProneVar("ErrorProneClasspath", &ErrorProneClasspath)
	errorProneVar("ErrorProneChecksError", &ErrorProneChecksError)
	errorProneVar("ErrorProneFlags", &ErrorProneFlags)

	pctx.StaticVariable("ErrorProneCmd",
		"${JavaCmd} -Xmx${JavacHeapSize} -Xbootclasspath/p:${ErrorProneJavacJar} "+
			"-cp ${ErrorProneJar}:${ErrorProneClasspath} "+
			"${ErrorProneFlags} ${CommonJdkFlags} ${ErrorProneChecksError}")

}
