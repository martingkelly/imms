#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <torch/Sequence.h>
#include <torch/MemoryDataSet.h>
#include <torch/KMeans.h>
#include <torch/Random.h>
#include <torch/EMTrainer.h>
#include <torch/DiagonalGMM.h>
#include <torch/NLLMeasurer.h>
#include <torch/MemoryXFile.h>

#include <immsutil.h>
#include <song.h>
#include <immsdb.h>
#include <appname.h>

#include "spectrum.h"
#include "strmanip.h"
#include "melfilter.h"
#include "fftprovider.h"

#define TEST

#define NUMITER     100
#define ENDACCUR    0.0001

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostringstream;
using std::fstream;

using namespace Torch;

const string AppName = ANALYZER_APP;

typedef uint16_t sample_t;

class HanningWindow
{
public:
    HanningWindow(unsigned size)
    {
        weights.resize(size);

        static const float alpha = 0.46;
        for (unsigned i = 0; i < size; ++i)
            weights[i] = (1 - alpha) -
                (alpha * cos((2 * M_PI * i)/(size - 1)));
    }
    void apply(double *data, unsigned size)
    {
        assert(size == weights.size());
        for (unsigned i = 0; i < size; ++i)
            data[i] *= weights[i];
    }
private:
    vector<double> weights;
};

struct Gaussian
{
    Gaussian() {};
    Gaussian(float weight, float *means, float *vars) : weight(weight)
    {
        memcpy(data[0], means, sizeof(float) * NUMCEPSTR);
        memcpy(data[1], vars, sizeof(float) * NUMCEPSTR);
    }
    float weight;
    float data[2][NUMCEPSTR];
};

struct GMM
{
    GMM(DiagonalGMM &gmm)
    {
        for(int i = 0; i < NUMGAUSS; i++)
            gauss[i] = Gaussian(exp(gmm.log_weights[i]),
                    gmm.means[i], gmm.var[i]);
    }
    Gaussian gauss[NUMGAUSS];
};

class AnalyzerShared
{
public:
    AnalyzerShared() : hanwin(WINDOWSIZE)
        { Random::seed(); }
    FFTWisdom wisdom;
    FFTProvider<WINDOWSIZE> pcmfft;
    FFTProvider<NUMMEL> specfft;
    MelFilterBank mfbank;
    HanningWindow hanwin;
};

int usage()
{
    cout << "usage: analyzer <filename>" << endl;
    return -1;
}

int analyze(const string &path, AnalyzerShared &shared)
{
    if (access(path.c_str(), R_OK))
    {
        cerr << "analyzer: Could not open file " << path << endl;
        return -4;
    }

    string epath = rex.replace(path, "'", "'\"'\"'", Regexx::global);
    ostringstream command;
    command << "nice -n 15 sox \'" << epath << "\' -t .raw -w -u -c 1 -r "
        << SAMPLERATE << " -";
#ifdef DEBUG
    cout << "analyzer: Executing: " << command.str() << endl;
#endif
    FILE *p = popen(command.str().c_str(), "r");

    if (!p)
    {
        cerr << "analyzer: Could not open pipe!" << endl;
        return -2;
    }

    sample_t indata[WINDOWSIZE];
    vector<double> outdata(NUMFREQS);

#ifdef DEBUG
    StackTimer t;
#endif

    int r = fread(indata, sizeof(sample_t), OVERLAP, p);

    if (r != OVERLAP)
        return -3;

    Sequence cepseq(0, NUMCEPSTR);
    MemoryDataSet cepdat;
    Sequence *cepseq_p = &cepseq;
    cepdat.setInputs(&cepseq_p, 1);

    try {
#ifndef TEST
        SpectrumAnalyzer analyzer(path);

        if (analyzer.is_known())
        {
            cout << "analyzer: Already analyzed - skipping." << endl;
            return 1;
        }
#endif

        while (fread(indata + OVERLAP, sizeof(sample_t), READSIZE, p)
                == READSIZE)
        {
            for (int i = 0; i < WINDOWSIZE; ++i)
                shared.pcmfft.input()[i] = (double)indata[i];

            // window the data
            shared.hanwin.apply(shared.pcmfft.input(), WINDOWSIZE);

            // fft to get the spectrum
            shared.pcmfft.execute();

            // calculate the power spectrum
            for (int i = 0; i < NUMFREQS; ++i)
                outdata[i] = pow(shared.pcmfft.output()[i][0], 2) +
                    pow(shared.pcmfft.output()[i][1], 2);

#ifndef TEST
            analyzer.integrate_spectrum(outdata);
#endif

            // apply mel filter bank
            vector<double> melfreqs;
            shared.mfbank.apply(outdata, melfreqs);

            // compute log energy
            for (int i = 0; i < NUMMEL; ++i)
                melfreqs[i] = log(melfreqs[i]);

            // another fft to get the MFCCs
            shared.specfft.apply(melfreqs);

            float cepstrum[NUMCEPSTR];
            for (int i = 0; i < NUMCEPSTR; ++i)
                cepstrum[i] = shared.specfft.output()[i][0];

            cepseq.addFrame(cepstrum, true);

            // finally shift the already read data
            memmove(indata, indata + READSIZE, OVERLAP * sizeof(sample_t));
        }

    }
    catch (std::string &s) { cerr << s << endl; }

    pclose(p);

#ifdef TEST
    cerr << "obtained " << cepseq.n_frames << " frames" << endl;
#endif

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
    gmm.display();

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return usage();

    StackLockFile lock(get_imms_root() + ".analyzer_lock");
    if (!lock.isok())
    {
        cerr << "analyzer: Another instance already active - exiting." << endl;
        return -7;
    }

    // clean up after XMMS
    for (int i = 3; i < 255; ++i)
        close(i);

    nice(15);

    AnalyzerShared shared;

    ImmsDb immsdb;
    for (int i = 1; i < argc; ++i)
        analyze(path_normalize(argv[i]), shared);
}
