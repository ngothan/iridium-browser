export const description = `
Execution tests for the 'refract' builtin function

T is vecN<I>
I is AbstractFloat, f32, or f16
@const fn refract(e1: T ,e2: T ,e3: I ) -> T
For the incident vector e1 and surface normal e2, and the ratio of indices of
refraction e3, let k = 1.0 -e3*e3* (1.0 - dot(e2,e1) * dot(e2,e1)).
If k < 0.0, returns the refraction vector 0.0, otherwise return the refraction
vector e3*e1- (e3* dot(e2,e1) + sqrt(k)) *e2.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { toVector, TypeF32, TypeF16, TypeVec } from '../../../../../util/conversion.js';
import { FP, FPKind } from '../../../../../util/floating_point.js';
import {
  sparseVectorF32Range,
  sparseVectorF16Range,
  sparseF32Range,
  sparseF16Range,
} from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import { allInputSources, Case, IntervalFilter, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

// Using a bespoke implementation of make*Case and generate*Cases here
// since refract is the only builtin with the API signature
// (vec, vec, scalar) -> vec

/**
 * @returns a Case for `refract`
 * @param kind what type of floating point numbers to operate on
 * @param i the `i` param for the case
 * @param s the `s` param for the case
 * @param r the `r` param for the case
 * @param check what interval checking to apply
 * */
function makeCase(
  kind: FPKind,
  i: number[],
  s: number[],
  r: number,
  check: IntervalFilter
): Case | undefined {
  const fp = FP[kind];
  i = i.map(fp.quantize);
  s = s.map(fp.quantize);
  r = fp.quantize(r);

  const vectors = fp.refractInterval(i, s, r);
  if (check === 'finite' && vectors.some(e => !e.isFinite())) {
    return undefined;
  }

  return {
    input: [toVector(i, fp.scalarBuilder), toVector(s, fp.scalarBuilder), fp.scalarBuilder(r)],
    expected: fp.refractInterval(i, s, r),
  };
}

/**
 * @returns an array of Cases for `refract`
 * @param kind what type of floating point numbers to operate on
 * @param param_is array of inputs to try for the `i` param
 * @param param_ss array of inputs to try for the `s` param
 * @param param_rs array of inputs to try for the `r` param
 * @param check what interval checking to apply
 */
function generateCases(
  kind: FPKind,
  param_is: number[][],
  param_ss: number[][],
  param_rs: number[],
  check: IntervalFilter
): Case[] {
  // Cannot use `cartesianProduct` here due to heterogeneous param types
  return param_is
    .flatMap(i => {
      return param_ss.flatMap(s => {
        return param_rs.map(r => {
          return makeCase(kind, i, s, r, check);
        });
      });
    })
    .filter((c): c is Case => c !== undefined);
}

// Cases: f32_vecN_[non_]const
const f32_vec_cases = ([2, 3, 4] as const)
  .flatMap(n =>
    ([true, false] as const).map(nonConst => ({
      [`f32_vec${n}_${nonConst ? 'non_const' : 'const'}`]: () => {
        return generateCases(
          'f32',
          sparseVectorF32Range(n),
          sparseVectorF32Range(n),
          sparseF32Range(),
          nonConst ? 'unfiltered' : 'finite'
        );
      },
    }))
  )
  .reduce((a, b) => ({ ...a, ...b }), {});

// Cases: f16_vecN_[non_]const
const f16_vec_cases = ([2, 3, 4] as const)
  .flatMap(n =>
    ([true, false] as const).map(nonConst => ({
      [`f16_vec${n}_${nonConst ? 'non_const' : 'const'}`]: () => {
        return generateCases(
          'f16',
          sparseVectorF16Range(n),
          sparseVectorF16Range(n),
          sparseF16Range(),
          nonConst ? 'unfiltered' : 'finite'
        );
      },
    }))
  )
  .reduce((a, b) => ({ ...a, ...b }), {});

export const d = makeCaseCache('refract', {
  ...f32_vec_cases,
  ...f16_vec_cases,
});

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`abstract float tests`)
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();

g.test('f32_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'f32_vec2_const' : 'f32_vec2_non_const'
    );
    await run(
      t,
      builtin('refract'),
      [TypeVec(2, TypeF32), TypeVec(2, TypeF32), TypeF32],
      TypeVec(2, TypeF32),
      t.params,
      cases
    );
  });

g.test('f32_vec3')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'f32_vec3_const' : 'f32_vec3_non_const'
    );
    await run(
      t,
      builtin('refract'),
      [TypeVec(3, TypeF32), TypeVec(3, TypeF32), TypeF32],
      TypeVec(3, TypeF32),
      t.params,
      cases
    );
  });

g.test('f32_vec4')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec4s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'f32_vec4_const' : 'f32_vec4_non_const'
    );
    await run(
      t,
      builtin('refract'),
      [TypeVec(4, TypeF32), TypeVec(4, TypeF32), TypeF32],
      TypeVec(4, TypeF32),
      t.params,
      cases
    );
  });

g.test('f16_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f16 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .beforeAllSubcases(t => {
    t.selectDeviceOrSkipTestCase('shader-f16');
  })
  .fn(async t => {
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'f16_vec2_const' : 'f16_vec2_non_const'
    );
    await run(
      t,
      builtin('refract'),
      [TypeVec(2, TypeF16), TypeVec(2, TypeF16), TypeF16],
      TypeVec(2, TypeF16),
      t.params,
      cases
    );
  });

g.test('f16_vec3')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f16 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .beforeAllSubcases(t => {
    t.selectDeviceOrSkipTestCase('shader-f16');
  })
  .fn(async t => {
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'f16_vec3_const' : 'f16_vec3_non_const'
    );
    await run(
      t,
      builtin('refract'),
      [TypeVec(3, TypeF16), TypeVec(3, TypeF16), TypeF16],
      TypeVec(3, TypeF16),
      t.params,
      cases
    );
  });

g.test('f16_vec4')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f16 tests using vec4s`)
  .params(u => u.combine('inputSource', allInputSources))
  .beforeAllSubcases(t => {
    t.selectDeviceOrSkipTestCase('shader-f16');
  })
  .fn(async t => {
    const cases = await d.get(
      t.params.inputSource === 'const' ? 'f16_vec4_const' : 'f16_vec4_non_const'
    );
    await run(
      t,
      builtin('refract'),
      [TypeVec(4, TypeF16), TypeVec(4, TypeF16), TypeF16],
      TypeVec(4, TypeF16),
      t.params,
      cases
    );
  });
