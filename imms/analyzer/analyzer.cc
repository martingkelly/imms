#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <string.h>
#include <fftw3.h>
#include <stdint.h>
#include <math.h>

#include <utils.h>

#include "spectrum.h"

#define WINDOW_SIZE 512
#define OVERLAP     292
#define READSIZE   (WINDOW_SIZE - OVERLAP)

using std::cout;
using std::endl;
using std::string;

typedef uint16_t sample_t;

int main(int argc, char *argv[])
{
    if (argc != 2 || access(argv[1], R_OK))
    {
        cout << "usage: analyzer <filename>" << endl;
        return -1;
    }

    string command = "mpg123 -s -r 22050 --mono ";
    command += '"' + string(argv[1]) + '"';
    cout << "running " << command.c_str() << endl;
    FILE *p = popen(command.c_str(), "r");

    if (!p)
    {
        cout << "could not open pipe!" << endl;
        return -2;
    }

    const int nfreqs = WINDOW_SIZE / 2 + 1;

    sample_t indata[WINDOW_SIZE];
    fftwf_complex outfftdata[nfreqs];
    float *infftdata = (float*)outfftdata;
    float outdata[nfreqs];
    int counter = 0;

    float maxes[nfreqs];
    memset(maxes, 0, sizeof(maxes));
    
    StackTimer t;

    int r = fread(indata, sizeof(sample_t), OVERLAP, p);

    if (r < OVERLAP)
        return -3;

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(WINDOW_SIZE,
            infftdata, outfftdata, 0);

    BeatKeeper bpm("bpm");

    while (fread(indata + OVERLAP, sizeof(sample_t), READSIZE, p) == READSIZE)
    {
        for (int i = 0; i < WINDOW_SIZE; ++i)
            infftdata[i] = indata[i];

        fftwf_execute(plan);

        for (int i = 0; i < nfreqs; ++i)
        {
            outdata[i] = log(pow(outfftdata[i][0], 2)
                    + pow(outfftdata[i][1], 2));
            if (outdata[i] > maxes[i])
                maxes[i] = outdata[i];
        }
    
        float power;
        for (int i = 0; i < 5; ++i)
            power += outdata[i];

        bpm.integrate_beat(power);

        memmove(indata, indata + READSIZE, OVERLAP);
        counter++;
    }

    bpm.guess_actual_bpm();

    fftwf_destroy_plan(plan);

    cout << "processed " << counter << " windows" << endl;

    for (int i = 0; i < nfreqs; ++i)
        cout << maxes[i] << endl;
}
