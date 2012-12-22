#include <opencv2/opencv.hpp>

int main(int argc, char *argv[])
{
    // Bild laden
    cv::Mat img = cv::imread("puzzle.jpg");

    // Fenster erstellen
    cv::namedWindow("Binary", 0);
    cv::namedWindow("Kontur", 0);

    // Grauwert Bild erzeugen
    cv::Mat imgGrey;
    cv::cvtColor(img, imgGrey, CV_BGR2GRAY);


    // Binarisieren
    cv::Mat imgThresh;
    cv::threshold(imgGrey, imgThresh, 30, 255, cv::THRESH_BINARY);

    // Entrauschen: 2x "öffnen" mit 3x3 Kernel
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3), cv::Point(1, 1));
    cv::morphologyEx(imgThresh, imgThresh, cv::MORPH_OPEN, kernel, cv::Point(-1,-1), 2);

    // Bild anzeigen
    cv::imshow("Binary", imgThresh);

    // Kantenerkennung:
    //cv::Mat imgEdge;
    //cv::Canny(imgThresh, imgEdge, 50, 160);

    // Konturen suchen
    cv::Mat imgThreshTemp = imgThresh;
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(imgThreshTemp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    // Gesamte Liste mit den Konturen durchgehen
    for(unsigned int i = 0; i < contours.size(); ++i)
    {
        double featArcLength = cv::arcLength(cv::Mat(contours[i]), true);

        // Kontur aus der Liste löschen sofern die Länge zu klein
        if(featArcLength < 50)
        {
            contours.erase(contours.begin()+i);  // Kontur löschen
            i -= 1;  // Index auf die letzte Kontur zeigen lassen
        }
    }

    // Minimal umschliessendes Rechteck bestimmen
    std::vector<cv::RotatedRect> minRect(contours.size());

    for(unsigned int i = 0; i < contours.size(); i++)
    {
        minRect[i] = cv::minAreaRect(cv::Mat(contours[i]));
    }



    // Zeichnen jeder Kontur und jedes Rechtecks
    cv::Mat imgCont = img;
    for(unsigned int i = 0; i< contours.size(); i++)
       {
            // Kontur
            cv::drawContours(imgCont, contours, i, cv::Scalar(255,0,0), 2, 8);
            // Rechteck
            cv::Point2f rect_points[4];
            minRect[i].points(rect_points);
            for(int j = 0; j < 4; j++)
            {
                cv::line(imgCont, rect_points[j], rect_points[(j+1)%4], cv::Scalar(0,0,255), 2, CV_AA);
            }
       }

    // Bild anzeigen
    cv::imshow("Kontur", imgCont);

    // Warten auf einen Tastendruck
    cv::waitKey(0);
}


