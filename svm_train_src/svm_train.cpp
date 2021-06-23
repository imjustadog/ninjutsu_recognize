#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "svm.h"
#include "svm_train.h"
#include <map>
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))


struct svm_parameter param;		// set by parse_command_line
struct svm_problem prob;		// set by read_problem
struct svm_node *x_space;
struct svm_model *model;

void do_svm_predict(char model_file_name[], vector<double> svm_predict_input)
{
  int i, j;
  double predict_label;

  struct svm_model* model_saved;
  model_saved = svm_load_model(model_file_name);

  int attr_size = svm_predict_input.size();
  x_space = Malloc(struct svm_node, (attr_size + 1));

  for(j = 0;j < attr_size;j++)
  {
    x_space[j].index = j + 1;
    x_space[j].value = svm_predict_input[j];
  }
  x_space[j].index = -1;
  x_space[j].value = 0;
  
  predict_label = svm_predict(model_saved,x_space);

  printf("predict:%f\n",predict_label);

  //svm_free_and_destroy_model(&model_saved);
  //free(x_space);
}


void do_svm_train(char model_file_name[], vector<vector<double>> svm_input)
{
  int i, j;
  const char *error_msg;
  int input_size = svm_input.size();
  int attr_size = svm_input[0].size();
  double input_label[input_size] = {1};


  param.svm_type = ONE_CLASS;
  param.kernel_type = RBF;
  param.degree = 3;
  param.gamma = 1.0 / attr_size;	// 1/num_features
  param.coef0 = 0;
  param.nu = 0.05;
  param.cache_size = 100;
  param.C = 1;
  param.eps = 1e-3;
  param.p = 0.1;
  param.shrinking = 1;
  param.probability = 0;
  param.nr_weight = 0;
  param.weight_label = NULL;
  param.weight = NULL;
  
  prob.l = input_size;
  prob.y = input_label;
  prob.x = Malloc(struct svm_node *,prob.l);
  x_space = Malloc(struct svm_node, prob.l * (attr_size + 1));
  
  for(i = 0;i < input_size;i++)
  {
    for(j = 0;j < attr_size; j++)
    {
      x_space[i * (attr_size + 1) + j].index = j + 1;
      x_space[i * (attr_size + 1) + j].value = svm_input[i][j];
    }
    x_space[i * (attr_size + 1) + j].index = -1;
    x_space[i * (attr_size + 1) + j].value = 0;

    prob.x[i] = &x_space[i * (attr_size + 1)];
  }

  error_msg = svm_check_parameter(&prob,&param);

  if(error_msg)
  {
    fprintf(stderr,"ERROR: %s\n",error_msg);
    exit(1);
  }

  else
  {
    printf("param ok\n");
    model = svm_train(&prob,&param);
    if(svm_save_model(model_file_name,model))
    {
      fprintf(stderr, "can't save model to file %s\n", model_file_name);
      exit(1);
    }
    printf("save ok\n");
    svm_free_and_destroy_model(&model);
  }
  svm_destroy_param(&param);
  free(x_space);
  free(prob.x);
  return ;
}
