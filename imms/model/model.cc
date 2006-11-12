#include "torch/ConnectedMachine.h"
#include "torch/XFile.h"
#include "torch/DiskXFile.h"
#include "torch/MemoryXFile.h"
#include "torch/Linear.h"
#include "torch/LogSoftMax.h"
#include "torch/MeanVarNorm.h"
#include "torch/MemoryDataSet.h"
#include "torch/OneHotClassFormat.h"
#include "torch/SVMClassification.h"
#include "torch/Tanh.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

#include "model.h"
#include "song.h"
#include "immsutil.h"
#include "distance.h"

using namespace Torch;

using std::endl;
using std::cout;
using std::cerr;
using std::vector;
using std::auto_ptr;

static const int num_inputs = NUM_FEATURES;
static const int num_hidden = 25;
static const int num_outputs = 2;
static const int stdv = 12;

extern char _binary____data_svm_similarity_start;
extern char _binary____data_svm_similarity_end;

class XFileModeSetter {
public:
    XFileModeSetter() { DiskXFile::setLittleEndianMode(); }
};

static const XFileModeSetter xfilemodesetter;

class MyMemoryDataSet : public MemoryDataSet {
public:
    MyMemoryDataSet(int xn_inputs) {
        n_inputs = xn_inputs;
    }
};

class Normalizer {
public:
    Normalizer(int num_inputs) : fake(num_inputs), normalizer(&fake) {}
    void load(XFile *file)
    {
        normalizer.loadXFile(file);
    }
    void normalize(Sequence *sequence)
    {
        normalizer.preProcessInputs(sequence);
    }
private:
    MyMemoryDataSet fake;
    MeanVarNorm normalizer;
};

class SVMModel : public Model {
public:
    SVMModel()
        : kernel(1./(stdv*stdv)), svm(&kernel), normalizer(num_inputs)
    {
        auto_ptr<XFile> model;
        string filename = get_imms_root("svm-similarity");
        if (file_exists(filename))
        {
            LOG(INFO) << "Overriding the built in model with " << filename;
            model.reset(new DiskXFile(filename.c_str(), "r"));
        }
        else
        {
            static const size_t data_size = &_binary____data_svm_similarity_end
                - &_binary____data_svm_similarity_start;
            model.reset(new MemoryXFile(
                        &_binary____data_svm_similarity_start, data_size));
        }
        normalizer.load(model.get());
        svm.loadXFile(model.get());
    }

    float evaluate(float *features) {
        // Garbage, I tell you!!
        Sequence feat_seq(0, num_inputs);
        feat_seq.addFrame(features);

        normalizer.normalize(&feat_seq);

        svm.forward(&feat_seq);
        return svm.outputs->frames[0][0] / 3;
    }
     
private:
    GaussianKernel kernel;
    SVMClassification svm;
    Normalizer normalizer;
};

SVMSimilarityModel::SVMSimilarityModel()
    : SimilarityModel(new SVMModel()) 
{ }

SimilarityModel::SimilarityModel(Model *model) : model(model)
{
}

SimilarityModel::~SimilarityModel() {
}

float SimilarityModel::evaluate(float *features)
{
    return model->evaluate(features);
}

float SimilarityModel::evaluate(const Song &s1, const Song &s2)
{
    MixtureModel mm1, mm2;
    float b1[BEATSSIZE], b2[BEATSSIZE];

    if (!s1.get_acoustic(&mm1, b1))
        return 0;
    if (!s2.get_acoustic(&mm2, b2))
        return 0;

    return evaluate(mm1, b1, mm2, b2);
}

float SimilarityModel::evaluate(const MixtureModel &mm1, float *beats1,
        const MixtureModel &mm2, float *beats2) {
    vector<float> features;
    extract_features(mm1, beats1, mm2, beats2, &features);
    float feat_array[NUM_FEATURES];
    std::copy(features.begin(), features.end(), feat_array);
    return evaluate(feat_array);
}

static float find_max(float a[BEATSSIZE])
{
    return *std::max_element(a, a + BEATSSIZE);
}

static float find_min(float a[BEATSSIZE])
{
    return *std::min_element(a, a + BEATSSIZE);
}

static void add_partitions(const MixtureModel &mm, vector<float> *f)
{
    static const int num_partitions = 3;
    float sums[num_partitions];
    for (int i = 0; i < num_partitions; ++i)
        sums[i] = 0;
    for (int i = 0; i < NUMGAUSS; ++i)
    {
        const Gaussian &g = mm.gauss[i];
        for (int j = 0; j < NUMCEPSTR; ++j)
            sums[j / (NUMCEPSTR / num_partitions)] += g.weight * g.means[j];
    }
    for (int i = 0; i < num_partitions; ++i)
        f->push_back(sums[i]);
} 

void SimilarityModel::extract_features(
        const MixtureModel &mm1, float *beats1,
        const MixtureModel &mm2, float *beats2,
        vector<float> *f)
{
    f->push_back(EMD::raw_distance(mm1, mm2));
    f->push_back(EMD::raw_distance(beats1, beats2));

    add_partitions(mm1, f);
    add_partitions(mm2, f);

    f->push_back(find_max(beats1));
    f->push_back(find_max(beats2));

    f->push_back(find_min(beats1));
    f->push_back(find_min(beats2));
}
