#ifndef __MELFILTER_H
#define __MELFILTER_H

#include <vector>
#include <string>

using std::vector;

#define NUMMEL      40

class MelFilter
{
public:
    MelFilter(int leftf, int centerf, int rightf);
    double apply(const vector<double> &data);
    string print();
protected:
    int start;
    vector<float> weights;
};

class MelFilterBank
{
public:
    MelFilterBank();
    void apply(const vector<double> &data, vector<double> &mfc);
    string print();
protected:
    vector<MelFilter> filters;
};

#endif
