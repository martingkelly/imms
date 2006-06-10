#ifndef __MODEL_H
#define __MODEL_H

#include <memory>
#include <vector>

#define NUM_FEATURES 12

struct SimilarityModelPrivate;
class Song;
struct MixtureModel;

class SimilarityModel
{
public:
    SimilarityModel();
    ~SimilarityModel();
    float evaluate(const Song &s1, const Song &s2);
    float evaluate(const MixtureModel &mm1, float *beats1,
                   const MixtureModel &mm2, float *beats2);
    float evaluate(float *features, bool normalize = true);
    static void extract_features(
            const MixtureModel &mm1, float *beats1,
            const MixtureModel &mm2, float *beats2,
            std::vector<float> *features);
private:
    std::auto_ptr<SimilarityModelPrivate> impl;
};

#endif
