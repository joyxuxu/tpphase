#include <RcppArmadillo.h>
// [[Rcpp::depends(RcppArmadillo)]]
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include "utils.h"

using namespace Rcpp;
using namespace std;
#define NUM_CLASS 4

// # sourceCpp("./r/extra.cpp")
// # comb_info_t0 = find_combination(HMM$undecided_pos, HMM$pos_possibility, HMM$p_tmax[1], HMM$time_pos[1], dat_info$ref_start);
// # a = remake_linkage(linkage_in[, 1:6], 6)
// comb_element_dbl(a$miss_list[[4]], a$flag_list[[4]], a$num_list[[4]])
// # t0 = limit_comb_t0(comb_info_t0$combination, HMM$hidden_states, comb_info_t0$location, linkage_in, comb_info_t0$num, 0, HMM$num_states[1]);
// l2 <- mc_linkage(linkage_in[, 8:11], 4)


vector<vector<int> > cart_product_dbl (const vector<vector<int> > &v) {
  vector<vector<int> > s = {{}};
  for (const auto& u : v) {
    vector<vector<int> > r;
    for (const auto& x : s) {
      for (const auto y : u) {
        r.push_back(x);
        r.back().push_back(y);
      }
    }
    s = move(r);
  }
  return s;
}
// [[Rcpp::export]]
List hash_vec(IntegerVector x) {
  int n = x.size();
  std::map<int, vector<int>> map;
  for (int i = 0; i < n; i++)
    map[x[i]].push_back(i);
  int nres = map.size();
  IntegerVector idx(nres);
  List all_id(nres);
  
  int i = 0; 
  for (auto itr = map.begin(); itr != map.end(); ++itr) { 
    idx[i] = itr->first;
    all_id[i++] = wrap(itr->second);
  }
 
  Rcout << idx << "\n";
  for(int m = 0; m < idx.size(); ++m) {
    IntegerVector id = all_id[m];
    IntegerVector a = x[id];
    Rcout << a << "\n";
  }
  return List::create( _["all_id"] = all_id, _["idx"] = idx );
}

IntegerMatrix dereplicate_states(IntegerMatrix new_combination, List hidden_states, IntegerVector location, 
                                 IntegerVector possible_states, unsigned int num, unsigned int num_states) {
  unsigned int i, j, k;
  // unsigned int heter_snp = 0;
  IntegerMatrix new_comb;
  // IntegerVector heter_id(num);
  // for (i = 0; i < num; ++i)
  //   if (possible_states[i] == 6)
  //     heter_id[heter_snp++] = i;
  // // Rcout <<  heter_snp << "\n";
  // if (heter_snp == 1) { // only on1 heter snp
  //   unsigned int count = 0;
  //   IntegerMatrix comb(num_states, num);
  //   // Rcout << heter_id[0] << "\n";
  //   for(i = 0; i < num_states; ++i) {
  //     if(new_combination(i, heter_id[0]) != 4) {
  //       if(new_combination(i, heter_id[0]) != 1 && new_combination(i, heter_id[0]) != 3) // exclude one of AAAC AACA
  //         comb(count++, _) = new_combination(i, _);
  //     } else { // for AACC and CCAA, only keep 1
  //         for(j = 0; j < num; ++j) {
  //           if(new_combination(i, j) == 0)
  //             comb(count, j) = 1;
  //           else if(new_combination(i, j) == 1)
  //             comb(count, j) = 0;
  //         }
  //         comb(count++, heter_id[0]) = 5;
  //     }
  //   }
  //   // Rcout << count << "\n";
  //   IntegerMatrix tmp = comb(Range(0, count - 1), _);
  //   // print_intmat(tmp);
  //   // further remove duplicate
  //   List derep = hash_mat(tmp);
  //   IntegerVector idx = derep["idx"];
  //   new_comb = ss(tmp, idx);
  // } else {
    // first remove replicate by choosing only 1 invariant rows
    IntegerMatrix sub_hap(NUM_CLASS, num);
    IntegerMatrix long_hap(num_states, NUM_CLASS * num);
    for(i = 0; i < num_states; ++i) {
      IntegerVector comb = new_combination(i, _);
      for (k = 0; k < NUM_CLASS; ++k) {
        for (j = 0; j < num; ++j) {
          IntegerMatrix hidden = hidden_states[location[j]];
          int id = comb[j];
          sub_hap(k, j) = hidden(id, k);
        }
      }
      IntegerMatrix ordered_hap = sort_mat(sub_hap, NUM_CLASS, num);
      // print_intmat(ordered_hap);
      IntegerVector tmp = matrix2vec(ordered_hap);
      // Rcout << tmp << "\n";
      long_hap(i, _) = tmp;
    }
    // hash long_hap
    List info = hash_mat(long_hap);
    IntegerVector idx = info["idx"];
    new_comb = ss(new_combination, idx);
  // }
  return(new_comb);
}
  
  
  
IntegerMatrix comb_element_dbl(List len, IntegerVector flag, unsigned int row) {
  vector<vector<int> > vec;
  int col, i, j;
  for (i = 0; i < len.size(); i++) {
    if(flag[i])
      continue;
    IntegerVector row_vec = len[i];
    col = row_vec.size();
    vector<int> v1(col);
    for (j = 0; j < col; j++) {
      v1[j] = row_vec[j];
    }
    vec.push_back(v1);
  }
  // for (int i = 0; i < vec.size(); i++) { 
  //   for (int j = 0; j < vec[i].size(); j++) 
  //     Rcout << vec[i][j] << " "; 
  //   Rcout << "\n"; 
  // }
  vector<vector<int> > res = cart_product_dbl(vec);
  IntegerMatrix out(res.size(), row);
  for(i = 0; i < res.size(); ++i)
    for(j = 0; j < row; ++j)
      out(i, j) = res[i][j];
  return(out);
}

IntegerMatrix remake_linkage(IntegerMatrix sub_link, unsigned int num) {
  unsigned int i, j, k, i1;
  arma::mat sub_uni = unique_rows(as<arma::mat>(sub_link));
  IntegerMatrix link_uni = wrap(sub_uni);
  List new_link(link_uni.nrow());
  unsigned int total_row = 0;
  for (i = 0; i < link_uni.nrow(); ++i) {
    List nuc_info = unique_map(link_uni(i, _));
    IntegerVector nuc = nuc_info["values"];
    IntegerVector nuc_count = nuc_info["lengths"];
    if (nuc.size() == 1 && nuc[0] == -1) // skip the read does not cover any site
      continue;
    // if(nuc_count[0] == num - 1 || nuc_count[0] == 1) 
    //   continue;
    if(nuc[0] != -1) { // read covers all site
      total_row++;
      new_link[i] = link_uni(i, _);
      continue;
    }
    IntegerVector idx(num);
    for(j = 0; j < num; ++j)
      if(link_uni(i, j) == -1)
        idx(j) = 1; // indicate -1 is here
      
      int move_out = 0;
      // if this read is contained in others
      for (i1 = 0; i1 < link_uni.nrow(); ++i1) {
        if (i1 == i)
          continue;
        int count = 0;
        // List nuc_info = unique_map(link_uni(i1, _));
        // IntegerVector nuc1 = nuc_info["values"];
        // IntegerVector nuc_count1 = nuc_info["lengths"];
        // if(nuc_count1[0] >= nuc_count[0])
        //   continue;
        for(j = 0; j < num; ++j)
          if(!idx(j))
            if(link_uni(i, j) == link_uni(i1, j))
              count++;
            if(count == num - nuc_count[0]) {
              // Rcout << i << "move" << "\n";
              move_out = 1;
              break;
            }
      }
      if(move_out)
        continue;
      List missing(num);
      IntegerVector flag(num);
      int in_row_num = 0;
      for (j = 0; j < num; ++j) {
        if (link_uni(i, j) == -1) {
          List nuc_col = unique_map(link_uni(_, j));
          IntegerVector nuc_unique = nuc_col["values"];
          // Rcout << nuc_unique << "\n";
          if(nuc_unique[0] == -1)
            missing[j] = nuc_unique[Range(1, nuc_unique.size() - 1)];
          else
            missing[j] = nuc_unique;
          in_row_num++;
        } else
          flag[j] = 1;
      }
      
      int add_row = 0;
      if(in_row_num != 1) {
        IntegerMatrix missing_rows = comb_element(missing, flag, in_row_num);
        add_row = missing_rows.nrow();
        IntegerMatrix new_link_i(add_row, num);
        int count = 0;
        for (j = 0; j < num; ++j) { // make fake reads with missing linkage info
          if(flag[j] != 1)
            new_link_i(_, j) = missing_rows(_, count++);
          else
            for (k = 0; k < add_row; ++k)
              new_link_i(k, j) = link_uni(i, j); // repeat the non-missing ones
        }
        new_link[i] = new_link_i;
      } else {
        IntegerMatrix new_link_i;
        IntegerVector tmp;
        for (j = 0; j < num; ++j)
          if(flag[j] != 1) {
            tmp = missing[j];
            add_row = tmp.size();
            new_link_i = IntegerMatrix(add_row, num);
          }
          for (j = 0; j < num; ++j) { // make fake reads with missing linkage info
            if(flag[j] != 1) {
              new_link_i(_, j) = tmp;
            } else {
              for (k = 0; k < add_row; ++k)
                new_link_i(k, j) = link_uni(i, j);
            }
          }
          new_link[i] = new_link_i;
      }
      total_row += add_row;
  }
  
  IntegerMatrix new_link_out(total_row, num);
  total_row = 0;
  for(i = 0; i < link_uni.nrow(); ++i) {
    if(new_link[i] == R_NilValue)
      continue;
    IntegerVector tmp = new_link[i];
    tmp.attr("dim") = Dimension(tmp.size()/num, num);
    IntegerMatrix new_link_i = as<IntegerMatrix>(tmp);
    for (k = 0; k < tmp.size()/num; ++k)
      new_link_out(total_row++, _) = new_link_i(k, _);
  }
  // finally, remove duplcated rows
  arma::mat new_linkage = unique_rows(as<arma::mat>(new_link_out));
  IntegerMatrix out = wrap(new_linkage);
  return(out);
}

IntegerVector best_branch(IntegerMatrix link, List transition, NumericVector initial, 
                          List possi_nuc, int i) {
  unsigned int j, k, l, m , w;
  List comb_in(link.ncol());
  List llk_in(link.ncol());
  int id = 0;
  // possible or determined nuc at each position
  for(j = 0; j < link.ncol(); ++j) {
    IntegerVector nuc = possi_nuc[j];
    if(link(i, j) != -1) {
      for(l = 0 ; l < nuc.size(); ++l)
        if(link(i, j) == nuc[l]) {
          id = l;
          break;
        }
        comb_in(j) = link(i, j);
    } else {
      comb_in(j) = nuc;
    }
  }
  IntegerVector flag(link.ncol());
  // IntegerMatrix poss_reads = comb_element(comb_in, flag, link.ncol());
  // get state likelihood
  for(j = 0; j < link.ncol(); ++j) {
    // Rcout << j << "\t";
    IntegerVector nuc = possi_nuc[j];
    if(link(i, j) != -1) {
      for(l = 0 ; l < nuc.size(); ++l)
        if(link(i, j) == nuc[l]) {
          id = l;
          // Rcout << "nuc " << nuc[l] << "\t";
          break;
        }
        if(j == 0) {
          llk_in(j) = initial[id];
          // Rcout << "ini " << initial[id] << "\t";
        } else {
          NumericMatrix trans = transition[j - 1];
          IntegerVector nuc2 = possi_nuc[j - 1];
          int id1 = 0;
          if(link(i, j - 1) != -1) {
            for(l = 0 ; l < nuc2.size(); ++l)
              if(link(i, j - 1) == nuc2[l]) {
                // Rcout << "nuc(j-1) " << nuc2[l] << "\t";
                id1 = l;
                break;
              }
              llk_in(j) = trans(id1, id);
              // Rcout << "trans " << trans(id1, id) << "\n";
          } else{
            // Rcout << "trans: all\n";
            llk_in(j) = trans(_, id);
          }
        }
    } else {
      if(j == 0)
        llk_in[j] = initial;
      else {
        NumericMatrix trans = transition[j - 1];
        IntegerVector nuc2 = possi_nuc[j - 1];
        if(link(i, j - 1) != -1) {
          for(l = 0 ; l < nuc2.size(); ++l)
            if(link(i, j) == nuc2[l]) {
              // Rcout << "nuc(j-1) " << nuc2[l] << "\n";
              id = l;
              break;
            }
            llk_in(j) = trans(id, _);
        } else {
          llk_in(j) = trans;
        }
      }
    }
  }
  IntegerVector hidden_state(llk_in.size());
  List path(llk_in.size());
  List backptr(llk_in.size() - 1);
  int b_next = 0;
  for(k = 0; k < llk_in.size(); ++k) {
    // Rcout << k << "\n";
    IntegerVector nuc = comb_in(k);
    NumericVector path_t(nuc.size());
    IntegerVector backptr_t(nuc.size());
    if(k == 0) {
      NumericVector trans = llk_in(k);
      for(l = 0; l < nuc.size(); ++l) {
        path_t(l) = trans(l);
      }
      // Rcout << path_t << "\n";
    } else {
      NumericVector tran = llk_in(k);
      int len = tran.size();
      int nrow = len/nuc.size();
      tran.attr("dim") = Dimension(nrow, nuc.size());
      NumericMatrix trans = as<NumericMatrix>(tran);
      NumericVector path_last = path[k - 1];
      for(m = 0; m < trans.ncol(); ++m) {
        double max = -INFINITY;
        int max_id = 0;
        for(w = 0; w < trans.nrow(); ++w) {
          double max_prob = path_last(w) + trans(w, m);
          if (max_prob > max) {
            max = max_prob;
            max_id = w;
          }
        }
        path_t(m) = max;
        backptr_t[m] = max_id;
      }
      backptr(k - 1) = backptr_t;
      // Rcout << path_t << "\n";
      // Rcout << backptr_t << "\n";
    }
    path(k) = path_t;
  }
  // Rcout << "decode\n";
  k = llk_in.size() - 1;
  double max = -INFINITY;
  IntegerVector nuc = comb_in(k);
  NumericVector path_t = path(k);
  for(m = 0; m < nuc.size(); ++m) {
    if (path_t(m) > max) {
      b_next = m;
      max = path_t(m);
    }
  }
  hidden_state(k) = nuc[b_next];
  
  while (k--) {
    // Rcout << k << "\n";
    IntegerVector nuc = comb_in(k);
    IntegerVector backptr_t = backptr(k);
    // Rcout << backptr_t << "\n";
    b_next = backptr_t[b_next];
    hidden_state(k) = nuc[b_next];
  }
  
  // get the combination of reads and corresponding transition matrix
  // List ls = List::create( // possible comb
  //   Named("poss_reads") = poss_reads,
  //   Named("comb_in") = comb_in,
  //   Named("llk_in") = llk_in, 
  //   Named("read") = hidden_state);
  
  return(hidden_state);
}

// [[Rcpp::export]]
IntegerMatrix mc_linkage(IntegerMatrix sub_link, int num) {
  unsigned int i, j, k, l;
  //remove non-covered reads
  NumericVector initial;
  IntegerMatrix uni;
  IntegerMatrix link_pre(sub_link.nrow(), sub_link.ncol());
  int count = 0;
  for(i = 0; i < sub_link.nrow(); ++i) {
    IntegerVector read = sub_link(i, _);
    int rowsum = sum(read);
    if(rowsum == -num)
      continue;
    link_pre(count++, _) = sub_link(i, _);
  }
  IntegerMatrix link = link_pre(Range(0, count - 1), _);
  // if there is only one read link them, then include all the possible combinations
  int complete_seq = 0;
  for(k = 0; k < link.nrow(); ++k) {
    int flg = 0;
    for(j = 0 ; j < link.ncol(); ++j) 
      if(link(k, j) == -1) {
        flg = 1;
        break;
      }
      if(!flg)
        complete_seq++;
  }
  if(complete_seq == 1) {
    Rcout << "only 1 linked read\n";
    uni = remake_linkage(link, num);
  } else {
    List transition(link.ncol() - 1);
    List possi_nuc(link.ncol());
    for(j = 0 ; j < link.ncol() - 1; ++j) {
      List nuc_info = unique_map(link(_, j));
      IntegerVector nuc = nuc_info["values"];
      IntegerVector nuc_count = nuc_info["lengths"];
      // Rcout << j << "\t" << nuc << "\t" << nuc_count << "\n";
      int state1 = nuc.size();
      int start = 0;
      possi_nuc[j] = nuc;
      if(nuc[0] == -1) {
        state1 = nuc.size() - 1;
        start = 1;
        possi_nuc[j] = nuc[Range(1, nuc.size() - 1)];
      }
      if(j == 0) {
        double total = count;
        initial = IntegerVector(state1);
        if(nuc[0] == -1) {
          for(k = 1; k < nuc.size(); ++k)
            initial[k - 1] = log(nuc_count[k]/(total - nuc_count[0])); // in the ascending order
        }
        else {
          for(k = 0; k < nuc.size(); ++k)
            initial[k] = log(nuc_count[k]/total);
        }
      }
      // unique rows and the count
      List nuc1_info = unique_map(link(_, j + 1));
      IntegerVector nuc1 = nuc1_info["values"];
      int state2 = nuc1.size();
      possi_nuc[j + 1] = nuc1;
      if(nuc1[0] == -1) {
        possi_nuc[j + 1] = nuc1[Range(1, nuc1.size() - 1)];
        state2 = nuc1.size() - 1;
      }
      
      IntegerMatrix link_unique(link.nrow(), 2);
      int unique_ct = 0;
      for(k = 0; k < link.nrow(); ++k)
        if(link(k, j + 1) != -1 && link(k, j) != -1) {
          link_unique(unique_ct, 0) = link(k, j );
          link_unique(unique_ct++, 1) = link(k, j + 1);
        }
        // for(k = 0; k < link.nrow(); ++k)
        //   for(l = 0; l < 2; ++l)
        //     Rcout << link_unique(k, j) << "\t";
        IntegerMatrix in_link = link_unique(Range(0, unique_ct - 1), _);
        List unique_row = hash_mat(in_link);
        List all_id = unique_row["all_id"];
        IntegerVector idx = unique_row["idx"];
        NumericMatrix trans(state1, state2);
        for(k = 0; k < state1; ++k)
          for(i = 0; i < state2; ++i)
            trans(k, i) = R_NegInf;
        // Rcout << state1 << "\t" << state2 << "\n";
        List nuc_info_uni = unique_map(in_link(_, 0));
        IntegerVector nuc_count_uni = nuc_info_uni["lengths"];
        IntegerVector nuc_uni = nuc_info_uni["values"];
        
        for(k = start; k < nuc.size(); ++k) {
          // Rcout<< "\n" << nuc[k] << "\n";
          int nuc_id = 0;
          for(l = 0; l < nuc_uni.size(); ++l)
            if(nuc_uni[l] == nuc[k]) {
              nuc_id = l;
              break;
            }
            for(i = 0; i < idx.size(); ++i) {
              double de = nuc_count_uni[nuc_id];
              IntegerVector sub_read = in_link(idx[i], _);
              // Rcout << "unique sub " << sub_read << "\n";
              // IntegerVector sub_read = ordered_read(i, _);
              // if(sub_read[1] == -1) {
              //   de--;
              //   continue;
              // }
              if(nuc[k] != sub_read[0])
                continue;
              IntegerVector sub_id = all_id[i];
              // Rcout << "uniques " << sub_id << "\n";
              double nu = sub_id.size();
              int col_id = 0;
              int flag = 0;
              if(state2 < nuc1.size())
                flag = 1;
              for(l = 0; l < nuc1.size(); ++l)
                if(nuc1[l] != -1 && nuc1[l] == sub_read[1])
                  col_id = l - flag;
                // Rcout <<"col_id " << col_id << "\n" ;
                // if(nuc[k] == sub_read[0]) {
                // Rcout <<"de " << de << " nu " << nu<< "\n" ;
                trans(k - start, col_id) = log(nu/de); // log likelihood
                // }
            }
        }
        transition[j] = trans;
    }
    // 
    // // now use MC to impute the missing nuc
    arma::mat uniqu_link = unique_rows(as<arma::mat>(link));
    IntegerMatrix sub_uni_link = wrap(uniqu_link);
  
    IntegerMatrix mc_reads(sub_uni_link.nrow(), sub_uni_link.ncol());
    for(i = 0; i < sub_uni_link.nrow(); ++i) {
      int flag = 0;
      for(j = 0; j < sub_uni_link.ncol(); ++j) {
        if(sub_uni_link(i, j) == -1) {
          flag = 1;
          break;
        }
      }
      if(!flag) {
        mc_reads(i, _) = sub_uni_link(i, _);
        continue;
      }
      IntegerVector tmp = sub_uni_link(i, _);
      Rcout << tmp << "\n";
      mc_reads(i, _) = best_branch(sub_uni_link, transition, initial, possi_nuc, i);
    }
    arma::mat uniqu = unique_rows(as<arma::mat>(mc_reads));
    uni = wrap(uniqu);
  }
  // List ls = List::create(
  //   Named("transition") = transition, // possible comb
  //   Named("initial") = initial,
  //   Named("possi_nuc") = possi_nuc,
  //   Named("link") = sub_uni_link,
  //   Named("reads") = uni);
  // 
  return(uni);
}
/*
 Given a binary tree, print out all of its root-to-leaf
 paths, one per line. Uses a recursive helper to do the work.
 */
// void printPaths(struct node* node) {
//   int path[1000];
//   printPathsRecur(node, path, 0);
// }

// /*
//  Recursive helper function -- given a node, and an array containing
//  the path from the root node up to but not including this node,
//  print out all the root-leaf paths.
//  */
// void printPathsRecur(struct node* node, int path[], int pathLen) {
//   if (node==NULL) return;
//   
//   // append this node to the path array
//   path[pathLen] = node->data;
//   pathLen++;
//   
//   // it's a leaf, so print the path that led to here
//   if (node->left==NULL && node->right==NULL) {
//     printArray(path, pathLen);
//   }
//   else {
//     // otherwise try both subtrees
//     printPathsRecur(node->left, path, pathLen);
//     printPathsRecur(node->right, path, pathLen);
//   }
// }
// 
// // Utility that prints out an array on a line.
// void printArray(int ints[], int len) {
//   int i;
//   for (i=0; i<len; i++) {
//     Rcout << ints[i] << "\t";
//   }
//   Rcout << "\n";
// }





// List hash_mat(IntegerMatrix x) {
//   int n = x.nrow() ;
//   int nc = x.ncol() ;
//   std::vector<string> hashes(n) ;
//   // arma::Mat<int> X = as<arma::Mat<int>>(x);
//   for (int i = 0; i < n; i++) {
//     string s = "";  
//     for(int j = 0; j < nc; j++)  
//       s += to_string(x(i,j));  
//     hashes[i] = s;
//   }
//   
//   std::unordered_map<string, int> map;
//   for (int i = 0; i < n; i++)
//     map[hashes[i]];
// 
//   int nres = map.size();
//   IntegerVector idx(nres);
//   
//   int i = 0; 
//   for (auto itr = map.begin(); itr != map.end(); ++itr) { 
//     idx[i] = itr->second;
//   } 
//  
//   return List::create(  _["idx"] = idx );
// 
// }




// 
// List limit_comb_t0(IntegerMatrix combination, List hidden_states, IntegerVector location,
//                    IntegerMatrix linkage_info, unsigned int num, unsigned int start_idx, unsigned int num_states) {
//   unsigned int i, j, k, idx, m;
//   IntegerMatrix sub_hap(NUM_CLASS, num);
//   IntegerMatrix old_sub_link = linkage_info(_, Range(start_idx, start_idx + num - 1));
//   IntegerVector exclude(num_states);
//   int count, linkage_len, all_excluded;
//   linkage_len = num - 1; 
//   // int cut_off;
//   // all_excluded = num_states;
//   //remake the linkage
//   IntegerMatrix sub_link = mc_linkage(old_sub_link, num);
//   unsigned int n_observation = sub_link.nrow();
//   // while (all_excluded == num_states) {
//   //   cut_off = NUM_CLASS;
//   //   // Rcout << "linkage length " << linkage_len << "\n"; //actual linkage length + 1
//   //   while (cut_off >= 2 && all_excluded == num_states) {
//   all_excluded = 0;
//   for (m = 0; m < num_states; ++m) {
//     exclude(m) = 0;
//     IntegerVector comb = combination(m, _);
//     // Rcout << comb << "\t\t";
//     count = 0;
//     for (k = 0; k < NUM_CLASS; ++k) {
//       // Rcout << "k" << k << "\n";
//       for (j = 0; j < num; ++j) {
//         IntegerMatrix hidden = hidden_states[location[j]];
//         idx = comb[j];
//         sub_hap(k, j) = hidden(idx, k);
//         // Rcout << sub_hap(k, j) << "\t";
//       }
//       // Rcout << "\n read " << "\n";
//       for (i = 0; i < n_observation; i++) {
//         int flag = 0;
//         for (j = 0; j < num - 1; ++j) {
//           // Rcout << sub_link(i, j) << "\t" << sub_link(i, j + 1);
//           // if (sub_link(i, j) != -1 && sub_link(i, j) != 4)
//           if (sub_hap(k, j) == sub_link(i, j) && sub_hap(k, j + 1) == sub_link(i, j + 1))
//             flag++;
//         }
//         // Rcout << "\n" << flag << "\n" ;
//         if (flag >= linkage_len) {
//           count++;
//           break;
//         }
//       }
//     }
//     // Rcout << "count "<< count << "\n";
//     if (count != NUM_CLASS) {
//       exclude(m) = 1;
//       all_excluded++;
//     }
//   }
//   //     cut_off--; //TODO:how to make sure include more relaiable possibles
//   //   }
//   //   linkage_len--;
//   // }
//   // Rcout << exclude << "\n";
//   List out = List::create(
//     Named("num_states") = num_states - all_excluded,
//     Named("exclude") = exclude);
//   return(out);
// }
// // trans_indicator: indicate which state can transfer to which, further_limit indicate some states should not be considered
// List prepare_ini_hmm (unsigned int t_max, IntegerVector num_states, List trans_indicator, List further_limit) {
//   List trans_new_ind(t_max - 1);
//   unsigned int w, m, t;
//   
//   for(t = 0; t < t_max - 1; ++t) {
//     // Rcout << t << "\n";
//     IntegerVector more_limit_last = further_limit[t];
//     IntegerVector more_limit = further_limit[t + 1];
//     IntegerMatrix trans_ind = trans_indicator[t];
//     IntegerMatrix trans_ind_new(trans_ind.nrow(), num_states[t + 1]);
//     int count2 = 0;
//     int count = 0;
//     for (w = 0; w < trans_ind.ncol(); ++w)
//       if(!more_limit[w])
//         trans_ind_new(_, count++) = trans_ind(_, w);
//       IntegerMatrix trans_ind_new2(num_states[t], num_states[t + 1]);
//       for (m = 0; m < trans_ind.nrow(); ++m)
//         if(!more_limit_last[m])
//           trans_ind_new2(count2++, _) = trans_ind_new(m, _);
//         trans_new_ind[t] = trans_ind_new2;
//   }
//   
//   return(trans_new_ind);
// }

IntegerMatrix unique_overlap(IntegerVector overlapped, IntegerVector exclude_last, IntegerMatrix overlap_comb, IntegerVector overlap_loci, 
                             unsigned int overlap_new_states, unsigned int overlap_num_states) {
  unsigned int i, m;
  // unsigned int n_observation = linkage_info.nrow();
  //decide the first t 
  unsigned int overlap_len = overlapped.size();
  // limit the space based on the last limition first
  int start_overlap = 0;
  for(i = 0; i < overlap_loci.size(); ++i)
    if (overlap_loci[i] == overlapped[0]) {
      start_overlap = i;
      break;
    }
    
    //find the unique states
    IntegerMatrix comb_last(overlap_new_states, overlap_len);
    unsigned int count = 0;
    for (m = 0; m < overlap_num_states; ++m)
      if(!exclude_last[m]) {
        IntegerVector tmp = overlap_comb(m, _);
        comb_last(count++, _) = tmp[Range(start_overlap, start_overlap + overlap_len - 1)];
      }
    arma::mat out = unique_rows(as<arma::mat>(comb_last));
    return(wrap(out));
}


// struct VectorHasher {
//   int operator()(const vector<int> &V) const {
//     int hash=0;
//     for(int i=0;i<V.size();i++) {
//       hash^=V[i];
//     }
//     return hash;
//   }
// };
// 
// std::unordered_map<intvec, int, VectorHasher> map;

// trans_indicator: indicate which state can transfer to which, further_limit indicate some states should not be considered
// List prepare_ini_hmm2 (unsigned int t_max, IntegerVector num_states, List trans_indicator, List further_limit) {
//   List trans_new_ind(t_max - 1);
//   // List emit(t_max);
//   unsigned int w, m, t;
//   // 
//   // IntegerVector more_limit = further_limit[0];
//   // IntegerMatrix trans_ind = trans_indicator[0];
//   // IntegerMatrix trans_id_new(num_states[0], num_states[1]);
//   // int count = 0;
//   // for (w = 0; w < trans_ind.ncol(); ++w)
//   //   if(!more_limit[w])
//   //     trans_id_new(_, count++) = trans_ind(_, w);
//   //   trans_new_ind[0] = trans_id_new;
//   
//   for(t = 0; t < t_max - 1; ++t) {
//     // Rcout << t << "\n";
//     IntegerVector more_limit_last = further_limit[t];
//     IntegerVector more_limit = further_limit[t + 1];
//     IntegerMatrix trans_ind = trans_indicator[t];
//     IntegerMatrix trans_ind_new(trans_ind.nrow(), num_states[t + 1]);
//     int count2 = 0;
//     int count = 0;
//     for (w = 0; w < trans_ind.ncol(); ++w)
//       if(!more_limit[w])
//         trans_ind_new(_, count++) = trans_ind(_, w);
//       for (m = 0; m < trans_ind.nrow(); ++m)
//         if(!more_limit_last[m])
//           trans_ind_new2(count2++, _) = trans_ind_new(m, _);
//         trans_new_ind[t] = trans_ind_new2;
//   }
//   
//   return(trans_new_ind);
// }
// 
// List trans_permit(IntegerVector num_states, List combination, List loci, int t_max) {
//   List trans_permits(t_max - 1);
//   unsigned int t, j, m, w;
//   
//   for(t = 0; t < t_max - 1; ++t) {
//     if(num_states[t + 1] != 1) {
//       IntegerMatrix trans(num_states[t], num_states[t + 1]);
//       IntegerMatrix comb_t1 = combination[t];
//       IntegerMatrix comb_t2 = combination[t + 1];
//       IntegerVector location_t1 = loci[t];
//       IntegerVector location_t2 = loci[t + 1];
//       // Rcout << location_t1 << "\n";
//       // Rcout << location_t2 << "\n";
//       // get the overlapped region, this moght be different from the overlapped states we had
//       int id = 0;
//       for(j = 0; j < location_t1.size(); ++j) 
//         if(location_t1[j] == location_t2[0]) {
//           id = j;
//           break;
//         }
//       int end = 0;
//       for(j = id; j < location_t1.size(); ++j)
//         if(location_t1[j] == location_t2[j - id])
//           end = j;
//         
//         for(m = 0; m < num_states[t]; ++m) {
//           IntegerVector hap_t1 = comb_t1(m, _);
//           for(w = 0; w < num_states[t + 1]; ++w) {
//             IntegerVector hap_t2 = comb_t2(w, _);
//             for(j = id; j < end; ++j)
//               if (hap_t1(j) != hap_t2(j - id)) {
//                 trans(m, w) = 1; // represents m cannot transfer to w
//                 break;
//               }
//           }
//         }
//         trans_permits(t) = trans;
//     } else {
//       IntegerMatrix temp(num_states[t], num_states[t + 1]);
//       trans_permits(t) = temp;
//     }
//   }
// 
//   return(trans_permits);
// }

  // List ini_hmm (unsigned int t_max,  IntegerVector num_states, List trans_indicator) {
  //   NumericVector phi(num_states[0]);
  //   List trans(t_max - 1);
  //   // List emit(t_max);
  //   unsigned int w, m, t;
  //   
  //   for(w = 0; w < num_states[0]; ++w)
  //     phi[w] = 1/double(num_states[0]);
  //   
  //   for(t = 0; t < t_max - 1; ++t) {
  //     NumericMatrix transition(num_states[t], num_states[t + 1]);
  //     // 
  //     // if(trans_indicator.isNotNull()) {
  //     IntegerMatrix trans_ind = trans_indicator[t];
  //     for (w = 0; w < num_states[t]; ++w) {
  //       int new_num = num_states[t + 1];
  //       for (m = 0; m < num_states[t + 1]; ++m)
  //         if (trans_ind(w, m)) // 1 means not the same, so cannot b transferred
  //           new_num--;
  //         // Rcout << new_num << "\n";
  //         for (m = 0; m < num_states[t + 1]; ++m)
  //           if (!trans_ind(w, m))
  //             transition(w, m) = 1/double(new_num);
  //     }
  //     // } else {
  //     // for (w = 0; w < num_states[t]; ++w)
  //     //   for (m = 0; m < num_states[t + 1]; ++m) 
  //     //     transition(w, m) = 1/double(num_states[t + 1]);
  //     // }
  //     trans(t) = transition;
  //   }
  //   // for(t = 0; t < t_max; ++t) {
  //   //   NumericVector emission(num_states[t]);
  //   //   for (w = 0; w < num_states[t]; ++w)
  //   //     emission(w) = 1/double(num_states[t]);
  //   //   emit(t) = emission;
  //   // }
  //   // 
  //   List par_hmm = List::create(
  //     Named("phi") = phi,
  //     // Named("emit") = emit
  //     Named("trans") = trans);
  //   return(par_hmm);
  // }
  // List trans_permit2(IntegerVector num_states, List combination, int t_max, IntegerVector undecided_pos, 
  //                   IntegerVector time_pos, IntegerVector p_tmax, int hap_min_pos) {
  //   List trans_permits(t_max - 1);
  //   List further_limit_col(t_max - 1);
  //   List further_limit_row(t_max - 1);
  //   IntegerVector new_num_states(t_max);
  //   unsigned int t, j, m, w;
  //   int end_t1, begin_t2, end_t2;
  //   for (t = 0; t < t_max; ++t)
  //     new_num_states[t] = num_states[t];
  //   
  //   for (t = 0; t < t_max - 1; ++t) {
  //     if(num_states[t + 1] != 1 && num_states[t] != 1) { // assume if no. state = 1, then it can transfer to all
  //       end_t1 = time_pos[t] + p_tmax[t];
  //       begin_t2 = time_pos[t + 1];
  //       end_t2 = time_pos[t + 1] + p_tmax[t + 1];
  //       if(end_t1 > end_t2)
  //         end_t1 = end_t2; // take the smaller one
  //       IntegerMatrix full_hap_t1 = combination(t);
  //       IntegerMatrix full_hap_t2 = combination(t + 1);
  //       IntegerMatrix trans(num_states[t], num_states[t + 1]);
  //       int count = 0; // number of overlapped variational sites
  //       int count1 = 0;
  //       
  //       for(j = 0; j < undecided_pos.size(); ++j)
  //         if (undecided_pos[j] + hap_min_pos < end_t1 && undecided_pos[j] >= time_pos[t]) {
  //           count1++;
  //           if(undecided_pos[j] + hap_min_pos >= begin_t2)
  //             count++;
  //         }
  //         // Rcout << t << "\t" << count << "\n";
  //       int left = count1 - count;
  //         for(m = 0; m < num_states[t]; ++m) {
  //           IntegerVector hap_t1 = full_hap_t1(m, _);
  //           // Rcout << hap_t1 << "\n";
  //           for(w = 0; w < num_states[t + 1]; ++w) {
  //             IntegerVector hap_t2 = full_hap_t2(w, _);
  //             // Rcout << hap_t2 << "\t";
  //             for(j = 0; j < count; ++j)
  //               if (hap_t2(j) != hap_t1(j + left)) {
  //                 trans(m, w) = 1; // represents m cannot transfer to w
  //                 break;
  //               }
  //           }
  //         }
  //         trans_permits(t) = trans;
  //     } else {
  //       IntegerMatrix temp(num_states[t], num_states[t + 1]);
  //       trans_permits(t) = temp;
  //     }
  //   }
  //   // label the states that should be excluded (go forward)
  //   for(t = 0; t < t_max - 1; ++t) {
  //     if(num_states[t + 1] != 1 || num_states[t] != 1) { 
  //     // if for a row, all == 1 or for a column all == 1, then these two should be excluded
  //     IntegerVector more_limits_row(num_states[t]);
  //     IntegerVector more_limits_col(num_states[t + 1]);
  //     IntegerVector more_limits_col_last;
  //     IntegerMatrix trans = trans_permits(t);
  //     int exluded_col = 0;
  //     for(w = 0; w < num_states[t + 1]; ++w) {
  //       int num_col = 0;
  //       for(m = 0; m < num_states[t]; ++m) {
  //         if(trans(m, w) == 1)
  //           num_col++;
  //       }
  //       if(num_col == num_states[t]){
  //         more_limits_col[w] = 1; // meaning no state transfer to it
  //         exluded_col++;
  //       }
  //     }
  //     for(m = 0; m < num_states[t]; ++m) {
  //       int num_row = 0;
  //       for(w = 0; w < num_states[t + 1]; ++w) {
  //         if(trans(m, w) == 1 && more_limits_col[w] != 1)
  //           num_row++;
  //       }
  //       if(num_row == num_states[t + 1] - exluded_col){
  //         more_limits_row[m] = 1; // meaning it transfer to no state, considering the excluded column 
  //       }
  //     }
  //     // found the states that never appears
  //     further_limit_col(t) = more_limits_col;
  //     further_limit_row(t) = more_limits_row;
  //   } else {
  //     IntegerVector more_limits_col(num_states[t + 1]);
  //     IntegerVector more_limits_row(num_states[t]);
  //     further_limit_col(t) = more_limits_col;
  //     further_limit_row(t) = more_limits_row;
  //   }
  //   }
  //   // go backward
  //   for(t = t_max - 1; t <= 0; --t) {
  //     if(num_states[t + 1] != 1 || num_states[t] != 1) { 
  //       // if for a row, all == 1 or for a column all == 1, then these two should be excluded
  //       IntegerVector more_limits_row = further_limit_row(t);
  //       IntegerVector more_limits_col = further_limit_col[t - 1];
  //       
  //       int index;
  //       for(w = 0; w < more_limits_col.size(); ++w) {
  //         if(more_limits_row[w] != more_limits_col[w])
  //           index = 0;
  //       }
  //      
  //       // found the states that never appears
  //       further_limit_col(t) = more_limits_col;
  //       further_limit_row(t) = more_limits_row;
  //     } 
  //   }
  //   // get new number of states
  //   List further_limit(t_max);
  //   IntegerVector more_limits_row = further_limit_row[0];
  //   for (m = 0; m < num_states[0]; ++m)
  //     if (more_limits_row[m] == 1)
  //       new_num_states[0]--;
  //   further_limit[0] = further_limit_row[0];
  //   IntegerVector more_limits_col = further_limit_col[t_max - 2];
  //   for(w = 0; w < num_states[t_max - 1]; ++w) {
  //       if(more_limits_col[w] == 1)
  //         new_num_states[t_max - 1]--;
  //   }
  //   further_limit[t_max - 1] = further_limit_col[t_max - 2];
  //   
  //   for(t = 1; t < t_max - 1; ++t) {
  //     more_limits_row = further_limit_row(t);
  //     more_limits_col = further_limit_col(t - 1);
  //     IntegerVector combine(more_limits_row.size());
  //     // find the union of these two vector
  //     for(w = 0; w < num_states[t]; ++w) {
  //       if(more_limits_col[w] == 1 || more_limits_row[w] == 1)
  //         combine[w] = 1;
  //     }
  //     further_limit[t] = combine; // indicates some states cannot appear
  //     for(w = 0; w < num_states[t]; ++w)
  //       if(combine[w] == 1)
  //         new_num_states[t]--;
  //   }
  //   
  //   List out = List::create(
  //     Named("trans_permits") = trans_permits,
  //     Named("further_limit") = further_limit, 
  //     Named("new_num_states") = new_num_states);
  //   
  //   return(out);
  // }

// vector<vector<int> > cart_product (const vector<vector<int> > &v) {
//   vector<vector<int> > s = {{}};
//   for (const auto& u : v) {
//     vector<vector<int> > r;
//     for (const auto& x : s) {
//       for (const auto y : u) {
//         r.push_back(x);
//         r.back().push_back(y);
//       }
//     }
//     s = move(r);
//   }
//   return s;
// }
// 
// IntegerMatrix call_cart_product(IntegerVector len) {
//   unsigned int row = len.size();
//   vector<vector<int> > vec(row);
//   unsigned int col, count, i, j;
//   for (i = 0; i < row; i++) {
//     count = 1;
//     col = len[i];
//     vec[i] = vector<int>(col);
//     for (j = 0; j < col; j++)
//       vec[i][j] = count++;
//   }
//   vector<vector<int> > res = cart_product(vec);
//   IntegerMatrix out(res.size(), row);
//   for(i = 0; i < res.size(); ++i)
//     for(j = 0; j < row; ++j) 
//       out(i, j) = res[i][j] - 1; //minus 1 for the index in C
//   
//   return(out);
// }
// 
// List find_combination(IntegerVector undecided_pos, IntegerVector pos_possibility, 
//                       unsigned int p_tmax, unsigned int time_pos, int hap_min_pos) {
//   //possible combination of the rest non-unique loci
//   IntegerVector location(pos_possibility.size());
//   IntegerVector location_len(pos_possibility.size());
//   unsigned int num = 0;
//   for(unsigned int i = 0; i < pos_possibility.size(); ++i)
//     if(time_pos - hap_min_pos <= undecided_pos[i] && undecided_pos[i] < time_pos + p_tmax - hap_min_pos) {
//       location(num) = undecided_pos[i];
//       location_len(num++) = pos_possibility[i];
//     }
//     IntegerMatrix combination = call_cart_product(location_len[Range(0, num - 1)]);
//     List ls = List::create(
//       Named("combination") = combination, // possible comb
//       Named("num") = num, // number of comb sites at time t
//       Named("location") = location[Range(0, num - 1)]); // undecided site at time t [here assume alignment starts from 0]
//     return(ls);
// }

// 
// List limit_comb(IntegerMatrix combination, List hidden_states, IntegerVector location,
//                 IntegerMatrix linkage_info, unsigned int num, unsigned int start_idx, unsigned int num_states) {
//   unsigned int i, j, k, idx, m;
//   unsigned int n_observation = linkage_info.nrow();
//   IntegerMatrix sub_hap(NUM_CLASS, num);
//   IntegerMatrix sub_link = linkage_info(_, Range(start_idx, start_idx + num - 1));
//   IntegerVector exclude(num_states);
//   int count, linkage_len, all_excluded;
//   linkage_len = num/2;
//   
//   for(i = 0 ; i < n_observation; ++i)
//     for (j = 0; j < num; ++j)
//      Rcout << sub_link(i, j) << "\t";
//   Rcout << "\n";
//   all_excluded = num_states;
//   while (all_excluded == num_states) {
//     all_excluded = 0;
//     Rcout << "linkage length " << linkage_len << "\n"; //actual linkage length + 1
//     for (m = 0; m < num_states; ++m) {
//       exclude(m) = 0;
//       IntegerVector comb = combination(m, _);
//       count = 0;
//       for (k = 0; k < NUM_CLASS; ++k) {
//         for (j = 0; j < num; ++j) {
//          
//           IntegerMatrix hidden = hidden_states[location[j]];
//           idx = comb[j];
//           sub_hap(k, j) = hidden(idx, k);
//           Rcout << sub_hap(k, j)  << "\t";
//         }
//         Rcout << "\n";
//         for (i = 0; i < n_observation; i++) {
//           int flag = 0;
//           for (j = 0; j < num - 1; ++j) {
//             // if (sub_link(i, j) != -1 && sub_link(i, j) != 4)
//             if (sub_hap(k, j) == sub_link(i, j) && sub_hap(k, j + 1) == sub_link(i, j + 1))
//               flag++;
//           }
//           if (flag >= linkage_len) {
//             count++;
//             break;
//           }
//         }
//       }
//       if (count != NUM_CLASS) {
//         exclude(m) = 1;
//         all_excluded++;
//       }
//     }
//     linkage_len--;
//   }
//   List out = List::create(
//     Named("num_states") = num_states - all_excluded,
//     Named("exclude") = exclude);
//   return(out);
// }

// get the overlapped variational region at each time t, notice T1: 1-30, T2: 10-20, T3: 15-40/22-45, then T3 has overlap with T1 but not T2
// List get_overlap(IntegerVector p_tmax, IntegerVector time_pos, IntegerVector num_states,
//                  IntegerVector undecided_pos, unsigned int t_max, int hap_min_pos)
// {
//   unsigned int start_t = 0;
//   // get the first t which has variation
//   for (unsigned int t = 0; t < t_max; ++t)
//     if(num_states[t] > 1) {
//       start_t = t;
//       break;
//     }
//     List overlapped(t_max);
//     List location(t_max);
//     IntegerVector overlapped_idx(t_max);
//     unsigned int begin, end, end1, min;
//     
//     for (unsigned int t = 0; t < t_max; ++t) {
//       
//       if (num_states[t] == 1) {
//         overlapped[t] = -1;
//         overlapped_idx[t] = -1;
//         location[t] = -1;
//         continue;
//       }
//       if(t == start_t) {
//         overlapped[t] = -1;
//         overlapped_idx[t] = -1;
//         begin = time_pos[start_t] - hap_min_pos;
//         end = time_pos[start_t] + p_tmax[start_t] - hap_min_pos;
//         int num = 0;
//         IntegerVector location_t(undecided_pos.size());
//         for (unsigned int m = 0; m < undecided_pos.size(); ++m)
//           if (undecided_pos[m] >= begin && undecided_pos[m] < end)
//             location_t(num++) = undecided_pos[m];
//         location[start_t] = location_t[Range(0, num - 1)];
//         continue;
//       }
//       begin = time_pos[t] - hap_min_pos;
//       end = time_pos[t] + p_tmax[t] - hap_min_pos;
//       // store location
//       int num = 0;
//       IntegerVector location_t(undecided_pos.size());
//       for (unsigned int m = 0; m < undecided_pos.size(); ++m)
//         if (undecided_pos[m] >= begin && undecided_pos[m] < end)
//           location_t(num++) = undecided_pos[m];
//       location[t] = location_t[Range(0, num - 1)];
//       int len = 0;
//       int id_t = 0;
//       int index = 0;
//       // Rcout << "location" << location_t[num - 1] << "\n";
//       for (unsigned int t1 = 0; t1 < t; ++t1) {
//         IntegerVector last_location = location[t1];
//         // begin1 = time_pos[t1] - hap_min_pos;
//         end1 = last_location[last_location.size() - 1];
//         // Rcout << end1 << "\t";
//         // end1 = time_pos[t1] + p_tmax[t1] - hap_min_pos;
//         if(begin <= end1) {
//           min = end1;
//           // minimum overlapped region
//           if(location_t[num - 1] < end1)
//             min = location_t[num - 1];
//           // find the time t which has the longest coverage
//           if(min - location_t[0] > len) {
//             // Rcout << t << " has coverage with " << t1 << "\n";
//             len = min - location_t[0];
//             id_t = min;
//             index = t1;
//           }
//         }
//       }
//       // Rcout<< "\n" << begin << "\t" << id_t << "\n";
//       int count = 0;
//       IntegerVector position(undecided_pos.size());
//       for (unsigned int m = 0; m < undecided_pos.size(); ++m) 
//         if (undecided_pos[m] >= begin && undecided_pos[m] <= id_t)
//           position(count++) = undecided_pos[m];
//       // Rcout << position << "\n";
//       if(count) { 
//         overlapped[t] = position[Range(0, count - 1)];
//         overlapped_idx[t] = index;
//       }
//     }
//     
//     List overlap = List::create(
//       Named("location") = location,
//       Named("overlapped") = overlapped,
//       Named("overlapped_id") = overlapped_idx, 
//       Named("start_t") = start_t);
//     return(overlap);
// }
// 
// List limit_comb_t0(IntegerMatrix combination, List hidden_states, IntegerVector location,
//                    IntegerMatrix linkage_info, unsigned int num, unsigned int start_idx, unsigned int num_states) {
//   unsigned int i, j, k, idx, m;
//   unsigned int n_observation = linkage_info.nrow();
//   IntegerMatrix sub_hap(NUM_CLASS, num);
//   IntegerMatrix sub_link = linkage_info(_, Range(start_idx, start_idx + num - 1));
//   IntegerVector exclude(num_states);
//   int count, linkage_len, all_excluded;
//   linkage_len = num/2; // change the linkage length to be the length appears in the read
//   int cut_off;
//   all_excluded = num_states;
//   while (all_excluded == num_states) {
//     cut_off = NUM_CLASS;
//     // Rcout << "linkage length " << linkage_len << "\n"; //actual linkage length + 1
//     while (cut_off >= 1 && all_excluded == num_states) {
//       all_excluded = 0;
//       for (m = 0; m < num_states; ++m) {
//         exclude(m) = 0;
//         IntegerVector comb = combination(m, _);
//         // Rcout << comb << "\t\t";
//         count = 0;
//         for (k = 0; k < NUM_CLASS; ++k) {
//           // Rcout << "k" << k << "\n";
//           for (j = 0; j < num; ++j) {
//             IntegerMatrix hidden = hidden_states[location[j]];
//             idx = comb[j];
//             sub_hap(k, j) = hidden(idx, k);
//             // Rcout << sub_hap(k, j) << "\t";
//           }
//           // Rcout << "read" << "\n";
//           for (i = 0; i < n_observation; i++) {
//             int flag = 0;
//             for (j = 0; j < num - 1; ++j) {
//               // Rcout << sub_link(i, j) << "\t" << sub_link(i, j + 1);
//               // if (sub_link(i, j) != -1 && sub_link(i, j) != 4)
//               if (sub_hap(k, j) == sub_link(i, j) && sub_hap(k, j + 1) == sub_link(i, j + 1))
//                 flag++;
//             }
//             // Rcout << "\n" << flag << "\n" ;
//             if (flag >= linkage_len) {
//               count++;
//               break;
//             }
//           }
//         }
//         // Rcout << "count "<< count << "\n";
//         if (count != cut_off) {
//           exclude(m) = 1;
//           all_excluded++;
//         }
//       }
//       cut_off--;
//     }
//     linkage_len--;
//   }
//   // Rcout << exclude << "\n";
//   List out = List::create(
//     Named("num_states") = num_states - all_excluded,
//     Named("exclude") = exclude);
//   return(out);
// }
// 
// 
// template <typename T>
// inline bool approx_equal_cpp(const T& lhs, const T& rhs, double tol = 0.00000001) {
//   return arma::approx_equal(lhs, rhs, "absdiff", tol);
// }
// 
// arma::mat unique_rows(const arma::mat& m) {
//   
//   arma::uvec ulmt = arma::zeros<arma::uvec>(m.n_rows);
//   
//   for (arma::uword i = 0; i < m.n_rows; i++) {
//     for (arma::uword j = i + 1; j < m.n_rows; j++) {
//       if (approx_equal_cpp(m.row(i), m.row(j))) { ulmt(j) = 1; break; }
//     }
//   }
//   
//   return m.rows(find(ulmt == 0));
//   
// }
// 
// List unique_map(const Rcpp::IntegerVector & v)
// {
//   // Initialize a map
//   std::map<double, int> Elt;
//   Elt.clear();
//   
//   // Count each element
//   for (int i = 0; i != v.size(); ++i)
//     Elt[ v[i] ] += 1;
//   
//   // Find out how many unique elements exist... 
//   int n_obs = Elt.size();
//   // If the top number, n, is greater than the number of observations,
//   // then drop it.  
//   int n = n_obs;
//   
//   // Pop the last n elements as they are already sorted. 
//   // Make an iterator to access map info
//   std::map<double,int>::iterator itb = Elt.end();
//   // Advance the end of the iterator up to 5.
//   std::advance(itb, -n);
//   
//   // Recast for R
//   NumericVector result_vals(n);
//   NumericVector result_keys(n);
//   
//   unsigned int count = 0;
//   // Start at the nth element and move to the last element in the map.
//   for (std::map<double,int>::iterator it = itb; it != Elt.end(); ++it) {
//     // Move them into split vectors
//     result_keys(count) = it->first;
//     result_vals(count) = it->second;
//     count++;
//   }
//   return List::create(Named("lengths") = result_vals,
//                       Named("values") = result_keys);
// }
// 
// IntegerMatrix unique_overlap(IntegerVector overlapped, IntegerVector exclude_last, IntegerMatrix overlap_comb, IntegerVector overlap_loci, 
//                             unsigned int overlap_new_states, unsigned int overlap_num_states) {
//   unsigned int i, m;
//   // unsigned int n_observation = linkage_info.nrow();
//   //decide the first t 
//   unsigned int overlap_len = overlapped.size();
//   // limit the space based on the last limition first
//   int start_overlap = 0;
//   for(i = 0; i < overlap_loci.size(); ++i)
//     if (overlap_loci[i] == overlapped[0]){
//       start_overlap = i;
//       break;
//     }
//     
//     //find the unique states
//     IntegerMatrix comb_last(overlap_new_states, overlap_len);
//     unsigned int count = 0;
//     for (m = 0; m < overlap_num_states; ++m)
//       if(!exclude_last[m]) {
//         IntegerVector tmp = overlap_comb(m, _);
//         comb_last(count++, _) = tmp[Range(start_overlap, start_overlap + overlap_len - 1)];
//       }
//     arma::mat out = unique_rows(as<arma::mat>(comb_last));
//     return(wrap(out));
// }
// 
// IntegerMatrix new_combination(List hmm_info, IntegerVector location, IntegerVector overlapped, IntegerVector exclude_last, IntegerMatrix overlap_comb, 
//                      IntegerVector overlap_loci, IntegerMatrix linkage_info, unsigned int overlap_new_states, unsigned int overlap_num_states) {
//   
//   IntegerMatrix first_comb = unique_overlap(overlapped, exclude_last, overlap_comb, overlap_loci, 
//                                             overlap_new_states, overlap_num_states);
//   //find combinatio of the rest position(include 1 overlap to make sure the linkage)
//   List hidden_states = hmm_info["hidden_states"];
//   IntegerVector pos_possibility = hmm_info["pos_possibility"];
//   IntegerVector undecided_pos = hmm_info["undecided_pos"];
//   unsigned int i, j, m, w, k;
//   int overlap_len = overlapped.size();
//   
//   IntegerVector left_loci = location[Range(overlap_len - 1, location.size() - 1)];
//   int count = 0;
//   int len = left_loci.size();
//   IntegerVector left_possible(len);
//   for(i = 0; i < undecided_pos.size(); ++i) {
//     if (undecided_pos[i] >= left_loci[0] && undecided_pos[i] <= left_loci[len - 1]) {
//       left_possible[count++] = pos_possibility[i];
//     }
//   }
//   
//   IntegerMatrix combination = call_cart_product(left_possible[Range(0, count - 1)]);
//   // get the appeared possiblilities at the overlapped position
//   IntegerVector last_col = first_comb(_, first_comb.ncol() - 1);
//   List first_uni = unique_map(last_col); // start might not from 0 (e.g. ailgnment starts from 2)
//   IntegerVector n1 = first_uni["lengths"];
//   IntegerVector allowed = first_uni["values"];
//   // IntegerVector allowed = unique(last_col);
//   IntegerVector first_col = combination(_, 0);
//   IntegerVector exist = unique(first_col);
//   List exclude_info(2);
//   int flag = 0;
//   int num = 0;
//   IntegerMatrix new_combination(combination.nrow(), combination.ncol());
//   unsigned int start_idx = 0;
//   for(i = 0; i < undecided_pos.size(); ++i)
//     if (undecided_pos[i] == left_loci[0]) {
//       start_idx = i;
//       break;
//     }
//   Rcout <<  "exists: " << exist << "\n";
//   Rcout << "allowed: " << n1 << "\n";
//   Rcout << allowed << "\n";
//   for(m = 0; m < exist.size(); ++m)
//     for(w = 0; w < allowed.size(); ++w)
//       if(exist[m] == allowed[w]) {
//         num++;
//         }
//   if(num != exist.size())
//     flag = 1;
//   num = 0;
//   if(flag) {
//     Rcout << "different unique at same col" << "\n";
//     for(m = 0; m < combination.nrow(); ++m)
//       for(w = 0; w < allowed.size(); ++w)
//         if(allowed(w) == first_col(m)) {
//           new_combination(num++, _) = combination(m, _);
//         }
//     exclude_info = limit_comb_t0(new_combination(Range(0, num - 1), _), hidden_states, left_loci, linkage_info, combination.ncol(), start_idx, num);
//   } else {
//     exclude_info = limit_comb_t0(combination, hidden_states, left_loci, linkage_info, combination.ncol(), start_idx, combination.nrow());
//   }
// 
//   // // Now give the limited combination
//   int num_states = exclude_info["num_states"];
//   IntegerVector exclude = exclude_info["exclude"];
//   Rcout << exclude << "\n";
//   Rcout << "left states:" << num_states << "\n";
//   Rcout << "first states " << first_comb.nrow() << "\n";
//   IntegerMatrix next_comb(num_states, combination.ncol());
//   count = 0;
//   // Now combine first and second part
//   if(flag) {
//     for(m = 0; m < num; ++m) {
//       if(!exclude[m])
//         next_comb(count++, _) = new_combination(m, _);
//     }
//   } else {
//     for(m = 0; m < combination.nrow(); ++m) {
//       if(!exclude[m])
//         next_comb(count++, _) = combination(m, _);
//     }
//   }
//   for(m = 0; m < count; ++m) {
//     for(k = 0; k < next_comb.ncol(); ++k) {
//       Rcout << next_comb(m, k) << "\t";
//     }
//     Rcout << "\n";}
//   IntegerVector new_col = next_comb(_, 0);
//   List second_uni = unique_map(new_col); // start might not from 0 (e.g. ailgnment starts from 2)
//   IntegerVector n2 = second_uni["lengths"];
//   IntegerVector possible = second_uni["values"];
//   int all = 0;
// 
//   for(m = 0; m < n2.size(); ++m)
//     all += n2[m] * n1[m];
//   IntegerMatrix final_comb(all, location.size());
//   Rcout << all << "\n";
//   all = 0;
//   
//   for(k = 0; k < last_col.size(); ++k)
//     for(w = 0; w < count; ++w) {
//       if(new_col(w) == last_col(k)) {
//         for(j = 0; j < overlap_len; ++j) {
//           Rcout << first_comb(k, j) << "\t";
//           final_comb(all, j) = first_comb(k, j);
//         }
//         for(i = 1; i < len; ++i) {
//           Rcout << next_comb(w, i) << "\t";
//           final_comb(all, i + overlap_len - 1) = next_comb(w, i);
//         }
//         all++;
//         Rcout << "\n";
//       }
//     }
//  
//   return(final_comb);
// }
// function to do left join by the last column of first and first of second

// IntegerMatrix left_join(IntegerMatrix first, IntegerMatrix second, int nrow, int ncol) {
//   IntegerMatrix final_comb(nrow, ncol);
//   int all = 0;
//   for(int k = 0; k < first.nrow(); ++k) {
//     for(int w = 0; w < second.nrow(); ++w) {
//       if(second(w, 0) == first(k, first.ncol() - 1)) {
//           for(int j = 0; j < first.ncol(); ++j) {
//           Rcout << first(k, j) << "\t";
//            final_comb(all, j) = first(k, j);
//           }
//           for(int i = 1; i < second.ncol(); ++i) {
//           Rcout << second(w, i) << "\t";
//           final_comb(all, i + first.ncol() - 1) = second(w, i);
//           }
//           all++;
//           Rcout << "\n";
//         }
//     }
//   }
//   return(final_comb);
// }
// 
// 
// IntegerMatrix fill_all_hap(List hidden_states, unsigned int hap_length, IntegerVector n_row) {
//   unsigned int j, k;
//   IntegerMatrix haplotype(NUM_CLASS, hap_length);
//   IntegerVector nuc_j(NUM_CLASS);
//   for(j = 0; j < hap_length; ++j)
//     if(n_row(j) == 1) { // fill the loci with only 1 possibility, can be optimized by using the part has been filled and add/trim
//       nuc_j = hidden_states[j];
//       for(k = 0; k < NUM_CLASS; ++k)
//         haplotype(k, j) = nuc_j[k];  
//     }
//     return haplotype;
// }
// 
// IntegerMatrix make_hap(List hidden_states, IntegerMatrix haplotype, IntegerVector location, unsigned int p_tmax,
//                        IntegerVector combination, unsigned int time_pos, unsigned int num, int hap_min_pos) {
//   unsigned int j, k, idx;
//   
//   for(j = 0; j < num; ++j) {
//     IntegerMatrix hidden = hidden_states[location[j]];
//     //Rcout << hidden << "\n";
//     idx = combination[j];
//     //Rcout << idx << "\n";
//     for(k = 0; k < NUM_CLASS; ++k)
//       haplotype(k, location[j]) = hidden(idx, k);
//   }
//   return(haplotype(_, Range(time_pos - hap_min_pos, time_pos + p_tmax - hap_min_pos - 1)));
// }

// 
// List full_hap_new (List hmm_info, IntegerMatrix linkage_info, unsigned int hap_length, int hap_min_pos) {
//   List hidden_states = hmm_info["hidden_states"];
//   IntegerVector num_states = hmm_info["num_states"];
//   IntegerVector time_pos = hmm_info["time_pos"];
//   IntegerVector p_tmax = hmm_info["p_tmax"];
//   IntegerVector n_row = hmm_info["n_row"];
//   IntegerVector pos_possibility = hmm_info["pos_possibility"];
//   IntegerVector undecided_pos = hmm_info["undecided_pos"];
//   unsigned int t_max = hmm_info["t_max"];
//   unsigned int t, m;
//   List full_hap(t_max);
//   List comb(t_max);
//   IntegerMatrix hap = fill_all_hap(hidden_states, hap_length, n_row);
//   IntegerVector new_num_states(t_max);
//   List overlap_info = get_overlap(p_tmax, time_pos, num_states, undecided_pos, t_max, hap_min_pos);
//   List overlapped = overlap_info["overlapped"];
//   IntegerVector overlapped_id = overlap_info["overlapped_id"];
//   int start_t = overlap_info["start_t"];
//   List loci = overlap_info["location"];
//   
//   //start t info
//   List comb_info_t0 = find_combination(undecided_pos, pos_possibility, p_tmax[start_t], time_pos[start_t], hap_min_pos);
//   IntegerVector location = comb_info_t0["location"];
//   IntegerMatrix combination = comb_info_t0["combination"];
//   unsigned int num = comb_info_t0["num"];
//   List t0 = limit_comb_t0(combination, hidden_states, location, linkage_info, num, 0, num_states[start_t]);
//   IntegerVector exclude = t0["exclude"];
//   IntegerVector exclude_last = exclude;
//   IntegerMatrix comb_in = combination; 
//   // get the states  
//   for(t = 0; t < t_max; ++t) {
//     List full_hap_t;
//       if(num_states[t] != 1) {
//         int count = 0;
//         if(t == start_t) {
//           // decide start_t first
//           new_num_states[t] = t0["num_states"];
//           IntegerMatrix new_comb(new_num_states[t], combination.ncol());
//           full_hap_t = List(new_num_states[t]);
//           for(m = 0; m < num_states[t]; ++m)
//             if(!exclude[m]) {
//               IntegerMatrix haplotype = make_hap(hidden_states, hap, location, p_tmax[t], combination(m, _), time_pos[t], num, hap_min_pos);
//               new_comb(count, _) = combination(m, _);
//               full_hap_t(count++) = haplotype;
//             }
//           comb[t] = new_comb;
//           Rcout << "start done" << "\n";
//         }
//         else {
//           int identical = 0;
//           int last_t = overlapped_id[t];
//           IntegerVector overlapped_t = overlapped[t];
//           IntegerVector loci_lastt = loci[last_t];
//           IntegerVector loci_currt = loci[t];
//           int old_state;
//           Rcout << t << "\t" << last_t << "\n";
//           Rcout << "overlapped: " << overlapped_t << "\n";
//           Rcout << "loci_lastt: " << loci_lastt << "\n";
//           Rcout << "loci_currt: " << loci_currt << "\n";
//           
//           if(loci_lastt[0] <= loci_currt[0] && loci_lastt[loci_lastt.size() - 1] >= loci_currt[loci_currt.size() - 1]) {
//             if(loci_lastt.size() > loci_currt.size()) { // if current is in its overlap
//               // get the unique overlapped combination from the last t
//               if(last_t == start_t) {
//                 exclude_last = IntegerVector(exclude.size());
//                 exclude_last = exclude;
//                 old_state = num_states[last_t];
//                 comb_in = combination;
//               }
//               else {
//                 exclude_last = IntegerVector(new_num_states[last_t]);
//                 for(m = 0; m < new_num_states[last_t]; ++m)
//                   exclude_last[m] = 0;
//                 old_state = new_num_states[last_t];
//                 IntegerMatrix tmp = comb[last_t];
//                 comb_in = IntegerMatrix(tmp.nrow(), tmp.ncol());
//                 comb_in = tmp;
//               }
//               Rcout << "last t:\t" << new_num_states[last_t] << "\told\t" << old_state << "\n";
//               IntegerMatrix new_comb = unique_overlap(overlapped_t, exclude_last, comb_in, loci_lastt, new_num_states[last_t], old_state);
//               new_num_states[t] = new_comb.nrow();
//               comb[t] = new_comb;
//               full_hap_t = List(new_num_states[t]);
//               for(m = 0; m < new_num_states[t]; ++m) {
//                 IntegerMatrix haplotype = make_hap(hidden_states, hap, loci_currt, p_tmax[t], new_comb(m, _), time_pos[t], loci_currt.size(), hap_min_pos);
//                 full_hap_t(count++) = haplotype;
//               }
//             } else if (loci_lastt.size() == loci_currt.size()) {
//               for(m = 0; m < loci_lastt.size(); ++m)
//                 if(loci_lastt[m] != loci_currt[m]) {
//                   identical = 1;
//                   break;
//                 }
//                 if(!identical) {
//                   Rcout << "same sites\n";
//                   comb[t] = comb[last_t];
//                   new_num_states[t] = new_num_states[last_t];
//                   full_hap_t = List(new_num_states[t]);
//                   full_hap_t = full_hap[last_t];
//                 }
//             }
//           } 
//           else {
//             if(last_t == start_t) {
//               exclude_last = IntegerVector(exclude.size());
//               exclude_last = exclude;
//               old_state = num_states[last_t];
//               comb_in = combination;
//             }
//             else {
//               exclude_last = IntegerVector(new_num_states[last_t]);
//               for(m = 0; m < new_num_states[last_t]; ++m)
//                 exclude_last[m] = 0;
//               old_state = new_num_states[last_t];
//               IntegerMatrix tmp = comb[last_t];
//               comb_in = IntegerMatrix(tmp.nrow(), tmp.ncol());
//               comb_in = tmp;
//             }
//             IntegerMatrix new_comb = new_combination(hmm_info, loci_currt, overlapped_t, exclude_last, comb_in,
//                               loci_lastt, linkage_info, new_num_states[last_t], old_state);
//             new_num_states[t] = new_comb.nrow();
//             comb[t] = new_comb;
//             full_hap_t = List(new_num_states[t]);
//             for(m = 0; m < new_num_states[t]; ++m) {
//               IntegerMatrix haplotype = make_hap(hidden_states, hap, loci_currt, p_tmax[t], new_comb(m, _), time_pos[t], loci_currt.size(), hap_min_pos);
//               full_hap_t(count++) = haplotype;
//             }
//           }
//         }
//       } 
//       else {
//         full_hap_t = List(1);
//         full_hap_t[0] = hap(_, Range(time_pos[t] - hap_min_pos, time_pos[t] + p_tmax[t] - hap_min_pos - 1));
//         new_num_states[t] = 1;
//         comb[t] = -1;
//       }
//       Rcout << "new no. states: " << new_num_states[t] << "\n";
//       full_hap[t] = full_hap_t;
//   }
//   
//   List out = List::create(
//     Named("full_hap") = full_hap,
//     Named("new_num_states") = new_num_states, 
//     Named("combination") = comb);
//   
//   return(out);
// }
// List limit_comb_rest(IntegerVector overlapped, IntegerVector exclude_last, IntegerMatrix overlap_comb, IntegerVector overlap_loci, 
//                      IntegerMatrix combination, List hidden_states, IntegerVector location, IntegerMatrix linkage_info, 
//                      unsigned int num, unsigned int start_idx, unsigned int num_states, unsigned int overlap_num_states) {
//   unsigned int i, j, k, idx, m, w;
//   unsigned int n_observation = linkage_info.nrow();
//   //decide the first t 
//   unsigned int overlap_len = overlapped.size();
//   // limit the space based on the last limition first
//   int start_overlap = 0;
//   
//   for(i = 0; i < overlap_loci.size(); ++i) {
//     if (overlap_loci[i] == overlapped[0]){
//       start_overlap = i;
//       break;
//     }
//   }
//   Rcout << start_overlap << "\n";
//   IntegerVector exclude(num_states);
//   for (m = 0; m < overlap_num_states; ++m)
//     if(exclude_last[m]) {
//       IntegerVector comb_last(overlap_len);
//       IntegerVector tmp = overlap_comb(m, _);
//       comb_last = tmp[Range(start_overlap, start_overlap + overlap_len - 1)];
//       Rcout << comb_last << "\n";
//       for(w = 0; w < num_states; ++w) {
//         exclude[w] = 1;
//         IntegerVector comb(overlap_len);
//         IntegerVector tmp2 = combination(w, _);
//         comb = tmp2[Range(0, overlap_len - 1)];
//         Rcout << comb << "\t";
//         // if the overlapped combination is the same, then exclude it as well, so this makes sure there is always one former stage transfer to the next stage
//         for(i = 0; i < overlap_len; ++i)
//           if(comb[i] != comb_last[i]){
//             exclude[w] = 0;
//             break;
//           }
//       }
//     }
//     Rcout << "first stage done" << "\n";
//     // further examine linkage info, if do not meet the linkage, remove as well
//     IntegerMatrix sub_hap(NUM_CLASS, num);
//     IntegerMatrix sub_link = linkage_info(_, Range(start_idx, start_idx + num - 1));
//     int count, linkage_len, all_excluded;
//     linkage_len = num/2; // change the linkage length to be the length appears in the read
//     
//     all_excluded = num_states;
//     while (all_excluded == num_states) {
//       all_excluded = 0;
//       // Rcout << "linkage length " << linkage_len << "\n"; //actual linkage length + 1
//       for (m = 0; m < num_states; ++m) {
//         if(!exclude[m]) {
//           IntegerVector comb = combination(m, _);
//           // Rcout << comb << "\t\t";
//           count = 0;
//           for (k = 0; k < NUM_CLASS; ++k) {
//             for (j = 0; j < num; ++j) {
//               IntegerMatrix hidden = hidden_states[location[j]];
//               idx = comb[j];
//               sub_hap(k, j) = hidden(idx, k);
//             }
//             for (i = 0; i < n_observation; i++) {
//               int flag = 0;
//               for (j = 0; j < num - 1; ++j) {
//                 // if (sub_link(i, j) != -1 && sub_link(i, j) != 4)
//                 if (sub_hap(k, j) == sub_link(i, j) && sub_hap(k, j + 1) == sub_link(i, j + 1))
//                   flag++;
//               }
//               if (flag >= linkage_len) {
//                 count++;
//                 break;
//               }
//             }
//           }
//           if (count != NUM_CLASS) {
//             exclude(m) = 1;
//             all_excluded++;
//           }
//         } else {
//           all_excluded++;
//           }
//       }
//       linkage_len--;
//     }
//     List out = List::create(
//       Named("num_states") = num_states - all_excluded,
//       Named("exclude") = exclude);
//     return(out);
// }


// List trans_permit2(IntegerVector num_states, List hap_info, int t_max, IntegerVector undecided_pos, 
//                   IntegerVector time_pos, IntegerVector p_tmax, int hap_min_pos) {
//   List trans_permits(t_max - 1);
//   unsigned int t, j, m, w, k;
//   int end_t1, begin_t2, end_t2;
//   IntegerVector position(undecided_pos.size());
//   for (t = 0; t < t_max - 1; ++t) {
//     if(num_states[t + 1] != 1) {
//       end_t1 = time_pos[t] + p_tmax[t];
//       begin_t2 = time_pos[t + 1];
//       end_t2 = time_pos[t + 1] + p_tmax[t + 1];
//     
//       if(end_t1 > end_t2)
//         end_t1 = end_t2; // take the smaller one
//       List full_hap_t1 = hap_info(t);
//       List full_hap_t2 = hap_info(t + 1);
//       IntegerMatrix trans(num_states[t], num_states[t + 1]);
//       int count = 0; // number of overlapped variational sites
//       int count1 = 0;
//       
//       for(j = 0; j < undecided_pos.size(); ++j)
//         if (undecided_pos[j] + hap_min_pos < end_t1 && undecided_pos[j] >= time_pos[t]) {
//           count1++;
//           if (undecided_pos[j] + hap_min_pos >= begin_t2)
//             position[count++] = undecided_pos(j) + hap_min_pos;
//         }
//         for(j = 0; j < count; ++j){
//           Rcout << position[j] - begin_t2 << "\t" << position[j] - time_pos[t] << "\n";
//         }
//         for(m = 0; m < num_states[t]; ++m) {
//           IntegerMatrix hap_t1 = full_hap_t1(m);
//           for(w = 0; w < num_states[t + 1]; ++w) {
//             IntegerMatrix hap_t2 = full_hap_t2(w);
//             for(j = 0; j < count; ++j){
//               // Rcout << position[j] - begin_t2 << "\t" << position[j] - time_pos[t] << "\n";
//               for(k = 0; k < NUM_CLASS; ++k)
//                 if (hap_t2(position[j] - begin_t2, k) != hap_t1(position[j] - time_pos[t], k)) {
//                   
//                   trans(m, w) = 1; // represents m cannot transfer to w
//                   break;
//                 }}
//           }
//         }
//         trans_permits(t) = trans;
//     } else {
//       IntegerMatrix temp(num_states[t], num_states[t + 1]);
//       trans_permits(t) = temp;
//     }
//   }
//   return(trans_permits);
// }



