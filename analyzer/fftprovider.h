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
#ifndef __FFTPROVIDER_H
#define __FFTPROVIDER_H

#include <memory>
#include <fftw3.h>

// Import and export FFTW Wisdom files. See FFTW docs at www.fftw.org.
//
// Wisdom is automatically used when you load or generate a Wisdom file
// (see "wisdom .. is stored in a global, private data structure" in the docs).
// Thus analyzer doesn't have to do anything to benefit from wisdom
// other than create an instance of FFTWisdom object, 
// which will import/generate wisdom as appropriate.
//
// See Also:
// http://www.fftw.org/doc/Introduction.html
// http://www.fftw.org/doc/Words-of-Wisdom_002dSaving-Plans.html#Words-of-Wisdom_002dSaving-Plans
class FFTWisdom
{
public:
    FFTWisdom();
    ~FFTWisdom();
private:
    bool shouldexport;
};

// Basic hooks for working with FFTW library (www.fftw.org).
template <int input_size>
class FFTProvider {
public:
    FFTProvider()
    {
        plan = fftw_plan_dft_r2c_1d(input_size, indata, outdata,
                FFTW_MEASURE | FFTW_PATIENT | FFTW_EXHAUSTIVE);
    }
    ~FFTProvider() { fftw_destroy_plan(plan); }
    void execute() { fftw_execute(plan); }
    void apply(const vector<double> &input)
    {
        for (int i = 0; i < input_size; ++i)
            indata[i] = input[i];
        execute();
    }
    double *input() { return indata; }
    fftw_complex *output() { return outdata; }
protected:
    double indata[input_size];
    fftw_complex outdata[input_size / 2 + 1];
    fftw_plan plan;
};


#endif  // __FFTPROVIDER_H
