#include <RcppArmadillo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "data_format.h"
using namespace Rcpp;
using namespace std;
// [[Rcpp::depends(RcppArmadillo)]]
#define NUM_CLASS 4
#define MLOGIT_CLASS 4

// [[Rcpp::export]]
