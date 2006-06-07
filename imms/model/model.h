#ifndef __MODEL_H
#define __MODEL_H

#include "torch/MeanVarNorm.h"
#include "torch/ConnectedMachine.h"

#define NUM_FEATURES 12

class SimilarityModel
{
public:
    SimilarityModel();
private:
    Torch::ConnectedMachine mlp;
    Torch::MeanVarNorm *normalizer;
};

#endif
