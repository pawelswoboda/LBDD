#include "bdd_var.h"
#include <cassert>

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

}
