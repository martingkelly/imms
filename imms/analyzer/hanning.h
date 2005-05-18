#ifndef __HANNING_H
#define __HANNING_H

#include <assert.h>
#include <vector>

class HanningWindow
{
public:
    HanningWindow(unsigned size)
    {
        weights.resize(size);

        static const float alpha = 0.46;
        for (unsigned i = 0; i < size; ++i)
            weights[i] = (1 - alpha) -
                (alpha * cos((2 * M_PI * i)/(size - 1)));
    }
    void apply(double *data, unsigned size)
    {
        assert(size == weights.size());
        for (unsigned i = 0; i < size; ++i)
            data[i] *= weights[i];
    }
private:
    std::vector<double> weights;
};

#endif  // __HANNING_H
