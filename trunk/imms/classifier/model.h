#ifndef __MODEL_H
#define __MODEL_H

#include <map>
#include <vector>
#include <string>
#include <torch/MemoryDataSet.h>

typedef std::map< std::vector<float> , int > DataMap;

class Model
{
public:
    Model(const std::string &name);
    void train(const DataMap &data);
    void test(const DataMap &data);
private:

    void prepare_data(const DataMap &data);
    void cleanup();

    std::string name;

    size_t size;
    Torch::MemoryDataSet dataset;
    Torch::Sequence **inputs, **outputs;
};

#endif
