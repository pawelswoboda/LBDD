#include "bdd_node.h"
#include "bdd_mgr.h"
#include <cassert>
#include <algorithm>

namespace BDD {

    //static std::random_device rd;
    //node_struct::unique_table_hash_gen = rd();
    //node_struct::unique_table_distribution = std::uniform_int_distribution<std::size_t>(0,hashtablesize-1); 

    void node_struct::init_new_node(std::size_t v, node_struct* l, node_struct* h) 
    {
        assert(v < std::pow(2,logvarsize));
        lo = l;
        hi = h;
        hi->xref++;
        lo->xref++;
        index = v;
        marked_ = 0;
        xref = 0;

        static std::random_device rd;
        static std::mt19937 unique_table_gen(rd());
        static std::uniform_int_distribution<std::size_t> unique_table_distribution(0,hashtablesize-1);

        hash_key = unique_table_distribution(unique_table_gen);

    }

    size_t node::nr_nodes()
    {
        assert(marked_ == 0);
        const size_t n = nr_nodes_impl();
        unmark();
        return n;
    }

    size_t node::nr_nodes_impl()
    {
        // TODO: make non-recursive
        if(is_terminal())
            return 0;
        assert(marked_ == 0);

        size_t n = 1;
        marked_ = 1;
        if(lo->marked_ == 0)
            n += lo->nr_nodes_impl();
        if(hi->marked_ == 0)
            n += hi->nr_nodes_impl();
        
        return n;
    }

    std::vector<node*> node::nodes_postorder()
    {
        assert(marked_ == 0);
        std::vector<node*> n;
        nodes_postorder_impl(n);
        unmark();
        assert(n.size() == nr_nodes());
        return n;
    }

    void node::nodes_postorder_impl(std::vector<node*>& n)
    {
        if(is_terminal())
            return;
        assert(marked_ == 0);

        marked_ = 1;
        if(lo->marked_ == 0)
            lo->nodes_postorder_impl(n);
        if(hi->marked_ == 0)
            hi->nodes_postorder_impl(n); 
        n.push_back(this);
    }

    void node::init_botsink(bdd_mgr* mgr)
    {
        this->bdd_mgr_1 = mgr;
        this->bdd_mgr_2 = mgr;
        this->xref = 1;
        this->index = botsink_index;
    }

    bool node::is_botsink() const
    {
        assert((this->index == topsink_index || this->index == botsink_index) == (this->lo == this->hi));
        return (this->index == botsink_index);
    }

    void node::init_topsink(bdd_mgr* mgr)
    {
        this->bdd_mgr_1 = mgr;
        this->bdd_mgr_2 = mgr;
        this->xref = 1;
        this->index = topsink_index;

    }

    bool node::is_topsink() const
    {
        assert((this->index == topsink_index || this->index == botsink_index) == (this->lo == this->hi));
        return (this->index == topsink_index);
    }

    std::size_t node_struct::hash_code() const
    {
        assert(lo != nullptr);
        assert(hi != nullptr);
        return (lo->index << 3) ^ (hi->index << 2);
    }

    void node_struct::mark()
    {
        if(is_terminal())
            return;
        if(!marked())
        {
            marked_ = 1;
            lo->mark();
            hi->mark();
        }
    }

    void node_struct::unmark()
    {
        if(is_terminal())
            return;
        if(marked())
        {
            marked_ = 0;
            lo->unmark();
            hi->unmark();
        }
    }

    bool node_struct::marked() const
    {
        return marked_;
    }

    std::vector<size_t> node_struct::variables()
    {
        std::vector<size_t> v;
        variables_impl(v);
        unmark();
        std::sort(v.begin(), v.end());
        v.erase( std::unique(v.begin(), v.end() ), v.end());
        return v;

    }

    void node_struct::variables_impl(std::vector<size_t>& v)
    {
        if(is_terminal())
            return;
        assert(marked_ == 0);

        marked_ = 1;
        v.push_back(index);
        if(lo->marked_ == 0)
            lo->variables_impl(v);
        if(hi->marked_ == 0)
            hi->variables_impl(v); 
    }

    // TODO: make implementations operate without stack
    void node_struct::recursively_revive()
    {
        xref= 0;
        //deadnodes--;
        if(lo->xref<0)
            lo->recursively_revive();
        else 
            lo->xref++;
        if(hi->xref<0)
            hi->recursively_revive();
        else 
            hi->xref++;
    }

    void node_struct::recursively_kill()
    {
        xref= -1;
        //deadnodes++;
        if(lo->xref==0)
            lo->recursively_kill();
        else 
            lo->xref--;
        if(hi->xref==0)
            hi->recursively_kill();
        else 
            hi->xref--;
    }

    void node_struct::deref()
    {
        if(xref == 0) 
            recursively_kill();
        else 
            xref--;
    }

    bdd_mgr* node_struct::find_bdd_mgr()
    {
        if(is_terminal())
            return bdd_mgr_1;
        if(lo->is_terminal())
            return lo->bdd_mgr_1;
        return hi->find_bdd_mgr(); 
    }

    node_ref::node_ref(node* p)
        : ref(p)
    {
        assert(ref != nullptr);
        ref->xref++;
    }

    node_ref::node_ref(const node_ref& o)
        : ref(o.ref)
    {
        assert(ref != nullptr);
        ref->xref++;
    }

    node_ref::~node_ref()
    {
        if(ref != nullptr) 
            ref->deref();
    }

    node_ref::node_ref(node_ref&& o)
    {
        std::swap(ref, o.ref);
    }

}
