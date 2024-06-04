#include <mlpack/core.hpp>
#include <mlpack.hpp>
#include <iostream>
#include <cstdlib>
#include <cstdio>
using namespace std;

// int main(){
//     // Train a Hoeffding tree on random numeric data; predict labels on test data:

//     // All data and labels are uniform random; 10 dimensional data, 5 classes.
//     // Replace with a data::Load() call or similar for a real application.
//     typedef arma::uword Feat_elem_t;
//     typedef arma::Row<size_t> Res_t;
//     typedef arma::Col<Feat_elem_t> Feat_vec_t;
//     typedef arma::Mat<Feat_elem_t> Feat_mat_t;

//     Feat_mat_t data(10, 1000, arma::fill::randn);
//     Res_t labels = arma::randi<Res_t>(1000,arma::distr_param(0,4));
//     std::cout <<"lables: "<< labels << std::endl;
//     Feat_mat_t testdata(10,100,arma::fill::randn);
//     Res_t prediction_labels;
//     mlpack::HoeffdingTree tree;
//     tree.Train(data, labels,5);
//     tree.Classify(testdata,prediction_labels);
//     std::cout<<"pred 0: "<< arma::accu(prediction_labels == 0) << std::endl;
//     std::cout<<"pred 1: "<< arma::accu(prediction_labels == 1) << std::endl;
//     std::cout<<"pred 2: "<< arma::accu(prediction_labels == 2) << std::endl;
//     std::cout<<"pred 3: "<< arma::accu(prediction_labels == 3) << std::endl;
//     std::cout<<"pred 4: "<< arma::accu(prediction_labels == 4) << std::endl;
//     return 0;
// }

int main(){
    int* p = (int*)malloc(sizeof(int));
    *p = 1;
    printf("%d\n",*p);
    // int* q = new int();
    // *q = 2;
    // printf("%d\n",*q);
    return 0;
}