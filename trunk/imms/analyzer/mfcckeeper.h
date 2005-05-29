#ifndef __MFCCKEEPER_H
#define __MFCCKEEPER_H

#define NUMCEPSTR   15
#define NUMGAUSS    5

#include <memory>

namespace Torch {
    class DiagonalGMM;
}

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

struct MFCCKeeperPrivate;

class MFCCKeeper
{
public:
    MFCCKeeper();
    ~MFCCKeeper();
    void process(float *capstrum);
    void finalize();
    void *get_result();

    static const int ResultSize = sizeof(MixtureModel);
protected:
    std::auto_ptr<MFCCKeeperPrivate> impl;

    MixtureModel result;
};

#endif
