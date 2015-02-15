#include <stdio.h>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("usage: Magic <Video_Path> [output_dir]\n");
  }

  std::string output;
  if(argc >= 3) {
    output = argv[2];
  } else {
    output = ".";
  }

  VideoCapture cap(argv[1]); // load the video
  if(!cap.isOpened())  // check if we succeeded
        return -1;

  // Write some info about File
  int frames = cap.get(CV_CAP_PROP_FRAME_COUNT);
  printf("Number of Frames: %d\n", frames);
  printf("FPS: %d\n",cap.get(CV_CAP_PROP_FPS));

  // Contains the working image
  Mat dst, in, temp;

  // Background
  // TODO: support loading from file
  Mat background;
  cap >> in;
  cvtColor(in, background, CV_RGB2GRAY);

  //Set image parameter
  vector<int> imageout_params;

  for(int i = 0; i <= frames;i++){
    cap >> in;
    cvtColor(in, dst, CV_RGB2GRAY, 0);
    absdiff(dst, background, temp);
    threshold(temp, dst, 0.5, 1, THRESH_BINARY_INV);
    imwrite(output + "/test-"+ std::to_string(i)+".tiff", dst, imageout_params);
  }

  // Idea:
  // threshold(InputArray src, OutputArray dst, double thresh, double maxval, int type)
  // type = THRESH_BINARY_INV
}