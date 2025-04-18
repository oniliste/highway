# Highway implementation details

[TOC]

## Introduction

This doc explains some of the Highway implementation details; understanding them
is mainly useful for extending the library. Bear in mind that Highway is a thin
wrapper over 'intrinsic functions' provided by the compiler.

## Vectors vs. tags

The key to understanding Highway is to differentiate between vectors and
zero-sized tag arguments. The former store actual data and are mapped by the
compiler to vector registers. The latter (`Simd<>` and `SizeTag<>`) are only
used to select among the various overloads of functions such as `Set`. This
allows Highway to use builtin vector types without a class wrapper.

Class wrappers are problematic for SVE and RVV because LLVM (or at least Clang)
does not allow member variables whose type is 'sizeless' (in particular,
built-in vectors). To our knowledge, Highway is the only C++ vector library that
supports SVE and RISC-V without compiler flags that indicate what the runtime
vector length will be. Such flags allow the compiler to convert the previously
sizeless vectors to known-size vector types, which can then be wrapped in
classes, but this only makes sense for use-cases where the exact hardware is
known and rarely changes (e.g. supercomputers). By contrast, Highway can run on
unknown hardware such as heterogeneous clouds or client devices without
requiring a recompile, nor multiple binaries.

Note that Highway does use class wrappers where possible, in particular NEON,
WASM and x86. The wrappers (e.g. Vec128) are in fact required on some platforms
(x86 and perhaps WASM) because Highway assumes the vector arguments passed e.g.
to `Add` provide sufficient type information to identify the appropriate
intrinsic. By contrast, x86's loosely typed `__m128i` built-in type could
actually refer to any integer lane type. Because some targets use wrappers and
others do not, incorrect user code may compile on some platforms but not others.
This is because passing class wrappers as arguments triggers argument-dependent
lookup, which would find the `Add` function even without namespace qualifiers
because it resides in the same namespace as the wrapper. Correct user code
qualifies each call to a Highway op, e.g. with a namespace alias `hn`, so
`hn::Add`. This works for both wrappers and built-in vector types.

## Adding a new target

Adding a target requires updating about ten locations: adding a macro constant
to identify it, hooking it into static and dynamic dispatch, detecting support
at runtime, and identifying the target name. The easiest and safest way to do
this is to search for one of the target identifiers such as `HWY_AVX3_DL`, and
add corresponding logic for your new target. Note the upper limits on the number
of targets per platform imposed by `HWY_MAX_DYNAMIC_TARGETS`.

## When to use -inl.h

By convention, files whose name ends with `-inl.h` contain vector code in the
form of inlined function templates. In order to support the multiple compilation
required for dynamic dispatch on platforms which provide several targets, such
files generally begin with a 'per-target include guard' of the form:

```
#if defined(HWY_PATH_NAME_INL_H_) == defined(HWY_TARGET_TOGGLE)
#ifdef HWY_PATH_NAME_INL_H_
#undef HWY_PATH_NAME_INL_H_
#else
#define HWY_PATH_NAME_INL_H_
#endif
// contents to include once per target
#endif  // HWY_PATH_NAME_INL_H_
```

This toggles the include guard between defined and undefined, which is
sufficient to 'reset' the include guard when beginning a new 'compilation pass'
for the next target. This is accomplished by simply re-#including the user's
translation unit, which may in turn `#include` one or more `-inl.h` files. As an
exception, `hwy/ops/*-inl.h` do not require include guards because they are all
included from highway.h, which takes care of this in a single location. Note
that platforms such as WASM and RISC-V which currently only offer a single
target do not require multiple compilation, but the same mechanism is used
without actually re-#including. For both of those platforms, it is possible that
additional targets will later be added, which means this mechanism will then be
required.

Instead of a -inl.h file, you can also use a normal .cc/.h component, where the
vector code is hidden inside the .cc file, and the header only declares a normal
non-template function whose implementation does `HWY_DYNAMIC_DISPATCH` into the
vector code. For an example of this, see
[vqsort.cc](../hwy/contrib/sort/vqsort.cc).

Considerations for choosing between these alternatives are similar to those for
regular headers. Inlining and thus `-inl.h` makes sense for short functions, or
when the function must support many input types and is defined as a template.
Conversely, non-inline `.cc` files make sense when the function is very long
(such that call overhead does not matter), and/or is only required for a small
set of input types. [Math functions](../hwy/contrib/math/math-inl.h)
can fall into either case, hence we provide both inline functions and `Call*`
wrappers.

## Use of macros

Highway ops are implemented for up to 12 lane types, which can make for
considerable repetition - even more so for RISC-V, which can have seven times as
many variants (one per LMUL in `[1/8, 8]`). The various backends
(implementations of one or more targets) differ in their strategies for handling
this, in increasing order of macro complexity:

*   `x86_*` and `wasm_*` simply write out all the overloads, which is
    straightforward but results in 4K-6K line files.

*   [arm_sve-inl.h](../hwy/ops/arm_sve-inl.h) defines 'type list'
    macros `HWY_SVE_FOREACH*` to define all overloads for most ops in a single
    line. Such an approach makes sense because SVE ops are quite orthogonal
    (i.e. generally defined for all types and consistent).

*   [arm_neon-inl.h](../hwy/ops/arm_neon-inl.h) also uses type list
    macros, but with a more general 'function builder' which helps to define
    custom function templates required for 'unusual' ops such as `ShiftLeft`.

*   [rvv-inl.h](../hwy/ops/rvv-inl.h) has the most complex system
    because it deals with both type lists and LMUL, plus support for widening or
    narrowing operations. The type lists thus have additional arguments, and
    there are also additional lists for LMUL which can be extended or truncated.

## Code reuse across targets

The set of Highway ops is carefully chosen such that most of them map to a
single platform-specific intrinsic. However, there are some important functions
such as `AESRound` which may require emulation, and are non-trivial enough that
we don't want to copy them into each target's implementation. Instead, we
implement such functions in
[generic_ops-inl.h](../hwy/ops/generic_ops-inl.h), which is included
into every backend. The functions there are typically templated on the vector
and/or tag types. To allow some targets to override these functions, we use the
same per-target include guard mechanism, e.g. `HWY_NATIVE_AES`.

For x86, we also avoid some duplication by implementing only once the functions
which are shared between all targets. They reside in
[x86_128-inl.h](../hwy/ops/x86_128-inl.h) and are also templated on the
vector type.

## Why scalar target

There can be various reasons to avoid using vector intrinsics:

*   The current CPU may not support any instruction sets generated by Highway
    (on x86, we only target S-SSE3 or newer because its predecessor SSE3 was
    introduced in 2004 and it seems unlikely that many users will want to
    support such old CPUs);
*   The compiler may crash or emit incorrect code for certain intrinsics or
    instruction sets;
*   We may want to estimate the speedup from the vector implementation compared
    to scalar code.

Highway provides the `HWY_SCALAR` target for such use-cases. It defines a
`Vec1<T>` wrapper class which is always a single-lane vector, and implements ops
using standard C++ instead of intrinsics. This will be slow, but at least it
allows checking the same code for correctness even when actual vectors are
unavailable.

With hindsight, this target turns out to be less useful because it is unable to
express some ops such as `AESRound`, `CLMulLower` or `TableLookupBytes` which
require at least 128-bit vectors. For all other targets, Highway guarantees at
least 128-bit vectors are supported. Thus tests and some user code may require
`#if HWY_TARGET != HWY_SCALAR` to prevent compile errors when using this target.
To avoid this while still enabling the three use cases above, we plan to
introduce a `HWY_EMU128` target which is the same, except that it would always
support `16/sizeof(T)` lanes of type T by adding loops to all the op
implementations.
