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

#ifndef SRC_TINT_LANG_HLSL_WRITER_AST_RAISE_CALCULATE_ARRAY_LENGTH_H_
#define SRC_TINT_LANG_HLSL_WRITER_AST_RAISE_CALCULATE_ARRAY_LENGTH_H_

#include <string>

#include "src/tint/lang/wgsl/ast/internal_attribute.h"
#include "src/tint/lang/wgsl/ast/transform/transform.h"

namespace tint::hlsl::writer {

/// CalculateArrayLength is a transform used to replace calls to arrayLength()
/// with a value calculated from the size of the storage buffer.
///
/// @note Depends on the following transforms to have been run first:
/// * SimplifyPointers
class CalculateArrayLength final
    : public Castable<CalculateArrayLength, ast::transform::Transform> {
  public:
    /// BufferSizeIntrinsic is an InternalAttribute that's applied to intrinsic
    /// functions used to obtain the runtime size of a storage buffer.
    class BufferSizeIntrinsic final : public Castable<BufferSizeIntrinsic, ast::InternalAttribute> {
      public:
        /// Constructor
        /// @param generation_id the identifier of the program that owns this node
        /// @param nid the unique node identifier
        BufferSizeIntrinsic(GenerationID generation_id, ast::NodeID nid);
        /// Destructor
        ~BufferSizeIntrinsic() override;

        /// @return "buffer_size"
        std::string InternalName() const override;

        /// Performs a deep clone of this object using the program::CloneContext `ctx`.
        /// @param ctx the clone context
        /// @return the newly cloned object
        const BufferSizeIntrinsic* Clone(ast::CloneContext& ctx) const override;
    };

    /// Constructor
    CalculateArrayLength();
    /// Destructor
    ~CalculateArrayLength() override;

    /// @copydoc ast::transform::Transform::Apply
    ApplyResult Apply(const Program& program,
                      const ast::transform::DataMap& inputs,
                      ast::transform::DataMap& outputs) const override;
};

}  // namespace tint::hlsl::writer

#endif  // SRC_TINT_LANG_HLSL_WRITER_AST_RAISE_CALCULATE_ARRAY_LENGTH_H_
