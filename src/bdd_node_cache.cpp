#include "bdd_node_cache.h"
#include <cassert>

namespace BDD {

    bdd_node_cache::bdd_node_cache()
    {
        static_assert(bdd_node_page_size > 2);
        mem_node = std::unique_ptr<bdd_node_page>(new bdd_node_page);

        // add terminal nodes
        botsink_ = &mem_node.get()->data[0];
        botsink_->init_botsink();

        topsink_ = &mem_node.get()->data[1];
        topsink_->init_topsink();
        
        nodeavail = nullptr;
        nodeptr = &mem_node.get()->data[2];
    }

    void bdd_node_cache::increase_cache()
    {
        std::unique_ptr<bdd_node_page> new_bdd_node_page = std::unique_ptr<bdd_node_page>(new bdd_node_page); 
        std::swap(new_bdd_node_page.get()->next, mem_node);
        std::swap(new_bdd_node_page, mem_node);

        assert(nodeavail == nullptr);
        nodeptr = &(mem_node.get()->data[0]);
    }

    node* bdd_node_cache::reserve_node()
    {
        total_nodes++;
        node* r = nodeavail;
        if(r != nullptr)
        {
            nodeavail = nodeavail->next_available;
            return r;
        }
        else
        {
            r = nodeptr;
            assert(&(mem_node.get()->data[0]) <= nodeptr);
            assert(&(mem_node.get()->data[0]) + mem_node.get()->data.size() >= nodeptr);
            if(std::distance(&(mem_node.get()->data[0]), nodeptr) < mem_node.get()->data.size())
            {
                nodeptr++;
                return r;
            }
            else
            {
                total_nodes--;
                increase_cache();
                return reserve_node();
            }
        }
        assert(false);
        return nullptr;
    }

    void bdd_node_cache::free_node(node* p)
    {
        assert(p->xref <= 0);
        p->next_available = nodeavail;
        nodeavail = p;
        total_nodes--;
    }


}
