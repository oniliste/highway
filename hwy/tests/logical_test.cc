// Copyright 2019 Google LLC
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>  // memcmp

#include "hwy/aligned_allocator.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "tests/logical_test.cc"
#include "hwy/foreach_target.h"
#include "hwy/highway.h"
#include "hwy/tests/test_util-inl.h"

HWY_BEFORE_NAMESPACE();
namespace hwy {
namespace HWY_NAMESPACE {

struct TestLogicalInteger {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const auto v0 = Zero(d);
    const auto vi = Iota(d, 0);
    const auto ones = VecFromMask(d, Eq(v0, v0));
    const auto v1 = Set(d, 1);
    const auto vnot1 = Set(d, T(~T(1)));

    HWY_ASSERT_VEC_EQ(d, v0, Not(ones));
    HWY_ASSERT_VEC_EQ(d, ones, Not(v0));
    HWY_ASSERT_VEC_EQ(d, v1, Not(vnot1));
    HWY_ASSERT_VEC_EQ(d, vnot1, Not(v1));

    HWY_ASSERT_VEC_EQ(d, v0, And(v0, vi));
    HWY_ASSERT_VEC_EQ(d, v0, And(vi, v0));
    HWY_ASSERT_VEC_EQ(d, vi, And(vi, vi));

    HWY_ASSERT_VEC_EQ(d, vi, Or(v0, vi));
    HWY_ASSERT_VEC_EQ(d, vi, Or(vi, v0));
    HWY_ASSERT_VEC_EQ(d, vi, Or(vi, vi));

    HWY_ASSERT_VEC_EQ(d, vi, Xor(v0, vi));
    HWY_ASSERT_VEC_EQ(d, vi, Xor(vi, v0));
    HWY_ASSERT_VEC_EQ(d, v0, Xor(vi, vi));

    HWY_ASSERT_VEC_EQ(d, vi, AndNot(v0, vi));
    HWY_ASSERT_VEC_EQ(d, v0, AndNot(vi, v0));
    HWY_ASSERT_VEC_EQ(d, v0, AndNot(vi, vi));

    HWY_ASSERT_VEC_EQ(d, v0, OrAnd(v0, v0, v0));
    HWY_ASSERT_VEC_EQ(d, v0, OrAnd(v0, vi, v0));
    HWY_ASSERT_VEC_EQ(d, v0, OrAnd(v0, v0, vi));
    HWY_ASSERT_VEC_EQ(d, vi, OrAnd(v0, vi, vi));
    HWY_ASSERT_VEC_EQ(d, vi, OrAnd(vi, v0, v0));
    HWY_ASSERT_VEC_EQ(d, vi, OrAnd(vi, vi, v0));
    HWY_ASSERT_VEC_EQ(d, vi, OrAnd(vi, v0, vi));
    HWY_ASSERT_VEC_EQ(d, vi, OrAnd(vi, vi, vi));

    auto v = vi;
    v = And(v, vi);
    HWY_ASSERT_VEC_EQ(d, vi, v);
    v = And(v, v0);
    HWY_ASSERT_VEC_EQ(d, v0, v);

    v = Or(v, vi);
    HWY_ASSERT_VEC_EQ(d, vi, v);
    v = Or(v, v0);
    HWY_ASSERT_VEC_EQ(d, vi, v);

    v = Xor(v, vi);
    HWY_ASSERT_VEC_EQ(d, v0, v);
    v = Xor(v, v0);
    HWY_ASSERT_VEC_EQ(d, v0, v);
  }
};

HWY_NOINLINE void TestAllLogicalInteger() {
  ForIntegerTypes(ForPartialVectors<TestLogicalInteger>());
}

struct TestLogicalFloat {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const auto v0 = Zero(d);
    const auto vi = Iota(d, 0);

    HWY_ASSERT_VEC_EQ(d, v0, And(v0, vi));
    HWY_ASSERT_VEC_EQ(d, v0, And(vi, v0));
    HWY_ASSERT_VEC_EQ(d, vi, And(vi, vi));

    HWY_ASSERT_VEC_EQ(d, vi, Or(v0, vi));
    HWY_ASSERT_VEC_EQ(d, vi, Or(vi, v0));
    HWY_ASSERT_VEC_EQ(d, vi, Or(vi, vi));

    HWY_ASSERT_VEC_EQ(d, vi, Xor(v0, vi));
    HWY_ASSERT_VEC_EQ(d, vi, Xor(vi, v0));
    HWY_ASSERT_VEC_EQ(d, v0, Xor(vi, vi));

    HWY_ASSERT_VEC_EQ(d, vi, AndNot(v0, vi));
    HWY_ASSERT_VEC_EQ(d, v0, AndNot(vi, v0));
    HWY_ASSERT_VEC_EQ(d, v0, AndNot(vi, vi));

    auto v = vi;
    v = And(v, vi);
    HWY_ASSERT_VEC_EQ(d, vi, v);
    v = And(v, v0);
    HWY_ASSERT_VEC_EQ(d, v0, v);

    v = Or(v, vi);
    HWY_ASSERT_VEC_EQ(d, vi, v);
    v = Or(v, v0);
    HWY_ASSERT_VEC_EQ(d, vi, v);

    v = Xor(v, vi);
    HWY_ASSERT_VEC_EQ(d, v0, v);
    v = Xor(v, v0);
    HWY_ASSERT_VEC_EQ(d, v0, v);
  }
};

HWY_NOINLINE void TestAllLogicalFloat() {
  ForFloatTypes(ForPartialVectors<TestLogicalFloat>());
}

struct TestCopySign {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const auto v0 = Zero(d);
    const auto vp = Iota(d, 1);
    const auto vn = Iota(d, T(-1E5));  // assumes N < 10^5

    // Zero remains zero regardless of sign
    HWY_ASSERT_VEC_EQ(d, v0, CopySign(v0, v0));
    HWY_ASSERT_VEC_EQ(d, v0, CopySign(v0, vp));
    HWY_ASSERT_VEC_EQ(d, v0, CopySign(v0, vn));
    HWY_ASSERT_VEC_EQ(d, v0, CopySignToAbs(v0, v0));
    HWY_ASSERT_VEC_EQ(d, v0, CopySignToAbs(v0, vp));
    HWY_ASSERT_VEC_EQ(d, v0, CopySignToAbs(v0, vn));

    // Positive input, positive sign => unchanged
    HWY_ASSERT_VEC_EQ(d, vp, CopySign(vp, vp));
    HWY_ASSERT_VEC_EQ(d, vp, CopySignToAbs(vp, vp));

    // Positive input, negative sign => negated
    HWY_ASSERT_VEC_EQ(d, Neg(vp), CopySign(vp, vn));
    HWY_ASSERT_VEC_EQ(d, Neg(vp), CopySignToAbs(vp, vn));

    // Negative input, negative sign => unchanged
    HWY_ASSERT_VEC_EQ(d, vn, CopySign(vn, vn));

    // Negative input, positive sign => negated
    HWY_ASSERT_VEC_EQ(d, Neg(vn), CopySign(vn, vp));
  }
};

struct TestIfVecThenElse {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    RandomState rng;

    using TU = MakeUnsigned<T>;  // For all-one mask
    const Rebind<TU, D> du;
    const size_t N = Lanes(d);
    auto in1 = AllocateAligned<T>(N);
    auto in2 = AllocateAligned<T>(N);
    auto vec_lanes = AllocateAligned<TU>(N);
    auto expected = AllocateAligned<T>(N);

    // Each lane should have a chance of having mask=true.
    for (size_t rep = 0; rep < AdjustedReps(200); ++rep) {
      for (size_t i = 0; i < N; ++i) {
        in1[i] = static_cast<T>(Random32(&rng));
        in2[i] = static_cast<T>(Random32(&rng));
        vec_lanes[i] = (Random32(&rng) & 16) ? static_cast<TU>(~TU(0)) : TU(0);
      }

      const auto v1 = Load(d, in1.get());
      const auto v2 = Load(d, in2.get());
      const auto vec = BitCast(d, Load(du, vec_lanes.get()));

      for (size_t i = 0; i < N; ++i) {
        expected[i] = vec_lanes[i] ? in1[i] : in2[i];
      }
      HWY_ASSERT_VEC_EQ(d, expected.get(), IfVecThenElse(vec, v1, v2));
    }
  }
};

HWY_NOINLINE void TestAllIfVecThenElse() {
  ForAllTypes(ForPartialVectors<TestIfVecThenElse>());
}

HWY_NOINLINE void TestAllCopySign() {
  ForFloatTypes(ForPartialVectors<TestCopySign>());
}

struct TestZeroIfNegative {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const auto v0 = Zero(d);
    const auto vp = Iota(d, 1);
    const auto vn = Iota(d, T(-1E5));  // assumes N < 10^5

    // Zero and positive remain unchanged
    HWY_ASSERT_VEC_EQ(d, v0, ZeroIfNegative(v0));
    HWY_ASSERT_VEC_EQ(d, vp, ZeroIfNegative(vp));

    // Negative are all replaced with zero
    HWY_ASSERT_VEC_EQ(d, v0, ZeroIfNegative(vn));
  }
};

HWY_NOINLINE void TestAllZeroIfNegative() {
  ForFloatTypes(ForPartialVectors<TestZeroIfNegative>());
}

struct TestIfNegative {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const auto v0 = Zero(d);
    const auto vp = Iota(d, 1);
    const auto vn = Or(vp, SignBit(d));

    // Zero and positive remain unchanged
    HWY_ASSERT_VEC_EQ(d, v0, IfNegativeThenElse(v0, vn, v0));
    HWY_ASSERT_VEC_EQ(d, vn, IfNegativeThenElse(v0, v0, vn));
    HWY_ASSERT_VEC_EQ(d, vp, IfNegativeThenElse(vp, vn, vp));
    HWY_ASSERT_VEC_EQ(d, vn, IfNegativeThenElse(vp, vp, vn));

    // Negative are replaced with 2nd arg
    HWY_ASSERT_VEC_EQ(d, v0, IfNegativeThenElse(vn, v0, vp));
    HWY_ASSERT_VEC_EQ(d, vn, IfNegativeThenElse(vn, vn, v0));
    HWY_ASSERT_VEC_EQ(d, vp, IfNegativeThenElse(vn, vp, vn));
  }
};

HWY_NOINLINE void TestAllIfNegative() {
  ForFloatTypes(ForPartialVectors<TestIfNegative>());
  ForSignedTypes(ForPartialVectors<TestIfNegative>());
}

struct TestBroadcastSignBit {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const auto s0 = Zero(d);
    const auto s1 = Set(d, -1);  // all bit set
    const auto vpos = And(Iota(d, 0), Set(d, LimitsMax<T>()));
    const auto vneg = Sub(s1, vpos);

    HWY_ASSERT_VEC_EQ(d, s0, BroadcastSignBit(vpos));
    HWY_ASSERT_VEC_EQ(d, s0, BroadcastSignBit(Set(d, LimitsMax<T>())));

    HWY_ASSERT_VEC_EQ(d, s1, BroadcastSignBit(vneg));
    HWY_ASSERT_VEC_EQ(d, s1, BroadcastSignBit(Set(d, LimitsMin<T>())));
    HWY_ASSERT_VEC_EQ(d, s1, BroadcastSignBit(Set(d, LimitsMin<T>() / 2)));
  }
};

HWY_NOINLINE void TestAllBroadcastSignBit() {
  ForSignedTypes(ForPartialVectors<TestBroadcastSignBit>());
}

struct TestTestBit {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const size_t kNumBits = sizeof(T) * 8;
    for (size_t i = 0; i < kNumBits; ++i) {
      const auto bit1 = Set(d, T(1ull << i));
      const auto bit2 = Set(d, T(1ull << ((i + 1) % kNumBits)));
      const auto bit3 = Set(d, T(1ull << ((i + 2) % kNumBits)));
      const auto bits12 = Or(bit1, bit2);
      const auto bits23 = Or(bit2, bit3);
      HWY_ASSERT(AllTrue(d, TestBit(bit1, bit1)));
      HWY_ASSERT(AllTrue(d, TestBit(bits12, bit1)));
      HWY_ASSERT(AllTrue(d, TestBit(bits12, bit2)));

      HWY_ASSERT(AllFalse(d, TestBit(bits12, bit3)));
      HWY_ASSERT(AllFalse(d, TestBit(bits23, bit1)));
      HWY_ASSERT(AllFalse(d, TestBit(bit1, bit2)));
      HWY_ASSERT(AllFalse(d, TestBit(bit2, bit1)));
      HWY_ASSERT(AllFalse(d, TestBit(bit1, bit3)));
      HWY_ASSERT(AllFalse(d, TestBit(bit3, bit1)));
      HWY_ASSERT(AllFalse(d, TestBit(bit2, bit3)));
      HWY_ASSERT(AllFalse(d, TestBit(bit3, bit2)));
    }
  }
};

HWY_NOINLINE void TestAllTestBit() {
  ForIntegerTypes(ForPartialVectors<TestTestBit>());
}

struct TestPopulationCount {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    RandomState rng;
    size_t N = Lanes(d);
    auto data = AllocateAligned<T>(N);
    auto popcnt = AllocateAligned<T>(N);
    for (size_t i = 0; i < AdjustedReps(1 << 18) / N; i++) {
      for (size_t i = 0; i < N; i++) {
        data[i] = static_cast<T>(rng());
        popcnt[i] = static_cast<T>(PopCount(data[i]));
      }
      HWY_ASSERT_VEC_EQ(d, popcnt.get(), PopulationCount(Load(d, data.get())));
    }
  }
};

HWY_NOINLINE void TestAllPopulationCount() {
  ForUnsignedTypes(ForPartialVectors<TestPopulationCount>());
}

// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace hwy
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace hwy {
HWY_BEFORE_TEST(HwyLogicalTest);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllLogicalInteger);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllLogicalFloat);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllIfVecThenElse);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllCopySign);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllZeroIfNegative);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllIfNegative);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllBroadcastSignBit);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllTestBit);
HWY_EXPORT_AND_TEST_P(HwyLogicalTest, TestAllPopulationCount);
}  // namespace hwy

#endif
