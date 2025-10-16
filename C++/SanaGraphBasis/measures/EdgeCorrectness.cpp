// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include "Misc.hpp"
#include "EdgeCorrectness.hpp"
#include <string>
#include <vector>

EdgeCorrectness::EdgeCorrectness(const Graph* G1, const Graph* G2, int graphNum) : Measure(G1, G2, "ec") {
    denominatorGraph = graphNum;
}

EdgeCorrectness::~EdgeCorrectness() {
}

double EdgeCorrectness::eval(const Alignment& A) {
    switch(denominatorGraph) {
    case 1: return (double) A.numAlignedEdges(*G1, *G2)/G1->getNumEdges(); break;
    case 2: return (double) A.numAlignedEdges(*G1, *G2)/G2->getNumEdges(); break;
    default: Fatal("unknown denominatorGraph %d in EdgeCorrectness::eval", denominatorGraph); return 0; break;
    }
}
