#include <torch/KMeans.h>
#include <torch/Random.h>
#include <torch/EMTrainer.h>
#include <torch/NLLMeasurer.h>
#include <torch/MemoryXFile.h>

#include <sqlite++.h>

#include "mfcckeeper.h"

#define NUMITER     100
#define ENDACCUR    0.0001

using namespace Torch;

Gaussian::Gaussian(float weight, float *means_, float *vars_) : weight(weight)
{
    memcpy(means, means_, sizeof(float) * NUMCEPSTR);
    memcpy(vars, vars_, sizeof(float) * NUMCEPSTR);
}

void MixtureModel::init(DiagonalGMM &gmm)
{
    for(int i = 0; i < NUMGAUSS; i++)
        gauss[i] = Gaussian(exp(gmm.log_weights[i]),
                gmm.means[i], gmm.var[i]);
}

SQLQuery &operator<<(SQLQuery &q, const MixtureModel &a)
{
    q.bind(&a.gauss, sizeof(a.gauss));
    return q;
}

MFCCKeeper::MFCCKeeper() : cepseq(0, NUMCEPSTR), cepseq_p(&cepseq)
{
    cepdat.setInputs(&cepseq_p, 1);
    Random::seed();
}

void MFCCKeeper::process(float *cepstrum)
{
    cepseq.addFrame(cepstrum, true);
}

void MFCCKeeper::finalize()
{
    KMeans kmeans(cepdat.n_inputs, NUMGAUSS);
    kmeans.setROption("prior weights", 0.001);

    EMTrainer kmeans_trainer(&kmeans);
    kmeans_trainer.setIOption("max iter", NUMITER);
    kmeans_trainer.setROption("end accuracy", ENDACCUR);

    MeasurerList kmeans_measurers;
    MemoryXFile memfile1;
    NLLMeasurer nll_kmeans_measurer(kmeans.log_probabilities,
            &cepdat, &memfile1);
    kmeans_measurers.addNode(&nll_kmeans_measurer);

    DiagonalGMM gmm(cepdat.n_inputs, NUMGAUSS, &kmeans_trainer);
    gmm.setROption("prior weights", 0.001);
    gmm.setOOption("initial kmeans trainer measurers", &kmeans_measurers);

    // Measurers on the training dataset
    MeasurerList measurers;
    MemoryXFile memfile2;
    NLLMeasurer nll_meas(gmm.log_probabilities, &cepdat, &memfile2);
    measurers.addNode(&nll_meas);

    EMTrainer trainer(&gmm);
    trainer.setIOption("max iter", NUMITER);
    trainer.setROption("end accuracy", ENDACCUR);

    trainer.train(&cepdat, &measurers);

    result = gmm;

#ifdef DEBUG
    gmm.display();
#endif
}

void *MFCCKeeper::get_result()
{
    return result.gauss;
}
