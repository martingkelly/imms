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

#include "spectrum.h"

#define SCALE       100000.0

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;
using std::vector;

typedef uint16_t sample_t;

int main(int argc, char *argv[])
{
    if (argc != 2 || access(argv[1], R_OK))
    {
        cout << "usage: analyzer <filename>" << endl;
        return -1;
    }

    ostringstream command;
    command << "sox \"" <<  argv[1] << "\" -t .raw -w -u -c 1 "
        "-r " << SAMPLERATE << " -";
    cout << "running " << command.str() << endl;
    FILE *p = popen(command.str().c_str(), "r");

    if (!p)
    {
        cout << "could not open pipe!" << endl;
        return -2;
    }

    sample_t indata[WINDOWSIZE];
    fftwf_complex outfftdata[NFREQS];
    float infftdata[WINDOWSIZE];
    float outdata[NFREQS];
    int counter = 0;

    float maxes[NFREQS];
    memset(maxes, 0, sizeof(maxes));
    
    StackTimer t;

    int r = fread(indata, sizeof(sample_t), OVERLAP, p);

    if (r != OVERLAP)
        return -3;

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(WINDOWSIZE,
            infftdata, outfftdata, 0);

    SpectrumAnalyzer analyzer;

    while (fread(indata + OVERLAP, sizeof(sample_t), READSIZE, p) == READSIZE)
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
        counter++;
    }

    pclose(p);

    fftwf_destroy_plan(plan);

    analyzer.finalize();

    cout << "processed " << counter << " windows" << endl;
}
