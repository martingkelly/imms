#ifndef __MODEL_H
#define __MODEL_H

#include <map>
#include <vector>
#include <string>

typedef std::map< std::vector<float> , int > DataMap;

class Model
{
public:
    Model(const std::string &name);
    float train(const DataMap &data);
private:
    std::string name;
};

#endif
