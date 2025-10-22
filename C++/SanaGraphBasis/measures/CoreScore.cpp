// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#include "CoreScore.hpp"

CoreScoreData::CoreScoreData() {}

CoreScoreData::CoreScoreData(uint n1, uint n2) {
    numPegSamples = vector<unsigned long>(n1, 0);
    pegHoleFreq = Matrix<unsigned long>(n1, n2);
    weightedPegHoleFreq_pBad = Matrix<double>(n1, n2);
    totalWeightedPegWeight_pBad = vector<double>(n1, 0);
    weightedPegHoleFreq_1mpBad = Matrix<double>(n1, n2);
    totalWeightedPegWeight_1mpBad = vector<double>(n1, 0);
    weightedPegHoleFreq_pwPBad = Matrix<double>(n1, n2);
    totalWeightedPegWeight_pwPBad = vector<double>(n1, 0);
    weightedPegHoleFreq_1mpwPBad = Matrix<double>(n1, n2);
    totalWeightedPegWeight_1mpwPBad = vector<double>(n1, 0);
}


/*The following is designed so that every single node from both networks
  is printed at least once. First, we find for every single node (across
  BOTH networks) what it's *highest* score is with a partner in the other
  network. Once we compute every node's highest score, we then go and find
  the smallest such score, and call it Smin. This defines the minimum score
  that we want to output, and it guarantees that every node appears in at
  least one aligned node-pair according to this score.  We output this Smin. */
double CoreScoreData::trimCoreScore(Matrix<unsigned long>& Freq, vector<unsigned long>& numPegSamples) {
    uint n1 = Freq.size(), n2 = Freq[0].size();
    vector<double> high1(n1,0.0);
    vector<double> high2(n2,0.0);
    for (uint i=0; i<n1; i++) {
        double denom =  1.0 / (double)numPegSamples[i];
        for (uint j=0;j<n2; j++) {
            double score = Freq[i][j] * denom;
            if (score > high1[i]) high1[i] = score;
            if (score > high2[j]) high2[j] = score;
        }
    }
    double Smin = high1[0];
    for (uint i=0;i<n1;i++) if (high1[i] < Smin) Smin = high1[i];
    for (uint j=0;j<n2;j++) if (high2[j] < Smin) Smin = high2[j];
    return Smin;
}
double CoreScoreData::trimCoreScore(Matrix<double>& Freq, vector<double>& totalPegWeight) {
    uint n1 = Freq.size(), n2 = Freq[0].size();
    vector<double> high1(n1,0.0);
    vector<double> high2(n2,0.0);
    for (uint i=0; i<n1; i++) {
        double denom =  1.0 / (double)totalPegWeight[i];
        for (uint j=0;j<n2; j++) {
            double score = Freq[i][j] * denom;
            if (score > high1[i]) high1[i] = score;
            if (score > high2[j]) high2[j] = score;
        }
    }
    double Smin = high1[0];
    for (uint i=0;i<n1;i++) if (high1[i] < Smin) Smin = high1[i];
    for (uint j=0;j<n2;j++) if (high2[j] < Smin) Smin = high2[j];
    return Smin;
}

void CoreScoreData::incChangeOp(uint source, uint betterHole, double pBad, double meanPBad)
{
    numPegSamples[source]++;
    pegHoleFreq[source][betterHole]++;
    totalWeightedPegWeight_pBad[source] += meanPBad;
    weightedPegHoleFreq_pBad[source][betterHole] += meanPBad;
    totalWeightedPegWeight_1mpBad[source] += 1-meanPBad;
    weightedPegHoleFreq_1mpBad[source][betterHole] += 1-meanPBad;
    totalWeightedPegWeight_pwPBad[source] += pBad;
    weightedPegHoleFreq_pwPBad[source][betterHole] += pBad;
    totalWeightedPegWeight_1mpwPBad[source] += 1-pBad;
    weightedPegHoleFreq_1mpwPBad[source][betterHole] += 1-pBad;
}

void CoreScoreData::incSwapOp(uint source1, uint source2, uint betterDest1, uint betterDest2, double pBad, double meanPBad)
{
    numPegSamples[source1]++; numPegSamples[source2]++;
    pegHoleFreq[source1][betterDest1]++; pegHoleFreq[source2][betterDest2]++;
    totalWeightedPegWeight_pBad[source1] += meanPBad;
    weightedPegHoleFreq_pBad[source1][betterDest1] += meanPBad;
    totalWeightedPegWeight_pBad[source2] += meanPBad;
    weightedPegHoleFreq_pBad[source2][betterDest2] += meanPBad;

    totalWeightedPegWeight_1mpBad[source1] += 1-meanPBad;
    weightedPegHoleFreq_1mpBad[source1][betterDest1] += 1-meanPBad;
    totalWeightedPegWeight_1mpBad[source2] += 1-meanPBad;
    weightedPegHoleFreq_1mpBad[source2][betterDest2] += 1-meanPBad;
    
    totalWeightedPegWeight_pwPBad[source1] += pBad;
    weightedPegHoleFreq_pwPBad[source1][betterDest1] += pBad;
    totalWeightedPegWeight_pwPBad[source2] += pBad;
    weightedPegHoleFreq_pwPBad[source2][betterDest2] += pBad;

    totalWeightedPegWeight_1mpwPBad[source1] += 1-pBad;
    weightedPegHoleFreq_1mpwPBad[source1][betterDest1] += 1-pBad;
    totalWeightedPegWeight_1mpwPBad[source2] += 1-pBad;
    weightedPegHoleFreq_1mpwPBad[source2][betterDest2] += 1-pBad;
}
