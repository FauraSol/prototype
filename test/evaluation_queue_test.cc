#include "ml_util.hpp"
#include "log.hpp"
// #include "../src/include/cmdline.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
using std::vector, std::string, std::cout, std::endl,std::stringstream,std::ifstream;
using my_ml::EQ_item, my_ml::EvaluationQueue;
int main(int argc, char* argv[]){
    // cmdline::parser parser;
    // parser.parse(argc,argv);
    // parser.add<string>("filename", 0, "filename", false, "");

    EvaluationQueue<uint64_t> eq;
    ifstream file("/home/zsq/Rcmp/preprocess.csv");
    string line;
    while(std::getline(file,line)){
        stringstream ss(line);
        string cell;
        vector<string> row;
        EQ_item eq_item;
        while(std::getline(ss,cell,',')){
            row.push_back(cell);
        }
        assert(row.size() == 7);
        eq_item.page_id = std::stoull(row[0]);
        eq_item.last_access_ = std::stoull(row[1]);
        eq_item.op = std::stoi(row[2]);
        eq_item.function_id = std::stoi(row[3]);
        eq_item.size = std::stoull(row[4]);
        eq_item.addr = std::stoull(row[5]);
        eq_item.pc = std::stoull(row[6]);
        auto ret = eq.enqueue(eq_item.page_id,std::move(eq_item));
        if(ret.has_value()){
            auto v = ret.value();
            cout << "page id: " << v.page_id << \
                " heatness: " << v.heatness_ << \
                " is hot?: " << v.is_hot_ << endl;
        }
    }
    return 0;
}
//page_id,n_instr,operation,function_id,size,address,pc
//g++ evaluation_queue_test.cc -o evaluation_queue_test -std=c++17 -O0 -g -Wall -Wextra -I../include -I../../include