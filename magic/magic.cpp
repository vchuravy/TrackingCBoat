#include "magic.h"

using namespace cv;

Point point1, point2; /* vertical points of the bounding box */
bool drag = false;
bool select_flag = false;
Rect rect; /* bounding box */
Mat img, roiImg; /* roiImg - the part of the image in the bounding box */

void mouseHandler(int event, int x, int y, int flags, void* param){
    if (event == CV_EVENT_LBUTTONDOWN && !drag){
        /* left button clicked. ROI selection begins */
        point1 = Point(x, y);
        drag = true;
    }

    if (event == CV_EVENT_MOUSEMOVE && drag){
        /* mouse dragged. ROI being selected */
        Mat img1 = img.clone();
        point2 = Point(x, y);
        rectangle(img1, point1, point2, CV_RGB(255, 0, 0), 3, 8, 0);
        imshow("image", img1);
    }

    if (event == CV_EVENT_LBUTTONUP && drag){
        point2 = Point(x, y);
        rect = Rect(point1.x,point1.y,x-point1.x,y-point1.y);
        drag = false;
        roiImg = img(rect);
    }

    if (event == CV_EVENT_LBUTTONUP){
       /* ROI selected */
        select_flag = true;
        drag = false;
    }
}

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
  int frames = cap.get(CV_CAP_PROP_FRAME_COUNT);
  double fps = cap.get(CV_CAP_PROP_FPS);
  printf("Number of Frames: %d\n", frames);
  printf("FPS: %f\n", fps);

  // Contains the working image
  Mat dst, in, temp;

  if(argc >= 5) {
    in = imread(argv[4]);
  } else {
    cap >> in;
  }

  img = in.clone();
  imshow("image", img);
  int k;
  while(1){
    cvSetMouseCallback("image", mouseHandler, NULL);
    if (select_flag){
        imshow("ROI", roiImg);  //show the image bounded by the box
    }
    rectangle(img, rect, CV_RGB(255, 0, 0), 3, 8, 0);
    imshow("image", img);
    k = waitKey(10);
    // if(k != -1) {
    //   printf("Key pressed %d\n", k);
    // }
    if (k == 27){ //ESC
        destroyAllWindows();
        waitKey(1);
        break;
    }
  }

  if(!select_flag){
    rect = Rect(0,0,img.cols, img.rows);
  }

  // Background
  temp = in(rect);
  Mat background;
  cvtColor(temp, background, CV_RGB2GRAY);

  vector<Vec3f> circles;

  /// Apply the Hough Transform to find the circles
  HoughCircles(background, circles, CV_HOUGH_GRADIENT, 1, 300, 200, 100, 0, 0);

  /// Draw the circles detected
  for( size_t i = 0; i < circles.size(); i++ )
  {
      Point2f center(circles[i][0], circles[i][1]);
      float radius = circles[i][2];
      // circle center
      circle(temp, center, 3, Scalar(0,255,0), -1, 8, 0 );
      // circle outline
      circle(temp, center, radius, Scalar(0,0,255), 3, 8, 0 );
      printf("Circle %d with r=%f at x=%f, y=%f\n", i, radius, center.x, center.y);
   }

  imshow("image", temp);
  waitKey(0);
  destroyWindow("image");
  waitKey(1);

  //Set image parameter
  vector<int> imageout_params;
  imwrite(output + "/bg_circle.tiff", temp, imageout_params);
  imwrite(output + "/bg.tiff", background, imageout_params);

  std::vector<Point2i> points;
  Point2f center;
  float radius;
  int nonZero;

  std::ofstream fs (output + "/positions.csv", std::ofstream::out);
  fs << "Frame,Time,Radius,x,y\n";

  for(int i = 0; i <= frames-1;i++){
    cap >> in;
    if(i > skip_frames) {
    temp = in(rect);
    cvtColor(temp, dst, CV_RGB2GRAY, 0);
    absdiff(dst, background, temp);
    threshold(temp, dst, 30, 255, THRESH_BINARY);

    points.clear();
    nonZero = countNonZero(dst);
    if(nonZero > 0) {
      findNonZero(dst, points);
      minEnclosingCircle(points, center, radius);

      if(nonZero/(M_PI * radius * radius) >= 0.3){
        fs << i <<","<< i/fps <<","<< radius <<","<< center.x <<","<< center.y << "\n";
      } else {
        std::cerr << "\n" << i << ": Found point with r=" << radius << " but less than 30% of pixels are visible. c_p = " << nonZero << "\n";
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