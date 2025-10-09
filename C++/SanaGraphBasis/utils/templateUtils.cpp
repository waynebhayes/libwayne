// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

//same as utils, but for templated functions
//it is included from utils. because it contains templates, it has no header itself

template <typename T>
string toStringWithPrecision(const T val, const int n) {
    ostringstream oss;
    oss.precision(n);
    oss << std::fixed << val;
    return oss.str();
}
