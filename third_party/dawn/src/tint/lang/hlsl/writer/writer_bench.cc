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

#include <string>

#include "src/tint/cmd/bench/bench.h"
#include "src/tint/lang/hlsl/writer/writer.h"

namespace tint::hlsl::writer {
namespace {

void GenerateHLSL(benchmark::State& state, std::string input_name) {
    auto res = bench::LoadProgram(input_name);
    if (!res) {
        state.SkipWithError(res.Failure().reason.str());
        return;
    }
    for (auto _ : state) {
        auto gen_res = Generate(res->program, {});
        if (!gen_res) {
            state.SkipWithError(gen_res.Failure().reason.str());
        }
    }
}

TINT_BENCHMARK_PROGRAMS(GenerateHLSL);

}  // namespace
}  // namespace tint::hlsl::writer
