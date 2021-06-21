#include "svm_inference.hpp"

svm_inference::svm_inference(const char *basepath)
{
  DIR *dir;
  struct dirent *ptr;
  char file_path[100];
  string file_name;
  
  if((dir=opendir(basepath)) == NULL)
  {
    printf("Open model error...\n");
    exit(1);
  }

  while ((ptr=readdir(dir)) != NULL)
  { 
    if(ptr->d_type == 8)
    {
      strcpy(file_path, basepath);
      strcat(file_path, ptr->d_name);
      file_name = string(ptr->d_name);
      file_name = file_name.substr(0, file_name.rfind(".")); 
      ninjutsu_model[file_name] = svm_load_model(file_path);
    }
  }
}

svm_inference::~svm_inference()
{
  for(auto model:ninjutsu_model)
  {
    svm_free_and_destroy_model(&(model.second));
  }
}

void svm_inference::do_svm_preprocess(vector<double> data_input)
{
  int i;
  int attr_size = data_input.size();
  x_space = (struct svm_node *)malloc(((attr_size + 1))*sizeof(struct svm_node));

  for(i = 0;i < attr_size;i++)
  {
    x_space[i].index = i + 1;
    x_space[i].value = data_input[i];
  }
  x_space[i].index = -1;
  x_space[i].value = 0;
}


string svm_inference::do_svm_inference()
{
  string res = "";

  double predict_label;
  for(auto model:ninjutsu_model)
  {
    predict_label = svm_predict(model.second,x_space);
    if(predict_label > 0) res = model.first;
  }
  cout << "gesture: " << res << endl;

  return res;
}

void svm_inference::do_svm_postprocess()
{
  free(x_space);
}

