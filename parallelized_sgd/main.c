#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "data.h"
#include "problem.h"
#include "psgd_analysis.h"
#include "linear_regression.h"
#include "logistic_regression.h"
#include "hogwild.h"
#include "naive_psgd.h"
#include "example_algo.h"
#include "segmented_hogwild.h"






int main(int argc, char **argv) {
	int rc;
	data_t data;
	log_t log;

	// First argument is the number of threads to run,
	// Second argument is the data filename,
	// Third argument is the problem type,
	// Fourth argument is the algorithm,
	// Fifth argument is the total number of iterations,
	// Sixth argument is the number of points to log,
	// Seventh argument is the stepsize,
	if (argc < 7+1) {
		printf("Usage: ./run <num_threads> <data_filename> <problem type> <algorithm name> <total # iterations> <# log points> <stepsize>\n");
		printf("\tValid problem types: linearregression, logisticregression\n");
		printf("\tValid algorithms: hogwild, exampleindependent, exampleshared, naive, segmentedhogwild\n");
		exit(-1);
	}
	int num_threads = atoi(argv[1]);
	char *filename = argv[2];
	char *problem_type = argv[3];
	char *algorithm_name = argv[4];
	int total_num_iters = atoi(argv[5]);
	int num_log_pts = atoi(argv[6]);
	double stepsize = atof(argv[7]);

	// Read data file
	rc = read_and_alloc_data(filename, &data);
	if (rc)
		exit(-1);

	// Setup problem based on arguments
	problem_t problem;
	if (strcmp(problem_type, "linearregression") == 0) {
		problem.gradient = linreg_gradient;
	} else if (strcmp(problem_type, "logisticregression") == 0) {
		problem.gradient = logreg_gradient;
	} else { // default: linearregression
		printf("WARNING: Unrecognized problem type. Defaulting to linearregression\n");
		problem_type = "linearregression";
		problem.gradient = linreg_gradient;
	}
	if (strcmp(algorithm_name, "hogwild") == 0) {
		problem.algo_update_func = hogwild;
		problem.algo_init_func = hogwild_initialize;
		problem.algo_deinit_func = hogwild_deinitialize;
	} else if (strcmp(algorithm_name, "exampleindependent") == 0) {
		problem.algo_update_func = example_algo_update_independent;
		problem.algo_init_func = example_algo_initialize;
		problem.algo_deinit_func = example_algo_deinitialize;
	} else if (strcmp(algorithm_name, "exampleshared") == 0) {
		problem.algo_update_func = example_algo_update_shared;
		problem.algo_init_func = example_algo_initialize;
		problem.algo_deinit_func = example_algo_deinitialize;
	} else if (strcmp(algorithm_name, "naive") == 0) {
		problem.algo_update_func = naive_psgd;
		problem.algo_init_func = naive_psgd_initialize;
		problem.algo_deinit_func = naive_psgd_deinitialize;
	} else if (strcmp(algorithm_name, "segmentedhogwild") == 0) {
		problem.algo_update_func = segmented_hogwild;
		problem.algo_init_func = segmented_hogwild_initialize;
		problem.algo_deinit_func = segmented_hogwild_deinitialize;
	} else { // default: hogwild
		printf("WARNING: Unrecognized algorithm. Defaulting to hogwild\n");
		algorithm_name = "hogwild";
		problem.algo_update_func = hogwild;
		problem.algo_init_func = hogwild_initialize;
		problem.algo_deinit_func = hogwild_deinitialize;
	}
	problem.stepsize = stepsize;
	problem.num_total_iter = total_num_iters;
	problem.num_log_points = num_log_pts;
	set_current_problem(problem);

	// Initialize
	log_initialize(&log, data.num_features);
	timerstats_t main_thread_stats;
	timerstats_t *threads_stats = (timerstats_t *) malloc(num_threads * sizeof(timerstats_t));
	timerstats_t *gradient_stats = (timerstats_t *) malloc(num_threads * sizeof(timerstats_t));
	timerstats_t *coord_update_stats = (timerstats_t *) malloc(num_threads * sizeof(timerstats_t));

	// Run general analysis
	rc = run_psgd_general_analysis(num_threads, &data, &log, &main_thread_stats, threads_stats, gradient_stats, coord_update_stats);
	if (rc) {
		printf("Error running general analysis for LinearRegression with HOGWILD!\n");
		exit(rc);
	}

	// Write results to file
	const int max_input_filenam_len = 200;
	char results_dir[max_input_filenam_len+1];
	create_results_dir(algorithm_name, problem_type, filename, results_dir);
	rc = write_results_to_file(num_threads, results_dir, &log, main_thread_stats, threads_stats, gradient_stats, coord_update_stats);
	if (rc)
		exit(-1);

	free(threads_stats);
	free(gradient_stats);
	free(coord_update_stats);
	log_free(&log);
	dealloc_data(&data);
}
