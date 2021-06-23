#ifndef _SVM_TRAIN_H
#define _SVM_TRAIN_H

#include <vector>
using namespace std;

void do_svm_train(char model_file_name[], vector<vector<double>> svm_input);
void do_svm_predict(char model_file_name[], vector<double> svm_predict_input);

#endif


