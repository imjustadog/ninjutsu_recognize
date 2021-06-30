#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>
#include <vector>
#include "cnrt.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>

#include "svm_inference.hpp"
#include "effect_overlay.hpp"

using namespace std;
using namespace cv;


const int POSE_PAIRS[20][2] =
{
  {0,1}, {1,2}, {2,3}, {3,4},         // thumb
  {0,5}, {5,6}, {6,7}, {7,8},         // index
  {0,9}, {9,10}, {10,11}, {11,12},    // middle
  {0,13}, {13,14}, {14,15}, {15,16},  // ring
  {0,17}, {17,18}, {18,19}, {19,20}   // small
};


double GetTickCount()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000)/1000.0;
}

int main(int argc,char *argv[])
{
  CNRT_CHECK(cnrtInit(0)); 

  //if enough mlu device online
  unsigned int dev_num = 0; 
  CNRT_CHECK(cnrtGetDeviceCount(&dev_num));
  if (dev_num == 0)
    return -1;

  //assign mlu device to use
  unsigned int dev_id = 0;
  cnrtDev_t dev;
  CNRT_CHECK(cnrtGetDeviceHandle(&dev, dev_id));
  CNRT_CHECK(cnrtSetCurrentDevice(dev));

  // prepare model name
  cnrtModel_t model;
  const char *model_path="openpose_hand_firstconv_int8_320x240_cali_220core4.cambricon";
  CNRT_CHECK(cnrtLoadModel(&model, model_path));

  // load extract function
  cnrtFunction_t function;
  const char *function_name="subnet0";
  CNRT_CHECK(cnrtCreateFunction(&function));
  CNRT_CHECK(cnrtExtractFunction(&function, model, function_name));

  //assign specified channel is needeed
  int dev_channel=-1;
  if(dev_channel>=0)
    CNRT_CHECK(cnrtSetCurrentChannel((cnrtChannelType_t)dev_channel));

  // get model input/output data size
  int64_t *inputSizeS, *outputSizeS;
  cnrtDataType_t *inputTypeS, *outputTypeS;
  int inputNum, outputNum;
  CNRT_CHECK(cnrtGetInputDataSize(&inputSizeS,&inputNum,function));
  CNRT_CHECK(cnrtGetOutputDataSize(&outputSizeS,&outputNum,function));
  CNRT_CHECK(cnrtGetInputDataType(&inputTypeS,&inputNum,function));
  CNRT_CHECK(cnrtGetOutputDataType(&outputTypeS,&outputNum,function));

  // prepare data on cpu
  void **inputCpuPtrS = (void **)malloc(sizeof(void *) * inputNum);
  void **outputCpuPtrS = (void **)malloc(sizeof(void *) * outputNum);
  void **outputCpuNchwPtrS = (void **)malloc(sizeof(void *) * outputNum);

  // allocate I/O data memory on MLU
  void **inputMluPtrS = (void **)malloc(sizeof(void *) * inputNum);
  void **outputMluPtrS = (void **)malloc(sizeof(void *) * outputNum);

  int dimNum;
  int *dimValues;

  // prepare input buffer
  int input_width;
  int input_height;
  int batch_size;
  for (int i = 0; i < inputNum; i++) 
  {
    // converts data format when using new interface model
    inputCpuPtrS[i] = (void *)malloc(inputSizeS[i]); 
    // malloc mlu memory
    CNRT_CHECK(cnrtMalloc(&inputMluPtrS[i],inputSizeS[i]));
    // get input dim
    CNRT_CHECK(cnrtGetInputDataShape(&dimValues,&dimNum,i,function));						 
    printf("input shape:\n");
    for(int y=0;y<dimNum;y++)
    {
      printf("%d ",dimValues[y]);
    }
    printf("\n");

    input_width=dimValues[2];
    input_height=dimValues[1];
    batch_size=dimValues[0];
    free(dimValues);
  }

  // prepare output buffer
  int output_width;
  int output_height;
  int output_channel;
  for (int i = 0; i < outputNum; i++) 
  {
    // converts data format when using new interface model
    outputCpuPtrS[i] = (void *)malloc(outputSizeS[i]);
    // malloc mlu memory
    CNRT_CHECK(cnrtMalloc(&outputMluPtrS[i],outputSizeS[i]));
    // get output dim
    CNRT_CHECK(cnrtGetOutputDataShape(&dimValues,&dimNum,i,function));		
    int count=1;
    printf("output shape:\n");
    for(int y=0;y<dimNum;y++)
    {
      printf("%d ",dimValues[y]);
      count=count*dimValues[y];
    }
    printf("\n");	

    output_width=dimValues[2];
    output_height=dimValues[1];	
    output_channel=dimValues[3];
    outputCpuNchwPtrS[i] = (void *)malloc(count*sizeof(float));
    free(dimValues);
  }

  // prepare parameters for cnrtInvokeRuntimeContext
  void **param;
  param = (void **)malloc(sizeof(void *) * (inputNum + outputNum));
  for (int i = 0; i < inputNum; i++) 
  {
    param[i] = inputMluPtrS[i];
  }
  for (int i = 0; i < outputNum; i++) 
  {
    param[i + inputNum] = outputMluPtrS[i];
  }

  // setup runtime ctx
  cnrtRuntimeContext_t ctx;
  CNRT_CHECK(cnrtCreateRuntimeContext(&ctx,function,NULL));

  // bind device
  CNRT_CHECK(cnrtSetRuntimeContextDeviceId(ctx,dev_id));
  CNRT_CHECK(cnrtInitRuntimeContext(ctx,NULL));

  // compute offline
  cnrtQueue_t queue;
  CNRT_CHECK(cnrtRuntimeContextCreateQueue(ctx,&queue));

  //set invoke param
  unsigned int affinity=1<<dev_channel;
  cnrtInvokeParam_t invokeParam;
  invokeParam.invoke_param_type=CNRT_INVOKE_PARAM_TYPE_0;
  invokeParam.cluster_affinity.affinity=&affinity;

  //create notifier 
  cnrtNotifier_t notifier_start;
  cnrtNotifier_t notifier_end;
  CNRT_CHECK(cnrtRuntimeContextCreateNotifier(ctx,&notifier_start));
  CNRT_CHECK(cnrtRuntimeContextCreateNotifier(ctx,&notifier_end));


  int image_width;
  int image_height;
  cv::VideoCapture capture(0);
  uint8_t ch;
  
  const char *svm_model_path = "./model/";
  svm_inference svm_ninjutsu(svm_model_path);
  effect_overlay effect_ninjutsu;
  string current_effect = "";
  int effect_ret;

  int input_idx=0;
  int output_idx=0;
  unsigned char *ptr=(unsigned char *)inputCpuPtrS[input_idx];


  while(1)
  {
    //get time start
    auto t0=GetTickCount();

    cv::Mat input_image;		
    capture >> input_image;
    cv::flip(input_image, input_image, 1);

    image_width = input_image.cols;
    image_height = input_image.rows;
    cv::Mat input_image_resized;
    cv::resize(input_image,input_image_resized,cv::Size(input_width,input_height),CV_INTER_AREA);		

    //write image data to cpu input buffer(ptr->inputCpuPtrS)
    cv::Mat net_input_data_rgba(input_height,input_width,CV_8UC3,ptr); 	
    input_image_resized.copyTo(net_input_data_rgba);

    // copy cpu input to mlu
    CNRT_CHECK(cnrtMemcpy(inputMluPtrS[input_idx],inputCpuPtrS[input_idx],inputSizeS[input_idx],CNRT_MEM_TRANS_DIR_HOST2DEV));

    // run the inference
    CNRT_CHECK(cnrtPlaceNotifier(notifier_start, queue));
    CNRT_CHECK(cnrtInvokeRuntimeContext(ctx,param,queue,&invokeParam));
    CNRT_CHECK(cnrtPlaceNotifier(notifier_end, queue));    
    CNRT_CHECK(cnrtSyncQueue(queue));   

    // copy mlu result to cpu
    CNRT_CHECK(cnrtMemcpy(outputCpuPtrS[output_idx],outputMluPtrS[output_idx],outputSizeS[output_idx],CNRT_MEM_TRANS_DIR_DEV2HOST));

    float hwtime;
    CNRT_CHECK(cnrtNotifierDuration(notifier_start, notifier_end, &hwtime));	

    int dim_order[4] = {0, 3, 1, 2};
    CNRT_CHECK(cnrtGetOutputDataShape(&dimValues,&dimNum,output_idx,function));
    float *output_ptr;

    if(dimNum==4)
    {
      //NHWC->NCHW half->float32
      CNRT_CHECK(cnrtTransOrderAndCast(reinterpret_cast<void*>(outputCpuPtrS[output_idx]), outputTypeS[output_idx],
            reinterpret_cast<void*>(outputCpuNchwPtrS[output_idx]), CNRT_FLOAT32,
            nullptr, dimNum, dimValues, dim_order));
    }
    else
    {
      //half->float32
      if(outputTypeS[output_idx]!=CNRT_FLOAT32)
      {
        CNRT_CHECK(cnrtCastDataType(reinterpret_cast<void*>(outputCpuPtrS[output_idx]),
              outputTypeS[output_idx],
              reinterpret_cast<void*>(outputCpuNchwPtrS[output_idx]),
              CNRT_FLOAT32,
              outputSizeS[output_idx]/2,nullptr));
        output_ptr=(float*)outputCpuNchwPtrS[output_idx];
      }			
      else
      {
        output_ptr=(float*)outputCpuPtrS[output_idx];
      }
    }
    free(dimValues);


    vector<cv::Point> points(output_channel);
    double prob;
    float *temp = (float *)(outputCpuNchwPtrS[output_idx]); 
    for (int n = 0; n < output_channel; n++)
    {
      cv::Mat probMap(output_height, output_width, CV_32F, temp);
      temp += output_height * output_width;
      //cv::resize(probMap, probMap, cv::Size(input_width, input_height));

      Point maxLoc;
      cv::minMaxLoc(probMap, 0, &prob, 0, &maxLoc);
      points[n].x = maxLoc.x * (image_width / output_width) + 4;
      points[n].y = maxLoc.y * (image_height / output_height) + 4;
    }

    cv::Point hand_center;
    hand_center.x = points[9].x;
    hand_center.y = points[9].y;

    cv::Mat output_image;

    input_image.copyTo(output_image);

    int nPairs = sizeof(POSE_PAIRS)/sizeof(POSE_PAIRS[0]);
    for (int n = 0; n < nPairs; n++)
    {
      // lookup 2 connected body/hand parts
      cv::Point2f partA = points[POSE_PAIRS[n][0]];
      cv::Point2f partB = points[POSE_PAIRS[n][1]];

      if (partA.x<=0 || partA.y<=0 || partB.x<=0 || partB.y<=0)
        continue;

      cv::line(output_image, partA, partB, cv::Scalar(0,255,255), 6);
      cv::circle(output_image, partA, 4, cv::Scalar(0,0,255), -1);
      cv::circle(output_image, partB, 4, cv::Scalar(0,0,255), -1);
    }


    string svm_result;
    string seq_result;
    vector<double> svm_input;
    double pmax = INT_MIN;
    double pmin = INT_MAX;
    int effect_ret;

    for(int n = 1;n < output_channel - 1; n++)
    {
      points[n].x = points[n].x - points[0].x;
      points[n].y = points[n].y - points[0].y;

      pmax = (points[n].x > pmax)?points[n].x:pmax;
      pmax = (points[n].y > pmax)?points[n].y:pmax;
      pmin = (points[n].x < pmin)?points[n].x:pmin;
      pmin = (points[n].y < pmin)?points[n].y:pmin;
    }
    for(int n = 1;n < output_channel - 1; n++)
    {
      svm_input.push_back(points[n].x / (pmax - pmin));
      svm_input.push_back(points[n].y / (pmax - pmin));
    }

    svm_ninjutsu.do_svm_preprocess(svm_input);
    svm_result = svm_ninjutsu.do_svm_inference();
    svm_ninjutsu.do_svm_postprocess();
    seq_result = effect_ninjutsu.find_seq(svm_result);

    if(seq_result != "")
    {
      current_effect = seq_result;
      effect_ret = effect_ninjutsu.add_effect(current_effect, input_image, hand_center, 1);
    }

    if(current_effect != "")
    {
      effect_ret = effect_ninjutsu.add_effect(current_effect, input_image, hand_center, 0);
      if(effect_ret)
        current_effect = "";
    }
    

    //get time end
    auto t1=GetTickCount();

    printf("HardwareTime:%f(ms) TotalTime:%f(ms)\n",hwtime/1000.0,t1-t0);

    cv::resize(input_image,input_image,cv::Size(640,480),CV_INTER_AREA);		
    cv::resize(output_image,output_image,cv::Size(480,320),CV_INTER_AREA);		

    cv::imshow("openpose", output_image);
    cv::imshow("ninjutsu", input_image);
    ch = cv::waitKey(1);
  }

  CNRT_CHECK(cnrtSetCurrentDevice(dev));
  CNRT_CHECK(cnrtDestroyQueue(queue));
  CNRT_CHECK(cnrtDestroyFunction(function));
  CNRT_CHECK(cnrtUnloadModel(model));

  cnrtDestroyNotifier(&notifier_start);
  cnrtDestroyNotifier(&notifier_end);

  for (int i = 0; i < inputNum; i++) 
  {
    free(inputCpuPtrS[i]);
    cnrtFree(inputMluPtrS[i]);
  }
  for (int i = 0; i < outputNum; i++) 
  {
    free(outputCpuPtrS[i]);
    free(outputCpuNchwPtrS[i]);
    cnrtFree(outputMluPtrS[i]);
  }


  free(inputCpuPtrS);
  free(outputCpuPtrS);
  free(param);

  cnrtDestroyRuntimeContext(ctx);

  return 0;	
}


