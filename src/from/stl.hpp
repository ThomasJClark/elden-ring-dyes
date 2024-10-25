#pragma once

#include <string>
#include <vector>

#include "dl_allocator.hpp"

namespace from
{

template <typename T> using vector = std::vector<T, DLKR::DLAllocatorAdapter<T>>;

template <typename C>
using basic_string = std::basic_string<C, std::char_traits<C>, DLKR::DLAllocatorAdapter<C>>;

typedef basic_string<wchar_t> wstring;

}
