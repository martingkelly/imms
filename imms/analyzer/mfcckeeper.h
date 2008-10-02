#ifndef __MFCCKEEPER_H
#define __MFCCKEEPER_H

#define NUMCEPSTR   15
#define NUMGAUSS    5
#define NUMFEATURES (NUMCEPSTR*3)

#include <memory>

namespace Torch {
    class DiagonalGMM;
}

struct Gaussian
{
    static const int NumDimensions = NUMCEPSTR * 3;
    Gaussian() {};
    Gaussian(float weight, float *means, float *vars);
    float weight;
    float means[NumDimensions];
    float vars[NumDimensions];
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
    const MixtureModel &get_result();

    static const int ResultSize = sizeof(Gaussian) * NUMGAUSS;
protected:
    std::auto_ptr<MFCCKeeperPrivate> impl;
    int sample_number;
    float last_frame[NUMCEPSTR], last_delta[NUMCEPSTR];
    MixtureModel result;
};

#endif
