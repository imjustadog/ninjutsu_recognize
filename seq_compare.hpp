#ifndef _SEQ_COMPARE_H
#define _SEQ_COMPARE_H

#include <iostream>
#include <unordered_map>
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
    unordered_map<string, int> seq_map{{"r60", 0}, {"24bgiokr2", 0}, {"q9qzqn3q", 0}, {"w", 0}};
    string current_seq = "";
  public:
    seq_compare();
    ~seq_compare();
    string find_seq(string current_gesture);
};

#endif


