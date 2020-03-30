#include <RcppArmadillo.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "baumwelch.h"

using namespace Rcpp;
using namespace std;

#define MLOGIT_CLASS 4
#define NUM_CLASS 4

// [[Rcpp::depends(RcppArmadillo)]]

/*
 * For old version (sample haplotypes without cat the reads together since the reads covers all the way along to the end)
 */

// [[Rcpp::export]]
List sample_hap (List dat_info, IntegerVector start, IntegerVector idx, IntegerVector hap_deletion_len) {
  List deletion = dat_info["deletion"];
  IntegerVector del_flag = deletion["del_flag"];
  IntegerVector del_id_all = deletion["del_id_all"];
  IntegerVector del_ref_pos = deletion["del_ref_pos"];
  IntegerVector ref_pos = dat_info["ref_pos"];
  IntegerVector obs = dat_info["nuc"];
  IntegerVector length = dat_info["length"];
  int del_total = deletion["del_total"];
  int hap_max_pos = dat_info["ref_length_max"];
  int hap_min_pos = dat_info["ref_start"];
  int hap_length = hap_max_pos - hap_min_pos;
  unsigned int i, j, m;
  unsigned int sum = 0;
  unsigned int count = 0;
  IntegerMatrix hap_nuc(NUM_CLASS, hap_length);
  
  for(i = 0; i < NUM_CLASS; ++i)
    sum += hap_deletion_len[i];
  IntegerVector hap_ref_pos(sum);
  IntegerVector strat_id(NUM_CLASS);
  //Rcout << start << "\n";
  for (i = 0; i < NUM_CLASS; ++i) {
    //Rprintf("\n%d\n", i);
    for (j = 0; j < length[idx[i] - 1]; ++j)  {
      //Rprintf("%d\t", obs[start[i] + j]);
      hap_nuc(i, ref_pos[start[i] + j]) = obs[start[i] + j]; //idx start from 1!!!
    }
    if (del_flag[idx[i] - 1])
      for (m = 0; m < del_total; ++m)
        if (del_id_all[m] == idx[i]) {
          hap_nuc(i, del_ref_pos[m]) = 4;
          hap_ref_pos[count++] = del_ref_pos[m];
        }
  }
  
  for (i = 0; i < NUM_CLASS; ++i) {
    if (hap_deletion_len[i] != 0) {
      if(i != 0)
        for (j = 0; j < i; ++j)
          strat_id[i] += hap_deletion_len[j];
    }
    else
      strat_id[i] = -1; //record the deletion starting id in vector hap_ref_pos for each hap
  }
  
  List ls = List::create(
    Named("hap") = hap_nuc,
    Named("deletion_pos") = hap_ref_pos, // this position is relative to the alignment start position
    Named("hap_deletion_len") = hap_deletion_len,
    Named("hap_del_start_id") = strat_id);
  return ls;
}

/*
 * Initialize based on the variant site and ramdomly sample 4 haplotypes
 */
// [[Rcpp::export]]
List sample_hap2(List hmm_info, unsigned int hap_length, int hap_min_pos) 
{
  List hidden_states = hmm_info["hidden_states"];
  IntegerVector num_states = hmm_info["num_states"];
  IntegerVector time_pos = hmm_info["time_pos"];
  IntegerVector p_tmax = hmm_info["p_tmax"];
  IntegerVector n_row = hmm_info["n_row"];
  IntegerVector pos_possibility = hmm_info["pos_possibility"];
  IntegerVector undecided_pos = hmm_info["undecided_pos"];
  unsigned int m, k, j;
  
  IntegerMatrix haplotype = fill_all_hap(hidden_states, hap_length, n_row);
  List comb_info = find_combination(undecided_pos, pos_possibility, hap_length, time_pos[0], hap_min_pos);
  IntegerVector location = comb_info["location"];
  IntegerMatrix combination = comb_info["combination"];
  unsigned int num = comb_info["num"];
  unsigned int total = combination.nrow();
  m = R::runif(0, total - 1);
  IntegerMatrix haplotype_out = make_hap(hidden_states, haplotype, location, hap_length, combination(m, _), time_pos[0], num, hap_min_pos);
  
  int gap_in = 0;
  for(k = 0; k < NUM_CLASS; ++k) 
    for(j = 0; j < hap_length; ++j)
      if(haplotype_out(k, j) == -1)
        gap_in = 1;
      
  List ls = List::create(
    Named("haplotype") = haplotype_out,
    Named("gap_in") = gap_in);
  return(ls);
}





