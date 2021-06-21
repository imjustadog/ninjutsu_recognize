#ifndef _SEQ_COMPARE_H
#define _SEQ_COMPARE_H

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

using namespace std;

class seq_compare
{
  private:
    vector<string> seq_list{"r60", "24bgiokr2", "q9qzqn3q", "w"};
    string current_seq = "";
  public:
    seq_compare();
    ~seq_compare();
    string find_seq(string current_gesture);
};

#endif


