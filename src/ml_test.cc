#include "ml_util.hpp"
#include "evaluation_queue.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
using namespace my_ml;
using namespace std;
int main(){
    EvaluationQueue<Feat_elem_t> eq;
    ifstream file("/home/zsq/Rcmp/preprocess.csv");
    string line;
    vector<Pred_res_t> mem_pred;
    vector<Pred_res_t> mem_true;
    Feat_mat_t init_data(F_FEATURE_NUM,1,arma::fill::randn);
    arma::Row<Pred_res_t> init_label = arma::randi<arma::Row<Pred_res_t>>(1,arma::distr_param(0,1));
    HoeffdingTreeClassifier clf;
    clf.learn_many(init_data,init_label);
    int cnt = 0;
    while(std::getline(file,line)){
        stringstream ss(line);
        string cell;
        vector<string> row;
        EQ_item eq_item;
        Feat_vec_t feat_vec(F_FEATURE_NUM);
        while(std::getline(ss,cell,',')){
            row.push_back(cell);
        }
        assert(row.size() == 7);
        feat_vec[F_PAGE_ID] = static_cast<Feat_elem_t> (std::stoull(row[0]));
        feat_vec[F_OP] = static_cast<Feat_elem_t> (std::stoi(row[2]));
        feat_vec[F_FUNCTION_ID] = static_cast<Feat_elem_t> (std::stoi(row[3]));
        feat_vec[F_SIZE] = static_cast<Feat_elem_t> (std::stoull(row[4]));
        feat_vec[F_ADDR] = static_cast<Feat_elem_t> (std::stoull(row[5]));
        feat_vec[F_PC] = static_cast<Feat_elem_t> (std::stoull(row[6]));
        Pred_res_t y_pred = clf.predict_one(feat_vec);
        mem_pred.push_back(y_pred);
        auto ret = eq.enqueue(feat_vec[F_PAGE_ID],std::move(feat_vec));
        if(ret.has_value()){
            auto v = ret.value();
            cout << "page id: " << v.feat_vec[F_PAGE_ID] << \
                " heatness: " << v.heatness_ << \
                " is hot?: " << v.is_hot_ << endl;
            clf.learn_one(v.feat_vec,v.is_hot_);
            mem_true.push_back(v.is_hot_);
        }
    }
    int n = mem_true.size();
        int right = 0;
        for(int i=100;i<n;++i){
            if(mem_pred[i] == mem_true[i]){
                right++;
            }
        }
        cout << "total num: " << n << " right: " << right << endl;
        cout << "accuacy: " << right * 1.0 / n << endl;
    return 0;
}