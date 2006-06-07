#include "torch/Linear.h"
#include "torch/Tanh.h"
#include "torch/LogSoftMax.h"

#include "torch/DiskXFile.h"

#include "model.h"
#include "immsutil.h"

using namespace Torch;

SimilarityModel::SimilarityModel() {
    const int num_inputs = NUM_FEATURES;
    const int num_hidden = NUM_FEATURES * 2;
    const int num_outputs = 2;
    Linear *c1 = new Linear(num_inputs, num_hidden);
    Tanh *c2 = new Tanh(num_hidden);
    Linear *c3 = new Linear(num_hidden, num_outputs);

    mlp.addFCL(c1);
    mlp.addFCL(c2);
    mlp.addFCL(c3);

    LogSoftMax *c4 = new LogSoftMax(num_outputs);
    mlp.addFCL(c4);

    mlp.build();
    mlp.setPartialBackprop();

    DiskXFile::setLittleEndianMode();
    DiskXFile model_file(get_imms_root("model").c_str(), "r");
    mlp.loadXFile(&model_file);
}
