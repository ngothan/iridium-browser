// Copyright 2022 The Tint Authors.
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

#ifndef SRC_TINT_LANG_WGSL_HELPERS_FLATTEN_BINDINGS_H_
#define SRC_TINT_LANG_WGSL_HELPERS_FLATTEN_BINDINGS_H_

#include <optional>
#include "src/tint/lang/wgsl/program/program.h"

namespace tint::writer {

/// If needed, remaps resource numbers of `program` to a flat namespace: all in
/// group 0 within unique binding numbers.
/// @param program A valid program
/// @return A new program with bindings remapped if needed
std::optional<Program> FlattenBindings(const Program& program);

}  // namespace tint::writer

#endif  // SRC_TINT_LANG_WGSL_HELPERS_FLATTEN_BINDINGS_H_
