#ifndef ML_UTIL_H
#define ML_UTIL_H

#include <mlpack/core.hpp>
#include <mlpack.hpp>
#include "concurrent_hashmap.hpp"
#include <boost/stacktrace.hpp>

typedef struct Features{
    int test = 1;
}Features;

typedef struct Statistics{
    double heat;
}Statistics;

inline const char *get_funcname(const char *src) {
    int status = 99;
    const char *f = abi::__cxa_demangle(src, nullptr, nullptr, &status);
    return f == nullptr ? src : f;
}

namespace my_ml{

enum feature_index : arma::uword{
    //control flow info
    F_PC,          
    //system info 
    F_CLIENT_ID,
    F_LOCAL_USED_RATE,  //assume limited local mem and infinite cxl mem
    
    F_FEATURE_NUM
};

// enum feature_index : arma::uword{
//     F_PAGE_ID,          
//     F_OP,
//     F_FUNCTION_ID,
//     F_SIZE,
//     F_ADDR,
//     F_PC,
//     F_FEATURE_NUM
// };
using Feat_elem_t = arma::uword;
using Pred_res_t = size_t;
using Feat_vec_t = arma::Col<Feat_elem_t>;
using Feat_mat_t = arma::Mat<Feat_elem_t>;

class HoeffdingTreeClassifier{
public:
    
    HoeffdingTreeClassifier(){

    }

    ~HoeffdingTreeClassifier() = default;

    void learn_one(const Features &x, uint64_t y){
        
    }

    void learn_one(arma::Col<Feat_elem_t> x, uint64_t y){
        tree.Train(x, y);
    }

    void learn_many(arma::Mat<Feat_elem_t> xs, arma::Row<Pred_res_t> ys){
        tree.Train(xs,ys,2);
    }

    Pred_res_t predict_one(const Features &x){
        //tree.Classify(x);
        return 0;
    }

    Pred_res_t predict_one(Feat_vec_t x){
        return tree.Classify(x);
    }

    void access_sample(Feat_vec_t x){
    }

    void access_sample(const Features &x){
        // auto ret = eq.enqueue(eq_item.page_id,std::move(eq_item));
        // if(ret.has_value()){
        //     auto v = ret.value();
        //     learn_one(x, v.is_hot_);
        // }
        
    }

private:
    // arma::Col<Feat_elem_t> Feat_struc_2_arma(Features &x){
        
    // }

    mlpack::HoeffdingTree<mlpack::GiniImpurity> tree;
    ConcurrentHashMap<void*, Statistics> cache; // <key, value, hash> key可能先转成uint64然后用std::hash
};
}




#endif