// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef MEASURE_HPP
#define MEASURE_HPP
#include <string>
#include "../Graph.hpp"
#include "../utils/utils.hpp"
#include "../Alignment.hpp"
#include "../utils/Timer.hpp"

class Measure {
public:
    Measure(const Graph* G1, const Graph* G2, const string& name);
    virtual ~Measure();
    virtual double eval(const Alignment& A) =0;
    string getName();
    virtual bool isLocal();
    virtual double balanceWeight();
protected:
    const Graph* G1;
    const Graph* G2;
private:
    string name;
};

#endif
