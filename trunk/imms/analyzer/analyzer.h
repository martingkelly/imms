#ifndef __ANALYZER_H
#define __ANALYZER_H

#define WINDOWSIZE      512
#define OVERLAP         256
#define READSIZE        (WINDOWSIZE - OVERLAP)

#define SAMPLERATE      22050
#define MAXFRAMES       ((SAMPLERATE*60*4)/READSIZE)

#define WINPERSEC       (SAMPLERATE / (WINDOWSIZE - OVERLAP)) 
#define NUMFREQS        (WINDOWSIZE / 2 + 1)
#define MAXFREQ         (SAMPLERATE / 2)
#define FREQDELTA       ROUND(MAXFREQ / (float)NUMFREQS)
#define MINFREQ         FREQDELTA

#endif
