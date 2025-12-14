#pragma once

#include <cstddef>
#include <new>
#include <type_traits>

namespace nsqueue::details 
{

#if defined(__cpp_lib_hardward_interference_size)
constexpr std::size_t cacheLineSize = std::hardward_destructive_interference_size;
#else
constexpr std::size_t cacheLineSize = 64;
#endif

#if defined(__GNUC__) || defined(__clang__)
#define NSQ_LIKELY(x) __builtin_expect(!!(x), 1)
#define NSQ_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define NSQ_LIKELY(x) (x)
#define NSQ_UNLIKELY(x) (x)
#endif

}
