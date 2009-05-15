#include "torch/ConnectedMachine.h"
#include "torch/DiskXFile.h"
#include "torch/Linear.h"
#include "torch/LogSoftMax.h"
#include "torch/MeanVarNorm.h"
#include "torch/MemoryDataSet.h"
#include "torch/OneHotClassFormat.h"
#include "torch/Tanh.h"

#include <iostream>
#include <vector>
#include <algorithm>

#include "model.h"
#include "song.h"
#include "immsutil.h"
#include "distance.h"

using namespace Torch;

using std::endl;
using std::cout;
using std::cerr;
using std::vector;

static const int num_inputs = NUM_FEATURES;
static const int num_hidden = 25;
static const int num_outputs = 2;

class XFileModeSetter {
public:
    XFileModeSetter() { DiskXFile::setLittleEndianMode(); }
};

static XFileModeSetter xfilemodesetter;

class MyMemoryDataSet : public MemoryDataSet {
public:
    MyMemoryDataSet(int xn_inputs) {
        n_inputs = xn_inputs;
    }
};

class MLPModel : public Model {
public:
    MLPModel(const string &filename)
        : class_format(num_outputs), fake(num_inputs), normalizer(&fake) 
    {
        build_model();

        DiskXFile file(filename.c_str(), "r");
        normalizer.loadXFile(&file);
        mlp.loadXFile(&file);
        LOG(ERROR) << "model loading complete" << endl;
    }

    void build_model() {
        Linear *c1 = new(&alloc) Linear(num_inputs, num_hidden);
        Tanh *c2 = new(&alloc) Tanh(num_hidden);
        Linear *c3 = new(&alloc) Linear(num_hidden, num_outputs);

        mlp.addFCL(c1);
        mlp.addFCL(c2);
        mlp.addFCL(c3);

        LogSoftMax *c4 = new(&alloc) LogSoftMax(num_outputs);
        mlp.addFCL(c4);

        mlp.build();
        mlp.setPartialBackprop();
    }

    float evaluate(float *features) {
        // Garbage, I tell you!!
        Sequence feat_seq(0, NUM_FEATURES);
        feat_seq.addFrame(features);

        normalizer.preProcessInputs(&feat_seq);

        mlp.forward(&feat_seq);
        return (mlp.outputs->frames[0][1] - mlp.outputs->frames[0][0]) / 5;
    }
     
private:
    Allocator alloc;
    OneHotClassFormat class_format;
    ConnectedMachine mlp;
    MyMemoryDataSet fake;
    MeanVarNorm normalizer;
};

MLPSimilarityModel::MLPSimilarityModel()
    : SimilarityModel(new MLPModel(get_imms_root("model"))) 
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
