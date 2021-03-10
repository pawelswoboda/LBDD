#pragma once

#include "bdd_mgr.h"
#include <vector>
#include <iterator>
#include <unordered_map> // TODO: replace with faster hash map

namespace BDD {

    struct bdd_instruction {
        size_t lo;
        size_t hi;
        size_t index;

        constexpr static  size_t botsink_index = std::numeric_limits<size_t>::max()-1;
        bool is_botsink() const { return index == botsink_index; }
        static bdd_instruction botsink() { return {botsink_index, botsink_index, botsink_index}; }

        constexpr static  size_t topsink_index = std::numeric_limits<size_t>::max();
        bool is_topsink() const { return index == topsink_index; }
        static bdd_instruction topsink() { return {topsink_index, topsink_index, topsink_index}; }

        bool is_terminal() const { return lo == hi; }

        bool operator==(const bdd_instruction& o) const { return lo == o.lo && hi == o.hi && index == o.index; }
    };

    struct bdd_instruction_hasher {
        size_t operator()(const bdd_instruction& bdd) const { return std::hash<size_t>()(bdd.lo) ^ std::hash<size_t>()(bdd.hi) ^ std::hash<size_t>()(bdd.index); }
    };

    template<size_t N>
    struct array_hasher {
        size_t hash_combine(size_t lhs, size_t rhs) const
        {
            lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
            return lhs;
        }
           

        size_t operator()(const std::array<size_t,N>& a) const 
        {
             size_t h = std::hash<size_t>()(a[0]);
             for(std::size_t i=1; i<N; ++i) {
                 h = hash_combine(h, std::hash<size_t>()(a[i]));
             }   
             return h; 
        } 
    };

    class bdd_collection_entry;

    // convenience class for wrapping bdd node
    class bdd_collection_node
    {
        friend class std::hash<bdd_collection_node>;
        public:
            bdd_collection_node(const size_t _i, const bdd_collection_entry& _bce);
            bdd_collection_node lo() const;
            bdd_collection_node low() const { return lo(); }
            bdd_collection_node hi() const;
            bdd_collection_node high() const { return hi(); }
            size_t variable() const;
            bool is_botsink() const;
            bool is_topsink() const;
            bool is_terminal() const;
            bdd_collection_node next_postorder() const;
            bool operator==(const bdd_collection_node& o) const;
            bool operator!=(const bdd_collection_node& o) const;
            bdd_collection_node operator=(const bdd_collection_node& o);

        private:
            size_t i; 
            const bdd_collection_entry& bce;
    };

    // convenience class to make bdd_collection bdds behave similary to node_ref
    class bdd_collection_entry {
        friend class bdd_collection_node;
        public:
            bdd_collection_entry(const size_t _bdd_nr, bdd_collection& _bdd_col);
            std::vector<size_t> variables();
            bdd_collection_entry operator&(bdd_collection_entry& o);
            bdd_collection_node operator[](const size_t i) const;
            size_t nr_nodes() const;
            size_t nr_nodes(const size_t variable) const;
            template<typename ITERATOR>
                void rebase(ITERATOR var_map_begin, ITERATOR var_map_end);
            template<typename VAR_MAP>
                void rebase(const VAR_MAP& var_map);

            bdd_collection_node root_node() const;
            bdd_collection_node first_node_postorder() const;
            bdd_collection_node botsink() const;
            bdd_collection_node topsink() const;

        private:
            const size_t bdd_nr;
            bdd_collection& bdd_col;
    };

    class bdd_collection {
        friend class bdd_collection_node;
        friend class bdd_collection_entry;
        public:
            // synthesize bdd unless it has too many nodes. If so, return std::numeric_limits<size_t>::max()
            size_t bdd_and(const size_t i, const size_t j, const size_t node_limit = std::numeric_limits<size_t>::max());
            size_t bdd_and(const int i, const int j, const size_t node_limit = std::numeric_limits<size_t>::max()) { return bdd_and(size_t(i), size_t(j), node_limit); }
            template<typename BDD_ITERATOR>
                size_t bdd_and(BDD_ITERATOR bdd_begin, BDD_ITERATOR bdd_end, const size_t node_limit = std::numeric_limits<size_t>::max());

            size_t add_bdd(node_ref bdd);
            node_ref export_bdd(bdd_mgr& mgr, const size_t bdd_nr) const;
            size_t nr_bdds() const { return bdd_delimiters.size()-1; }
            size_t size() const { return nr_bdds(); }
            size_t nr_bdd_nodes(const size_t bdd_nr) const;
            size_t nr_bdd_nodes(const size_t bdd_nr, const size_t variable) const;
            template<typename ITERATOR>
                bool evaluate(const size_t bdd_nr, ITERATOR var_begin, ITERATOR var_end) const;
            template<typename ITERATOR>
                void rebase(const size_t bdd_nr, ITERATOR var_map_begin, ITERATOR var_map_end);
            template<typename VAR_MAP>
                void rebase(const size_t bdd_nr, const VAR_MAP& var_map);
            std::vector<size_t> variables(const size_t bdd_nr) const;
            // remove bdds with indices occurring in iterator
            template<typename ITERATOR>
                void remove(ITERATOR bdd_it_begin, ITERATOR bdd_it_end);

            bdd_collection_entry operator[](const size_t bdd_nr);

            template<typename STREAM>
                void export_graphviz(const size_t bdd_nr, STREAM& s) const;

        private:
            size_t bdd_and_impl(const size_t i, const size_t j, const size_t node_limit);
            template<size_t N>
                size_t bdd_and(const std::array<size_t,N>& bdds, const size_t node_limit);
            template<size_t N>
            size_t bdd_and_impl(const std::array<size_t,N>& bdds, std::unordered_map<std::array<size_t,N>,size_t,array_hasher<N>>& generated_nodes, const size_t node_limit);
            size_t splitting_variable(const bdd_instruction& k, const bdd_instruction& l) const;
            size_t add_bdd_impl(node_ref bdd);
            bool is_bdd(const size_t i) const;

            std::vector<bdd_instruction> bdd_instructions;
            std::vector<size_t> bdd_delimiters = {0};

            // temporary memory for bdd synthesis
            std::vector<bdd_instruction> stack; // for computing bdd meld;

            std::unordered_map<std::array<size_t,2>,size_t, array_hasher<2>> generated_nodes; // given nodes of left and right bdd, has melded template be generated?
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
            }
        }

    template<typename VAR_MAP>
        void bdd_collection::rebase(const size_t bdd_nr, const VAR_MAP& var_map)
        {
            assert(bdd_nr < nr_bdds());
            for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]; ++i)
            {
                bdd_instruction& bdd = bdd_instructions[i];
                const size_t rebase_index = [&]() {
                    if(bdd.is_terminal())
                        return bdd.index;
                    else
                    {
                        assert(var_map.count(bdd.index) > 0);
                        return var_map.find(bdd.index)->second;
                    }
                }();
                bdd.index = rebase_index;
            } 
        }

    template<typename ITERATOR>
        void bdd_collection::remove(ITERATOR bdd_it_begin, ITERATOR bdd_it_end)
        {
            const size_t nr_bdds_remove = std::distance(bdd_it_begin, bdd_it_end);
            assert(std::distance(bdd_it_begin, bdd_it_end) <= nr_bdds());
            assert(std::is_sorted(bdd_it_begin, bdd_it_end));
            assert(std::unique(bdd_it_begin, bdd_it_end) == bdd_it_end);

            if(bdd_it_begin == bdd_it_end)
                return;

            std::vector<size_t> new_bdd_delimiters;
            new_bdd_delimiters.reserve(bdd_delimiters.size() - nr_bdds_remove);
            for(size_t i=0; i<=*bdd_it_begin; ++i)
                new_bdd_delimiters.push_back(bdd_delimiters[i]);

            std::vector<bdd_instruction> new_bdd_instructions;
            for(size_t i=0; i<new_bdd_delimiters.back(); ++i)
                new_bdd_instructions.push_back(bdd_instructions[i]);

            auto bdd_it = bdd_it_begin;
            for(size_t bdd_nr=*bdd_it; bdd_nr<nr_bdds(); ++bdd_nr)
            {
                if(bdd_nr == *bdd_it) // skip bdd
                {
                    ++bdd_it;
                }
                else // move bdd
                {
                    assert(bdd_delimiters[bdd_nr] >= new_bdd_delimiters.back());
                    const size_t offset_delta = bdd_delimiters[bdd_nr] - new_bdd_delimiters.back();
                    for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]; ++i)
                    {
                        const bdd_instruction& bdd_instr = bdd_instructions[i];
                        if(!bdd_instr.is_terminal())
                        {
                            assert(bdd_instr.lo >= offset_delta);
                            assert(bdd_instr.hi >= offset_delta);
                            new_bdd_instructions.push_back({bdd_instr.lo - offset_delta, bdd_instr.hi - offset_delta, bdd_instr.index});
                        }
                        else
                            new_bdd_instructions.push_back(bdd_instr);
                    }
                    new_bdd_delimiters.push_back(new_bdd_instructions.size());
                    assert(new_bdd_delimiters.back() - new_bdd_delimiters[ new_bdd_delimiters.size()-2 ] > 2);
                }
            }
            assert(bdd_it == bdd_it_end);

            std::swap(bdd_delimiters, new_bdd_delimiters);
            std::swap(bdd_instructions, new_bdd_instructions);

            for(size_t i=0; i<nr_bdds(); ++i)
            {
                assert(is_bdd(i));
            }
        }

    template<size_t N, typename ITERATOR>
        std::array<size_t,N> construct_array(ITERATOR begin, ITERATOR end)
        {
            assert(std::distance(begin,end) == N);
            std::array<size_t,N> a;
            auto it = begin;
            for(size_t i=0; i<N; ++i, ++it)
                a[i] = *it;
            return a;
        }

    template<typename BDD_ITERATOR>
        size_t bdd_collection::bdd_and(BDD_ITERATOR bdd_begin, BDD_ITERATOR bdd_end, const size_t node_limit)
        {
            const size_t nr_bdds = std::distance(bdd_begin, bdd_end);

            switch(nr_bdds) {
                case 1: return *bdd_begin;
                case 2: { const size_t i = *bdd_begin; ++bdd_begin; const size_t j = *bdd_begin; return bdd_and(i, j, node_limit); }
                case 3: { auto b = construct_array<3>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 4: { auto b = construct_array<4>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 5: { auto b = construct_array<5>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 6: { auto b = construct_array<6>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 7: { auto b = construct_array<7>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 8: { auto b = construct_array<8>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 9: { auto b = construct_array<9>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 10: { auto b = construct_array<10>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 11: { auto b = construct_array<11>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 12: { auto b = construct_array<12>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 13: { auto b = construct_array<13>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 14: { auto b = construct_array<14>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 15: { auto b = construct_array<15>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 16: { auto b = construct_array<16>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 17: { auto b = construct_array<17>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 18: { auto b = construct_array<18>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 19: { auto b = construct_array<19>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 20: { auto b = construct_array<20>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 21: { auto b = construct_array<21>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 22: { auto b = construct_array<22>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 23: { auto b = construct_array<23>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 24: { auto b = construct_array<24>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 25: { auto b = construct_array<25>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 26: { auto b = construct_array<26>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 27: { auto b = construct_array<27>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 28: { auto b = construct_array<28>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 29: { auto b = construct_array<29>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 30: { auto b = construct_array<30>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 31: { auto b = construct_array<31>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 32: { auto b = construct_array<32>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 33: { auto b = construct_array<33>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 34: { auto b = construct_array<34>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 35: { auto b = construct_array<35>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 36: { auto b = construct_array<36>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 37: { auto b = construct_array<37>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 38: { auto b = construct_array<38>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 39: { auto b = construct_array<39>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 40: { auto b = construct_array<40>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 41: { auto b = construct_array<41>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 42: { auto b = construct_array<42>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 43: { auto b = construct_array<43>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 44: { auto b = construct_array<44>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 45: { auto b = construct_array<45>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 46: { auto b = construct_array<46>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 47: { auto b = construct_array<47>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 48: { auto b = construct_array<48>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                case 49: { auto b = construct_array<49>(bdd_begin, bdd_end); return bdd_and(b, node_limit); }
                default: throw std::runtime_error("generic and not implemented.");
            }
        }

    template<typename ITERATOR>
        void bdd_collection_entry::rebase(ITERATOR var_map_begin, ITERATOR var_map_end) 
        { 
            bdd_col.rebase(bdd_nr, var_map_begin, var_map_end);
        }

    template<typename VAR_MAP>
        void bdd_collection_entry::rebase(const VAR_MAP& var_map) 
        { 
            bdd_col.rebase(bdd_nr, var_map);
        }


    template<typename STREAM>
        void bdd_collection::export_graphviz(const size_t bdd_nr, STREAM& s) const
        {
            assert(bdd_nr < nr_bdds());
            s << "digraph BDD\n";
            s << "{\n";

            std::unordered_map<size_t,std::string> clusters;
            std::unordered_map<size_t, size_t> cluster_nodes;

            for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]; ++i)
            {
                const bdd_instruction& bdd_instr = bdd_instructions[i];
                if(bdd_instr.is_terminal())
                {
                    std::string& str = clusters[std::numeric_limits<size_t>::max()];
                    if(bdd_instr.is_topsink())
                        str += std::to_string(i - bdd_delimiters[bdd_nr]) + " [label=\"top\"];\n";
                    else if(bdd_instr.is_botsink())
                        str += std::to_string(i - bdd_delimiters[bdd_nr]) + " [label=\"bot\"];\n";
                    else
                        assert(false); 
                }
                else
                {
                    std::string& str = clusters[bdd_instr.index];
                    str += std::to_string(i - bdd_delimiters[bdd_nr]) + " [label=\"" += std::to_string(bdd_instr.index) + "\"];\n"; 
                    cluster_nodes[bdd_instr.index] = i - bdd_delimiters[bdd_nr];
                }

                /*
                if(bdd_instr.is_terminal())
                    s << i - bdd_delimiters[bdd_nr] << " [label=\"top\"];\n";
                else if(bdd_instr.is_botsink())
                    s << i - bdd_delimiters[bdd_nr] << " [label=\"bot\"];\n";
                else
                    s << i - bdd_delimiters[bdd_nr] << " [label=\"" << bdd_instr.index << "\"];\n";
                    */
            }

            for(auto& [idx, str] : clusters)
            {
                s << "subgraph cluster_" << idx << " {\n";
                s << str;
                s << "color = blue\n";
                s << "}\n"; 
            }

            // add invisible arrows between clusters
            std::vector<std::array<size_t,2>> cluster_nodes2;
            for(auto [x,y] : cluster_nodes)
                cluster_nodes2.push_back({x,y});
            std::sort(cluster_nodes2.begin(), cluster_nodes2.end(), [](const auto a, const auto b) { return a[0] < b[0]; });
            for(size_t c=0; c<cluster_nodes2.size()-1; ++c)
                s << cluster_nodes2[c][1] << " -> " << cluster_nodes2[c+1][1] << " [style=invis];\n";

            for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]; ++i)
            {
                const bdd_instruction& bdd_instr = bdd_instructions[i];
                if(bdd_instr.is_terminal())
                    continue;

                s << i - bdd_delimiters[bdd_nr] << " -> " << bdd_instr.hi - bdd_delimiters[bdd_nr] << ";\n";
                s << i - bdd_delimiters[bdd_nr] << " -> " << bdd_instr.lo - bdd_delimiters[bdd_nr] << "[style=\"dashed\"];\n";
            } 
            s << "}\n";
        }

}

// inject hash function for bdd_collection_node into std namespace
namespace std
{
    template<> struct hash<BDD::bdd_collection_node>
    {
        size_t operator()(const BDD::bdd_collection_node& bdd) const
        {
            return std::hash<size_t>()(bdd.i);
        }
    };
}
