#include <RcppArmadillo.h>
#include <stdio.h>
#include <stdlib.h>

#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>

using namespace Rcpp;

// [[Rcpp::depends(RcppArmadillo)]]

#define MLOGIT_CLASS 4
#define NUM_CLASS 4
#define PD_LENGTH 10

double site_likelihood (unsigned int i, unsigned int K,
                        List par, List dat_info, NumericMatrix haplotype);

double site_likelihood (unsigned int i, unsigned int K, 
                        List par, List dat_info, IntegerMatrix haplotype)
{
  unsigned int j, l;
  double qua_in, read_pos_in, ref_pos_in;
  //unsigned int index = 0;
  double tail, sum;
  double xb = 0;
  
  IntegerVector qua = dat_info["qua"];
  IntegerVector obs = dat_info["nuc"];
  IntegerVector ref_pos = dat_info["ref_pos"];
  IntegerVector read_pos = dat_info["read_pos"];
  IntegerVector length = dat_info["length"];
  IntegerVector index = dat_info["start_id"];
  IntegerMatrix ref_index = dat_info["ref_idx"];
  NumericMatrix beta = par["beta"];
  
  NumericVector hap_nuc(MLOGIT_CLASS - 1);
  NumericVector hnuc_qua(MLOGIT_CLASS - 1);
  NumericVector pred_beta(MLOGIT_CLASS - 1);
  double read_llk = 0.;
  
  //NumericVector site_llk(length[i]);
  
  // /* find the index of ith observation (except for the first one) */
  // if (i != 0)
  //   for (k = 0; k < i; ++k)
  //     index += length[k];
  
  /* Use XY_ENCODING */
  for(j = 0; j < length[i]; ++j) {
    
    //Rprintf("j %d, position %d\n", j, index[i] + j);
    qua_in = qua[index[i] + j];
    read_pos_in = read_pos[index[i] + j];
    ref_pos_in = ref_pos[index[i] + j];
    // 
    for (l = 0; l < MLOGIT_CLASS - 1; ++l) {
     hap_nuc[l] = 0;
     hnuc_qua[l] = 0;
     pred_beta[l] = 0;
    }
    
    //Rcout << "haplotype " << haplotype(K, ref_pos_in) << "\t";
    // Extract the haplotypes at the reference position
    if(haplotype(K, ref_pos_in) == 1) {
      hap_nuc[0] = 1;
      hnuc_qua[0] = qua_in;
    } else if(haplotype(K, ref_pos_in) == 3) {
      hap_nuc[1] = 1;
      hnuc_qua[1] = qua_in;
    } else if(haplotype(K, ref_pos_in) == 2) {
      hap_nuc[2] = 1;
      hnuc_qua[2] = qua_in;
    }
    
    arma::vec predictor = {1, read_pos_in, ref_pos_in, qua_in, hap_nuc[0], hap_nuc[1], 
                                   hap_nuc[2], hnuc_qua[0], hnuc_qua[1], hnuc_qua[2]};
    
    // cblas_dgemv(CblasRowMajor, CblasNoTrans, MLOGIT_CLASS - 1, PD_LENGTH, 1, beta, PD_LENGTH, 
    //             predictor, 1, 0, pred_beta, 1);
    arma::mat beta_ar = as<arma::mat>(beta);
    arma::vec pb = beta_ar.t() * predictor;
    pred_beta = as<NumericVector>(wrap(pb));
    
    //Rcout << "predictor : " << predictor.t() << "\n";
    //Rcout << "beta_ar : " << beta_ar << "\n";
    
    sum = 0.0;
    for (l = 0; l < MLOGIT_CLASS - 1; ++l)
      sum += exp(pred_beta[l]);
    tail = log(1/(1 + sum));
    
    if(obs[index[i] + j] == 1) {
      xb = pred_beta[0];
    } else if(obs[index[i] + j] == 3) {
      xb = pred_beta[1];
    } else if(obs[index[i] + j] == 2) {
      xb = pred_beta[2];
    } else if(obs[index[i] + j] == 0) {
      xb = 0;
    }
    //Rcout << "xb : " << xb << "\n";
    //Rcout << exp(xb + tail) << "\t";
    read_llk += xb + tail; /* Notice here use log likelihood, not likelihood */
  }
  
  return read_llk;
} /* site_likelihood */


// [[Rcpp::export]]
List em_eta (List par, List dat_info, IntegerMatrix haplotype) {
  
  int n_observation = dat_info["n_observation"];
  //IntegerVector length = dat_info["length"];
  //int hap_length = haplotype.ncol();
  // int hap_length = max(dat_info["ref_pos"]);
  double full_llk;
  
  NumericMatrix w_ic(n_observation, NUM_CLASS);
  NumericVector eta = par["eta"];
  NumericVector mixture_prop_updated(NUM_CLASS);
  IntegerVector excluded_read = par["excluded_read"];
  NumericMatrix read_class_llk(n_observation, NUM_CLASS);
  NumericMatrix sum_weight(n_observation);
  NumericMatrix beta = par["beta"];
  // NumericVector m_hap_llk(NUM_CLASS * hap_length * MLOGIT_CLASS);
  // m_hap_llk.attr("dim") = Dimension(NUM_CLASS, hap_length, MLOGIT_CLASS);
  
  unsigned int i, k;
  //double sum_weight;
  //Rcout << "eta" << eta << "\n";
  //Rcout << "beta" << beta << "\n";
  /* e step */
  for (i = 0; i < n_observation; ++i) {
    // Exclude the read has -inf likelihood
    if(excluded_read[i] == 1)
      continue;
    /* reset eta %*% llk at each class for each read */
    NumericVector weight_llk(NUM_CLASS);
    /* compute the likelihood for each read under each haplotype */
    //sum_weight = 0.;
    for (k = 0; k < NUM_CLASS; ++k) {
      read_class_llk(i, k) = site_likelihood(i, k, par, dat_info, haplotype);
      weight_llk[k] = eta[k] * exp(read_class_llk(i, k));
      //Rprintf("\n read %d class %d llk %f; wei_lk %.30lf\n", i, k, read_class_llk(i, k), weight_llk[k]);
      sum_weight[i] += weight_llk[k];
    }
    //Rprintf("sum_weight %.30lf\n", sum_weight[i]);
    for (k = 0; k < NUM_CLASS; ++k)
      w_ic(i, k) = weight_llk[k]/sum_weight[i];
    
    /* some reads may not be explained by either of the haplotype */
    if (exp(read_class_llk(i, 0)) == 0 || isnan(w_ic(i, 0)))
      excluded_read[i] = 1;
  }
  //Rcout << "w_ic : " << w_ic << "\n";
  //Rcout << "read class llk : " << read_class_llk << "\n";
  
  full_llk = 0.;
  int count = 0;
  //double mixture_llk = 0.;
  /* Record the full likelihood for terminating EM
   Some of the likelihood is 0, exclude those reads for now */
  for (i = 0; i < n_observation; ++i) {
    //mixture_llk = 0.;
    if (excluded_read[i] == 1)
    {
      count++;
      continue;
    }
    //for (k = 0; k < NUM_CLASS; ++k)
      //mixture_llk += eta[k] * exp(read_class_llk(i, k)); // Store weight_llk[i, k] to reduce time
    full_llk += log(sum_weight[i]);
  }
  
  /* update eta, still exclude nan */
  int count_all;
  for (k = 0; k < NUM_CLASS; ++k) {
    //mixture_prop[k] = 0;
    count_all = 0;
    for (i = 0; i < n_observation; ++i) {
      if (excluded_read[i] == 1)
        continue;
      count_all++;
      mixture_prop_updated[k] += w_ic(i, k);
    }
  }
  /* assume the excluded reads do not come from any of the haplotypes */
  for (k = 0; k < NUM_CLASS; ++k)
    mixture_prop_updated[k] = mixture_prop_updated[k]/count_all;
  
  k = 0;
  IntegerVector excluded_id(count);
  for (i = 0; i < n_observation; ++i)
    if (excluded_read[i] == 1)
      excluded_id[k++] = i + 1; // Plus one for the use of R (index starts from 1)
  
  Rcout << "full loglik : " << full_llk << "\n";
  Rcout << "Mixture proportions : " << mixture_prop_updated << "\n";

  List ls_par = List::create(
      Named("mixture_prop") = mixture_prop_updated,
      Named("w_ic") = w_ic, 
      Named("beta") = beta,
      Named("excluded_read") = excluded_read);
  
  List ls = List::create(
    Named("full_llk") = full_llk,
    Named("param") = ls_par,
    Named("excluded_id") = excluded_id);
  
  return ls;
}