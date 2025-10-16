// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef COMPUTEGRAPHLETS_HPP
#define COMPUTEGRAPHLETS_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <iostream>
#include <fstream>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include "utils.hpp"

namespace computeGraphletsSource {

std::vector<std::vector<uint>> computeGraphlets(int maxGraphletSize, FILE *fp);

}

#endif /* COMPUTEGRAPHLETS_HPP */
