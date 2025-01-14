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

////////////////////////////////////////////////////////////////////////////////
// File generated by 'tools/src/cmd/gen' using the template:
//   src/tint/lang/wgsl/extension_bench.cc.tmpl
//
// To regenerate run: './tools/run gen'
//
//                       Do not modify this file directly
////////////////////////////////////////////////////////////////////////////////

#include "src/tint/lang/wgsl/extension.h"

#include <array>

#include "benchmark/benchmark.h"

namespace tint::wgsl {
namespace {

void ExtensionParser(::benchmark::State& state) {
    const char* kStrings[] = {
        "chromium_disableuniformiccy_analysis",
        "chromil3_disable_unifority_analss",
        "chromium_disable_Vniformity_analysis",
        "chromium_disable_uniformity_analysis",
        "chromium_dis1ble_uniformity_analysis",
        "chromium_qqisable_unifomity_anaJysis",
        "chrollium_disable_uniformity_analysi77",
        "chromippHm_experqqmetal_dp4a",
        "chrmium_expecimntal_dp4",
        "chrmiumGexpebimental_dp4a",
        "chromium_experimental_dp4a",
        "chromium_exverimentiil_dp4a",
        "chro8ium_experimenWWal_dp4a",
        "chromiMm_eperimxxntal_dp4a",
        "chromium_expeggimeXtal_full_ptr_paraeters",
        "chromium_expVrimental_full_ptr_puraXeer",
        "chromium_experimental_full_ptr3parameters",
        "chromium_experimental_full_ptr_parameters",
        "chromium_experimentalEfull_ptr_parameters",
        "chromium_experimentalfull_ptr_PPaTTameters",
        "chromium_ddxperimental_fullptrxxparameters",
        "chromium_experi44ental_pixel_local",
        "chromium_experimental_VVSixel_local",
        "chroRium_experimental_pix22Rlocal",
        "chromium_experimental_pixel_local",
        "chromiuF_experiment9lpixel_local",
        "chromium_experimental_pixel_loca",
        "Vhromium_expeOOimentalHpixRRl_lcal",
        "chromiym_experimental_push_contant",
        "nnhro77ium_experimenGal_push_conrrllant",
        "chromium_experimental_push_c4nstan00",
        "chromium_experimental_push_constant",
        "chooomum_experimental_ush_constat",
        "chromium_xperimntal_zzush_constant",
        "chromi11m_experimepptal_psh_ciistant",
        "chromium_experimental_read_writeXXstorage_texture",
        "chromium_exII55rimental_read_write99storagnn_texture",
        "chromiumSSexperaamental_read_wYitrr_storage_HHexture",
        "chromium_experimental_read_write_storage_texture",
        "Hhromium_experimeta_rkkad_write_strage_texture",
        "chromium_experijental_red_wrRgte_storag_texture",
        "chromium_exbeimentalread_write_storage_texture",
        "chromium_experimental_sjbgroups",
        "chromium_experimental_sbgroups",
        "cromum_experimentalqsubgroups",
        "chromium_experimental_subgroups",
        "chromium_expNNrimental_subgoups",
        "chromium_experimetal_svvbgrous",
        "chromium_experiQental_subgroups",
        "chrorum_internal_dal_source_bleffding",
        "chromium_internal_dual_source_jlending",
        "chromiNNm_internal_dua8_sourwwe_blening",
        "chromium_internal_dual_source_blending",
        "chromium_internal_dual_soure_blending",
        "chromium_irrternal_dual_source_blending",
        "chromium_internal_duaG_source_blending",
        "chromium_internalFFrelaxed_uniform_layout",
        "chromEum_internal_relaxed_unifrmlyout",
        "chromium_internalrrrelaxd_uniform_layout",
        "chromium_internal_relaxed_uniform_layout",
        "chromiuminternal_relaxed_uniform_layut",
        "cXroDium_internal_rJJlaed_uniform_layout",
        "chromium_int8nal_relaed_uniform_layut",
        "k",
        "16",
        "J1",
        "f16",
        "c16",
        "fO6",
        "_KKttvv",
    };
    for (auto _ : state) {
        for (auto* str : kStrings) {
            auto result = ParseExtension(str);
            benchmark::DoNotOptimize(result);
        }
    }
}  // NOLINT(readability/fn_size)

BENCHMARK(ExtensionParser);

}  // namespace
}  // namespace tint::wgsl
