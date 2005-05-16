/*
  Levenshtein.c v2003-05-10
  Python extension computing Levenshtein distances, string similarities,
  median strings and other goodies.

  Copyright (C) 2002-2003 David Necas (Yeti) <yeti@physics.muni.cz>.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "levenshtein.h"

/****************************************************************************
 *
 * Basic stuff, Levenshtein distance
 *
 ****************************************************************************/

#define EPSILON 1e-14

/*
 * Levenshtein distance between string1 and string2.
 *
 * Replace cost is normally 1, and 2 with nonzero xcost.
 */
LEV_STATIC_PY size_t
lev_edit_distance(size_t len1, const lev_byte *string1,
                  size_t len2, const lev_byte *string2,
                  size_t xcost)
{
  size_t i;
  size_t *row;  /* we only need to keep one row of costs */
  size_t *end;
  size_t half;

  /* strip common prefix */
  while (len1 > 0 && len2 > 0 && *string1 == *string2) {
    len1--;
    len2--;
    string1++;
    string2++;
  }

  /* strip common suffix */
  while (len1 > 0 && len2 > 0 && string1[len1-1] == string2[len2-1]) {
    len1--;
    len2--;
  }

  /* catch trivial cases */
  if (len1 == 0)
    return len2;
  if (len2 == 0)
    return len1;

  /* make the inner cycle (i.e. string2) the longer one */
  if (len1 > len2) {
    size_t nx = len1;
    const lev_byte *sx = string1;
    len1 = len2;
    len2 = nx;
    string1 = string2;
    string2 = sx;
  }
  /* check len1 == 1 separately */
  if (len1 == 1) {
    if (xcost)
      return len2 + 1 - 2*(memchr(string2, *string1, len2) != NULL);
    else
      return len2 - (memchr(string2, *string1, len2) != NULL);
  }
  len1++;
  len2++;
  half = len1 >> 1;

  /* initalize first row */
  row = (size_t*)malloc(len2*sizeof(size_t));
  if (!row)
    return (size_t)(-1);
  end = row + len2 - 1;
  for (i = 0; i < len2 - (xcost ? 0 : half); i++)
    row[i] = i;

  /* go through the matrix and compute the costs.  yes, this is an extremely
   * obfuscated version, but also extremely memory-conservative and relatively
   * fast.  */
  if (xcost) {
    for (i = 1; i < len1; i++) {
      size_t *p = row + 1;
      const lev_byte char1 = string1[i - 1];
      const lev_byte *char2p = string2;
      size_t D = i;
      size_t x = i;
      while (p <= end) {
        if (char1 == *(char2p++))
          x = --D;
        else
          x++;
        D = *p;
        D++;
        if (x > D)
          x = D;
        *(p++) = x;
      }
    }
  }
  else {
    /* in this case we don't have to scan two corner triangles (of size len1/2)
     * in the matrix because no best path can go throught them. note this
     * breaks when len1 == len2 == 2 so the memchr() special case above is
     * necessary */
    row[0] = len1 - half - 1;
    for (i = 1; i < len1; i++) {
      size_t *p;
      const lev_byte char1 = string1[i - 1];
      const lev_byte *char2p;
      size_t D, x;
      /* skip the upper triangle */
      if (i >= len1 - half) {
        size_t offset = i - (len1 - half);
        size_t c3;

        char2p = string2 + offset;
        p = row + offset;
        c3 = *(p++) + (char1 != *(char2p++));
        x = *p;
        x++;
        D = x;
        if (x > c3)
          x = c3;
        *(p++) = x;
      }
      else {
        p = row + 1;
        char2p = string2;
        D = x = i;
      }
      /* skip the lower triangle */
      if (i <= half + 1)
        end = row + len2 + i - half - 2;
      /* main */
      while (p <= end) {
        size_t c3 = --D + (char1 != *(char2p++));
        x++;
        if (x > c3)
          x = c3;
        D = *p;
        D++;
        if (x > D)
          x = D;
        *(p++) = x;
      }
      /* lower triangle sentinel */
      if (i <= half) {
        size_t c3 = --D + (char1 != *char2p);
        x++;
        if (x > c3)
          x = c3;
        *p = x;
      }
    }
  }

  i = *end;
  free(row);
  return i;
}

/****************************************************************************
 *
 * Editops and other difflib-like stuff.
 *
 ****************************************************************************/

static inline LevEditOp*
editops_from_cost_matrix(size_t len1, const lev_byte *string1, size_t o1,
                         size_t len2, const lev_byte *string2, size_t o2,
                         size_t *matrix, size_t *n)
{
  size_t *p;
  size_t i, j, pos;
  LevEditOp *ops;
  int dir = 0;

  pos = *n = matrix[len1*len2 - 1];
  if (!*n) {
    free(matrix);
    return NULL;
  }
  ops = (LevEditOp*)malloc((*n)*sizeof(LevEditOp));
  if (!ops) {
    free(matrix);
    *n = (size_t)(-1);
    return NULL;
  }
  i = len1 - 1;
  j = len2 - 1;
  p = matrix + len1*len2 - 1;
  while (i || j) {
    /* prefer contiuning in the same direction */
    if (dir < 0 && j && *p == *(p - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_INSERT;
      ops[pos].spos = i + o1;
      ops[pos].dpos = --j + o2;
      p--;
      continue;
    }
    if (dir > 0 && i && *p == *(p - len2) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_DELETE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = j + o2;
      p -= len2;
      continue;
    }
    if (i && j && *p == *(p - len2 - 1)
        && string1[i - 1] == string2[j - 1]) {
      /* don't be stupid like difflib, don't store LEV_EDIT_KEEP */
      i--;
      j--;
      p -= len2 + 1;
      dir = 0;
      continue;
    }
    if (i && j && *p == *(p - len2 - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_REPLACE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = --j + o2;
      p -= len2 + 1;
      dir = 0;
      continue;
    }
    /* we cant't turn directly from -1 to 1, in this case it would be better
     * to go diagonally, but check it (dir == 0) */
    if (dir == 0 && j && *p == *(p - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_INSERT;
      ops[pos].spos = i + o1;
      ops[pos].dpos = --j + o2;
      p--;
      dir = -1;
      continue;
    }
    if (dir == 0 && i && *p == *(p - len2) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_DELETE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = j + o2;
      p -= len2;
      dir = 1;
      continue;
    }
    /* coredump right now, later might be too late ;-) */
    assert("lost in the cost matrix" == NULL);
  }
  free(matrix);

  return ops;
}

/*
 * Find (some) edit sequence from string1 to string2.
 */
LEV_STATIC_PY LevEditOp*
lev_editops_find(size_t len1, const lev_byte *string1,
                 size_t len2, const lev_byte *string2,
                 size_t *n)
{
  size_t len1o, len2o;
  size_t i;
  size_t *matrix; /* cost matrix */

  /* strip common prefix */
  len1o = 0;
  while (len1 > 0 && len2 > 0 && *string1 == *string2) {
    len1--;
    len2--;
    string1++;
    string2++;
    len1o++;
  }
  len2o = len1o;

  /* strip common suffix */
  while (len1 > 0 && len2 > 0 && string1[len1-1] == string2[len2-1]) {
    len1--;
    len2--;
  }
  len1++;
  len2++;

  /* initalize first row and column */
  matrix = (size_t*)malloc(len1*len2*sizeof(size_t));
  if (!matrix) {
    *n = (size_t)(-1);
    return NULL;
  }
  for (i = 0; i < len2; i++)
    matrix[i] = i;
  for (i = 1; i < len1; i++)
    matrix[len2*i] = i;

  /* find the costs and fill the matrix */
  for (i = 1; i < len1; i++) {
    size_t *prev = matrix + (i - 1)*len2;
    size_t *p = matrix + i*len2;
    size_t *end = p + len2 - 1;
    const lev_byte char1 = string1[i - 1];
    const lev_byte *char2p = string2;
    size_t x = i;
    p++;
    while (p <= end) {
      size_t c3 = *(prev++) + (char1 != *(char2p++));
      x++;
      if (x > c3)
        x = c3;
      c3 = *prev + 1;
      if (x > c3)
        x = c3;
      *(p++) = x;
    }
  }

  /* find the way back */
  return editops_from_cost_matrix(len1, string1, len1o,
                                  len2, string2, len2o,
                                  matrix, n);
}

/*
 * Find matching blocks.
 *
 * Returns a newly allocated array.  len1, len2 are string lengths.
 */
LEV_STATIC_PY LevMatchingBlock*
lev_editops_matching_blocks(size_t len1,
                            size_t len2,
                            size_t n,
                            const LevEditOp *ops,
                            size_t *nmblocks)
{
  size_t nmb, i, spos, dpos;
  LevEditType type;
  const LevEditOp *o;
  LevMatchingBlock *mblocks, *mb;

  /* compute the number of matching blocks */
  nmb = 0;
  o = ops;
  spos = dpos = 0;
  type = LEV_EDIT_KEEP;
  for (i = n; i; ) {
    /* simply pretend there are no keep blocks */
    while (o->type == LEV_EDIT_KEEP && --i)
      o++;
    if (!i)
      break;
    if (spos < o->spos || dpos < o->dpos) {
      nmb++;
      spos = o->spos;
      dpos = o->dpos;
    }
    type = o->type;
    switch (type) {
      case LEV_EDIT_REPLACE:
      do {
        spos++;
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_DELETE:
      do {
        spos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_INSERT:
      do {
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      default:
      break;
    }
  }
  if (spos < len1 || dpos < len2)
    nmb++;

  /* fill the info */
  mb = mblocks = (LevMatchingBlock*)malloc(nmb*sizeof(LevOpCode));
  if (!mblocks) {
    *nmblocks = (size_t)(-1);
    return NULL;
  }
  o = ops;
  spos = dpos = 0;
  type = LEV_EDIT_KEEP;
  for (i = n; i; ) {
    /* simply pretend there are no keep blocks */
    while (o->type == LEV_EDIT_KEEP && --i)
      o++;
    if (!i)
      break;
    if (spos < o->spos || dpos < o->dpos) {
      mb->spos = spos;
      mb->dpos = dpos;
      mb->len = o->spos - spos;
      spos = o->spos;
      dpos = o->dpos;
      mb++;
    }
    type = o->type;
    switch (type) {
      case LEV_EDIT_REPLACE:
      do {
        spos++;
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_DELETE:
      do {
        spos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_INSERT:
      do {
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      default:
      break;
    }
  }
  if (spos < len1 || dpos < len2) {
    assert(len1 - spos == len2 - dpos);
    mb->spos = spos;
    mb->dpos = dpos;
    mb->len = len1 - spos;
    mb++;
  }
  assert((size_t)(mb - mblocks) == nmb);

  *nmblocks = nmb;
  return mblocks;
}
