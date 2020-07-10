#include "bdd_mgr.h"
#include <cassert>

namespace BDD {

    bdd_mgr::bdd_mgr()
        :
        memo_(node_cache_)
    {}

    size_t bdd_mgr::add_variable()
    {
        vars.emplace_back(vars.size(), *this);
        return vars.size()-1;
    }

    node* bdd_mgr::projection(const size_t var)
    {
        assert(var < vars.size());
        return vars[var].unique_find(node_cache_.botsink(), node_cache_.topsink());
        //return vars[var].projection();
    }

    node* bdd_mgr::neg_projection(const size_t var)
    {
        assert(var < vars.size());
        return vars[var].unique_find(node_cache_.topsink(), node_cache_.botsink()); 
    }

    node* bdd_mgr::negate(node* p)
    {
        const std::size_t v = p->index;
        assert(v < nr_variables());
        return vars[v].unique_find(p->hi, p->lo);
    }

    node* bdd_mgr::and_rec(node* f, node* g)
    {
        if(f == g)
        {
            f->xref++;
            return f;
        }
        if(f > g)
            std::swap(f,g);
        //assert(g != node_cache_.topsink() && g != node_cache_.botsink()); // we must also check g for being top or bottom sink

        if(f == node_cache_.topsink())
        {
            g->xref++;
            return g;
        }
        else if(f == node_cache_.botsink())
        {
            node_cache_.botsink()->xref++;
            return node_cache_.botsink();
        }
        else if(g == node_cache_.topsink())
        {
            f->xref++;
            return f;
        }
        else if(g == node_cache_.botsink())
        {
            node_cache_.botsink()->xref++;
            return node_cache_.botsink(); 
        }

        node* r = memo_.cache_lookup(f, g, memo_struct::and_symb());
        if(r != nullptr)
            return r;

        var& f_var = vars[f->index];
        var& g_var = vars[g->index];
        var& v = [&]() -> var& {
            if(&f_var < &g_var)
                return f_var;
            else 
                return g_var;
        }();

        node* r0 = and_rec(&v == &f_var ? f->lo : f, &v == &g_var ? g->lo : g);
        if(r0 == nullptr)
            return nullptr;
        node* r1 = and_rec(&v == &f_var ? f->hi : f, &v == &g_var ? g->hi : g);
        if(r1 == nullptr)
        {
            r0->deref();
            return nullptr;
        }
        
        r = v.unique_find(r0, r1);
        if(r != nullptr)
            memo_.cache_insert(f, g, memo_struct::and_symb(), r);
        return r; 
    }

    node* bdd_mgr::or_rec(node* f, node* g)
    {
        // trivial cases
        if(f == g)
        {
            f->xref++;
            return f;
        }

        if(f > g)
            std::swap(f,g);

        if(f == node_cache_.topsink())
        {
            node_cache_.topsink()->xref++;
            return node_cache_.topsink();
        }
        else if(f == node_cache_.botsink())
        {
            g->xref++;
            return g;
        }
        else if(g == node_cache_.topsink())
        {
            node_cache_.topsink()->xref++;
            return node_cache_.topsink(); 
        }
        else if(g == node_cache_.botsink())
        {
            f->xref++;
            return f;
        }

        node* r = memo_.cache_lookup(f, g, memo_struct::or_symb());
        if(r != nullptr)
            return r;

        // find recursively
        var& f_var = vars[f->index];
        var& g_var = vars[g->index];
        var& v = [&]() -> var& {
            if(&f_var < &g_var)
                return f_var;
            else 
                return g_var;
        }();

        node* r0 = or_rec(&v == &f_var ? f->lo : f, &v == &g_var ? g->lo : g);
        if(r0 == nullptr)
            return nullptr;
        node* r1 = or_rec(&v == &f_var ? f->hi : f, &v == &g_var ? g->hi : g);
        if(r1 == nullptr)
        {
            r0->deref();
            return nullptr;
        }
        
        r = v.unique_find(r0, r1);
        if(r != nullptr)
            memo_.cache_insert(f, g, memo_struct::or_symb(), r);
        return r; 
    }

    node* bdd_mgr::xor_rec(node* f, node* g)
    {
        // trivial cases
        if(f == g)
        {
            node_cache_.botsink()->xref++;
            return node_cache_.botsink();
        }

        if(f > g)
            std::swap(f,g);

        if(f == node_cache_.botsink())
        {
            g->xref++;
            return g;
        }
        else if(g == node_cache_.botsink())
        {
            f->xref++;
            return f;
        }

        node* r = memo_.cache_lookup(f, g, memo_struct::xor_symb());
        if(r != nullptr)
            return r;

        // find recursively
        var& vf = f->is_topsink() ? vars[0] : vars[f->index];
        assert(g->index < nr_variables());
        var& vg = vars[g->index];
        var& v = *std::max(&vg,&vf);

        node* r0 = xor_rec(&v == &vf ? f->lo : f, &v == &vg ? g->lo : g);
        if(r0 == nullptr)
            return nullptr;
        node* r1 = xor_rec(&v == &vf ? f->hi : f, &v == &vg ? g->hi : g);
        if(r1 == nullptr)
        {
            r0->deref();
            return nullptr;
        }
        
        r = v.unique_find(r0, r1);
        if(r != nullptr)
            memo_.cache_insert(f, g, memo_struct::xor_symb(), r);
        return r; 
    }

    node* bdd_mgr::ite_rec(node* f, node* g, node* h)
    {
        // trivial cases
        if(f->is_topsink())
        {
            g->xref++;
            return g;
        }
        if(f->is_botsink())
        {
            h->xref++;
            return h;
        }

        if(g == f || g->is_topsink())
            return or_rec(f,h);
        if(h == f || h->is_botsink())
            return and_rec(f,g);

        if(g == h)
        {
            g->xref++;
            return g;
        }

        if(g->is_botsink() && h->is_topsink())
            return xor_rec(get_node_cache().topsink(), f);

        node* r= memo_.cache_lookup(f,g,h);
        if(r != nullptr)
            return r;

        var& vf = vars[f->index];
        var& vg = g->is_terminal() ? vars.back() : vars[g->index];
        var& vh = h->is_terminal() ? vars.back() : vars[h->index];

        //var& v = *std::min({&vf,&vg,&vh}); // compilation error on gcc-8.1?
        var& v = *std::min(std::min(&vf,&vg),&vh);

        node* r0 = ite_rec(
                (&vf == &v ? f->lo : f),
                (&vg == &v ? g->lo : g),
                (&vh == &v ? h->lo : h)
                );
        assert(r0 != nullptr);

        node* r1 = ite_rec(
                (&vf == &v ? f->hi : f),
                (&vg == &v ? g->hi : g),
                (&vh == &v ? h->hi : h)
                );
        assert(r1 != nullptr);

        r = v.unique_find(r0, r1);
        assert(r != nullptr);
        memo_.cache_insert(f,g,h,r);
        return r; 
    }

}
