#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
#include <fftw3.h>
#include <stdint.h>
#include <math.h>

#include <utils.h>
#include <song.h>
#include <immsdb.h>

#include "spectrum.h"

#define SCALE       100000.0

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostringstream;

typedef uint16_t sample_t;

int usage()
{
    cout << "usage: analyzer <filename>" << endl;
    return -1;
}

float infftdata[WINDOWSIZE];
fftwf_complex outfftdata[NFREQS];
fftwf_plan plan;

int analyze(const string &path)
{
    if (access(path.c_str(), R_OK))
    {
        cerr << "analyzer: Could not open file " << path << endl;
        return -4;
    }

    ostringstream command;
    command << "sox \"" << path << "\" -t .raw -w -u -c 1 -r "
        << SAMPLERATE << " -";
    cout << "analyzer: Executing: " << command.str() << endl;
    FILE *p = popen(command.str().c_str(), "r");

    if (!p)
    {
        cerr << "analyzer: Could not open pipe!" << endl;
        return -2;
    }

    sample_t indata[WINDOWSIZE];
    float outdata[NFREQS];

    float maxes[NFREQS];
    memset(maxes, 0, sizeof(maxes));
    
    StackTimer t;

    int r = fread(indata, sizeof(sample_t), OVERLAP, p);

    if (r != OVERLAP)
        return -3;

    try {
        SpectrumAnalyzer analyzer(path);

        if (analyzer.is_known())
        {
            cout << "analyzer: Already analyzed - skipping." << endl;
            return 1;
        }

        while (fread(indata + OVERLAP, sizeof(sample_t), READSIZE, p)
                == READSIZE)
        {
            for (int i = 0; i < WINDOWSIZE; ++i)
                infftdata[i] = (float)indata[i];

            fftwf_execute(plan);

            for (int i = 0; i < NFREQS; ++i)
            {
                outdata[i] = sqrt(pow(outfftdata[i][0] / SCALE, 2)
                        + pow(outfftdata[i][1] / SCALE, 2));
                if (outdata[i] > maxes[i])
                    maxes[i] = outdata[i];
            }

            analyzer.integrate_spectrum(outdata);

            memmove(indata, indata + READSIZE, OVERLAP * sizeof(sample_t));
        }

    }
    catch (std::string &s) { cerr << s << endl; }

    pclose(p);

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

    ImmsDb immsdb;

    plan = fftwf_plan_dft_r2c_1d(WINDOWSIZE, infftdata, outfftdata, 0);

    for (int i = 1; i < argc; ++i)
        analyze(argv[i]);

    fftwf_destroy_plan(plan);
}
