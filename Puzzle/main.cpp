#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include <iostream>
#include <fstream>

// Helper Funktion:
// Cosinus des Zwischenwinkels von Vektor pt0->pt1 und Vektor pt0->pt2
double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0)
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

int main(int argc, char *argv[])
{
    // wenn kein argument mit dem Dateinamen mitgegeben wird:
    // Dateiname = "puzzle.jpg"
    const char* filename = "puzzle.jpg";
    if(argc == 2)
    {
        filename = argv[1];
    }

    // Bild laden
    cv::Mat img = cv::imread(filename);

    // Grauwert Bild erzeugen
    cv::Mat imgGrey;
    cv::cvtColor(img, imgGrey, CV_BGR2GRAY);

    // Weichzeichnen (vor-entrauschen)
    cv::Mat imgGreyBlur;
    cv::blur(imgGrey, imgGreyBlur, cv::Size(3,3));

    // Binarisieren
    cv::Mat imgBin;
    cv::threshold(imgGreyBlur, imgBin, 30, 255, cv::THRESH_BINARY);

    // Entrauschen: 2x öffnen mit 3x3 Kernel
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3), cv::Point(1, 1));
    cv::morphologyEx(imgBin, imgBin, cv::MORPH_OPEN, kernel, cv::Point(-1,-1), 2);

    // Kantenerkennung:
    //cv::Mat imgEdge;
    //cv::Canny(imgBin, imgEdge, 50, 50);

    // Konturen suchen
    cv::Mat imgBinTemp = imgBin;
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(imgBinTemp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    // Gesamte Liste mit den Konturen durchgehen: zu kurze Konturen löschen
    for(unsigned int i = 0; i < contours.size(); ++i)
    {
        double featArcLength = cv::arcLength(cv::Mat(contours[i]), true);

        // Kontur aus der Liste löschen sofern sie zu kurz ist
        if(featArcLength < 50)
        {
            contours.erase(contours.begin()+i);  // Kontur löschen
            i -= 1;  // Index auf die letzte Kontur zeigen lassen
        }
    }

    // Zeichnen jeder Kontur
    cv::Mat imgCont = img.clone();
    //cv::RNG rng(12345);
    for(unsigned int i = 0; i< contours.size(); i++)
    {
        // Kontur zeichnen
        //cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));   // zufallsfarben
        cv::Scalar color = cv::Scalar(255,50,0);
        cv::drawContours(imgCont, contours, i, color, 10, 8);
    }

    // Konturen filtern
    std::vector<std::vector<cv::Point> > pointsApprox;
    std::vector<cv::Point> pointApproxTemp;
    for(unsigned int i = 0; i < contours.size(); i++)
    {
        cv::approxPolyDP(cv::Mat(contours[i]), pointApproxTemp, 10, true);
        pointsApprox.push_back(pointApproxTemp);

        // gefilterte Kontur zeichnen
        const cv::Point* p = &pointsApprox[i][0];
        int n = (int)pointsApprox[i].size();
        cv::polylines(imgCont, &p, &n, 1, true, cv::Scalar(0,255,255), 2, CV_AA);
    }

    std::vector<std::vector<cv::Point> > corners;

    unsigned int number = 3;
    std::fstream datei("contour3_angle.txt", std::ios::out);

    // Ecken erkennen
    for(unsigned int i = 0; i < contours.size(); i++)
    {
        for(unsigned int j = 0; j < pointsApprox[i].size(); j++)
        {
            // Winkel berechnen zwischen diesem und den zwei nächsten Vertices.
            double cosine = std::fabs(angle(pointsApprox[i][(j+2)%pointsApprox[i].size()], pointsApprox[i][j], pointsApprox[i][(j+1)%pointsApprox[i].size()]));

            if(i == number)
            {
                datei << pointsApprox[number][j].x << '\t' << pointsApprox[number][j].y << '\t' << cosine << std::endl;
            }

            if(cosine < 0.15)
            {
                //corners[i].push_back(pointsApprox[i][(j+1)%pointsApprox[i].size()]);
                std::cout << "figure " << i << ": " << pointsApprox[i][(j+1)%pointsApprox[i].size()].x << ' ' << pointsApprox[i][(j+1)%pointsApprox[i].size()].y << std::endl;
                cv::ellipse(imgCont, pointsApprox[i][(j+1)%pointsApprox[i].size()], cv::Size(10,10), 0, 0, 360, cv::Scalar(0,0,255), 2, CV_AA);
            }
        }
    }

    datei.close();

    // Fenster erstellen
    cv::namedWindow("Kontur", 0);

    // Bild anzeigen
    cv::imshow("Kontur", imgCont);

    // Warten auf einen Tastendruck
    cv::waitKey(0);
}
