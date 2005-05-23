#ifndef __MFCCKEEPER_H
#define __MFCCKEEPER_H

#include <torch/Sequence.h>
#include <torch/MemoryDataSet.h>
#include <torch/DiagonalGMM.h>

#define NUMCEPSTR   10
#define NUMGAUSS    3

struct Gaussian
{
    Gaussian() {};
    Gaussian(float weight, float *means, float *vars);
    float weight;
    float means[NUMCEPSTR];
    float vars[NUMCEPSTR];
};

struct MixtureModel
{
    MixtureModel() {}
    MixtureModel(Torch::DiagonalGMM &gmm)
        { init(gmm); }
    MixtureModel &operator =(Torch::DiagonalGMM &gmm)
        { init(gmm); return *this; }
    Gaussian gauss[NUMGAUSS];
private:
    void init(Torch::DiagonalGMM &gmm);
};

class MFCCKeeper
{
public:
    MFCCKeeper();
    void process(float *capstrum);
    void finalize();
    void *get_result();

    static const int ResultSize = sizeof(MixtureModel);
protected:
    Torch::Sequence cepseq;
    Torch::Sequence* cepseq_p;
    Torch::MemoryDataSet cepdat;

    MixtureModel result;
};

#endif
