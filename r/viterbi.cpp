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

#define NUM_CLASS 4
using namespace Rcpp;
using namespace std;

// [[Rcpp::depends(RcppArmadillo)]]

// [[Rcpp::export]]
List viterbi(List hmm_info, List dat_info, List hap_info, List par_hmm) {
  List hidden_states = hmm_info["hidden_states"];
  NumericVector phi = par_hmm["phi"];
  List trans = par_hmm["trans"];
  List emit = par_hmm["emit"];
  IntegerVector num_states = hmm_info["num_states"];
  IntegerVector p_tmax = hmm_info["p_tmax"];
  IntegerVector time_pos = hmm_info["time_pos"];
  unsigned int t_max = hmm_info["t_max"];
  int hap_max_pos = dat_info["ref_length_max"];
  int hap_min_pos = dat_info["ref_start"];
  int hap_length = hap_max_pos - hap_min_pos;
  unsigned int t, m, w, k, j, max_id;
  IntegerMatrix hap_final(NUM_CLASS, hap_length);
  /*
   * compute log probabilities table
   */
  List path(t_max);
  List backptr(t_max - 1);
  double max;
  double max_prob = 0;
  for (t = 0; t < t_max; ++t) {
    IntegerVector backptr_t(num_states[t]);
    NumericVector path_t(num_states[t]);
    NumericVector emission = emit[t];
    if (t == 0) {
      for(m = 0; m < num_states[t]; ++m) 
        path_t(m) = phi[m] + emission(m);
    } else {
      NumericMatrix transition = trans[t - 1];
      for(m = 0; m < num_states[t]; ++m) {
        NumericVector path_last = path[t - 1];
        max = -INFINITY;
        for(w = 0; w < num_states[t - 1]; ++w) {
          max_prob = path_last(w) + transition(w, m);
          if (max_prob > max) {
            max = max_prob;
            max_id = w;
          }
        }
        path_t(m) = emission(m) + max;
        backptr_t[m] = max_id;
      }
      backptr(t - 1) = backptr_t;
    }
    path(t) = path_t;
  }
 /*
  * find the path (backtrace)
  */
  IntegerVector hidden_state(t_max);
  t = t_max - 1;
  max = -INFINITY;
  int b_next = 0;
  NumericVector path_t = path(t);
  for(m = 0; m < num_states[t]; ++m) {
    if (path_t(m) > max) {
      b_next = m;
      max = path_t(m);
    }
  }
  hidden_state(t) = b_next;
  while (t--) {
    IntegerVector backptr_t = backptr(t);
    b_next = backptr_t[b_next];
    hidden_state(t) = b_next;
  }

  /*
   * make the haplotype (return the index as well to find the assignment)
   */
  for(t = 0; t < t_max; ++t) {
    List full_hap_t = hap_info(t);
    IntegerMatrix hap_t = full_hap_t[hidden_state(t)];
    for(j = time_pos[t] - hap_min_pos; j < time_pos[t] + p_tmax[t] - hap_min_pos; ++j)
      for(k = 0; k < NUM_CLASS; ++k)
        hap_final(k, j) = hap_t(k, j - time_pos[t] + hap_min_pos);
  }
  
  List ls = List::create(
    Named("hap_final") = hap_final,
    Named("chosed_state") = hidden_state);
  return ls;
}
