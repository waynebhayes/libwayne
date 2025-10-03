// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef LOCALMEASURE_HPP
#define LOCALMEASURE_HPP
#include "../Measure.hpp"

class LocalMeasure: public Measure {
public:
    LocalMeasure(const Graph* G1, const Graph* G2, const string& name);
    virtual ~LocalMeasure() =0;
    virtual double eval(const Alignment& A);
    bool isLocal();
    vector<vector<float>>* getSimMatrix();
    void writeSimsWithNames(string outfile);
    double balanceWeight();

protected:
    void loadBinSimMatrix(string simMatrixFileName);
    virtual void initSimMatrix() =0;
    
    vector<vector<float>> sims;
    static const string autogenMatricesFolder;
};

#endif
