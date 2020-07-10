#pragma once

#include <memory>
#include <array>
#include <vector>
#include "bdd_node.h"
#include "bdd_node_cache.h"

namespace BDD {

    constexpr static std::size_t log_nr_unique_table_pages = 10;
    constexpr static std::size_t nr_unique_table_pages = static_cast<std::size_t>(1) << log_nr_unique_table_pages; // 1024
    constexpr static std::size_t unique_table_page_mask = (nr_unique_table_pages-1);

    constexpr static std::size_t log_nr_unique_table_slots_per_page = 10;
    constexpr static std::size_t nr_unique_table_slots_per_page = static_cast<std::size_t>(1) << log_nr_unique_table_slots_per_page; // 1024;
    constexpr static std::size_t unique_table_slots_mask = (nr_unique_table_slots_per_page-1);

    constexpr static std::size_t log_max_hash_size = log_nr_unique_table_pages + log_nr_unique_table_slots_per_page;

    constexpr static double min_unique_table_fill = 1.0/8.0;
    constexpr static double max_unique_table_fill = 3.0/4.0;
    //constexpr static std::size_t max_hash_pages = (((static_cast<std::size_t>(1) << log_max_hash_size) + slots_per_page - 1) / slots_per_page);
    constexpr static std::size_t initial_page_mem_size = 512;

class unique_table_page {
    public:
    // TODO: make union of below two variables
    std::array<node*, nr_unique_table_slots_per_page> data;
    unique_table_page* next_available = nullptr; 
};

class unique_table_page_cache {
    public:
        constexpr static std::size_t nr_pages_simultaneous_allocation = 1024;

        unique_table_page_cache();
        ~unique_table_page_cache();
        unique_table_page* reserve_page();
        void free_page(unique_table_page* p); 
        std::size_t nr_pages() const { return pages.size() * nr_pages_simultaneous_allocation; }

    private: 
        void increase_cache();
        unique_table_page* page_avail; // stack of pages for reuse
        std::vector<unique_table_page*> pages;
};


class bdd_mgr; // forward declaration

class var_struct {
    public:
        var_struct(const std::size_t index, bdd_mgr& _bdd_mgr);
        ~var_struct();
        var_struct(var_struct&&);
        var_struct(const var_struct&) = delete;

        std::size_t hash_table_size() const; 
        std::size_t hash_code(node* p) const;
        std::size_t hash_code(node* l, node* r) const;
        std::array<size_t,2> base_indices(const std::size_t hash) const;
        node* fetch_node(const std::size_t hash) const;
        std::size_t next_free_slot(const std::size_t hash) const;
        void store_node(const std::size_t hash, node* p);
        std::size_t nr_pages_debug() const;
        std::size_t nr_pages() const;
        node* unique_table_lookup(node* l, node* h);
        void double_unique_table_size();
        node* unique_find(const std::size_t index, node* l,node* h); 
        node* unique_find(node* l,node* h); 
        std::size_t unique_table_size() const;
        //node* projection() const;

    private:
        const size_t var;
        void initialize_unique_table();

        //node* proj = nullptr; // address of the projection function x_v
        node* repl = nullptr; // address of replacement function y_v
        std::size_t free; // number of unused slots in the unique table for v
        std::size_t dead_nodes;
        std::size_t timer = 0;
        constexpr static std::size_t timerinterval = 1024;
        constexpr static double dead_fraction = 1.0;
        std::size_t mask; // number of pages for the unique table minus 1
        std::array<unique_table_page*, nr_unique_table_pages> base; // base addresses for its pages
        std::size_t name; // user's name (subscript) for this variable
        unsigned int timestamp; // time stamp for composition
        int aux; // flag used by sifting algorithm
        struct var_struct *up, *down; // the neighboring active variables

        bdd_mgr& bdd_mgr_;
};

using var = var_struct;

}
