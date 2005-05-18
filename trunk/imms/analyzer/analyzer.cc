#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <immsutil.h>
#include <appname.h>

#include "spectrum.h"
#include "strmanip.h"
#include "melfilter.h"
#include "fftprovider.h"
#include "mfcckeeper.h"
#include "hanning.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostringstream;

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

    StackTimer t;

    size_t frames = 0;

    sample_t indata[WINDOWSIZE];
    vector<double> outdata(NUMFREQS);

    MFCCKeeper mfcckeeper;

    int r = fread(indata, sizeof(sample_t), OVERLAP, p);

    if (r != OVERLAP)
        return -3;

    while (fread(indata + OVERLAP, sizeof(sample_t), READSIZE, p) == READSIZE)
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

        // compute log energy
        for (int i = 0; i < NUMMEL; ++i)
            melfreqs[i] = log(melfreqs[i]);

        // another fft to get the MFCCs
        specfft.apply(melfreqs);

        float cepstrum[NUMCEPSTR];
        for (int i = 0; i < NUMCEPSTR; ++i)
            cepstrum[i] = specfft.output()[i][0];

        mfcckeeper.process(cepstrum);

        // finally shift the already read data
        memmove(indata, indata + READSIZE, OVERLAP * sizeof(sample_t));

        ++frames;
    }

    pclose(p);

#ifdef DEBUG
    cerr << "obtained " << frames << " frames" << endl;
#endif

    mfcckeeper.finalize();
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
        cerr << "analyzer: Another instance already active - exiting." << endl;
        return -7;
    }

    // clean up after XMMS
    for (int i = 3; i < 255; ++i)
        close(i);

    nice(15);

    Analyzer analyzer;

    for (int i = 1; i < argc; ++i)
    {
        if (analyzer.analyze(path_normalize(argv[i])))
            cerr << "analyzer: Could not process " << argv[i] << endl;
    }
}
