// Copyright 2020 Google LLC
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "hwy/examples/skeleton.h"

#include <stdio.h>

// First undef to prevent error when re-included.
#undef HWY_TARGET_INCLUDE
// For runtime dispatch, specify the name of the current file (unfortunately
// __FILE__ is not reliable) so that foreach_target.h can re-include it.
#define HWY_TARGET_INCLUDE "hwy/examples/skeleton.cc"
// Generates code for each enabled target by re-including this source file.
#include "hwy/foreach_target.h"

// Must come after foreach_target.h to avoid redefinition errors.
#include "hwy/highway.h"

// Optional, can instead add HWY_ATTR to all functions.
HWY_BEFORE_NAMESPACE();
namespace skeleton {
// This namespace name is unique per target, which allows code for multiple
// targets to co-exist in the same translation unit.
namespace HWY_NAMESPACE {

// Highway ops reside here; ADL does not find templates nor builtins.
using namespace hwy::HWY_NAMESPACE;

// Computes log2 by converting to a vector of floats. Compiled once per target.
template <class DF>
HWY_ATTR_NO_MSAN void OneFloorLog2(const DF df,
                                   const uint8_t* HWY_RESTRICT values,
                                   uint8_t* HWY_RESTRICT log2) {
  // Type tags for converting to other element types (Rebind = same count).
  const RebindToSigned<DF> d32;
  const Rebind<uint8_t, DF> d8;

  const auto u8 = Load(d8, values);
  const auto bits = BitCast(d32, ConvertTo(df, PromoteTo(d32, u8)));
  const auto exponent = Sub(ShiftRight<23>(bits), Set(d32, 127));
  Store(DemoteTo(d8, exponent), d8, log2);
}

void CodepathDemo() {
  // Highway defaults to portability, but per-target codepaths may be selected
  // via #if HWY_TARGET == HWY_SSE4 or by testing capability macros:
#if HWY_HAVE_INTEGER64
  const char* gather = "Has int64";
#else
  const char* gather = "No int64";
#endif
  printf("Target %s: %s\n", hwy::TargetName(HWY_TARGET), gather);
}

void FloorLog2(const uint8_t* HWY_RESTRICT values, size_t count,
               uint8_t* HWY_RESTRICT log2) {
  CodepathDemo();

  const ScalableTag<float> df;
  const size_t N = Lanes(df);
  size_t i = 0;
  for (; i + N <= count; i += N) {
    OneFloorLog2(df, values + i, log2 + i);
  }
  for (; i < count; ++i) {
    CappedTag<float, 1> d1;
    OneFloorLog2(d1, values + i, log2 + i);
  }
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace skeleton
HWY_AFTER_NAMESPACE();

// The table of pointers to the various implementations in HWY_NAMESPACE must
// be compiled only once (foreach_target #includes this file multiple times).
// HWY_ONCE is true for only one of these 'compilation passes'.
#if HWY_ONCE

namespace skeleton {

// This macro declares a static array used for dynamic dispatch; it resides in
// the same outer namespace that contains FloorLog2.
HWY_EXPORT(FloorLog2);

// This function is optional and only needed in the case of exposing it in the
// header file. Otherwise using HWY_DYNAMIC_DISPATCH(FloorLog2) in this module
// is equivalent to inlining this function.
void CallFloorLog2(const uint8_t* HWY_RESTRICT in, const size_t count,
                   uint8_t* HWY_RESTRICT out) {
  // This must reside outside of HWY_NAMESPACE because it references (calls the
  // appropriate one from) the per-target implementations there.
  return HWY_DYNAMIC_DISPATCH(FloorLog2)(in, count, out);
}

// Optional: anything to compile only once, e.g. non-SIMD implementations of
// public functions provided by this module, can go inside #if HWY_ONCE.

}  // namespace skeleton
#endif  // HWY_ONCE
