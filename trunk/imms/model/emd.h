#ifndef _EMD_H
#define _EMD_H
/*
   emd.h

   Last update: 3/24/98

   An implementation of the Earth Movers Distance.
   Based of the solution for the Transportation problem as described in
   "Introduction to Mathematical Programming" by F. S. Hillier and 
   G. J. Lieberman, McGraw-Hill, 1990.

   Copyright (C) 1998 Yossi Rubner
   Computer Science Department, Stanford University
   E-Mail: rubner@cs.stanford.edu   URL: http://vision.stanford.edu/~rubner

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

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINITIONS */
#define MAX_SIG_SIZE   300
#define MAX_ITERATIONS 500
#define EPSILON        1e-6
#define EMDINF         1e20

typedef int feature_t;

typedef struct
{
  int n;                /* Number of features in the signature */
  feature_t *Features;  /* Pointer to the features vector */
  float *Weights;       /* Pointer to the weights of the features */
} signature_t;

typedef struct
{
  int from;             /* Feature number in signature 1 */
  int to;               /* Feature number in signature 2 */
  float amount;         /* Amount of flow from "from" to "to" */
} flow_t;


float emd(signature_t *Signature1, signature_t *Signature2,
	  float (*func)(feature_t *, feature_t *),
	  flow_t *Flow, int *FlowSize);

#ifdef __cplusplus
}
#endif

#endif
