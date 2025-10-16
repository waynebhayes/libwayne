// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef TIMER_HPP
#define TIMER_HPP
#include <string>
using namespace std;

class Timer {
public:
    Timer();

    void start();
    double elapsed() const;
    string elapsedString() const;

private:
    long long startTime;
    static long long get();
};


#endif
