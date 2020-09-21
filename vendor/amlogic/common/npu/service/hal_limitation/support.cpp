/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#include <algorithm>
#include <iostream>
#include "support.hpp"

namespace hal {
namespace limitation {
bool IArgList::match(const std::vector<nnrt::OperandType>& args) const {
    auto tailOptionalArgs =
        std::count_if(m_Arguments.begin(), m_Arguments.end(), [](const ArgsType::value_type& arg) {
            return arg.second.second == OPTIONAL;
        });

    // If any argument in the middle of the list declared as an optional argument, take this
    // argument as required
    auto requiredArgc = m_Arguments.size() - tailOptionalArgs;

    if (args.size() < requiredArgc || args.size() > m_Arguments.size()) {
        //VIV: [TODO] Replace std::cout with logging module
        //std::cout << "{debug}" << __FUNCTION__ << " : "
        //          << "Argument not match-> size not match" << std::endl;
        return false;
    }

    bool isMatch = true;
    auto a = args.begin();
    auto b = m_Arguments.begin();
    while(a != args.end() && b != m_Arguments.end()) {
        if (*a != b->second.first) {
            isMatch = false;
            break;
        }
        ++a, ++b;
    }

    return isMatch;
}

int32_t IArgList::ArgPos(const std::string& argName) const {
    int32_t pos{0};

    for (auto& arg : m_Arguments) {
        if (arg.first == argName) {
            return pos;
        }
        ++pos;
    }

    return -1;
}

void IArgList::printf() const {
    std::cout << std::endl;
    std::cout << "|--> display arguments start<--|" << std::endl;
    for (auto& arg : m_Arguments) {
        std::cout << "[" << arg.first << "] : data_type =" << (uint8_t)arg.second.first
                  << ", optional = " << arg.second.second << std::endl;
    }
    std::cout << std::endl;

    std::cout << "|--> display arguments end  <--|" << std::endl;
    std::cout << std::endl;
}

const IArgList* OpSpecCollection::match(const std::string& opName,
                                        const std::vector<nnrt::OperandType>& args) const {
    auto found = m_Collection.find(opName);
    if (m_Collection.end() != found) {
        for (const auto& argList : found->second) {
            if (argList->match(args)) {
                return argList;
            }
        }
    }

    return nullptr;
}
}
}
