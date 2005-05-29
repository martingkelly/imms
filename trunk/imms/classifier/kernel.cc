#include <assert.h>

#include "kernel.h"
#include "distance.h"

real EMDKernel::eval(Sequence *x, Sequence *y)
{
    assert(x->n_real_frames == 1 && y->n_real_frames == 1);
    int uid1 = (int)x->frames[0][0], uid2 = (int)y->frames[0][0];
    float dist = song_cepstr_distance(uid1, uid2);
    assert(dist >= 0);
    return dist;
}
