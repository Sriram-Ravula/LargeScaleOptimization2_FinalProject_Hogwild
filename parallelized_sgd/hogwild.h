/*
 * HOGWILD! algorithm from TODO
 */

#ifndef _hogwild_h_
#define _hogwild_h_

#include "timer.h"


int hogwild(double *iterate, data_t *data, int thread_num);


int hogwild_initialize(int num_features, int num_threads);
int hogwild_deinitialize(void);


// This will be a lower bound on the number of collisions in
// atomic increment. Since it is not made thread-safe, threads
// may overwrite each others' increments.
extern unsigned int hogwild_num_atomic_dec_collisions;


#endif // _hogwild_h_
