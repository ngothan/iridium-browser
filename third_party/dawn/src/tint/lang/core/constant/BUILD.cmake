# Copyright 2023 The Tint Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

################################################################################
# File generated by 'tools/src/cmd/gen' using the template:
#   tools/src/cmd/gen/build/BUILD.cmake.tmpl
#
# To regenerate run: './tools/run gen'
#
#                       Do not modify this file directly
################################################################################

################################################################################
# Target:    tint_lang_core_constant
# Kind:      lib
################################################################################
tint_add_target(tint_lang_core_constant lib
  lang/core/constant/clone_context.h
  lang/core/constant/composite.cc
  lang/core/constant/composite.h
  lang/core/constant/eval.cc
  lang/core/constant/eval.h
  lang/core/constant/manager.cc
  lang/core/constant/manager.h
  lang/core/constant/node.cc
  lang/core/constant/node.h
  lang/core/constant/scalar.cc
  lang/core/constant/scalar.h
  lang/core/constant/splat.cc
  lang/core/constant/splat.h
  lang/core/constant/value.cc
  lang/core/constant/value.h
)

tint_target_add_dependencies(tint_lang_core_constant lib
  tint_lang_core
  tint_lang_core_type
  tint_utils_containers
  tint_utils_diagnostic
  tint_utils_ice
  tint_utils_id
  tint_utils_macros
  tint_utils_math
  tint_utils_memory
  tint_utils_result
  tint_utils_rtti
  tint_utils_symbol
  tint_utils_text
  tint_utils_traits
)

################################################################################
# Target:    tint_lang_core_constant_test
# Kind:      test
################################################################################
tint_add_target(tint_lang_core_constant_test test
  lang/core/constant/composite_test.cc
  lang/core/constant/eval_binary_op_test.cc
  lang/core/constant/eval_bitcast_test.cc
  lang/core/constant/eval_builtin_test.cc
  lang/core/constant/eval_construction_test.cc
  lang/core/constant/eval_conversion_test.cc
  lang/core/constant/eval_indexing_test.cc
  lang/core/constant/eval_member_access_test.cc
  lang/core/constant/eval_runtime_semantics_test.cc
  lang/core/constant/eval_test.h
  lang/core/constant/eval_unary_op_test.cc
  lang/core/constant/helper_test.h
  lang/core/constant/manager_test.cc
  lang/core/constant/scalar_test.cc
  lang/core/constant/splat_test.cc
  lang/core/constant/value_test.cc
)

tint_target_add_dependencies(tint_lang_core_constant_test test
  tint_api_common
  tint_lang_core
  tint_lang_core_constant
  tint_lang_core_intrinsic
  tint_lang_core_ir
  tint_lang_core_type
  tint_lang_core_type_test
  tint_lang_wgsl
  tint_lang_wgsl_ast
  tint_lang_wgsl_intrinsic
  tint_lang_wgsl_program
  tint_lang_wgsl_reader
  tint_lang_wgsl_resolver
  tint_lang_wgsl_resolver_test
  tint_lang_wgsl_sem
  tint_utils_containers
  tint_utils_diagnostic
  tint_utils_ice
  tint_utils_id
  tint_utils_macros
  tint_utils_math
  tint_utils_memory
  tint_utils_reflection
  tint_utils_result
  tint_utils_rtti
  tint_utils_symbol
  tint_utils_text
  tint_utils_traits
)

tint_target_add_external_dependencies(tint_lang_core_constant_test test
  "gtest"
)
