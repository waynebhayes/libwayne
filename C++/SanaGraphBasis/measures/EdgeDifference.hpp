// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef EDGEDIFFERENCE_HPP
#define EDGEDIFFERENCE_HPP
#include "Measure.hpp"

class EdgeDifference: public Measure {
public:
    EdgeDifference(const Graph* G1, const Graph* G2);
    virtual ~EdgeDifference();
    double eval(const Alignment& A);

    static double adjustSumToTargetScore(double edgeDifferenceSum, uint pairsCount);
    static double getEdgeDifferenceSum(const Graph *G1, const Graph *G2, const Alignment &A);
};

#endif //EDGEDIFFERENCE_HPP
