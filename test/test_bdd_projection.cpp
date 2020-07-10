#include "bdd_mgr.h"
#include "test.h"

using namespace BDD;

int main(int argc, char** argv)
{
    bdd_mgr mgr;
    // add 3 variables
    mgr.add_variable();
    mgr.add_variable();
    mgr.add_variable();
    node* n0 = mgr.negate(mgr.projection(0));
    node* n1 = mgr.negate(mgr.projection(1));
    node* n2 = mgr.negate(mgr.projection(2));

    std::array<bool,3> labeling = {0,0,0};
    test(mgr.projection(0)->evaluate(labeling.begin(), labeling.end()) == false);
    test(n0->evaluate(labeling.begin(), labeling.end()) == true);
    labeling[0] = 1;
    test(mgr.projection(0)->evaluate(labeling.begin(), labeling.end()) == true);
    test(n0->evaluate(labeling.begin(), labeling.end()) == false);

    test(mgr.projection(1)->evaluate(labeling.begin(), labeling.end()) == false);
    test(n1->evaluate(labeling.begin(), labeling.end()) == true);
    labeling[1] = 1;
    test(mgr.projection(1)->evaluate(labeling.begin(), labeling.end()) == true);
    test(n1->evaluate(labeling.begin(), labeling.end()) == false);

    test(mgr.projection(2)->evaluate(labeling.begin(), labeling.end()) == false);
    test(n2->evaluate(labeling.begin(), labeling.end()) == true);
    labeling[2] = 1;
    test(mgr.projection(2)->evaluate(labeling.begin(), labeling.end()) == true);
    test(n2->evaluate(labeling.begin(), labeling.end()) == false);
}

