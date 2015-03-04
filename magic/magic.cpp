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
    in = imread(output + "/bg.tiff");
  }

  // Background
  Mat background;
  cvtColor(in, background, COLOR_RGB2GRAY);
  temp = background;

  // Load circle
  std::ifstream infile(output + "/petriDish.txt");
  std::vector<Point3f> circles;

  int c;
  float radius, x, y;
  while (infile >> c >> radius >> x >> y){
    Point3f tc;
    tc.z = radius;
    tc.x = x;
    tc.y = y;
    circles.push_back(tc);
  }

  //Create mask based on circles[0]
  Point2f center = Point2f(circles[0].x, circles[0].y);
  radius = circles[0].z;
  Mat mask = Mat::zeros(background.rows, background.cols, CV_8UC1);
  circle(mask, center, radius, Scalar(255,255,255), -1, 8, 0 ); //-1 means filled

  background = Mat::zeros(background.rows, background.cols, CV_8UC1);
  temp.copyTo(background, mask ); // copy values of temp to background if mask is > 0.

  int nLabels;
  Mat labels(background.size(), CV_32S);
  Mat stats, centers;

  // Points in bb
  std::vector<Point2i> points;

  //stats
  int area;
  int w;
  int h;

  std::ofstream fs (output + "/positions.csv", std::ofstream::out);
  fs << "Frame,Time,Radius,x,y\n";

  for(int i = 0; i < frames-1;i++){
    cap >> in;
    if(i > skip_frames) {
      if(in.size() != background.size()){
        std::cerr << "Frame " << i << "is not valid\n";
        continue;
      }
    temp = Mat::zeros(background.rows, background.cols, CV_8UC1);
    in.copyTo(temp, mask ); // copy values of in to temp if mask is > 0.
    cvtColor(temp, dst, COLOR_RGB2GRAY, 0);
    absdiff(dst, background, temp);
    threshold(temp, dst, 30, 255, THRESH_BINARY);

    nLabels = connectedComponentsWithStats(dst, labels, stats, centers, 8, CV_32S);
    int label;
    // skip background i=0
    for (int j=1; j< nLabels;j++) {
      // get stats
      area = stats.at<int>(j,CC_STAT_AREA);
      if(area >= 80 && area <= 180) {
        w = stats.at<int>(j,CC_STAT_WIDTH);
        h = stats.at<int>(j,CC_STAT_HEIGHT);
        // w/h >= 0.7 && h/w >= 0.7 the bounding box of a circle is roughly a square.
        if((h/ double(w)) >=0.7 && (w/ double(h)) >=0.7) {
            points.clear();
            int bbX = stats.at<int>(j, CC_STAT_LEFT);
            int bbY = stats.at<int>(j, CC_STAT_TOP);
            for(int x = bbX; x < bbX + w; x++){
              for(int y = bbY; y < bbY + h; y++){
                // Mat::at(y,x) => Mat::at(Point(x,y)) see Docs.
                label = labels.at<int>(y,x);
                if(label == j){
                  points.push_back(Point2i(x,y));
                }
              }
            }
            minEnclosingCircle(points, center, radius);
            // Check that the points visible actually fill a circle.
            // This filters out artefacts that are diagonal - thus forming a square.
            if((area / (M_PI * radius * radius)) >= 0.5) {
              // take centroid of bb instead of fitted circle.
              fs << i <<","<< i/fps <<","<< radius <<","<< centers.at<double>(j,0) <<","<< centers.at<double>(j,1) << "\n";
              // printf("%d: Found cc%d at x=%f y=%f ",i, j, center.x, center.y);
              // printf("with points=%d r=%f area=%f\n", area, radius, (M_PI * radius * radius));
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