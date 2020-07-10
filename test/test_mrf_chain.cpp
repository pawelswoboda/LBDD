#include "bdd_mgr.h"
#include "test.h"
#include <vector>
#include <array>
#include <iostream> // TODO delete
#include <unistd.h>

using namespace BDD;

template<typename ITERATOR>
node* create_simplex(bdd_mgr& mgr, ITERATOR var_begin, ITERATOR var_end)
{
    std::vector<node*> nodes;
    nodes.push_back(mgr.or_rec(var_begin, var_end));
    for(size_t i=0; i<std::distance(var_begin, var_end); ++i) {
        for(size_t j=i+1; j<std::distance(var_begin, var_end); ++j) {
            nodes.push_back(mgr.or_rec(mgr.negate(*(var_begin+i)), mgr.negate(*(var_begin+j))));
        }
    }
    return mgr.and_rec(nodes.begin(), nodes.end());
}

template<typename ITERATOR>
node* create_marginalization_constraint(bdd_mgr& mgr, node* u, ITERATOR var_begin, ITERATOR var_end)
{
    std::vector<node*> bdds;
    std::vector<node*> all_vars({mgr.negate(u)});
    for(auto it=var_begin; it!=var_end; ++it)
    {
        bdds.push_back(mgr.or_rec(mgr.negate(*it), u));
        all_vars.push_back(*it);
    }
    bdds.push_back(mgr.or_rec(all_vars.begin(), all_vars.end()));
    return mgr.and_rec(bdds.begin(), bdds.end());
}

int main(int argc, char** argv)
{
    bdd_mgr mgr;

    const size_t nr_vars = 1000;
    const size_t nr_labels = 3;

    std::vector<node*> bdds;

    // create unary and pairwise variables interleaved
    std::vector<std::vector<node*>> unary_vars;
    std::vector<std::vector<std::vector<node*>>> pairwise_vars;
    for(size_t i=0; i<nr_vars; ++i)
    {
        unary_vars.push_back({});
        for(size_t j = 0; j<nr_labels; ++j)
        {
            unary_vars.back().push_back(mgr.projection(mgr.add_variable()));
        }
        bdds.push_back(create_simplex(mgr,unary_vars.back().begin(), unary_vars.back().end()));

        pairwise_vars.push_back({});
        for(size_t l1 = 0; l1<nr_labels; ++l1)
        {
            pairwise_vars.back().push_back({});
            for(size_t l2 = 0; l2<nr_labels; ++l2)
            {
                pairwise_vars.back().back().push_back(mgr.projection(mgr.add_variable()));
            }
        }
    }

    auto pairwise_var = [&](const std::array<size_t,2> v1, const std::array<size_t,2> v2) {
        const size_t offset = nr_vars * nr_labels;
        assert(v1[0] +1 == v2[0]);
        const size_t pairwise_offset = v1[0] * nr_labels * nr_labels;
        const size_t label_idx = v1[1]*nr_labels + v2[1]; 
        return offset + pairwise_offset + label_idx; 
    };

    for(size_t i=0; i+1<nr_vars; ++i)
    {
        for(size_t l1 = 0; l1<nr_labels; ++l1)
        {
            std::vector<node*> p;
            for(size_t l2 = 0; l2<nr_labels; ++l2)
                p.push_back(pairwise_vars[i][l1][l2]);
            bdds.push_back(create_marginalization_constraint(mgr, unary_vars[i][l1], p.begin(), p.end()));
        }

        for(size_t l2 = 0; l2<nr_labels; ++l2)
        {
            std::vector<node*> p;
            for(size_t l1 = 0; l1<nr_labels; ++l1)
                p.push_back(pairwise_vars[i][l1][l2]);
            bdds.push_back(create_marginalization_constraint(mgr, unary_vars[i+1][l2], p.begin(), p.end()));
        }
    }
    
    node* c = mgr.and_rec(bdds.begin(), bdds.end());
    
    std::cout << "nr mrf nodes: " << c->nr_nodes() << "\n";
    test(c->nr_nodes() == 47957, "nr of nodes for mrf chain not matching."); // nr valid for 1000 nodes and 3 labels
    //c->print(std::cout);
    c->deref();
    for(node* p : bdds)
        p->deref();
}

