#include <opencv2/opencv.hpp>
#include <stdlib.h>

void approx(int, void* );

int main(int argc, char *argv[])
{
    // wenn kein argument mit dem Dateinamen mitgegeben wird:
    // Dateiname = "puzzle.jpg"
    char* filename = "puzzle.jpg";
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

    // Minimal umschliessendes Rechteck bestimmen
    std::vector<cv::RotatedRect> rectMin(contours.size());

    for(unsigned int i = 0; i < contours.size(); i++)
    {
        rectMin[i] = cv::minAreaRect(cv::Mat(contours[i]));
    }



    // Zeichnen jeder Kontur und jedes Rechtecks
    cv::Mat imgCont = img.clone();
    //cv::RNG rng(12345);
    for(unsigned int i = 0; i< contours.size(); i++)
    {
        // Kontur zeichnen
        //cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));   // zufallsfarben
        cv::Scalar color = cv::Scalar(255,50,0);
        cv::drawContours(imgCont, contours, i, color, 10, 8);
        // Rechteck zeichnen
        cv::Point2f pointsRect[4];
        rectMin[i].points(pointsRect);
        for(int j = 0; j < 4; j++)
        {
            cv::line(imgCont, pointsRect[j], pointsRect[(j+1)%4], cv::Scalar(0,0,120), 4, CV_AA);
        }
    }

    // Kontur filtern
    std::vector<cv::Point> pointsApprox;
    for( size_t i = 0; i < contours.size(); i++ )
    {
        cv::approxPolyDP(cv::Mat(contours[i]), pointsApprox, 10, true);

        // gefilterte Kontur zeichnen
        const cv::Point* p = &pointsApprox[0];
        int n = (int)pointsApprox.size();
        cv::polylines(imgCont, &p, &n, 1, true, cv::Scalar(0,255,255), 2, CV_AA);
    }

    // Fenster erstellen
    cv::namedWindow("Kontur", 0);

    // Bild anzeigen
    cv::imshow("Kontur", imgCont);

    // Warten auf einen Tastendruck
    cv::waitKey(0);
}
