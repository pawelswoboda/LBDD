#include "bdd_collection.h"
#include <deque>
#include <cassert>
#include <unordered_set>
#include <iostream> // TODO: remove

namespace BDD {

    size_t bdd_collection::bdd_and(const size_t i, const size_t j, const size_t node_limit)
    {
        assert(i < nr_bdds());
        assert(j < nr_bdds());
        assert(stack.empty());
        assert(generated_nodes.empty());
        assert(reduction.empty());
        assert(node_limit > 2);

        // generate terminal vertices
        stack.push_back(bdd_instruction::botsink());
        stack.push_back(bdd_instruction::topsink());
        const size_t root_idx = bdd_and_impl(bdd_delimiters[i], bdd_delimiters[j], node_limit);

        if(root_idx != std::numeric_limits<size_t>::max())
        {
            assert(stack.size() > 2);
            const size_t offset = bdd_delimiters.back();
            for(ptrdiff_t s = stack.size()-1; s>=0; --s)
            {
                const bdd_instruction bdd_stack = stack[s];
                const size_t lo = offset + stack.size() - bdd_stack.lo - 1;
                const size_t hi = offset + stack.size() - bdd_stack.hi - 1;
                bdd_instructions.push_back({lo, hi, stack[s].index});
            }
            bdd_delimiters.push_back(bdd_instructions.size());
            assert(is_bdd(bdd_delimiters.size()-2));
        }

        generated_nodes.clear();
        reduction.clear();
        stack.clear();
        if(root_idx == std::numeric_limits<size_t>::max())
            return std::numeric_limits<size_t>::max();
        return bdd_delimiters.size()-2;
    }

    // given two bdd_instructions indices, compute new melded node, if it has not yet been created. Return index on stack.
    size_t bdd_collection::bdd_and_impl(const size_t f_i, const size_t g_i, const size_t node_limit)
    {
        // first, check whether node has been generated already
        if(generated_nodes.count({f_i,g_i}) > 0)
            return generated_nodes.find({f_i,g_i})->second;

        const bdd_instruction& f = bdd_instructions[f_i];
        const bdd_instruction& g = bdd_instructions[g_i];

        // check if instructions are terminals
        // TODO: more efficient
        if(f.is_terminal() && g.is_terminal())
            if(f.is_topsink() && g.is_topsink())
                return 1; //topsink position on stack
            else
                return 0; //botsink position on stack
        else if(f.is_terminal() && !g.is_terminal())
            if(f.is_botsink())
                return 0; //botsink position on stack
            else if(!f.is_terminal() && g.is_terminal())
                if(g.is_botsink())
                    return 0; //botsink position on stack 

        // compute lo and hi and see if they are present already. It not, add new branch instruction
        const size_t v = std::min(f.index, g.index);
        const size_t lo = bdd_and_impl(
                v == f.index ? f.lo : f_i,
                v == g.index ? g.lo : g_i,
                node_limit
                );
        if(lo == std::numeric_limits<size_t>::max())
            return std::numeric_limits<size_t>::max();
        const size_t hi = bdd_and_impl(
                v == f.index ? f.hi : f_i,
                v == g.index ? g.hi : g_i,
                node_limit
                );
        if(hi == std::numeric_limits<size_t>::max())
            return std::numeric_limits<size_t>::max();

        if(lo == hi)
            return lo;

        if(reduction.count({lo,hi,v}) > 0)
            return reduction.find({lo,hi,v})->second;

        stack.push_back({lo, hi, v});
        if(stack.size() > node_limit)
            return std::numeric_limits<size_t>::max();
        const size_t meld_idx = stack.size()-1;
        generated_nodes.insert(std::make_pair(std::array<size_t,2>{f_i,g_i}, meld_idx));
        reduction.insert(std::make_pair(bdd_instruction{lo,hi,v}, meld_idx));

        return meld_idx;
    }

    template<size_t N>
        size_t bdd_collection::bdd_and(const std::array<size_t,N>& bdds, const size_t node_limit)
        {
            for(const size_t bdd_nr : bdds)
            {
                assert(bdd_nr < nr_bdds());
            }
            assert(stack.empty());
            assert(generated_nodes.empty());
            assert(reduction.empty());
            assert(node_limit > 2);

            std::array<size_t,N> bdd_indices;
            for(size_t i=0; i<N; ++i)
                bdd_indices[i] = bdd_delimiters[bdds[i]];

            // generate terminal vertices
            stack.push_back(bdd_instruction::botsink());
            stack.push_back(bdd_instruction::topsink());
            std::unordered_map<std::array<size_t,N>,size_t,array_hasher<N>> generated_nodes;
            const size_t root_idx = bdd_and_impl(bdd_indices, generated_nodes, node_limit);

            if(root_idx != std::numeric_limits<size_t>::max())
            {
                assert(stack.size() > 2);
                const size_t offset = bdd_delimiters.back();
                for(ptrdiff_t s = stack.size()-1; s>=0; --s)
                {
                    const bdd_instruction bdd_stack = stack[s];
                    const size_t lo = offset + stack.size() - bdd_stack.lo - 1;
                    const size_t hi = offset + stack.size() - bdd_stack.hi - 1;
                    bdd_instructions.push_back({lo, hi, stack[s].index});
                }
                bdd_delimiters.push_back(bdd_instructions.size());
                assert(is_bdd(bdd_delimiters.size()-2));
            }

            //generated_nodes.clear();
            reduction.clear();
            stack.clear();
            if(root_idx == std::numeric_limits<size_t>::max())
                return std::numeric_limits<size_t>::max();
            return bdd_delimiters.size()-2;
        }

    // explicit instantiation of bdd_and
    template size_t bdd_collection::bdd_and<3>(const std::array<size_t,3>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<4>(const std::array<size_t,4>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<5>(const std::array<size_t,5>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<6>(const std::array<size_t,6>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<7>(const std::array<size_t,7>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<8>(const std::array<size_t,8>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<9>(const std::array<size_t,9>& bdds, const size_t node_limit);

    template size_t bdd_collection::bdd_and<10>(const std::array<size_t,10>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<11>(const std::array<size_t,11>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<12>(const std::array<size_t,12>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<13>(const std::array<size_t,13>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<14>(const std::array<size_t,14>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<15>(const std::array<size_t,15>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<16>(const std::array<size_t,16>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<17>(const std::array<size_t,17>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<18>(const std::array<size_t,18>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<19>(const std::array<size_t,19>& bdds, const size_t node_limit);

    template size_t bdd_collection::bdd_and<20>(const std::array<size_t,20>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<21>(const std::array<size_t,21>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<22>(const std::array<size_t,22>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<23>(const std::array<size_t,23>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<24>(const std::array<size_t,24>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<25>(const std::array<size_t,25>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<26>(const std::array<size_t,26>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<27>(const std::array<size_t,27>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<28>(const std::array<size_t,28>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<29>(const std::array<size_t,29>& bdds, const size_t node_limit);

    template size_t bdd_collection::bdd_and<30>(const std::array<size_t,30>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<31>(const std::array<size_t,31>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<32>(const std::array<size_t,32>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<33>(const std::array<size_t,33>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<34>(const std::array<size_t,34>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<35>(const std::array<size_t,35>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<36>(const std::array<size_t,36>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<37>(const std::array<size_t,37>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<38>(const std::array<size_t,38>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<39>(const std::array<size_t,39>& bdds, const size_t node_limit);

    template size_t bdd_collection::bdd_and<40>(const std::array<size_t,40>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<41>(const std::array<size_t,41>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<42>(const std::array<size_t,42>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<43>(const std::array<size_t,43>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<44>(const std::array<size_t,44>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<45>(const std::array<size_t,45>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<46>(const std::array<size_t,46>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<47>(const std::array<size_t,47>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<48>(const std::array<size_t,48>& bdds, const size_t node_limit);
    template size_t bdd_collection::bdd_and<49>(const std::array<size_t,49>& bdds, const size_t node_limit);

    // given two bdd_instructions indices, compute new melded node, if it has not yet been created. Return index on stack.
    template<size_t N>
    size_t bdd_collection::bdd_and_impl(const std::array<size_t,N>& bdds, std::unordered_map<std::array<size_t,N>,size_t,array_hasher<N>>& generated_nodes, const size_t node_limit)
    {
        // first, check whether node has been generated already
        if(generated_nodes.count(bdds) > 0)
            return generated_nodes.find(bdds)->second;

        std::array<bdd_instruction,N> bdd_instrs;
        for(size_t i=0; i<N; ++i)
            bdd_instrs[i] = bdd_instructions[bdds[i]];

        // check if all instructions are true
        const bool all_true_terminal = [&]() {
            for(bdd_instruction& f : bdd_instrs)
                if(!f.is_topsink())
                    return false;
            return true;
        }();
        if(all_true_terminal)
            return 1;

        const bool one_false_terminal = [&]() {
            for(bdd_instruction& f : bdd_instrs)
                if(f.is_botsink())
                    return true;
            return false; 
        }();
        if(one_false_terminal)
            return 0;

        const size_t v = [&]() {
            size_t idx = std::numeric_limits<size_t>::max();
            for(const bdd_instruction& f : bdd_instrs)
                idx = std::min(idx, f.index);
            return idx;
        }();

        std::array<size_t,N> lo_nodes;
        for(size_t i=0; i<N; ++i)
        {
            const bdd_instruction& f = bdd_instrs[i];
            lo_nodes[i] = f.index == v ? f.lo : bdds[i];
        }
        const size_t lo = bdd_and_impl(lo_nodes, generated_nodes, node_limit);
        if(lo == std::numeric_limits<size_t>::max())
            return std::numeric_limits<size_t>::max();

        std::array<size_t,N> hi_nodes;
        for(size_t i=0; i<N; ++i)
        {
            const bdd_instruction& f = bdd_instrs[i];
            hi_nodes[i] = f.index == v ? f.hi : bdds[i];
        }
        const size_t hi = bdd_and_impl(hi_nodes, generated_nodes, node_limit);
        if(hi == std::numeric_limits<size_t>::max())
            return std::numeric_limits<size_t>::max();

        if(lo == hi)
            return lo;

        if(reduction.count({lo,hi,v}) > 0)
            return reduction.find({lo,hi,v})->second;

        stack.push_back({lo, hi, v});
        if(stack.size() > node_limit)
            return std::numeric_limits<size_t>::max();
        const size_t meld_idx = stack.size()-1;
        generated_nodes.insert(std::make_pair(bdds, meld_idx));
        reduction.insert(std::make_pair(bdd_instruction{lo,hi,v}, meld_idx));

        return meld_idx;
    }

    size_t bdd_collection::add_bdd(node_ref root)
    {
        assert(bdd_delimiters.back() == bdd_instructions.size());

        auto nodes = root.nodes_bfs();
        for(size_t i=0; i<nodes.size(); ++i)
        {
            assert(!nodes[i].is_terminal());
            node_ref_hash.insert({nodes[i], bdd_instructions.size()});
            bdd_instructions.push_back(bdd_instruction{std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), nodes[i].variable()});
        }

        // TODO: not most efficient, record top and botsink above
        node_ref_hash.insert({nodes.back().botsink(), bdd_instructions.size()});
        bdd_instructions.push_back(bdd_instruction::botsink());
        assert(bdd_instructions.back().is_botsink());

        node_ref_hash.insert({nodes.back().topsink(), bdd_instructions.size()});
        bdd_instructions.push_back(bdd_instruction::topsink());
        assert(bdd_instructions.back().is_topsink());

        const size_t offset = bdd_delimiters.back();
        for(size_t i=0; i<nodes.size(); ++i)
        {
            assert(node_ref_hash.count(nodes[i].low()) > 0);
            assert(node_ref_hash.count(nodes[i].high()) > 0);
            bdd_instructions[offset + i].lo = node_ref_hash.find(nodes[i].low())->second;
            bdd_instructions[offset + i].hi = node_ref_hash.find(nodes[i].high())->second;
        }

        bdd_delimiters.push_back(bdd_instructions.size());

        // clean-up
        node_ref_hash.clear();
        assert(is_bdd(bdd_delimiters.size()-2));
        assert(nr_bdd_nodes(bdd_delimiters.size()-2) == nodes.size()+2);
        return bdd_delimiters.size()-2;
    }

    node_ref bdd_collection::export_bdd(bdd_mgr& mgr, const size_t bdd_nr) const
    {
        assert(bdd_nr < nr_bdds());
        assert(nr_bdd_nodes(bdd_nr) > 2);
        // TODO: use vector and shift indices by offset
        std::unordered_map<size_t, node_ref> bdd_instr_hash;
        assert(bdd_instructions[bdd_delimiters[bdd_nr+1]-2].is_terminal());
        assert(bdd_instructions[bdd_delimiters[bdd_nr+1]-1].is_terminal());
        for(ptrdiff_t i=bdd_delimiters[bdd_nr+1]-3; i>=ptrdiff_t(bdd_delimiters[bdd_nr]); --i)
        {
            const bdd_instruction& bdd_instr = bdd_instructions[i];
            assert(!bdd_instr.is_terminal());
            auto get_node_ref = [&](const size_t i) -> node_ref {
                assert(i < bdd_instructions.size());

                if(bdd_instructions[i].is_botsink())
                {
                    std::cout << "returning botsink\n";
                    return mgr.botsink();
                }

                if(bdd_instructions[i].is_topsink())
                {
                    std::cout << "returning topsink\n";
                    return mgr.topsink();
                }

                assert(i >= bdd_delimiters[bdd_nr]);
                assert(i < bdd_delimiters[bdd_nr+1]);
                assert(bdd_instr_hash.count(i) > 0);
                return bdd_instr_hash.find(i)->second;
            };

            node_ref lo = get_node_ref(bdd_instr.lo);
            node_ref hi = get_node_ref(bdd_instr.hi);
            node_ref bdd = mgr.unique_find(bdd_instr.index, lo, hi);
            assert(bdd_instr_hash.count(i) == 0);
            bdd_instr_hash.insert({i, bdd});
        }
        assert(bdd_instr_hash.count(bdd_delimiters[bdd_nr]) > 0);
        return bdd_instr_hash.find(bdd_delimiters[bdd_nr])->second;
    }

    size_t bdd_collection::add_bdd_impl(node_ref bdd)
    {
        if(node_ref_hash.count(bdd) > 0)
            return node_ref_hash.find(bdd)->second;

        const size_t bdd_idx = bdd_instructions.size();
        bdd_instructions.push_back(bdd_instruction{std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), bdd.variable()});
        node_ref_hash.insert({bdd, bdd_idx});
        if(!bdd.is_terminal())
        {
            const size_t lo_idx = add_bdd_impl(bdd.low());
            const size_t hi_idx = add_bdd_impl(bdd.high());
            assert(lo_idx != hi_idx); 
            bdd_instructions[bdd_idx].lo = lo_idx;
            bdd_instructions[bdd_idx].hi = hi_idx;
        }
        else if(bdd.is_botsink())
            bdd_instructions[bdd_idx] = bdd_instruction::botsink();
        else    
        {
            assert(bdd.is_topsink());
            bdd_instructions[bdd_idx] = bdd_instruction::topsink();
        }

        return bdd_idx;
    }

    size_t bdd_collection::nr_bdd_nodes(const size_t i) const
    {
        assert(i < nr_bdds());
        return bdd_delimiters[i+1] - bdd_delimiters[i];
    }

    size_t bdd_collection::nr_bdd_nodes(const size_t bdd_nr, const size_t variable) const
    {
        assert(bdd_nr < nr_bdds());
        size_t nr_occurrences = 0;
        for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]; ++i)
            if(bdd_instructions[i].index == variable)
                ++nr_occurrences;
        return nr_occurrences;
    }

    bool bdd_collection::is_bdd(const size_t bdd_nr) const
    {
        assert(bdd_nr < nr_bdds());
        // first check whether each bdd lo and hi pointers are directed properly and do not point to same node
        if(nr_bdd_nodes(bdd_nr) < 2) // otherwise terminals are not present
            return false;
        if(!bdd_instructions[bdd_delimiters[bdd_nr+1]-1].is_terminal())
            return false;
        if(!bdd_instructions[bdd_delimiters[bdd_nr+1]-2].is_terminal())
            return false;

        std::unordered_set<bdd_instruction, bdd_instruction_hasher> bdd_nodes;
        for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]-2; ++i)
        {
            const bdd_instruction& bdd = bdd_instructions[i];
            if(bdd.is_terminal())
                return false;
            if(bdd.lo == bdd.hi)
                return false;
            if(i >= bdd.lo || i >= bdd.hi)
                return false;
            if(!bdd.is_terminal())
            {
                if(bdd.lo >= bdd_delimiters[bdd_nr+1])
                    return false;
                if(bdd.hi >= bdd_delimiters[bdd_nr+1])
                    return false;
                if(bdd.lo < bdd_delimiters[bdd_nr])
                    return false;
                if(bdd.hi < bdd_delimiters[bdd_nr])
                    return false;
            }
            if(bdd_nodes.count(bdd) > 0)
                return false;
            bdd_nodes.insert(bdd);
        }

        return true;
    }

    std::vector<size_t> bdd_collection::variables(const size_t bdd_nr) const
    {
        assert(bdd_nr < nr_bdds());
        std::vector<size_t> vars;
        for(size_t i=bdd_delimiters[bdd_nr]; i<bdd_delimiters[bdd_nr+1]-2; ++i)
        {
            if(vars.size() > 0 && bdd_instructions[i].index == vars.back())
                continue;
            vars.push_back(bdd_instructions[i].index);
        }

        assert(vars.size() > 0);
        std::sort(vars.begin(), vars.end());
        vars.erase( std::unique(vars.begin(), vars.end() ), vars.end());
        return vars;
    }

    bdd_collection_entry bdd_collection::operator[](const size_t bdd_nr)
    {
        return bdd_collection_entry(bdd_nr, *this);
    }

    //////////////////////////
    // bdd_collection_entry //
    //////////////////////////

    bdd_collection_entry::bdd_collection_entry(const size_t _bdd_nr, bdd_collection& _bdd_col)
        : bdd_nr(_bdd_nr),
        bdd_col(_bdd_col)
    {
        assert(bdd_nr < bdd_col.nr_bdds());
    }

    std::vector<size_t> bdd_collection_entry::variables()
    {
        return bdd_col.variables(bdd_nr);
    }

    size_t bdd_collection_entry::nr_nodes() const
    {
        return bdd_col.nr_bdd_nodes(bdd_nr);
    }

    size_t bdd_collection_entry::nr_nodes(const size_t variable) const
    {
        return bdd_col.nr_bdd_nodes(bdd_nr, variable);
    }

    bdd_collection_entry bdd_collection_entry::operator&(bdd_collection_entry& o)
    {
        assert(&bdd_col == &o.bdd_col);
        const size_t new_bdd_nr = bdd_col.bdd_and(bdd_nr, o.bdd_nr);
        return bdd_collection_entry(new_bdd_nr, bdd_col);
    }

    bdd_collection_node bdd_collection_entry::root_node() const
    {
        return bdd_collection_node(bdd_col.bdd_delimiters[bdd_nr], *this);
    }

    bdd_collection_node bdd_collection_entry::first_node_postorder() const
    {
        assert(nr_nodes() > 2);
        return bdd_collection_node(bdd_col.bdd_delimiters[bdd_nr+1]-3, *this);
    }

    bdd_collection_node bdd_collection_entry::botsink() const
    {
        bdd_collection_node node(bdd_col.bdd_delimiters[bdd_nr+1]-2, *this);
        assert(node.is_botsink());
        return node;
    }

    bdd_collection_node bdd_collection_entry::topsink() const
    {
        bdd_collection_node node(bdd_col.bdd_delimiters[bdd_nr+1]-1, *this);
        assert(node.is_topsink());
        return node;
    }

    bdd_collection_node bdd_collection_entry::operator[](const size_t i) const
    {
        bdd_collection_node node( bdd_col.bdd_delimiters[bdd_nr] + i, *(this));
        return node;

    }

    /////////////////////////
    // bdd_collection_node //
    /////////////////////////

    bdd_collection_node::bdd_collection_node(const size_t _i, const bdd_collection_entry& _bce)
        : i(_i),
        bce(_bce)
    {
        assert(i < bce.bdd_col.bdd_delimiters[bce.bdd_nr+1]);
        assert(i >= bce.bdd_col.bdd_delimiters[bce.bdd_nr]);
    }

    bdd_collection_node bdd_collection_node::lo() const
    {
        assert(!is_terminal());
        return {bce.bdd_col.bdd_instructions[i].lo, bce};
    }

    bdd_collection_node bdd_collection_node::hi() const
    {
        assert(!is_terminal());
        return {bce.bdd_col.bdd_instructions[i].hi, bce};
    }

    size_t bdd_collection_node::variable() const
    {
        return bce.bdd_col.bdd_instructions[i].index;
    }

    bool bdd_collection_node::is_botsink() const
    {
        return bce.bdd_col.bdd_instructions[i].is_botsink();
    }

    bool bdd_collection_node::is_topsink() const
    {
        return bce.bdd_col.bdd_instructions[i].is_topsink();
    }

    bool bdd_collection_node::is_terminal() const
    {
        return bce.bdd_col.bdd_instructions[i].is_terminal();
    }

    bdd_collection_node bdd_collection_node::next_postorder() const
    {
        assert(!is_terminal());
        assert(bce.root_node() != *this);
        return {i-1, bce};
    }

    bool bdd_collection_node::operator==(const bdd_collection_node& o) const
    {
        assert(&bce.bdd_col == &o.bce.bdd_col);
        return i == o.i;
    }

    bool bdd_collection_node::operator!=(const bdd_collection_node& o) const
    {
        return !(*this == o);
    }

    bdd_collection_node bdd_collection_node::operator=(const bdd_collection_node& o)
    {
        assert(&bce == &o.bce);
        i = o.i;
        return *this;
    }

}
