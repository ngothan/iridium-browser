// Copyright 2021 The Tint Authors.
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

#ifndef SRC_TINT_LANG_WGSL_AST_TRANSFORM_REMOVE_UNREACHABLE_STATEMENTS_H_
#define SRC_TINT_LANG_WGSL_AST_TRANSFORM_REMOVE_UNREACHABLE_STATEMENTS_H_

#include <string>
#include <unordered_map>

#include "src/tint/lang/wgsl/ast/transform/transform.h"

namespace tint::ast::transform {

/// RemoveUnreachableStatements is a Transform that removes all statements
/// marked as unreachable.
class RemoveUnreachableStatements final : public Castable<RemoveUnreachableStatements, Transform> {
  public:
    /// Constructor
    RemoveUnreachableStatements();

    /// Destructor
    ~RemoveUnreachableStatements() override;

    /// @copydoc Transform::Apply
    ApplyResult Apply(const Program& program,
                      const DataMap& inputs,
                      DataMap& outputs) const override;
};

}  // namespace tint::ast::transform

#endif  // SRC_TINT_LANG_WGSL_AST_TRANSFORM_REMOVE_UNREACHABLE_STATEMENTS_H_
