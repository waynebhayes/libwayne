// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef EDGERATIO_HPP
#define EDGERATIO_HPP
#include "Measure.hpp"

class EdgeRatio: public Measure {
public:
    EdgeRatio(const Graph* G1, const Graph* G2);
    virtual ~EdgeRatio();
    double eval(const Alignment& A);
    static double adjustSumToTargetScore(double edgeRatioSum, uint pairsCount);
    static double getEdgeRatioSum(const Graph *G1, const Graph *G2, const Alignment &A);
private:
    const int kErrorScore = -2;

    static double getRatio(double w1, double w2);
};

#endif
