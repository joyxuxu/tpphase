#ifndef DATA_FORMAT_H
#define DATA_FORMAT_H

#include <Rcpp.h>
using namespace Rcpp;
DataFrame format_data(List dat_info, IntegerMatrix haplotype, int time_pos = -1);
List unique_map(const Rcpp::IntegerVector & v);
int top_n_map(List unique_map);
int uni_sum(List unique_map, unsigned int cut_off);
#endif