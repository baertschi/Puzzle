#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <stdlib.h>

cv::RNG rng(12345);
int thresh = 5;
int max_thresh = 200;
cv::Mat imgCont;
std::vector<std::vector<cv::Point> > contours;

void approx(int, void* );

int main(int argc, char *argv[])
{
    // Bild laden
    cv::Mat img = cv::imread("puzzle.jpg");

    // Fenster erstellen
    cv::namedWindow("Binary", 0);
    cv::namedWindow("Kontur", 1);

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
    //cv::Canny(imgThresh, imgEdge, 50, 50);

    // Konturen suchen
    cv::Mat imgThreshTemp = imgThresh;
    //std::vector<std::vector<cv::Point> > contours;
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
    /*cv::Mat*/ imgCont = img;
    for(unsigned int i = 0; i< contours.size(); i++)
    {
        // Kontur
        //cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
        cv::Scalar color = cv::Scalar(255,50,0);
        cv::drawContours(imgCont, contours, i, color, 10, 8);
        // Rechteck
        cv::Point2f rect_points[4];
        minRect[i].points(rect_points);
        for(int j = 0; j < 4; j++)
        {
            cv::line(imgCont, rect_points[j], rect_points[(j+1)%4], cv::Scalar(0,0,120), 4, CV_AA);
        }
    }

    // Bild anzeigen
    cv::imshow("Kontur", imgCont);

    // Kontur in Datei ablegen wenn Aufrufargument == "Puzzle.exe <filename> <Konturnummer>"
    if(argc == 3)
    {
        char* filename = argv[1];
        int number = argv[2][0] - '0';

        std::fstream datei(filename, std::ios::out);

        for(unsigned int i = 0; i < contours[number].size(); i++)
        {
            datei << contours[number][i].x << '\t' << contours[number][i].y << std::endl;
        }
        datei.close();
    }


    // slider bereitstellen
    cv::createTrackbar("epsilon", "Kontur", &thresh, max_thresh, approx);
    approx(0, 0);

    // Warten auf einen Tastendruck
    cv::waitKey(0);
}


void approx(int, void* )
{
    cv::Mat imgTemp = imgCont.clone();
    std::vector<cv::Point> approx;

    for( size_t i = 0; i < contours.size(); i++ )
    {
        cv::approxPolyDP(cv::Mat(contours[i]), approx, thresh, true);   // cv::arcLength(cv::Mat(contours[i]), true)*0.02

        // draw the approximated vertices on the temp image
        const cv::Point* p = &approx[0];
        int n = (int)approx.size();
        cv::polylines(imgTemp, &p, &n, 1, true, cv::Scalar(0,255,255), 2, CV_AA);
    }

    // Show in a window
    cv::imshow("Kontur", imgTemp);
}
