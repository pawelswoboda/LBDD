#pragma once

#include "bdd_mgr.h"
#include <vector>
#include <unordered_map> // TODO: replace with faster hash map

namespace BDD {

    struct bdd_instruction {
        size_t lo;
        size_t hi;
        size_t index;

        constexpr static  size_t botsink_index = std::numeric_limits<size_t>::max()-1;
        bool is_botsink() const { return index == botsink_index; }

        constexpr static  size_t topsink_index = std::numeric_limits<size_t>::max();
        bool is_topsink() const { return index == topsink_index; }

        bool is_terminal() const { return lo == hi; }

        bool operator==(const bdd_instruction& o) const { return lo == o.lo && hi == o.hi && index == o.index; }
    };

    struct bdd_instruction_hasher {
        size_t operator()(const bdd_instruction& bdd) const { return std::hash<size_t>()(bdd.lo) ^ std::hash<size_t>()(bdd.hi) ^ std::hash<size_t>()(bdd.index); }
    };

    struct array_hasher {
        size_t operator()(const std::array<size_t,2>& a) const { return a[0] ^ a[1]; } 
    };

    class bdd_collection {
        public:
            // synthesize bdd unless it has too many nodes. If so, return std::numeric_limits<size_t>::max()
            size_t bdd_and(const size_t i, const size_t j, const size_t node_limit = std::numeric_limits<size_t>::max());
            size_t add_bdd(node_ref bdd);
            size_t nr_bdds() const { return bdd_delimiters.size()-1; }
            size_t nr_bdd_nodes(const size_t i) const;
            template<typename ITERATOR>
                bool evaluate(const size_t bdd_nr, ITERATOR var_begin, ITERATOR var_end) const;
            template<typename ITERATOR>
                void rebase(const size_t bdd_nr, ITERATOR var_map_begin, ITERATOR var_map_end);
            std::vector<size_t> variables(const size_t bdd_nr) const;
            // remove bdds with indices occurring in iterator
            template<typename ITERATOR>
                void remove(ITERATOR bdd_it_begin, ITERATOR bdd_it_end);

        private:
            size_t bdd_and_impl(const size_t i, const size_t j, const size_t node_limit);
            size_t splitting_variable(const bdd_instruction& k, const bdd_instruction& l) const;
            size_t add_bdd_impl(node_ref bdd);
            bool is_bdd(const size_t i) const;

            std::vector<bdd_instruction> bdd_instructions;
            std::vector<size_t> bdd_delimiters = {0};

            // temporary memory for bdd synthesis
            std::vector<bdd_instruction> stack; // for computing bdd meld;

            std::unordered_map<std::array<size_t,2>,size_t, array_hasher> generated_nodes; // given nodes of left and right bdd, has melded template be generated?
            std::unordered_map<bdd_instruction,size_t,bdd_instruction_hasher> reduction; // for generating a restricted graph. Given a variable index and left and right descendant, has node been generated?

            // node_ref -> index in bdd_instructions
            std::unordered_map<node_ref, size_t> node_ref_hash;
    };

    template<typename ITERATOR>
        bool bdd_collection::evaluate(const size_t bdd_nr, ITERATOR var_begin, ITERATOR var_end) const
        {
            assert(bdd_nr < nr_bdds());
            for(size_t i=bdd_delimiters[bdd_nr];;)
            {
                const bdd_instruction bdd = bdd_instructions[i];
                if(bdd.is_topsink())
                    return true;
                if(bdd.is_botsink())
                    return false;
                assert(bdd.index < std::distance(var_begin, var_end));
                const bool x = *(var_begin + bdd.index);
                if(x == true)
                    i = bdd.hi;
                else
                    i = bdd.lo;
            } 
        }

    template<typename ITERATOR>
        void bdd_collection::rebase(const size_t bdd_nr, ITERATOR var_map_begin, ITERATOR var_map_end)
        {
            assert(bdd_nr < nr_bdds());
            const size_t offset = bdd_instructions.size();
            for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]; ++i)
            {
                bdd_instruction& bdd = bdd_instructions[i];
                assert(bdd.index < std::distance(var_map_begin, var_map_end) || bdd.is_terminal());
                const size_t rebase_index = [&]() {
                    if(bdd.is_terminal())
                        return bdd.index;
                    else
                        return *(var_map_begin + bdd.index);
                }();
                bdd.index = rebase_index;
                //bdd_instructions.push_back({bdd.lo + offset, bdd.hi + offset, rebase_index});
            }
            //bdd_delimiters.push_back(bdd_instructions.size());
            //return bdd_delimiters.size()-2;
        }

    template<typename ITERATOR>
        void bdd_collection::remove(ITERATOR bdd_it_begin, ITERATOR bdd_it_end)
        {
            const size_t nr_bdds_remove = std::distance(bdd_it_begin, bdd_it_end);
            assert(std::distance(bdd_it_begin, bdd_it_end) <= nr_bdds());
            assert(std::is_sorted(bdd_it_begin, bdd_it_end));

            if(bdd_it_begin == bdd_it_end)
                return;

            auto bdd_it = bdd_it_begin;
            size_t c = bdd_delimiters[*bdd_it]; // where to move bdd instruction
            for(size_t bdd_nr=*bdd_it++; bdd_nr<nr_bdds(); ++bdd_nr)
            {
                if(bdd_nr == *bdd_it) // overwrite bdd
                {
                    ++bdd_it;
                }
                else
                {
                    std::copy(bdd_instructions.begin() + bdd_delimiters[bdd_nr],
                            bdd_instructions.begin() + bdd_delimiters[bdd_nr+1],
                            bdd_instructions.begin() + c);

                    bdd_delimiters[bdd_nr] = c; 
                    c += nr_bdd_nodes(bdd_nr);
                }
            }
            assert(bdd_it == bdd_it_end);

            bdd_delimiters.resize(bdd_delimiters.size() - nr_bdds_remove);
            bdd_delimiters.back() = c;
            bdd_instructions.resize(c);
        }

}
