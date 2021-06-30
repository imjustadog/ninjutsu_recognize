#include "effect_overlay.hpp"

const int WATER_RMAX = 10;
const int WATER_RMIN = 5;
const int WATER_COUNT = 30;
const int WATER_INTERVAL = 4;
const int SEED = 666;
const int TIME_TRANS = 40;
const int TIME_MOVE = 60;
const int TIME_STOP = 100;
const int FOX_INTERVAL = 30;
const int WLKC_START1 = 60;
const int WLKC_START2 = 70;
const int WLKC_START3 = 80;
const int WLKC_START4 = 90;

effect_overlay::effect_overlay()
{
}

effect_overlay::~effect_overlay()
{
}

int effect_overlay::draw_water(cv::Mat &frame, cv::Point hand_center, int flag_restart)
{
  static vector<vector<int>> water_kernel(WATER_COUNT + 1, vector<int>(3, 0));
  static int current_time = 0;
  default_random_engine dre(SEED);
  int ellipse_x;
  int ellipse_y;
  int point_x;
  int point_y;

  if(flag_restart)
  {
    uniform_int_distribution<int> y_pose_gen(frame.rows * 7 / 16, frame.rows * 9 / 16);
    uniform_int_distribution<int> r_gen(WATER_RMIN, WATER_RMAX);
    for(int i = 1;i < WATER_COUNT;i++)
    {
      water_kernel[i][0] = frame.cols * i / WATER_COUNT ;
      water_kernel[i][1] = y_pose_gen(dre);
      water_kernel[i][2] = r_gen(dre);
    } 
    current_time = 0;
  }

  cv::Mat frame_effect = frame.clone();
  for(int i = 1;i < WATER_COUNT;i++)
  {
    if(current_time < TIME_TRANS)
    {
      ellipse_x = min(current_time / WATER_INTERVAL, water_kernel[i][2]);
      ellipse_y = ellipse_x;
      point_x = water_kernel[i][0];
      point_y = water_kernel[i][1];
    }
    else if(current_time < TIME_MOVE)
    {
      ellipse_x = max(1, water_kernel[i][2] - (current_time - TIME_TRANS) / WATER_INTERVAL);
      ellipse_y = 2 * water_kernel[i][2] - ellipse_x;
      point_x = water_kernel[i][0];
      point_y = water_kernel[i][1];
    }
    else if(current_time < TIME_STOP)
    {
      ellipse_x = 1;
      ellipse_y = 2 * water_kernel[i][2] + (current_time - TIME_MOVE) / 3;
      point_x = water_kernel[i][0] + WATER_INTERVAL * (current_time - TIME_MOVE) * sin((270 - 180 / WATER_COUNT * i) * M_PI / 180.0);
      point_y = water_kernel[i][1] - WATER_INTERVAL * (current_time - TIME_MOVE) * cos((270 - 180 / WATER_COUNT * i) * M_PI / 180.0);
    }
    else
    {
        return 1;
    }
    
    ellipse(frame_effect, 
      cv::Point(point_x, point_y),
      cv::Size(ellipse_x, ellipse_y),
      270 - 180 / WATER_COUNT * i, 
      0, 360, 
      cv::Scalar(255, 129, 0), 
      -1,
      CV_AA);
  }

  cv::addWeighted(frame, 0.5, frame_effect, 0.5, 0, frame, -1);
  current_time++;

  return 0;
}

int effect_overlay::draw_lightning(cv::Mat &frame, cv::Point hand_center, int flag_restart)
{
  static int current_time = 0;
  static cv::Mat lightning_spark = cv::imread("spark.png");
  cv::Mat resized_spark;
  cv::Mat roi_spark;
  cv::Point spark_coord;
  int xmin = 0;
  int xmax = lightning_spark.cols;
  int ymin = 0;
  int ymax = lightning_spark.rows;

  if(flag_restart)
  {
    current_time = 0;
  }

  if(current_time % 4 == 1)
    cv::flip(lightning_spark, resized_spark, 1);
  else if(current_time % 4 == 2)
    cv::flip(lightning_spark, resized_spark, 0);
  else if(current_time % 4 == 3)
    cv::transpose(lightning_spark, resized_spark);
  else
    lightning_spark.copyTo(resized_spark);

  spark_coord.x = hand_center.x - resized_spark.cols / 2;
  spark_coord.y = hand_center.y - resized_spark.rows / 2;

  if(spark_coord.x < 0)
  {
    xmin = 0 - spark_coord.x;
    spark_coord.x = 0;
  }
  else if(spark_coord.x > frame.cols - resized_spark.cols)
  {
    xmax = frame.cols - spark_coord.x;
  }
  else if(spark_coord.x > frame.cols)
  {
    return 1;
  }


  if(spark_coord.y < 0)
  {
    ymin = 0 - spark_coord.y;
    spark_coord.y = 0;
  }
  else if(spark_coord.y > frame.rows - resized_spark.rows)
  {
    ymax = frame.rows - spark_coord.y;
  }
  else if(spark_coord.y > frame.rows)
  {
    return 1;
  }

  //cout << " xmin: " << xmin << " ymin: " << ymin << " xmax: " << xmax << " ymax: " << ymax << endl;
  //cout << " x: " << spark_coord.x << " y: " << spark_coord.y << endl;
  //cout << frame.cols << " " << frame.rows << endl;
    
  roi_spark = frame(cv::Rect(spark_coord.x, spark_coord.y, xmax - xmin, ymax - ymin));

  cv::addWeighted(roi_spark, 1.0, resized_spark(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin)), 0.5, 0, roi_spark, -1);

  current_time ++;

  if(current_time >= TIME_STOP)
  {
    return 1;
  }

  return 0;
}


int effect_overlay::draw_5t5(cv::Mat &frame, cv::Point hand_center, int flag_restart)
{
  static int current_time = 0;
  static cv::Mat wlkc1 = cv::imread("wlkc1.png");
  static cv::Mat wlkc2 = cv::imread("wlkc2.png");
  static cv::Mat wlkc3 = cv::imread("wlkc3.png");
  static cv::Mat wlkc4 = cv::imread("wlkc4.png");
  static cv::Mat wlkc5 = cv::imread("wlkc5.png");
  static cv::Mat wlkc6 = cv::imread("wlkc6.png");
  static cv::Mat wlkc7 = cv::imread("wlkc7.png");
  static cv::Mat wlkc8 = cv::imread("wlkc8.png");

  if(flag_restart)
  {
    current_time = 0;
  }

  if(current_time < WLKC_START1)
  {
    switch(current_time%4)
    {
      case 0:
        cv::addWeighted(frame, 1.0, wlkc1, 0.5, 0, frame, -1);
        break;
      case 1:
        cv::addWeighted(frame, 1.0, wlkc2, 0.5, 0, frame, -1);
        break;
      case 2:
        cv::addWeighted(frame, 1.0, wlkc3, 0.5, 0, frame, -1);
        break;
      case 3:
        cv::addWeighted(frame, 1.0, wlkc4, 0.5, 0, frame, -1);
        break;
      default:
        break;
    }
  }
  else if(current_time < WLKC_START1)
  {
    cv::addWeighted(frame, 1.0, wlkc5, 0.8, 0, frame, -1);
  }
  else if(current_time < WLKC_START2)
  {
    cv::addWeighted(frame, 1.0, wlkc6, 0.8, 0, frame, -1);
  }

  else if(current_time < WLKC_START3)
  {
    cv::addWeighted(frame, 1.0, wlkc7, 0.8, 0, frame, -1);
  }

  else if(current_time < WLKC_START4)
  {
    cv::addWeighted(frame, 1.0, wlkc8, 0.8, 0, frame, -1);
  }


  current_time ++;

  if(current_time >= TIME_STOP)
  {
    return 1;
  }

  return 0;
}


int effect_overlay::draw_fox(cv::Mat &frame, cv::Point hand_center, int flag_restart)
{
  static int current_time = 0;
  static cv::Mat fox = cv::imread("fox.png", CV_LOAD_IMAGE_UNCHANGED);
  cv::Mat resized_fox;
  cv::Mat roi_fox;
  cv::Point fox_coord;
  int current_width;
  int current_height;

  if(flag_restart)
  {
    current_time = 0;
  }

  //if(current_time % 4 == 1)
  //  cv::flip(fox, resized_fox, 0);
  //else if(current_time % 4 == 2)
  //  cv::transpose(fox, resized_fox);
  //else if(current_time % 4 == 3)
  //  cv::flip(fox, resized_fox, 1);
  //else
  //  fox.copyTo(resized_fox);

  current_height = fox.rows + current_time * FOX_INTERVAL;
  current_width = fox.cols + current_time * FOX_INTERVAL * fox.cols / fox.rows;
  cv::resize(fox,resized_fox,cv::Size(current_width,current_height),CV_INTER_AREA);		

  int xmin = 0;
  int xmax = resized_fox.cols;
  int ymin = 0;
  int ymax = resized_fox.rows;

  fox_coord.x = hand_center.x - resized_fox.cols / 2;
  fox_coord.y = hand_center.y - resized_fox.rows / 2;

  if(fox_coord.x < 0)
  {
    xmin = 0 - fox_coord.x;
    fox_coord.x = 0;
  }
  else if(fox_coord.x > frame.cols - resized_fox.cols)
  {
    xmax = frame.cols - fox_coord.x;
  }
  else if(fox_coord.x > frame.cols)
  {
    return 1;
  }

  if(fox_coord.y < 0)
  {
    ymin = 0 - fox_coord.y;
    fox_coord.y = 0;
  }
  else if(fox_coord.y > frame.rows - resized_fox.rows)
  {
    ymax = frame.rows - fox_coord.y;
  }
  else if(fox_coord.y > frame.rows)
  {
    return 1;
  }

  if(xmax - xmin > frame.cols)
  {
    xmax = xmin + frame.cols;
  }
  if(ymax - ymin > frame.rows)
  {
    ymax = ymin + frame.rows;
  }

  //cout << " xmin: " << xmin << " ymin: " << ymin << " xmax: " << xmax << " ymax: " << ymax << endl;
  //cout << " x: " << fox_coord.x << " y: " << fox_coord.y << endl;

  roi_fox = frame(cv::Rect(fox_coord.x, fox_coord.y, xmax - xmin, ymax - ymin));

  vector<cv::Mat> roi_channels;    
  vector<cv::Mat> fox_channels;    
  cv::split(roi_fox, roi_channels);    
  cv::split(resized_fox(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin)), fox_channels); 

  for (int i = 0; i < 3; i++)    
  {    
    roi_channels[i] = roi_channels[i].mul(255 - fox_channels[3], 1 / 255.0);    
    roi_channels[i] += fox_channels[i].mul(fox_channels[3], 1 / 255.0);    
  }

  cv::merge(roi_channels, roi_fox);

  current_time ++;

  if(current_time >= TIME_STOP)
  {
    return 1;
  }

  return 0;
}

int effect_overlay::add_effect(string current_gesture, cv::Mat &frame, cv::Point hand_center, int flag_restart)
{
  int ret;
  ret = (this->*seq_map[current_gesture])(frame, hand_center, flag_restart);
  return ret;
}


string effect_overlay::find_seq(string current_gesture)
{
  int i, j;
  string res = "";

  if(current_gesture == "") return res;
  if(current_gesture.back() == current_seq.back()) return res;
  else
  {
    current_seq += current_gesture;
    for(auto seq:seq_map)
    {
      i = seq.first.size() - 1;
      for(j = current_seq.size() - 1;(i >= 0) && (j >= 0);j--)
      {
        if((seq.first)[i] == current_seq[j]) i--;
      }
      if(i < 0)
      {
        current_seq = "";
        res = seq.first;
        break;
      }
    }
  }
  if(res != "") cout << "ninjutsu: " << res << endl;
  return res;
}
