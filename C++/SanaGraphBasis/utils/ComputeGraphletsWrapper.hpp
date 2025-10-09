// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef COMPUTEGRAPHLETSWRAPPER_H_
#define COMPUTEGRAPHLETSWRAPPER_H_

#include <vector>
#include <string>
#include "utils.hpp"
#include "../Graph.hpp"

using namespace std;

//wrapper around the computeGraphlets algorithm/file
class ComputeGraphletsWrapper {
public:
static vector<vector<uint>> loadGraphletDegreeVectors(const Graph& G, uint maxGraphletSize);
static vector<vector<uint>> computeGraphletDegreeVectors(const Graph& G, uint maxGraphletSize);
static void saveGraphletsAsSigs(const Graph& G, uint maxGraphletSize, const string& fileName);

private:
static bool newerGraphAvailable(const string& graphDir, const string& binaryDir);
static void writeMatrixToFile(const vector<vector<uint>>& matrix, const string& fileName);
static void writeMatrixToBinaryFile(const vector<vector<uint>>& matrix, const string& fileName);
static void readMatrixFromBinaryFile(vector<vector<uint>>& matrix, const string& fileName);
};

#endif /* COMPUTEGRAPHLETSWRAPPER_H_ */
