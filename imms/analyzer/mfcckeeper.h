#ifndef __MFCCKEEPER_H
#define __MFCCKEEPER_H

#include <torch/Sequence.h>
#include <torch/MemoryDataSet.h>

#define NUMCEPSTR   10
#define NUMGAUSS    3

class MFCCKeeper {
public:
    MFCCKeeper();
    void process(float *capstrum);
    void finalize();
protected:
    Torch::Sequence cepseq;
    Torch::Sequence* cepseq_p;
    Torch::MemoryDataSet cepdat;
};

#endif
