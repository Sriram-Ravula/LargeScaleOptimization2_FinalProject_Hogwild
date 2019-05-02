#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "data.h"
#include "problem.h"
#include "psgd_analysis.h"
#include "hogwild.h"


// This will be a lower bound on the number of collisions. Since
// it is not made thread-safe, threads may overwrite each others'
// increments.
unsigned int hogwild_num_atomic_dec_collisions = 0;


static sparse_array_t *sparse_sample_grads;
static double **scratchpad;
static int num_threads;
static unsigned int *rng_seedp; // seed state for random number generator, one for each thread

static ttimer_t *gradient_timers;
static ttimer_t *coord_update_timers;


static void atomic_decrement(double *dest, double dec_amt) {
	double ret_val;
	double orig_val;
	double dec_val;
	// Perform a compare-and-swap. If the update was of dest was
	//   atomic, the returned value should match the orig_val's
	//   read of the destination.
	do {
		orig_val = *dest;
		dec_val = orig_val - dec_amt;
		__atomic_exchange(dest, &dec_val, &ret_val, __ATOMIC_RELAXED);
		if (ret_val != orig_val)
			hogwild_num_atomic_dec_collisions++;
	} while (ret_val != orig_val);
}


int hogwild(double *iterate, data_t *data, int thread_num) {
	// Get random sample
	//    rand_r is reentrant (thread safe)
	int rand_index = rand_r(&rng_seedp[thread_num]) % data->num_samples;
	sparse_array_t sparse_sample_X = data->sparse_X[rand_index];
	double sample_y = data->y[rand_index];

	// Evaluate gradient
	sparse_sample_grads[thread_num].len = sparse_sample_X.len;
	if (track_gradient_coordupdate()) {
		timer_start(&gradient_timers[thread_num]);
	}
	gradient(iterate, sparse_sample_X, sample_y, &sparse_sample_grads[thread_num], scratchpad[thread_num]);
	if (track_gradient_coordupdate()) {
		timer_pause(&gradient_timers[thread_num]);
	}

	// Update coordinate individually and atomically
	if (track_gradient_coordupdate()) {
		timer_start(&coord_update_timers[thread_num]);
	}
	for (int i = 0; i < sparse_sample_grads[thread_num].len; i++) {
		int index = sparse_sample_grads[thread_num].pts[i].index;
		double value = sparse_sample_grads[thread_num].pts[i].value;
		atomic_decrement(&iterate[index], get_stepsize()*value);
	}
	if (track_gradient_coordupdate()) {
		timer_pause(&coord_update_timers[thread_num]);
	}
	/* TODO uncomment this
	*/
	
	return 0;
}


int hogwild_initialize(int num_features, int num_thr) {
	num_threads = num_thr;
	srand(time(NULL));
	// Initialize sparse_sample_grads
	sparse_sample_grads = (sparse_array_t *) malloc(num_threads*sizeof(sparse_array_t));
	for (int n = 0; n < num_threads; n++) {
		sparse_sample_grads[n].len = 0;
		sparse_sample_grads[n].pts = (sparse_point_t *) malloc(num_features * sizeof(sparse_point_t));
	}
	// Initialize scratchpad
	scratchpad = (double **) malloc(num_threads*sizeof(double *));
	for (int n = 0; n < num_threads; n++) {
		scratchpad[n] = (double *) malloc(num_features*sizeof(double));
	}
	// Initialize timers
	gradient_timers = (ttimer_t *) malloc(num_thr * sizeof(ttimer_t));
	coord_update_timers = (ttimer_t *) malloc(num_thr * sizeof(ttimer_t));
	for (int n = 0; n < num_threads; n++) {
#ifdef __linux__
		timer_initialize(&gradient_timers[n], TIMER_SCOPE_THREAD);
		timer_initialize(&coord_update_timers[n], TIMER_SCOPE_THREAD);
#endif // __linux__
#ifdef __APPLE__
		timer_initialize(&gradient_timers[n], TIMER_SCOPE_PROCESS);
		timer_initialize(&coord_update_timers[n], TIMER_SCOPE_PROCESS);
#endif // __APPLE__
	}
	// Initialize RNG seed states
	rng_seedp = (unsigned int *) malloc(num_thr * sizeof(unsigned int));
	for (int n = 0; n < num_thr; n++) {
		rng_seedp[n] = rand() + n;
	}
	return 0;
}


int hogwild_deinitialize(void) {
	// Free sparse_sample_grads
	for (int n = 0; n < num_threads; n++) {
		free(sparse_sample_grads[n].pts);
	}
	free(sparse_sample_grads);
	// Free scratchpad
	for (int n = 0; n < num_threads; n++) {
		free(scratchpad[n]);
	}
	free(scratchpad);
	// Free timers
	for (int n = 0; n < num_threads; n++) {
		timer_deinitialize(&gradient_timers[n]);
		timer_deinitialize(&coord_update_timers[n]);
	}
	free(gradient_timers);
	free(coord_update_timers);
	// Free RNG seed states
	free(rng_seedp);
	// Print lower bound on # of atomic_decrement collisions
	printf("There were at least %d collisions in atomic_decrement\n", hogwild_num_atomic_dec_collisions);
	return 0;
}


int timer_get_internal_timer_stats(timerstats_t *gradient_stats, timerstats_t *coord_update_stats) {
	int rc;
	for (int n = 0; n < num_threads; n++) {
		rc = timer_get_stats(&gradient_timers[n], &gradient_stats[n]);
		if (rc)
			return rc;
		rc = timer_get_stats(&coord_update_timers[n], &coord_update_stats[n]);
		if (rc)
			return rc;
	}
	return 0;
}
