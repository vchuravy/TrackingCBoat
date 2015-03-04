#include "magic.h"

using namespace cv;

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("usage: Prepare <Video_Path> [output_dir] [skip_frames]\n");
  }

  std::string output;
  if(argc >= 3) {
    output = argv[2];
  } else {
    output = ".";
  }

  int skip_frames = 0;
  if(argc >= 4) {
    skip_frames = std::atoi(argv[3]);
  }

  VideoCapture cap(argv[1]); // load the video
  if(!cap.isOpened())  // check if we succeeded
        return -1;

  // Contains the working image
  Mat dst, in, temp;

  if(argc >= 5) {
    in = imread(argv[4]);
  } else {
    cap >> in;
  }

  // Background
  Mat background;
  temp = in;
  cvtColor(in, background, COLOR_RGB2GRAY);
  // threshold(background.clone(), background, 10, 255, THRESH_TOZERO); 

  std::vector<Vec3f> circles;

  /// Apply the Hough Transform to find the circles
  HoughCircles(background, circles, HOUGH_GRADIENT, 1, 300, 200, 100, 0, 0);

  /// Draw the circles detected
  std::ofstream fsJs (output + "/petriDish.txt", std::ofstream::out);

  Point2f center;
  float radius;

  for( size_t i = 0; i < circles.size(); i++ )
  {
      center = Point2f(circles[i][0], circles[i][1]);
      radius = circles[i][2];
      // circle center
      circle(temp, center, 3, Scalar(0,255,0), -1, 8, 0 );
      // circle outline
      circle(temp, center, radius, Scalar(0,0,255), 3, 8, 0 );
      printf("Circle %d with r=%f at x=%f, y=%f\n", i, radius, center.x, center.y);

      fsJs << i << " " << radius << " " << center.x << " " << center.y ;
      if(i + 1 < circles.size())
        fsJs << "\n";
   }
  fsJs << std::endl;
  fsJs.close();

  imshow("image", temp);
  waitKey(0);
  destroyWindow("image");

  //Set image parameter
  std::vector<int> imageout_params;
  imwrite(output + "/bg_circle.tiff", temp, imageout_params);
  imwrite(output + "/bg.tiff", background, imageout_params);
}