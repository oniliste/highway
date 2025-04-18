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
#include <string.h>  // memset

#include "hwy/base.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "tests/swizzle_test.cc"
#include "hwy/foreach_target.h"
#include "hwy/highway.h"
#include "hwy/tests/test_util-inl.h"

HWY_BEFORE_NAMESPACE();
namespace hwy {
namespace HWY_NAMESPACE {

struct TestGetLane {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const auto v = Iota(d, T(1));
    HWY_ASSERT_EQ(T(1), GetLane(v));
  }
};

HWY_NOINLINE void TestAllGetLane() {
  ForAllTypes(ForPartialVectors<TestGetLane>());
}

struct TestDupEven {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const size_t N = Lanes(d);
    auto expected = AllocateAligned<T>(N);
    for (size_t i = 0; i < N; ++i) {
      expected[i] = static_cast<T>((static_cast<int>(i) & ~1) + 1);
    }
    HWY_ASSERT_VEC_EQ(d, expected.get(), DupEven(Iota(d, 1)));
  }
};

HWY_NOINLINE void TestAllDupEven() {
  ForUIF3264(ForShrinkableVectors<TestDupEven>());
}

struct TestDupOdd {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
#if HWY_TARGET != HWY_SCALAR
    const size_t N = Lanes(d);
    auto expected = AllocateAligned<T>(N);
    for (size_t i = 0; i < N; ++i) {
      expected[i] = static_cast<T>((static_cast<int>(i) & ~1) + 2);
    }
    HWY_ASSERT_VEC_EQ(d, expected.get(), DupOdd(Iota(d, 1)));
#else
    (void)d;
#endif
  }
};

HWY_NOINLINE void TestAllDupOdd() {
  ForUIF3264(ForShrinkableVectors<TestDupOdd>());
}

struct TestOddEven {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const size_t N = Lanes(d);
    const auto even = Iota(d, 1);
    const auto odd = Iota(d, static_cast<T>(1 + N));
    auto expected = AllocateAligned<T>(N);
    for (size_t i = 0; i < N; ++i) {
      expected[i] = static_cast<T>(1 + i + ((i & 1) ? N : 0));
    }
    HWY_ASSERT_VEC_EQ(d, expected.get(), OddEven(odd, even));
  }
};

HWY_NOINLINE void TestAllOddEven() {
  ForAllTypes(ForShrinkableVectors<TestOddEven>());
}

struct TestOddEvenBlocks {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const size_t N = Lanes(d);
    const auto even = Iota(d, 1);
    const auto odd = Iota(d, static_cast<T>(1 + N));
    auto expected = AllocateAligned<T>(N);
    for (size_t i = 0; i < N; ++i) {
      const size_t idx_block = i / (16 / sizeof(T));
      expected[i] = static_cast<T>(1 + i + ((idx_block & 1) ? N : 0));
    }
    HWY_ASSERT_VEC_EQ(d, expected.get(), OddEvenBlocks(odd, even));
  }
};

HWY_NOINLINE void TestAllOddEvenBlocks() {
  ForAllTypes(ForGEVectors<128, TestOddEvenBlocks>());
}

struct TestSwapAdjacentBlocks {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    const size_t N = Lanes(d);
    constexpr size_t kLanesPerBlock = 16 / sizeof(T);
    if (N < 2 * kLanesPerBlock) return;
    const auto vi = Iota(d, 1);
    auto expected = AllocateAligned<T>(N);
    for (size_t i = 0; i < N; ++i) {
      const size_t idx_block = i / kLanesPerBlock;
      const size_t base = (idx_block ^ 1) * kLanesPerBlock;
      const size_t mod = i % kLanesPerBlock;
      expected[i] = static_cast<T>(1 + base + mod);
    }
    HWY_ASSERT_VEC_EQ(d, expected.get(), SwapAdjacentBlocks(vi));
  }
};

HWY_NOINLINE void TestAllSwapAdjacentBlocks() {
  ForAllTypes(ForGEVectors<128, TestSwapAdjacentBlocks>());
}

struct TestTableLookupLanes {
  template <class T, class D>
  HWY_NOINLINE void operator()(T /*unused*/, D d) {
    using TI = MakeSigned<T>;
#if HWY_TARGET != HWY_SCALAR
    const size_t N = Lanes(d);
    const Rebind<TI, D> di;
    auto idx = AllocateAligned<TI>(N);
    memset(idx.get(), 0, N * sizeof(TI));
    auto expected = AllocateAligned<T>(N);
    const auto v = Iota(d, 1);

    if (N <= 8) {  // Test all permutations
      for (size_t i0 = 0; i0 < N; ++i0) {
        idx[0] = static_cast<TI>(i0);

        for (size_t i1 = 0; i1 < N; ++i1) {
          if (N >= 2) idx[1] = static_cast<TI>(i1);
          for (size_t i2 = 0; i2 < N; ++i2) {
            if (N >= 4) idx[2] = static_cast<TI>(i2);
            for (size_t i3 = 0; i3 < N; ++i3) {
              if (N >= 4) idx[3] = static_cast<TI>(i3);

              for (size_t i = 0; i < N; ++i) {
                expected[i] = static_cast<T>(idx[i] + 1);  // == v[idx[i]]
              }

              const auto opaque1 = IndicesFromVec(d, Load(di, idx.get()));
              const auto actual1 = TableLookupLanes(v, opaque1);
              HWY_ASSERT_VEC_EQ(d, expected.get(), actual1);

              const auto opaque2 = SetTableIndices(d, idx.get());
              const auto actual2 = TableLookupLanes(v, opaque2);
              HWY_ASSERT_VEC_EQ(d, expected.get(), actual2);
            }
          }
        }
      }
    } else {
      // Too many permutations to test exhaustively; choose one with repeated
      // and cross-block indices and ensure indices do not exceed #lanes.
      // For larger vectors, upper lanes will be zero.
      HWY_ALIGN TI idx_source[16] = {1,  3,  2,  2,  8, 1, 7, 6,
                                     15, 14, 14, 15, 4, 9, 8, 5};
      for (size_t i = 0; i < N; ++i) {
        idx[i] = (i < 16) ? idx_source[i] : 0;
        // Avoid undefined results / asan error for scalar by capping indices.
        if (idx[i] >= static_cast<TI>(N)) {
          idx[i] = static_cast<TI>(N - 1);
        }
        expected[i] = static_cast<T>(idx[i] + 1);  // == v[idx[i]]
      }

      const auto opaque1 = IndicesFromVec(d, Load(di, idx.get()));
      const auto actual1 = TableLookupLanes(v, opaque1);
      HWY_ASSERT_VEC_EQ(d, expected.get(), actual1);

      const auto opaque2 = SetTableIndices(d, idx.get());
      const auto actual2 = TableLookupLanes(v, opaque2);
      HWY_ASSERT_VEC_EQ(d, expected.get(), actual2);
    }
#else
    const TI index = 0;
    const auto v = Set(d, 1);
    const auto opaque1 = SetTableIndices(d, &index);
    HWY_ASSERT_VEC_EQ(d, v, TableLookupLanes(v, opaque1));
    const auto opaque2 = IndicesFromVec(d, Zero(d));
    HWY_ASSERT_VEC_EQ(d, v, TableLookupLanes(v, opaque2));
#endif
  }
};

HWY_NOINLINE void TestAllTableLookupLanes() {
  ForUIF3264(ForPartialVectors<TestTableLookupLanes>());
}


// NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace HWY_NAMESPACE
}  // namespace hwy
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace hwy {
HWY_BEFORE_TEST(HwySwizzleTest);
HWY_EXPORT_AND_TEST_P(HwySwizzleTest, TestAllGetLane);
HWY_EXPORT_AND_TEST_P(HwySwizzleTest, TestAllDupEven);
HWY_EXPORT_AND_TEST_P(HwySwizzleTest, TestAllDupOdd);
HWY_EXPORT_AND_TEST_P(HwySwizzleTest, TestAllOddEven);
HWY_EXPORT_AND_TEST_P(HwySwizzleTest, TestAllOddEvenBlocks);
HWY_EXPORT_AND_TEST_P(HwySwizzleTest, TestAllSwapAdjacentBlocks);
HWY_EXPORT_AND_TEST_P(HwySwizzleTest, TestAllTableLookupLanes);
}  // namespace hwy

#endif
