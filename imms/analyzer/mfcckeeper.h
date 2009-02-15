/*
  IMMS: Intelligent Multimedia Management System
  Copyright (C) 2001-2009 Michael Grigoriev

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef __MFCCKEEPER_H
#define __MFCCKEEPER_H

#define NUMCEPSTR   15
#define NUMGAUSS    5
#define NUMFEATURES (NUMCEPSTR*3)

#include <memory>

namespace Torch {
    class DiagonalGMM;
}

struct Gaussian
{
    static const int NumDimensions = NUMCEPSTR * 3;
    Gaussian() {};
    Gaussian(float weight, float *means, float *vars);
    float weight;
    float means[NumDimensions];
    float vars[NumDimensions];
};

struct MixtureModel
{
    MixtureModel() {}
    MixtureModel(Torch::DiagonalGMM &gmm)
        { init(gmm); }
    MixtureModel &operator =(Torch::DiagonalGMM &gmm)
        { init(gmm); return *this; }
    Gaussian gauss[NUMGAUSS];
private:
    void init(Torch::DiagonalGMM &gmm);
};

struct MFCCKeeperPrivate;

class MFCCKeeper
{
public:
    MFCCKeeper();
    ~MFCCKeeper();
    void process(float *capstrum);
    void finalize();
    const MixtureModel &get_result();

    static const int ResultSize = sizeof(Gaussian) * NUMGAUSS;
protected:
    std::auto_ptr<MFCCKeeperPrivate> impl;
    int sample_number;
    float last_frame[NUMCEPSTR], last_delta[NUMCEPSTR];
    MixtureModel result;
};

#endif
