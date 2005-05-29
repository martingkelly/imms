#ifndef __KERNEL_H
#define __KERNEL_H

#include <torch/Kernel.h>
#include "distance.h"

using namespace Torch;

class EMDKernel : public Kernel
{
public:
    virtual real eval(Sequence *x, Sequence *y);
}; 

#endif  // __KERNEL_H
