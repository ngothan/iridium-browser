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

#ifndef SRC_TINT_API_OPTIONS_BINDING_REMAPPER_H_
#define SRC_TINT_API_OPTIONS_BINDING_REMAPPER_H_

#include <unordered_map>

#include "src/tint/api/common/binding_point.h"
#include "src/tint/lang/core/access.h"

namespace tint {

/// Options used to specify mappings of binding points.
struct BindingRemapperOptions {
    /// BindingPoints is a map of old binding point to new binding point
    using BindingPoints = std::unordered_map<BindingPoint, BindingPoint>;

    /// A map of old binding point to new binding point
    BindingPoints binding_points;

    /// Reflect the fields of this class so that it can be used by tint::ForeachField()
    TINT_REFLECT(binding_points);
};

}  // namespace tint

#endif  // SRC_TINT_API_OPTIONS_BINDING_REMAPPER_H_
