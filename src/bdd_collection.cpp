#include "bdd_collection.h"
#include <deque>
#include <cassert>
#include <unordered_set>
#include <iostream> // TODO: remove

namespace BDD {

    constexpr static size_t botsink = std::numeric_limits<size_t>::max();
    constexpr static size_t topsink = std::numeric_limits<size_t>::max()-1;

    size_t bdd_collection::splitting_variable(const bdd_instruction& f, const bdd_instruction& g) const
    {
        // TODO: change order for common cases first
        if(f.is_terminal() && g.is_terminal())
            if(f.is_topsink() && g.is_topsink())
                return topsink;
            else
                return botsink;
        else if(f.is_terminal() && !g.is_terminal())
            if(f.is_topsink())
                return g.index;
            else
                return botsink;
        else if(!f.is_terminal() && g.is_terminal())
            if(g.is_topsink())
                return f.index;
            else
                return botsink;
        else
            return std::min(f.index, g.index); 
    }

    size_t bdd_collection::bdd_and(const size_t i, const size_t j, const size_t node_limit)
    {
        assert(i < nr_bdds());
        assert(j < nr_bdds());
        assert(stack.empty());
        assert(generated_nodes.empty());
        assert(reduction.empty());

        // generate terminal vertices
        stack.push_back({botsink, botsink, botsink});
        stack.push_back({topsink, topsink, topsink});
        const size_t root_idx = bdd_and_impl(bdd_delimiters[i], bdd_delimiters[j], node_limit);
        if(root_idx != std::numeric_limits<size_t>::max())
        {
        const size_t offset = bdd_delimiters.back();
        for(ptrdiff_t s = stack.size()-1; s>=0; --s)
        {
            const bdd_instruction bdd_stack = stack[s];
            const size_t lo = offset + stack.size() - bdd_stack.lo;
            const size_t hi = offset + stack.size() - bdd_stack.hi;
            bdd_instructions.push_back({lo, hi, stack[s].index});
        }
        bdd_delimiters.push_back(bdd_instructions.size());
        }

        generated_nodes.clear();
        reduction.clear();
        stack.clear();
        assert(is_bdd(2));
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
        const size_t v = std::min(f.index, g.index);//splitting_variable(f, g);
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

    size_t bdd_collection::add_bdd(node_ref root)
    {
        assert(bdd_delimiters.back() == bdd_instructions.size());

        auto nodes = root.nodes_bfs();
        for(size_t i=0; i<nodes.size(); ++i)
        {
            node_ref_hash.insert({nodes[i], bdd_instructions.size()});
            bdd_instructions.push_back(bdd_instruction{std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), nodes[i].variable()});
        }

        // TODO: not most efficient, record top and botsink above
        node_ref_hash.insert({nodes.back().botsink(), bdd_instructions.size()});
        bdd_instructions.push_back({botsink, botsink, botsink});

        node_ref_hash.insert({nodes.back().topsink(), bdd_instructions.size()});
        bdd_instructions.push_back({topsink, topsink, topsink});

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
        return bdd_delimiters.size()-2;
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
            bdd_instructions[bdd_idx].index = botsink;
        else    
        {
            assert(bdd.is_topsink());
            bdd_instructions[bdd_idx].index = topsink;
        }

        return bdd_idx;
    }

    size_t bdd_collection::nr_bdd_nodes(const size_t i) const
    {
        assert(i < nr_bdds());
        return bdd_delimiters[i+1] - bdd_delimiters[i];
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
            if(bdd_nodes.count(bdd) > 0)
                return false;
            bdd_nodes.insert(bdd);
        }

        return true;
    }

}
