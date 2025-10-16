// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef EDGEEXPOSURE_HPP
#define EDGEEXPOSURE_HPP
#include <vector>
#include <iostream>
#include "utils.hpp"
#include "Measure.hpp"
#include "localMeasures/LocalMeasure.hpp"
#include "../Graph.hpp"

class EdgeExposure : public Measure {
public:
    EdgeExposure(const Graph* G1, const Graph* G2);
    virtual ~EdgeExposure();
    double eval(const Alignment& A);

    static uint getMaxEdge();
    static uint numer, denom;

    static int numExposedEdges(const Alignment& A, const Graph& G1, const Graph& G2);
private:
	static uint EDGE_SUM, MAX_EDGE;
};
#endif
