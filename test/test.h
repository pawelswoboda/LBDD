#pragma once

#include <string>
#include "bdd_mgr.h"

inline void test(const bool cond, const std::string error = "")
{
    if(!cond)
        throw std::runtime_error(error);
}

template<typename LABEL_ITERATOR>
inline void test_labeling(BDD::node_ref p, LABEL_ITERATOR label_begin, LABEL_ITERATOR label_end, const bool result)
{
    test(p.evaluate(label_begin, label_end) == result, "BDD evaluation error");
};

