#include "bdd_mgr.h"
#include <cassert>

namespace BDD {

    bdd_mgr::bdd_mgr()
        : node_cache_(this),
        memo_(node_cache_)
    {}

    bdd_mgr::~bdd_mgr()
    {
        for(size_t i=0; i<vars.size(); ++i)
            vars[i].release_nodes(); 
    }

    size_t bdd_mgr::add_variable()
    {
        assert(vars.size() < maxvarsize);
        vars.emplace_back(vars.size(), *this);
        return vars.size()-1;
    }

    node_ref bdd_mgr::projection(const size_t var)
    {
        for(size_t i=vars.size(); i<=var; ++i)
            add_variable();
        assert(var < vars.size());
        return node_ref(vars[var].unique_find(node_cache_.botsink(), node_cache_.topsink()));
        //return vars[var].projection();
    }

    node_ref bdd_mgr::neg_projection(const size_t var)
    {
        assert(var < vars.size());
        return node_ref(vars[var].unique_find(node_cache_.topsink(), node_cache_.botsink())); 
    }

    node_ref bdd_mgr::negate(node_ref p)
    {
        const std::size_t v = p.variable();
        assert(v < nr_variables());
        return node_ref(vars[v].unique_find(p.ref->hi, p.ref->lo));
    }

    node_ref bdd_mgr::and_rec(node_ref f, node_ref g)
    {
        if(f == g)
        {
            //f->xref++;
            return f;
        }
        if(f.ref > g.ref)
            return and_rec(g,f);
        //assert(g != node_cache_.topsink() && g != node_cache_.botsink()); // we must also check g for being top or bottom sink

        if(f.ref == node_cache_.topsink())
        {
            //g->xref++;
            return g;
        }
        else if(f.ref == node_cache_.botsink())
        {
            //node_cache_.botsink()->xref++;
            return node_ref(node_cache_.botsink());
        }
        else if(g.ref == node_cache_.topsink())
        {
            //f->xref++;
            return f;
        }
        else if(g.ref == node_cache_.botsink())
        {
            //node_cache_.botsink()->xref++;
            return node_ref(node_cache_.botsink()); 
        }

        node* m = memo_.cache_lookup(f.ref, g.ref, memo_struct::and_symb());
        if(m != nullptr)
            return node_ref(m);

        var& f_var = vars[f.variable()];
        var& g_var = vars[g.variable()];
        var& v = [&]() -> var& {
            if(&f_var < &g_var)
                return f_var;
            else 
                return g_var;
        }();

        node_ref r0 = and_rec(&v == &f_var ? f.low() : f, &v == &g_var ? g.low() : g);
        if(r0.ref == nullptr)
            return nullptr;
        node_ref r1 = and_rec(&v == &f_var ? f.high() : f, &v == &g_var ? g.high() : g);
        if(r1.ref == nullptr)
        {
            assert(false);
            //r0->deref();
            return nullptr;
        }
        
        node* r = v.unique_find(r0.ref, r1.ref);
        assert(r != nullptr);
        //if(r != nullptr)
        memo_.cache_insert(f.ref, g.ref, memo_struct::and_symb(), r);
        return node_ref(r); 
    }

    node_ref bdd_mgr::or_rec(node_ref f, node_ref g)
    {
        // trivial cases
        if(f == g)
        {
            return f;
        }

        if(f.ref > g.ref)
            return or_rec(g,f);
            //std::swap(f,g);

        if(f.ref == node_cache_.topsink())
        {
            return node_ref(node_cache_.topsink());
        }
        else if(f.ref == node_cache_.botsink())
        {
            return g;
        }
        else if(g.ref == node_cache_.topsink())
        {
            return node_ref(node_cache_.topsink()); 
        }
        else if(g.ref == node_cache_.botsink())
        {
            return f;
        }

        node* m = memo_.cache_lookup(f.ref, g.ref, memo_struct::or_symb());
        if(m != nullptr)
            return m;

        // find recursively
        var& f_var = vars[f.variable()];
        var& g_var = vars[g.variable()];
        var& v = [&]() -> var& {
            if(&f_var < &g_var)
                return f_var;
            else 
                return g_var;
        }();

        node_ref r0 = or_rec(&v == &f_var ? f.low() : f, &v == &g_var ? g.low() : g);
        assert(r0.ref != nullptr);
        node_ref r1 = or_rec(&v == &f_var ? f.high() : f, &v == &g_var ? g.high() : g);
        assert(r1.ref != nullptr);
        
        node* r = v.unique_find(r0.ref, r1.ref);
        if(r != nullptr)
            memo_.cache_insert(f.ref, g.ref, memo_struct::or_symb(), r);
        return node_ref(r); 
    }

    node_ref bdd_mgr::xor_rec(node_ref f, node_ref g)
    {
        // trivial cases
        if(f == g)
        {
            return node_ref(node_cache_.botsink());
        }

        if(f.ref > g.ref)
            return xor_rec(g,f);
            //std::swap(f,g);

        if(f.ref == node_cache_.botsink())
        {
            return g;
        }
        else if(g.ref == node_cache_.botsink())
        {
            return f;
        }

        node* m = memo_.cache_lookup(f.ref, g.ref, memo_struct::xor_symb());
        if(m != nullptr)
            return node_ref(m);

        // find recursively
        var& vf = f.is_topsink() ? vars[0] : vars[f.variable()];
        assert(g.variable() < nr_variables());
        var& vg = vars[g.variable()];
        var& v = *std::max(&vg,&vf);

        node_ref r0 = xor_rec(&v == &vf ? f.low() : f, &v == &vg ? g.low() : g);
        assert(r0.ref != nullptr);
        node_ref r1 = xor_rec(&v == &vf ? f.high() : f, &v == &vg ? g.high() : g);
        assert(r1.ref != nullptr);
        
        node* r = v.unique_find(r0.ref, r1.ref);
        if(r != nullptr)
            memo_.cache_insert(f.ref, g.ref, memo_struct::xor_symb(), r);
        return node_ref(r); 
    }

    node_ref bdd_mgr::ite_rec(node_ref f, node_ref g, node_ref h)
    {
        // trivial cases
        if(f.is_topsink())
            return g;
        if(f.is_botsink())
            return h;

        if(g == f || g.is_topsink())
            return or_rec(f,h);
        if(h == f || h.is_botsink())
            return and_rec(f,g);

        if(g == h)
            return g;

        if(g.is_botsink() && h.is_topsink())
            return xor_rec(node_ref(get_node_cache().topsink()), f);

        node* m = memo_.cache_lookup(f.ref, g.ref, h.ref);
        if(m != nullptr)
            return node_ref(m);

        var& vf = vars[f.variable()];
        var& vg = g.is_terminal() ? vars.back() : vars[g.variable()];
        var& vh = h.is_terminal() ? vars.back() : vars[h.variable()];

        //var& v = *std::min({&vf,&vg,&vh}); // compilation error on gcc-8.1?
        var& v = *std::min(std::min(&vf,&vg),&vh);

        node_ref r0 = ite_rec(
                (&vf == &v ? f.low() : f),
                (&vg == &v ? g.low() : g),
                (&vh == &v ? h.low() : h)
                );
        assert(r0.ref != nullptr);

        node_ref r1 = ite_rec(
                (&vf == &v ? f.high() : f),
                (&vg == &v ? g.high() : g),
                (&vh == &v ? h.high() : h)
                );
        assert(r1.ref != nullptr);

        node* r = v.unique_find(r0.ref, r1.ref);
        assert(r != nullptr);
        memo_.cache_insert(f.ref, g.ref, h.ref, r);
        return node_ref(r); 
    }

    void bdd_mgr::collect_garbage()
    {
        for(size_t i=0; i<vars.size(); ++i)
            vars[i].remove_dead_nodes();

        memo_.purge();
    }

}
