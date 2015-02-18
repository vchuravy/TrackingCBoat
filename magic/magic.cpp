#include "magic.h"

using namespace cv;

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("usage: Magic <Video_Path> [output_dir] [skip_frames] [background]\n");
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

  // Write some info about File
  int frames = cap.get(CAP_PROP_FRAME_COUNT);
  double fps = cap.get(CAP_PROP_FPS);
  printf("Number of Frames: %d\n", frames);
  printf("FPS: %f\n", fps);

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
  std::ofstream fsJs (output + "/petriDish.json", std::ofstream::out);

  for( size_t i = 0; i < circles.size(); i++ )
  {
      Point2f center(circles[i][0], circles[i][1]);
      float radius = circles[i][2];
      // circle center
      circle(temp, center, 3, Scalar(0,255,0), -1, 8, 0 );
      // circle outline
      circle(temp, center, radius, Scalar(0,0,255), 3, 8, 0 );
      printf("Circle %d with r=%f at x=%f, y=%f\n", i, radius, center.x, center.y);

      fsJs << "\"Circle " << i << "\":";
      fsJs << "{" << "\"radius\":" <<  radius << ",\"x\":" << center.x << ",\"y\":" << center.y << "}";
      if(i + 1 < circles.size())
        fsJs << ",\n";
   }
   fsJs << "\n}" << std::endl;
   fsJs.close();

  imshow("image", temp);
  waitKey(0);
  destroyWindow("image");
  waitKey(0);

  //Set image parameter
  std::vector<int> imageout_params;
  imwrite(output + "/bg_circle.tiff", temp, imageout_params);
  imwrite(output + "/bg.tiff", background, imageout_params);

  //Create mask based on circles[0]
  Point2f center(circles[0][0], circles[0][1]);
  float radius = circles[0][2];
  Mat mask = Mat::zeros(background.rows, background.cols, CV_8UC1);
  circle(mask, center, radius, Scalar(255,255,255), -1, 8, 0 ); //-1 means filled

  GaussianBlur(background, temp, Size(3,3) , 3, 3, BORDER_DEFAULT);
  background = Mat::zeros(background.rows, background.cols, CV_8UC1);
  temp.copyTo(background, mask ); // copy values of temp to background if mask is > 0.

  std::vector<Point2i> points;
  int nonZero;

  std::ofstream fs (output + "/positions.csv", std::ofstream::out);
  fs << "Frame,Time,Radius,x,y\n";

  for(int i = 0; i <= frames-1;i++){
    cap >> in;
    if(i > skip_frames) {
    temp = Mat::zeros(background.rows, background.cols, CV_8UC1);
    in.copyTo(temp, mask ); // copy values of in to temp if mask is > 0.
    cvtColor(temp, dst, COLOR_RGB2GRAY, 0);
    absdiff(dst, background, temp);
    threshold(temp, dst, 30, 255, THRESH_BINARY);

    points.clear();
    nonZero = countNonZero(dst);
    if(nonZero > 0) {
      findNonZero(dst, points);
      
      Point2f centerMass(0,0);
      for(Point2i p : points){
        centerMass.x += p.x;
        centerMass.y += p.y;
      }
      centerMass.x /= points.size();
      centerMass.y /= points.size();

      minEnclosingCircle(points, center, radius);

      if(nonZero/(M_PI * radius * radius) >= 0.3){
        fs << i <<","<< i/fps <<","<< radius <<","<< center.x <<","<< center.y << "\n";
      } else {
        std::cerr << "\n" << i << ": Found point with r=" << radius << " x=" << center.x << " y=" << center.y << " c_p = " << nonZero;
        std::cerr << " center mass is x=" << centerMass.x << " y=" << centerMass.y << "\n";
        imwrite(output + "/error-"+ std::to_string(i)+".tiff", dst, imageout_params);
      }
    }
    std::cout << "\r" << i << "/" << frames;
    // imwrite(output + "/test-"+ std::to_string(i)+".tiff", dst, imageout_params);
    }
  }

  fs << std::endl;
  fs.close();
}