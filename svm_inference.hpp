#ifndef _SVM_INFERENCE_H
#define _SVM_INFERENCE_H

#include "svm.h"
#include <vector>
#include <unordered_map>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

using namespace std;

class svm_inference
{
  private:
    struct svm_node *x_space;
    unordered_map<string, svm_model *> ninjutsu_model;
  public:
    svm_inference(const char *basepath);
    ~svm_inference();
    void do_svm_preprocess(vector<double> data_input); 
    string do_svm_inference();
    void do_svm_postprocess();
};

#endif


