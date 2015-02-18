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

  int nLabels;
  Mat labels(background.size(), CV_32S);
  Mat stats, centers;

  //stats
  int area;
  int w;
  int h;
  double x;
  double y;

  std::ofstream fs (output + "/positions.csv", std::ofstream::out);
  fs << "Frame,Time,Radius,x,y\n";

  for(int i = 0; i < frames-1;i++){
    cap >> in;
    if(i > skip_frames) {
    temp = Mat::zeros(background.rows, background.cols, CV_8UC1);
    in.copyTo(temp, mask ); // copy values of in to temp if mask is > 0.
    cvtColor(temp, dst, COLOR_RGB2GRAY, 0);
    absdiff(dst, background, temp);
    threshold(temp, dst, 30, 255, THRESH_BINARY);

    nLabels = connectedComponentsWithStats(dst, labels, stats, centers, 8, CV_32S);

    // skip background i=0
    for (int j=1; j< nLabels;j++) {
      // get stats
      area = stats.at<int>(j,CC_STAT_AREA);
      if(area >= 20) {
        w = stats.at<int>(j,CC_STAT_WIDTH);
        h = stats.at<int>(j,CC_STAT_HEIGHT);
        // w/h >= 0.7 && h/w >= 0.7 the bounding box of a circle is roughly a square.
        if((h/ double(w)) >=0.7 && (w/ double(h)) >=0.7) {
          // The bounding box contains an ellipse and the visible points should be at least 30% of that area.
          if(area/(M_PI * (0.5*w) * (0.5*h)) >= 0.3) {
            x = centers.at<double>(j,0);
            y = centers.at<double>(j,1);
            fs << i <<","<< i/fps <<","<< 0.25*(w+h) <<","<< x <<","<< y << "\n";
            // printf("%d: Found cc%d at x=%f y=%f ",i, j, x, y);
            // printf("with area=%d w=%d h=%d\n", area, w, h);
          }
        }
      }
    }
    std::cout << "\r" << i << "/" << frames;
    // imwrite(output + "/test-"+ std::to_string(i)+".tiff", dst, imageout_params);
    }
  }

  fs << std::endl;
  fs.close();
}