#include "bdd_var.h"
#include "bdd_mgr.h"
#include <cassert>
#include <algorithm>

namespace BDD {

    unique_table_page_cache::unique_table_page_cache()
    {
        page_avail = nullptr; 
    }

    unique_table_page_cache::~unique_table_page_cache()
    {
        for(unique_table_page* p : pages)
            delete[] p;
    }

    void unique_table_page_cache::increase_cache()
    {
        assert(page_avail == nullptr);
        pages.push_back(new unique_table_page[nr_pages_simultaneous_allocation]);
        for(std::size_t i=0; i+1 < nr_pages_simultaneous_allocation; ++i)
            pages.back()[i].next_available = &(pages.back()[i+1]);
        page_avail = &(pages.back()[0]);
    }

    unique_table_page* unique_table_page_cache::reserve_page()
    {
        unique_table_page* r = page_avail;
        if(r != nullptr)
        {
            page_avail = page_avail->next_available;
            std::fill(r->data.begin(), r->data.end(), nullptr);
            return r;
        }
        else
        {
            increase_cache();
            return reserve_page();
        }
    }

    void unique_table_page_cache::free_page(unique_table_page* p)
    {
        p->next_available = page_avail;
        page_avail= p;
    }

    ////////////////////////////////////////
    
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
        //std::swap(repl, o.repl);
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
        assert(base[0] == nullptr);
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

    void var_struct::release_nodes()
    {
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
            base[i] = nullptr;
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
        //assert(mask+1 == nr_pages_debug());
        const std::size_t base_page = (hash >> log_nr_unique_table_pages) & mask;
        //assert(base_page < nr_pages_debug());
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

    size_t var_struct::nr_free_slots_debug() const
    {
        size_t n = 0;
        for(size_t i=0; i<nr_pages(); ++i)
            for(node* p : base[i]->data)
                if(p == nullptr)
                    n++;
        return n;

    }

    std::size_t var_struct::nr_pages() const
    {
        const std::size_t n = mask + 1;
        //assert(n == nr_pages_debug());
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

    double var_struct::occupied_rate() const
    {
        return double(unique_table_size() - free) / double(unique_table_size());
    }

    double var_struct::occupied_rate(const size_t new_nr_pages) const
    {
        const int page_diff = nr_pages() - new_nr_pages;
        const size_t new_free = free + page_diff * nr_unique_table_slots_per_page;
        const size_t new_cap = new_nr_pages * nr_unique_table_slots_per_page;
        return double(new_cap - new_free) / double(new_cap);
    }

    void var_struct::initialize_unique_table()
    {
        base[0] = bdd_mgr_.get_unique_table_page_cache().reserve_page();
        mask = 0;
        free = nr_unique_table_slots_per_page;
        std::fill(base.begin()+1, base.end(), nullptr);
    }

    void var_struct::remove_dead_nodes()
    {
        for(std::size_t k = 0; k < unique_table_size(); ++k)
        {
            node* p = fetch_node(k);
            if(p != nullptr && p->dead())
            {
                bdd_mgr_.get_node_cache().free_node(p);
                store_node(k, nullptr);
                free++;
                // move nodes that follow this one back if hash value indicates so
                size_t first_free_slot = k;
                for(size_t j = k+1;; ++j)
                {
                    const size_t jj = j % unique_table_size();
                    node* p = fetch_node(j);
                    if(p == nullptr)
                    {
                        k = j;
                        break;
                    }
                    if(p->dead())
                    {
                        store_node(j, nullptr);
                        free++;
                    }
                    else
                    {
                        // shift one place up if hash code says so
                        // TODO: can be made faster.
                        store_node(next_free_slot(hash_code(p)), p);
                    }
                }
            }
        }

        // reduce nr of pages if unique table too sparsely populated
        if(nr_pages() > 1 && occupied_rate() <= min_unique_table_fill)
        {
            const size_t new_nr_pages = [&]() {
                size_t n = nr_pages()/2;
                while(n >= 1 && occupied_rate(n) >= min_unique_table_fill)
                    n /= 2;
                return n;
            }();

            // remap occupied slots to first new_nr_pages
            for(std::size_t k = new_nr_pages*nr_unique_table_slots_per_page; k < unique_table_size(); ++k)
            {
                node* p = fetch_node(k);
                if(p != nullptr)
                {
                    const size_t next_free_slot = [&]() {
                        for(size_t l=k;; l++)
                        {
                            l = l % (new_nr_pages*nr_unique_table_slots_per_page);
                            if(fetch_node(l) == nullptr)
                                return l;
                        }
                        assert(false);
                        return size_t(0);
                    }();

                    store_node(next_free_slot, p);
                } 
            }

            for(size_t k = new_nr_pages; k < nr_pages(); ++k)
                bdd_mgr_.get_unique_table_page_cache().free_page(base[k]);

            mask = new_nr_pages-1;
        }
    }

    void var_struct::double_unique_table_size()
    {
        const std::size_t current_nr_pages = nr_pages();
        assert(current_nr_pages > 0);
        if(current_nr_pages == base.size()) {
            if(free == 0)
                throw std::runtime_error("Cannot increase unique table and no free slots available");
            return;
        }
        assert(free == nr_free_slots_debug());
        const std::size_t new_nr_pages = 2 * nr_pages();
        assert(new_nr_pages <= base.size());

        // allocate new pages
        for(std::size_t i = current_nr_pages; i < new_nr_pages; ++i)
            base[i] = bdd_mgr_.get_unique_table_page_cache().reserve_page();
        mask = (2 * mask) + 1;
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
                if(k > unique_table_size()/2)
                    break;
            }
        }
    }

    node* var_struct::unique_find(const std::size_t index, node* l, node* h)
    {
        //assert(index < l->find_bdd_mgr()->nr_variables()); 
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
            //if((++timer % timerinterval) == 0 && (dead_nodes > unique_table_size()/dead_fraction))
            if((++timer % timerinterval) == 0)
            {
                // TODO: implement
                //remove_dead_nodes();
                return unique_find(l, h);
            }
        }

        // allocate free node and add it to unique table
        if(occupied_rate() > max_unique_table_fill) // double number of base pages for unique table
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
