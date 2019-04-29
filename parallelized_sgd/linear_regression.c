#include "linear_regression.h"

int linreg_gradient(double *iterate, sparse_array_t sparse_sample_x, double sample_y, sparse_array_t *ret_sample_grad, double *scratchpad) {
    double XB_y = dot_sparse_dense(sparse_sample_x, iterate) - sample_y;
    int XT_XB_y_2 = scale_sparse(ret_sample_grad, sparse_sample_x, 2 * XB_y);
	return 0;
}
