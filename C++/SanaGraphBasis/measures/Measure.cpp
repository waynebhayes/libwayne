// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include "Measure.hpp"
#include <string>

Measure::Measure(const Graph* G1, const Graph* G2, const string& name): G1(G1), G2(G2), name(name) {};
Measure::~Measure() {}
string Measure::getName() { return name; }
bool Measure::isLocal() { return false; }
double Measure::balanceWeight() { return 0; }
