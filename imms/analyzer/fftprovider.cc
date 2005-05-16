#include <iostream>

#include <immsutil.h>

#include "fftprovider.h"

using std::cerr;
using std::endl;

FFTWisdom::FFTWisdom() : shouldexport(true)
{
    FILE *wisdom = fopen(get_imms_root(".fftw_wisdom").c_str(), "r");
    if (wisdom)
    {
        shouldexport = !fftw_import_wisdom_from_file(wisdom);
        fclose(wisdom);
    }
    else
        cerr << "analyzer: Growing wiser. This may take a while." << endl;
}


FFTWisdom::~FFTWisdom()
{
    if (!shouldexport)
        return;

    FILE *wisdom = fopen(get_imms_root(".fftw_wisdom").c_str(), "w");
    if (wisdom)
    {
        fftw_export_wisdom_to_file(wisdom);
        fclose(wisdom);
    }
    else
        cerr << "analyzer: Could not write to wisdom file!" << endl;
}
