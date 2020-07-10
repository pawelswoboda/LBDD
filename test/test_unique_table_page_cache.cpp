#include "bdd_var.h"
#include "test.h"
#include <vector>

using namespace BDD;

int main(int argc, char** argv)
{
    unique_table_page_cache cache;
    std::vector<unique_table_page*> v;
    const std::size_t n = 10000;
    for(std::size_t i=0; i<n; ++i)
    {
        v.push_back(cache.reserve_page());
        test(i + unique_table_page_cache::nr_pages_simultaneous_allocation >= cache.nr_pages() && i <= cache.nr_pages() , "page counting error when requesting pages");
    }

    for(std::size_t i=0; i<n; ++i)
    {
        cache.free_page(v[i]);
    } 
}

