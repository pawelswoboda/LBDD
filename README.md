# LBDD
BDDs for large variable numbers.

Implementation based on [Donald Knuth's BDD package](https://https://www-cs-faculty.stanford.edu/~knuth/programs/bdd14.w).
Supports up to 2^41 variables and contains a C++ interface.

Planned:
* Fast bdd synthesis with node limit, Algorithm S in Volume 4a of Knuth's "The Art of Computer Programming".
* Non-stack versions of operations in BDD base for very deep BDDs.
* Automatic selection of appropriate stack vs. non-stack version given BDD depth.
