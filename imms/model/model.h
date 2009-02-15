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
#ifndef __MODEL_H
#define __MODEL_H

#include <memory>
#include <vector>

#define NUM_FEATURES 12

class Song;
class MixtureModel;

class Model
{
public:
    virtual ~Model() {};
    virtual float evaluate(float *features) = 0;
};

class DummyModel : public Model
{
public:
    float evaluate(float *features) { return 0; }
};

class SimilarityModel
{
public:
    SimilarityModel(Model *model);
    ~SimilarityModel();
    float evaluate(const Song &s1, const Song &s2);
    float evaluate(const MixtureModel &mm1, float *beats1,
                   const MixtureModel &mm2, float *beats2);

    float evaluate(float *features);

    static void extract_features(
            const MixtureModel &mm1, float *beats1,
            const MixtureModel &mm2, float *beats2,
            std::vector<float> *features);
private:
    std::auto_ptr<Model> model;
};

class SVMSimilarityModel : public SimilarityModel {
public:
    SVMSimilarityModel();
};

#endif
