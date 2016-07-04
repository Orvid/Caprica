#pragma once

#ifdef _MSC_VER

#define ALWAYS_INLINE __forceinline
#define NEVER_INLINE __declspec(noinline)

#else
#error TODO: Add appropriate defines for GCC/Clang.
#endif
