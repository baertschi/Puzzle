#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

int main(int argc, char *argv[])
{
    std::vector<std::vector<cv::Point> > contour;
    contour.push_back(std::vector<cv::Point>());

    contour[0].push_back(cv::Point(10, 80));
    contour[0].push_back(cv::Point(80, 90));
    contour[0].push_back(cv::Point(50, 10));

    cv::Mat imgContour(cv::Size(100, 100), CV_8UC3);

    // this should now draw 3 connected points -> 2 lines in between.
    // but it draws a closed polygon over the 3 points??
    cv::drawContours(imgContour, contour, 0, cv::Scalar(255,0,0), 1, 8);

    cv::imshow("Contour", imgContour);

    cv::waitKey(0);

    return 0;
}
