#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>

#include <iostream>

#include <torch/Random.h>
#include <torch/SVMRegression.h>
#include <torch/QCTrainer.h>
#include <torch/MSEMeasurer.h>
#include <torch/MemoryXFile.h>
#include <torch/DiskXFile.h>
#include <torch/KFold.h>
#include <torch/Kernel.h>
#include <torch/MeanVarNorm.h>

#include <immsutil.h>

#include "model.h"

using namespace Torch;

using std::map;
using std::cerr;
using std::endl;

Model::Model(const std::string &name) : name(name)
{
    Random::seed();
}

void Model::prepare_data(const DataMap &data)
{
    size = data.size();

    inputs = new (Sequence*)[size];
    outputs = new (Sequence*)[size];

    int index = 0;
    for (DataMap::const_iterator i = data.begin();
            i != data.end(); ++i, ++index)
    {
        int frame_size = i->first.size();
        inputs[index] = new Sequence(1, frame_size);
        for (int j = 0; j < frame_size; ++j)
            inputs[index]->frames[0][j] = i->first[j];
        outputs[index] = new Sequence(1, 1);
        outputs[index]->frames[0][0] = i->second;
    }

    dataset.setInputs(inputs, size);
    dataset.setTargets(outputs, size);

    MeanVarNorm norm(&dataset);
    for (unsigned i = 0; i < size; ++i)
        norm.preProcessInputs(*(inputs + i));
}

void Model::cleanup()
{
    for (unsigned i = 0; i < size; ++i)
    {
        delete inputs[i];
        delete outputs[i];
    }

    delete[] inputs;
    delete[] outputs;
}

void Model::test(const DataMap &data)
{
    prepare_data(data);

    GaussianKernel kernel(1./100.);
    SVMRegression svm(&kernel);
    svm.setROption("C", 100);
    svm.setROption("eps regression", 0.7);

    DiskXFile modelfile(get_imms_root("models/" + name).c_str(), "r");
    svm.loadXFile(&modelfile);

    DiskXFile xfile(get_imms_root("error").c_str(), "w");
    MeasurerList measurers;
    MSEMeasurer msemes(svm.outputs, &dataset, &xfile);
    measurers.addNode(&msemes);

    QCTrainer trainer(&svm);
    trainer.test(&measurers);

    cleanup();
}

void Model::train(const DataMap &data)
{
    prepare_data(data);

    GaussianKernel kernel(1./100.);
    SVMRegression svm(&kernel);
    svm.setROption("C", 100);
    svm.setROption("eps regression", 0.7);

    DiskXFile xfile(get_imms_root("error").c_str(), "w");
    MeasurerList measurers;
    MSEMeasurer msemes(svm.outputs, &dataset, &xfile);
    measurers.addNode(&msemes);

    QCTrainer trainer(&svm);

#if 1
    trainer.setROption("end accuracy", 0.01);
    trainer.setIOption("iter shrink", 100);

    trainer.train(&dataset, NULL);
    message("%d SV with %d at bounds", svm.n_support_vectors,
            svm.n_support_vectors_bound);
#else
    KFold kfold(&trainer, 3);
    kfold.crossValidate(&dataset, NULL, &measurers);
#endif

    mkdir(get_imms_root("models").c_str(), 0755);
    DiskXFile modelfile(get_imms_root("models/" + name).c_str(), "w");
    svm.saveXFile(&modelfile);

    cleanup();
}
