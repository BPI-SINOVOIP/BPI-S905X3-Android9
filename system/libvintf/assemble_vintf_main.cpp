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

#include <getopt.h>

#include <iostream>

#include <android-base/strings.h>
#include <vintf/AssembleVintf.h>
#include "utils.h"

void help() {
    std::cerr << "assemble_vintf: Checks if a given manifest / matrix file is valid and \n"
                 "    fill in build-time flags into the given file.\n"
                 "assemble_vintf -h\n"
                 "               Display this help text.\n"
                 "assemble_vintf -i <input file>[:<input file>[...]] [-o <output file>] [-m]\n"
                 "               [-c [<check file>]]\n"
                 "               Fill in build-time flags into the given file.\n"
                 "    -i <input file>[:<input file>[...]]\n"
                 "               A list of input files. Format is automatically detected for the\n"
                 "               first file, and the remaining files must have the same format.\n"
                 "               Files other than the first file should only have <hal> defined;\n"
                 "               other entries are ignored.\n"
                 "    -o <output file>\n"
                 "               Optional output file. If not specified, write to stdout.\n"
                 "    -m\n"
                 "               a compatible compatibility matrix is\n"
                 "               generated instead; for example, given a device manifest,\n"
                 "               a framework compatibility matrix is generated. This flag\n"
                 "               is ignored when input is a compatibility matrix.\n"
                 "    -c [<check file>]\n"
                 "               After writing the output file, check compatibility between\n"
                 "               output file and check file.\n"
                 "               If -c is set but the check file is not specified, a warning\n"
                 "               message is written to stderr. Return 0.\n"
                 "               If the check file is specified but is not compatible, an error\n"
                 "               message is written to stderr. Return 1.\n"
                 "    --kernel=<version>:<android-base.cfg>[:<android-base-arch.cfg>[...]]\n"
                 "               Add a kernel entry to framework compatibility matrix.\n"
                 "               Ignored for other input format.\n"
                 "               <version> has format: 3.18\n"
                 "               <android-base.cfg> is the location of android-base.cfg\n"
                 "               <android-base-arch.cfg> is the location of an optional\n"
                 "               arch-specific config fragment, more than one may be specified\n"
                 "    -l, --hals-only\n"
                 "               Output has only <hal> entries. Cannot be used with -n.\n"
                 "    -n, --no-hals\n"
                 "               Output has no <hal> entries (but all other entries).\n"
                 "               Cannot be used with -l.\n";
}

int main(int argc, char** argv) {
    using namespace ::android::vintf;
    const struct option longopts[] = {{"kernel", required_argument, NULL, 'k'},
                                      {"hals-only", no_argument, NULL, 'l'},
                                      {"no-hals", no_argument, NULL, 'n'},
                                      {0, 0, 0, 0}};

    std::string outFilePath;
    auto assembleVintf = AssembleVintf::newInstance();
    int res;
    int optind;
    while ((res = getopt_long(argc, argv, "hi:o:mc:nl", longopts, &optind)) >= 0) {
        switch (res) {
            case 'i': {
                for (const auto& inFilePath : ::android::base::Split(optarg, ":")) {
                    if (!assembleVintf->openInFile(inFilePath.c_str())) {
                        std::cerr << "Failed to open " << inFilePath << std::endl;
                        return 1;
                    }
                }
            } break;

            case 'o': {
                outFilePath = optarg;
                if (!assembleVintf->openOutFile(optarg)) {
                    std::cerr << "Failed to open " << optarg << std::endl;
                    return 1;
                }
            } break;

            case 'm': {
                assembleVintf->setOutputMatrix();
            } break;

            case 'c': {
                if (!assembleVintf->openCheckFile(optarg)) {
                    std::cerr << "Failed to open " << optarg << std::endl;
                    return 1;
                }
            } break;

            case 'k': {
                if (!assembleVintf->addKernel(optarg)) {
                    std::cerr << "ERROR: Unrecognized --kernel argument." << std::endl;
                    return 1;
                }
            } break;

            case 'l': {
                if (!assembleVintf->setHalsOnly()) {
                    return 1;
                }
            } break;

            case 'n': {
                if (!assembleVintf->setNoHals()) {
                    return 1;
                }
            } break;

            case 'h':
            default: {
                help();
                return 1;
            } break;
        }
    }

    bool success = assembleVintf->assemble();

    return success ? 0 : 1;
}
