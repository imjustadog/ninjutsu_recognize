#include "seq_compare.hpp"

seq_compare::seq_compare()
{
}

seq_compare::~seq_compare()
{
}

string seq_compare::find_seq(string current_gesture)
{
  int i, j;
  string res = "";

  if(current_gesture == "") return res;
  if(current_gesture.back() == current_seq.back()) return res;
  else
  {
    current_seq += current_gesture;
    for(auto str:seq_list)
    {
      i = str.size() - 1;
      for(j = current_seq.size() - 1;(i >= 0) && (j >= 0);j--)
      {
        if(str[i] == current_seq[j]) i--;
      }
      if(i < 0)
      {
        current_seq = "";
        res = str;
        break;
      }
    }
  }
  if(res != "") cout << "ninjutsu: " << res << endl;
  return res;
}
