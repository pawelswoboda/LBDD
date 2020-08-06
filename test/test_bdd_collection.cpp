#include "bdd_collection.h"
#include "test.h"
#include <vector>
#include <iostream> // TODO: remove

using namespace BDD;

int main(int argc, char** argv)
{
    bdd_collection collection;
    bdd_mgr mgr;

    for(size_t i=0; i<8; ++i)
        mgr.add_variable();

    std::vector<node_ref> simplex_vars;
    for(size_t i=0; i<8; ++i)
        simplex_vars.push_back(mgr.projection(i));

    node_ref simplex_1 = mgr.simplex(simplex_vars.begin(), simplex_vars.begin()+5); 
    collection.add_bdd(simplex_1);
    test(collection.nr_bdd_nodes(0) == 9+2, "bdd conversion to collectionless failed");
    for(size_t l0 = 0; l0<1; ++l0)
        for(size_t l1 = 0; l1<1; ++l1)
            for(size_t l2 = 0; l2<1; ++l2)
                for(size_t l3 = 0; l3<1; ++l3)
                    for(size_t l4 = 0; l4<1; ++l4)
                    {
                        const std::array<size_t,5> labeling = {l0,l1,l2,l3,l4};
                        const size_t sum = std::accumulate(labeling.begin(), labeling.end(), 0);
                        if(sum == 0)
                            test(collection.evaluate(0, labeling.begin(), labeling.end()) == true, "simplex constraints false negative.");
                        else
                            test(collection.evaluate(0, labeling.begin(), labeling.end()) == false, "simplex constraints false positive.");
                    }

    node_ref simplex_2 = mgr.simplex(simplex_vars.begin()+3, simplex_vars.end());
    collection.add_bdd(simplex_2);
    test(collection.nr_bdd_nodes(1) == 9+2, "bdd conversion to collectionless failed");
    for(size_t l3 = 0; l3<1; ++l3)
        for(size_t l4 = 0; l4<1; ++l4)
            for(size_t l5 = 0; l5<1; ++l5)
                for(size_t l6 = 0; l6<1; ++l6)
                    for(size_t l7 = 0; l7<1; ++l7)
                    {
                        const std::array<size_t,8> labeling = {0,0,0,l3,l4,l5,l5,l7};
                        const size_t sum = std::accumulate(labeling.begin(), labeling.end(), 0);
                        if(sum == 0)
                            test(collection.evaluate(1, labeling.begin(), labeling.end()) == true, "simplex constraints false negative.");
                        else
                            test(collection.evaluate(1, labeling.begin(), labeling.end()) == false, "simplex constraints false positive.");
                    }


    collection.bdd_and(0,1);
    for(size_t l0 = 0; l0<1; ++l0)
        for(size_t l1 = 0; l1<1; ++l1)
            for(size_t l2 = 0; l2<1; ++l2)
                for(size_t l3 = 0; l3<1; ++l3)
                    for(size_t l4 = 0; l4<1; ++l4)
                        for(size_t l5 = 0; l5<1; ++l5)
                            for(size_t l6 = 0; l6<1; ++l6)
                                for(size_t l7 = 0; l7<1; ++l7)
                                {
                                    const std::array<size_t,8> labeling = {l0,l1,l2,l3,l4,l5,l6,l7};
                                    const size_t sum = std::accumulate(labeling.begin(), labeling.end(), 0);
                                    const bool ends_zero = (l0 == 0) &&(l1 == 0) && (l2 == 0) && (l5 == 0) && (l6 == 0) && (l7 == 0);
                                    if(sum == 0 && ends_zero)
                                        test(collection.evaluate(2, labeling.begin(), labeling.end()) == true, "simplex constraints false negative.");
                                    else
                                        test(collection.evaluate(2, labeling.begin(), labeling.end()) == false, "simplex constraints false positive.");
                                }
    const size_t nr_and_nodes = collection.nr_bdd_nodes(2);
    test(collection.bdd_and(0,1, nr_and_nodes-1) == std::numeric_limits<size_t>::max(), "node limit in bdd and not triggered.");
    test(collection.nr_bdds() == 3, "bdd added even though should not.");
    test(collection.bdd_and(0,1, nr_and_nodes) == 3, "node limit in bdd and triggered.");
}
