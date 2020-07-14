#include "bdd_var.h"
#include "bdd_mgr.h"
#include <algorithm>
#include <cassert>
#include <iostream> // TODO: delete

namespace BDD {

    var_struct::var_struct(const std::size_t index, bdd_mgr& _bdd_mgr)
        : var(index),
        bdd_mgr_(_bdd_mgr)
    {
        initialize_unique_table();
        //proj = unique_find(index, bdd_mgr_.get_node_cache().botsink(), bdd_mgr_.get_node_cache().topsink());
        //bdd_mgr_.get_node_cache().botsink()->xref++;
        //bdd_mgr_.get_node_cache().topsink()->xref++; 
    }

    var_struct::var_struct(var_struct&& o)
        : var(o.var),
        timer(o.timer),
        name(o.name),
        timestamp(o.timestamp),
        aux(o.aux),
        up(o.up),
        down(o.down),
        bdd_mgr_(o.bdd_mgr_) 
    {
        //std::swap(proj, o.proj);
        std::swap(repl, o.repl);
        std::swap(base, o.base);
        std::swap(mask, o.mask);
        std::swap(free, o.free);
    } 

    var_struct::~var_struct()
    {
        // TODO: remove all bdd nodes that are in the unique table
        //std::cout << "nr unique table pages: " << nr_pages_debug() << "\n";
        //if(proj != nullptr)
        //    proj->deref();
        for(std::size_t i=0; i<base.size(); ++i)
        {
            if(base[i] == nullptr)
                break;
            for(std::size_t j=0; j<base[i]->data.size(); ++j)
            {
                node* p = base[i]->data[j];
                if(p != nullptr)
                    bdd_mgr_.get_node_cache().free_node(p); 
            }
            bdd_mgr_.get_unique_table_page_cache().free_page(base[i]);
        }
    }

    std::size_t var_struct::hash_code(node* p) const
    {
        assert(p != nullptr);
        return hash_code(p->lo, p->hi);
    }

    std::size_t var_struct::hash_code(node* l, node* r) const
    {
        return l->hash_key ^ (2*r->hash_key);
    }

    std::array<size_t,2> var_struct::base_indices(const std::size_t hash) const
    {
        assert(mask >= 0);
        const std::size_t base_page = (hash >> log_nr_unique_table_pages) & mask;
        assert(base_page < nr_pages_debug());
        const std::size_t offset = hash % nr_unique_table_slots_per_page;
        return {base_page, offset}; 
    }

    node* var_struct::fetch_node(const std::size_t hash) const
    {
        const auto [base_page, offset] = base_indices(hash);
        return base[base_page]->data[offset];
    }

    std::size_t var_struct::next_free_slot(const std::size_t hash) const
    {
        for(std::size_t k = hash;; ++k)
            if(fetch_node(k) == nullptr)
                return k;
    }

    void var_struct::store_node(const std::size_t hash, node* p)
    {
        const auto [base_page, offset] = base_indices(hash);
        base[base_page]->data[offset] = p;
    }

    std::size_t var_struct::nr_pages_debug() const
    {
        for(std::size_t i = 0; i<base.size(); ++i)
            if(base[i] == nullptr)
                return i;
        return base.size();
    }

    std::size_t var_struct::nr_pages() const
    {
        const std::size_t n = mask + 1;
        assert(n == nr_pages_debug());
        return n;
    }

    std::size_t var_struct::unique_table_size() const
    {
        assert(mask + 1 == nr_pages());
        return nr_pages() * nr_unique_table_slots_per_page;
    }

    node* var_struct::unique_table_lookup(node* l, node* h)
    {
        assert(l != h);
        for(std::size_t hash = hash_code(l,h);; hash++)
        {
            node* p = fetch_node(hash);
            if(p == nullptr)
                return nullptr;
            if(p->lo == l && p->hi == h)
                return p;
        }
        throw std::runtime_error("unique table corrupted."); 
    }

    void var_struct::initialize_unique_table()
    {
        base[0] = bdd_mgr_.get_unique_table_page_cache().reserve_page();
        mask = 0;
        free = nr_unique_table_slots_per_page;
        std::fill(base.begin()+1, base.end(), nullptr);
    }

    void var_struct::double_unique_table_size()
    {
        const std::size_t current_nr_pages = nr_pages();
        assert(current_nr_pages > 0);
        const std::size_t new_nr_pages = 2 * nr_pages();

        // allocate new pages
        for(std::size_t i = current_nr_pages; i < new_nr_pages; ++i)
            base[i] = bdd_mgr_.get_unique_table_page_cache().reserve_page();
        mask = mask + mask + 1;
        free = free + current_nr_pages * nr_unique_table_slots_per_page;

        // rehash existing entries in the low half
        for(std::size_t k = 0; k < unique_table_size(); ++k)
        {
            node* p = fetch_node(k);
            if(p != nullptr)
            {
                // prevent propagation past this point
                store_node(k, nullptr);
                store_node(next_free_slot(hash_code(p)), p);
            }
            else
            {
                if(k > mask/2)
                    break;
            }
        }
    }

    node* var_struct::unique_find(const std::size_t index, node* l, node* h)
    {
        assert(index < l->find_bdd_mgr()->nr_variables()); 
        if(l==h) {
            //l->xref--;
            return l;
        }

        node* p = unique_table_lookup(l, h);

        if(p != nullptr) // node present
        {
            if(p->xref < 0)
            {
                //dead_nodes--;
                //p->xref= 0;
                return p;
            }
            else 
            {
                //l->xref--;
                //h->xref--;
                //p->xref++;
                return p;
            }
        }
        else // node not present
        {
            // garbage collection
            if((++timer % timerinterval) == 0 && (dead_nodes > unique_table_size()/dead_fraction))
            {
                assert(false);
                // TODO: implement
                //collect_garbage(0);
                return unique_find(l, h);
            }
        }

        // allocate free node and add it to unique table
        const double occupied_rate = (unique_table_size()
                - free) / double(unique_table_size()) ;
        if(occupied_rate > max_unique_table_fill) // double number of base pages for unique table
            double_unique_table_size();

        // allocate new node and insert it into unique table
        p = bdd_mgr_.get_node_cache().reserve_node();
        assert(p != nullptr);
        p->init_new_node(index,l,h);
        assert(free > 0);
        --free;
        store_node(next_free_slot(hash_code(p)), p);
        return p;
    }

    node* var_struct::unique_find(node* l, node* h)
    {
        //assert(proj != nullptr);
        return unique_find(var, l, h);
    }

    //node* var_struct::projection() const
    //{
    //    assert(proj != nullptr);
    //    return proj;
    //}
}
