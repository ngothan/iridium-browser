// Copyright 2020 The Tint Authors.
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

#ifndef SRC_TINT_LANG_SPIRV_WRITER_WRITER_H_
#define SRC_TINT_LANG_SPIRV_WRITER_WRITER_H_

#include <string>

#include "src/tint/lang/spirv/writer/common/options.h"
#include "src/tint/lang/spirv/writer/output.h"
#include "src/tint/utils/diagnostic/diagnostic.h"
#include "src/tint/utils/result/result.h"

// Forward declarations
namespace tint {
class Program;
}

namespace tint::spirv::writer {

/// Generate SPIR-V for a program, according to a set of configuration options.
/// The result will contain the SPIR-V or failure.
/// @param program the program to translate to SPIR-V
/// @param options the configuration options to use when generating SPIR-V
/// @returns the resulting SPIR-V and supplementary information, or failure.
Result<Output> Generate(const Program& program, const Options& options);

}  // namespace tint::spirv::writer

#endif  // SRC_TINT_LANG_SPIRV_WRITER_WRITER_H_
