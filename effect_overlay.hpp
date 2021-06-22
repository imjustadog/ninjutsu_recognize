#ifndef _EFFECT_OVERLAY_H
#define _EFFECT_OVERLAY_H

#include <iostream>
#include <unordered_map>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <random>
#include <algorithm> 
#include <math.h>

using namespace std;

class effect_overlay
{
  private:
    string current_seq = "";
    unordered_map<string, int(effect_overlay::*)(cv::Mat,int)> seq_map{{"r60", &effect_overlay::draw_lightning}, {"24bgiokr2", &effect_overlay::draw_lightning}, {"q9qzqn3q", &effect_overlay::draw_lightning}, {"w", &effect_overlay::draw_lightning}};
  public:
    effect_overlay();
    ~effect_overlay();
    int draw_water(cv::Mat frame, int flag_restart);
    int draw_lightning(cv::Mat frame, int flag_restart);
    void add_effect(string current_gesture, cv::Mat frame, int flag_restart);
    string find_seq(string current_gesture);
};

#endif


