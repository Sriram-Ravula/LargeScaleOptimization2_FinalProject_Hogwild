import numpy as np
import random

"""
Generates synthetic data which is sparse in each sample

-Arguments-
n: num samples
d: num features
sparsity_prop: proportion of features that are nonzero for each sample
problem: regression OR classification
noisy: whether to add white Gaussian noise
seed: random seed for reproducing results

-Returns-
b: the vector of labels/outputs
A: the data matrix
x: the true coefficient vector
"""
def gen_synth_sparse_data(n, d, Lasso = False, Lasso_sparsity = 0.1, data_sparsity = 0.1, problem = "regression", noisy = True, seed = 1000):

    num_nonzero = int(d*data_sparsity)
    num_zero = d - num_nonzero

    #Seed for reproducibility
    np.random.seed(seed)
    random.seed(seed)

    #Check if sparsity parameter correct
    if data_sparsity > 1:
        print("Must have sparsity_prop < 1! Try again!")
        exit(0)

    #Generate the sparse data matrix
    A_dense = np.random.normal(0, 1, (n, d))

    mask_arr = np.array([0]*num_zero + [1]*num_nonzero) #create the {0,1} mask matrix to project A onto the appropriate sparsity
    stacked = np.tile(mask_arr, (n, 1))
    mask = shuffle_rows(stacked)

    A = np.multiply(A_dense, mask)

    #Generate the coefficient vector with possible sparsity for Lasso problems
    x = np.random.normal(0, 1, d)

    if Lasso:
        x[random.sample(range(d), int((1-Lasso_sparsity) * d))] = 0 #replace all but Lasso_sparsity fraction of entries with 0

    #Generate the output vector with possible AWGN and binary quantization if the problem is classification
    b = np.dot(A, x)

    if noisy:
        b += np.random.normal(0, 1, n)

    if problem == "classification":
        b = np.sign(b)

    return [b, A, x]

"""
Write data to files to use in parallel algorithms

-Arguments-
b: the outputs/labels
A: the data matrix
x: the true coefficient vector
"""
def write_data(b, A, x):
    return 0

"""
Shuffles the elements in each row of a matrix independently

-Arguments-
arr: the array to shuffle
"""
def shuffle_rows(arr):
    x, y = arr.shape
    rows = np.indices((x, y))[0] #returns an (x,y) matrix whose elements represent the index of their corresponding rows
    cols = [np.random.permutation(y) for _ in range(x)] #returns an (x,y) matrix where each row is a random permutation of [0,...,y-1]

    return arr[rows, cols]








