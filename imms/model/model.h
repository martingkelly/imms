#ifndef __MODEL_H
#define __MODEL_H

#include <memory>
#include <vector>

#define NUM_FEATURES 12

class Song;
class MixtureModel;

class Model {
public:
    virtual ~Model() {};
    virtual float evaluate(float *features) = 0;
};

class SimilarityModel
{
public:
    SimilarityModel(Model *model);
    ~SimilarityModel();
    float evaluate(const Song &s1, const Song &s2);
    float evaluate(const MixtureModel &mm1, float *beats1,
                   const MixtureModel &mm2, float *beats2);

    float evaluate(float *features);

    static void extract_features(
            const MixtureModel &mm1, float *beats1,
            const MixtureModel &mm2, float *beats2,
            std::vector<float> *features);
private:
    std::auto_ptr<Model> model;
};

class MLPSimilarityModel : public SimilarityModel {
public:
    MLPSimilarityModel();
};

#endif
