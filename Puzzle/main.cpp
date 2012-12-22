#include <opencv2/opencv.hpp>
int main(int argc, char *argv[])
{
  // Neues Bild erzeugen
  cv::Mat img = cv::Mat(480, 640, CV_8UC3);
  // Zeichne einen Kreis, eine Linie und einen Text
  cv::circle(img, cv::Point(200,150), 100, cv::Scalar(0,255,0), 4);
  cv::line(img, cv::Point(50,20), cv::Point(500,400), cv::Scalar(255,0,0), 3);
  cv::putText(img, "OpenCV Testprogramm....", cv::Point(40,380),
  cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255,255,100), 2);
  // Bild anzeigen
  cv::imshow("Image", img);
  // Warten auf einen Tastendruck
  cv::waitKey(0);
}
