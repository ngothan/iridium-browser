// Copyright 2023 The Tint Authors.
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

#ifndef SRC_TINT_LANG_WGSL_HELPERS_APPLY_SUBSTITUTE_OVERRIDES_H_
#define SRC_TINT_LANG_WGSL_HELPERS_APPLY_SUBSTITUTE_OVERRIDES_H_

#include <optional>

// Forward declarations
namespace tint {
class Program;
}

namespace tint::wgsl {

/// If needed, returns a new program with all `override` declarations substituted with `const`
/// variables.
/// @param program A valid program
/// @return A new program with `override`s substituted, or std::nullopt if the program has no
/// `override`s.
std::optional<Program> ApplySubstituteOverrides(const Program& program);

}  // namespace tint::wgsl

#endif  // SRC_TINT_LANG_WGSL_HELPERS_APPLY_SUBSTITUTE_OVERRIDES_H_
