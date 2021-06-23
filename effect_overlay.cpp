#include "effect_overlay.hpp"

const int WATER_RMAX = 10;
const int WATER_RMIN = 5;
const int WATER_COUNT = 30;
const int CHANGE_INTERVAL = 4;
const int SEED = 666;
const int TIME_TRANS = 40;
const int TIME_MOVE = 60;
const int TIME_STOP = 100;

effect_overlay::effect_overlay()
{
}

effect_overlay::~effect_overlay()
{
}

int effect_overlay::draw_water(cv::Mat frame, cv::Point hand_center, int flag_restart)
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
      ellipse_x = min(current_time / CHANGE_INTERVAL, water_kernel[i][2]);
      ellipse_y = ellipse_x;
      point_x = water_kernel[i][0];
      point_y = water_kernel[i][1];
    }
    else if(current_time < TIME_MOVE)
    {
      ellipse_x = max(1, water_kernel[i][2] - (current_time - TIME_TRANS) / CHANGE_INTERVAL);
      ellipse_y = 2 * water_kernel[i][2] - ellipse_x;
      point_x = water_kernel[i][0];
      point_y = water_kernel[i][1];
    }
    else if(current_time < TIME_STOP)
    {
      ellipse_x = 1;
      ellipse_y = 2 * water_kernel[i][2] + (current_time - TIME_MOVE) / 3;
      point_x = water_kernel[i][0] + CHANGE_INTERVAL * (current_time - TIME_MOVE) * sin((270 - 180 / WATER_COUNT * i) * M_PI / 180.0);
      point_y = water_kernel[i][1] - CHANGE_INTERVAL * (current_time - TIME_MOVE) * cos((270 - 180 / WATER_COUNT * i) * M_PI / 180.0);
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

int effect_overlay::draw_lightning(cv::Mat frame, cv::Point hand_center, int flag_restart)
{
  static int current_time = 0;
  static cv::Mat lightning_spark = cv::imread("spark.png");
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

  spark_coord.x = hand_center.x - lightning_spark.cols / 2;
  spark_coord.y = hand_center.y - lightning_spark.rows / 2;

  if(spark_coord.x < 0)
  {
    xmin = 0 - spark_coord.x;
    spark_coord.x = 0;
  }
  if(spark_coord.x > frame.cols - lightning_spark.cols)
  {
    xmax = lightning_spark.cols - (frame.cols - spark_coord.x);
    spark_coord.x = frame.cols - lightning_spark.cols;
  }
  if(spark_coord.y < 0)
  {
    ymin = 0 - spark_coord.y;
    spark_coord.y = 0;
  }
  if(spark_coord.y > frame.rows - lightning_spark.rows)
  {
    ymax = lightning_spark.rows - (frame.rows - spark_coord.y);
    spark_coord.y = frame.rows - lightning_spark.rows;
  }

  //cout << " xmin: " << xmin << " ymin: " << ymin << " xmax: " << xmax << " ymax: " << ymax << endl;
  //cout << " x: " << spark_coord.x << " y: " << spark_coord.y << endl;
  //cout << frame.cols << " " << frame.rows << endl;

  if(current_time % 4 == 1)
    cv::flip(lightning_spark, lightning_spark, 1);
  else if(current_time % 4 == 2)
    cv::transpose(lightning_spark, lightning_spark);
  else if(current_time % 4 == 3)
    cv::flip(lightning_spark, lightning_spark, 0);
    
  roi_spark = frame(cv::Rect(spark_coord.x, spark_coord.y, xmax - xmin, ymax - ymin));

  cv::addWeighted(roi_spark, 1.0, lightning_spark(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin)), 0.5, 0, roi_spark, -1);

  current_time ++;

  if(current_time >= TIME_STOP)
  {
    return 1;
  }

  return 0;
}

int effect_overlay::add_effect(string current_gesture, cv::Mat frame, cv::Point hand_center, int flag_restart)
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
