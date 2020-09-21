/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AAPT_SPLIT_UTIL_H
#define AAPT_SPLIT_UTIL_H

#include "androidfw/StringPiece.h"

#include "AppInfo.h"
#include "Diagnostics.h"
#include "SdkConstants.h"
#include "filter/ConfigFilter.h"
#include "split/TableSplitter.h"
#include "util/Maybe.h"
#include "xml/XmlDom.h"

namespace aapt {

// Parses a configuration density (ex. hdpi, xxhdpi, 234dpi, anydpi, etc).
// Returns Nothing and logs a human friendly error message if the string was not legal.
Maybe<uint16_t> ParseTargetDensityParameter(const android::StringPiece& arg, IDiagnostics* diag);

// Parses a string of the form 'path/to/output.apk:<config>[,<config>...]' and fills in
// `out_path` with the path and `out_split` with the set of ConfigDescriptions.
// Returns false and logs a human friendly error message if the string was not legal.
bool ParseSplitParameter(const android::StringPiece& arg, IDiagnostics* diag, std::string* out_path,
                         SplitConstraints* out_split);

// Parses a set of config filter strings of the form 'en,fr-rFR' and returns an IConfigFilter.
// Returns nullptr and logs a human friendly error message if the string was not legal.
std::unique_ptr<IConfigFilter> ParseConfigFilterParameters(const std::vector<std::string>& args,
                                                           IDiagnostics* diag);

// Adjust the SplitConstraints so that their SDK version is stripped if it
// is less than or equal to the min_sdk. Otherwise the resources that have had
// their SDK version stripped due to min_sdk won't ever match.
std::vector<SplitConstraints> AdjustSplitConstraintsForMinSdk(
    int min_sdk, const std::vector<SplitConstraints>& split_constraints);

// Generates a split AndroidManifest.xml given the split constraints and app info. The resulting
// XmlResource does not need to be linked via XmlReferenceLinker.
// This will never fail/return nullptr.
std::unique_ptr<xml::XmlResource> GenerateSplitManifest(const AppInfo& app_info,
                                                        const SplitConstraints& constraints);

// Extracts relevant info from the AndroidManifest.xml.
Maybe<AppInfo> ExtractAppInfoFromBinaryManifest(const xml::XmlResource& xml_res,
                                                IDiagnostics* diag);

}  // namespace aapt

#endif /* AAPT_SPLIT_UTIL_H */
