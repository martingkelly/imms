#include <errno.h>
#include <iostream>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>

#include <immsutil.h>
#include <appname.h>
#include <song.h>
#include <immsdb.h>

#include "analyzer.h"
#include "strmanip.h"
#include "melfilter.h"
#include "fftprovider.h"
#include "soxprovider.h"
#include "mfcckeeper.h"
#include "beatkeeper.h"
#include "hanning.h"

using std::cout;
using std::cerr;
using std::endl;

typedef uint16_t sample_t;

const string AppName = ANALYZER_APP;

class Analyzer
{
public:
    Analyzer() : hanwin(WINDOWSIZE) { }
    int analyze(const string &path);
protected:
    FFTWisdom wisdom;
    FFTProvider<WINDOWSIZE> pcmfft;
    FFTProvider<NUMMEL> specfft;
    MelFilterBank mfbank;
    HanningWindow hanwin;
};

int Analyzer::analyze(const string &path)
{
    static const bool test_mode = 0;
    
    if (access(path.c_str(), R_OK))
    {
        LOG(ERROR) << "Could not open file " << path << endl;
        return -2;
    }

    Song song(path);
    if (!song.isok())
    {
        LOG(ERROR) << "Could not identify file " << path << endl;
        return -3;
    }

    if (!test_mode && song.isanalyzed())
    {
        LOG(ERROR) << path << endl;
        LOG(ERROR) << "File already analyzed. Skipping." << endl;
        return 0;
    }

    FILE *p = run_sox(path, SAMPLERATE);

    if (!p)
    {
        LOG(ERROR) << "Could not open pipe!" << endl;
        return -4;
    }

    StackTimer t;

    size_t frames = 0;

    sample_t indata[WINDOWSIZE];
    vector<double> outdata(NUMFREQS);

    MFCCKeeper mfcckeeper;
    BeatManager beatkeeper;

    int r = fread(indata, sizeof(sample_t), OVERLAP, p);

    if (r != OVERLAP)
        return -5;

    while (fread(indata + OVERLAP, sizeof(sample_t), READSIZE, p)
            == READSIZE && ++frames < MAXFRAMES)
    {
        for (int i = 0; i < WINDOWSIZE; ++i)
            pcmfft.input()[i] = (double)indata[i];

        // window the data
        hanwin.apply(pcmfft.input(), WINDOWSIZE);

        // fft to get the spectrum
        pcmfft.execute();

        // calculate the power spectrum
        for (int i = 0; i < NUMFREQS; ++i)
            outdata[i] = pow(pcmfft.output()[i][0], 2) +
                pow(pcmfft.output()[i][1], 2);

        // apply mel filter bank
        vector<double> melfreqs;
        mfbank.apply(outdata, melfreqs);

        beatkeeper.process(melfreqs);

        // compute log energy
        for (int i = 0; i < NUMMEL; ++i)
            melfreqs[i] = log(melfreqs[i]);

        // another fft to get the MFCCs
        specfft.apply(melfreqs);

        // discard the first mfcc
        float cepstrum[NUMCEPSTR];
        for (int i = 1; i <= NUMCEPSTR; ++i)
            cepstrum[i - 1] = specfft.output()[i][0];

        mfcckeeper.process(cepstrum);

        // finally shift the already read data
        memmove(indata, indata + READSIZE, OVERLAP * sizeof(sample_t));
    }

    r = pclose(p);
    if (r == -1)
        LOG(ERROR) << "pclose failed: " << strerror(errno) << endl;
    if (r > 0)
        LOG(INFO) << "sox process returned " << r << endl;

#ifdef DEBUG
    cerr << "obtained " << frames << " frames" << endl;
#endif

    // did we read enough data?
    if (test_mode || frames < 100)
        return 0;

    mfcckeeper.finalize();
    beatkeeper.finalize();

    song.set_acoustic(mfcckeeper.get_result(), beatkeeper.get_result());
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "usage: analyzer <filename> [<filename>] ..." << endl;
        return -1;
    }

    StackLockFile lock(get_imms_root() + ".analyzer_lock");
    if (!lock.isok())
    {
        LOG(ERROR) << "Another instance already active - exiting." << endl;
        return -7;
    }

    // clean up after XMMS
    for (int i = 3; i < 255; ++i)
        close(i);

    nice(15);

    ImmsDb immsdb;
    Analyzer analyzer;

    for (int i = 1; i < argc; ++i)
    {
        if (analyzer.analyze(path_normalize(argv[i])))
            LOG(ERROR) << "Could not process " << argv[i] << endl;
    }
}
