#ifndef __MODEL_H
#define __MODEL_H

#include <map>
#include <string>

class Model
{
public:
    Model(const std::string &name);
    float train(const std::map<int, int> &data);
private:
    std::string name;
};

#endif
