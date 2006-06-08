#ifndef __MODEL_H
#define __MODEL_H

#include <memory>
#include <vector>

#define NUM_FEATURES 12

struct SimilarityModelPrivate;
class Song;

class SimilarityModel
{
public:
    SimilarityModel();
    ~SimilarityModel();
    float evaluate(const Song &s1, const Song &s2);
    static bool extract_features(const Song &s1, const Song &s2,
            std::vector<float> *features);
    float evaluate(float *features, bool normalize = true);
private:
    std::auto_ptr<SimilarityModelPrivate> impl;
};

#endif
