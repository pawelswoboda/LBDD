#include "bdd.h"
#include <random>

namespace BDD {

    void node_struct::init_new_node(std::size_t v, node_struct* l, node_struct* h) 
    {
        lo = l;
        hi = h;
        index = v;
        hash_key = unique_table_distribution(unique_hash_table_gen);
    }

    bdd_mgr::bdd_mgr()
    {
        gb_init_rand(0);
        botsink= (node*)mem;
        topsink= botsink+1;
        o,botsink->lo= botsink->hi= addr_(botsink);
        o,topsink->lo= topsink->hi= addr_(topsink);
        oo,botsink->xref= topsink->xref= 0;
        oooo,botsink->index= gb_next_rand();
        oooo,topsink->index= gb_next_rand();
        totalnodes= 2;
        nodeptr= topsink+1;
        pageptr= topofmem;

        // allocate varmap
        for(k= 0;k<varsize;k++)varmap[k]= k;
        cache_init();
    }

    node* bdd_mgr::reserve_node()
    {
        node* r = nodeavail;
        if(r)
            nodeavail = nodeavail->next_available;
        else{
            r = nodeptr;
            if(std::distance(mem_node,nodeptr) < mem_node_size)
                nodeptr++;
            else{
                leasesonlife--;
                if(leasesonlife==0){
                    show_stats();exit(-98);
                }
                return nullptr;
            }
        }
        totalnodes++;
        return r;
    }

    void bdd_mgr::free_node(node* p)
    {
        p->next_available = nodeavail;
        nodeavail = p;
        totalnodes--;
    }

    std::size_t node_struct::hash_code() const
    {
        assert(lo != nullptr);
        assert(hi != nullptr);
        return (lo->index << 3) ^ (hi->index << 2);
    }

    node* bdd_mgr::unique_find(var*v,node*l,node*h)
    {
        if(l==h) {
            l->xref--;
            return l;
        }

    }


}
