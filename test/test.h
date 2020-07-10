#pragma once

#include <string>

inline void test(const bool cond, const std::string error = "")
{
    if(!cond)
        throw std::runtime_error(error);
}
