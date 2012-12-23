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


    // Ecken erkennen
    std::vector<std::vector<cv::Point> > corners;
    corners.clear();

    for(unsigned int i = 0; i < contours.size(); i++)
    {
        corners.push_back(std::vector<cv::Point>());
        for(unsigned int j = 0; j < pointsApprox[i].size(); j++)
        {
            // Winkel berechnen zwischen diesem und den zwei nächsten Vertices.
            double cosine = std::fabs(angle(pointsApprox[i][(j+2)%pointsApprox[i].size()], pointsApprox[i][j], pointsApprox[i][(j+1)%pointsApprox[i].size()]));

            // in datei abspeichern
            //datei << pointsApprox[number][j].x << '\t' << pointsApprox[number][j].y << '\t' << cosine << std::endl;

            // wenn Winkel > 81°, als Ecke abspeichern und visuell markieren
            if(cosine < 0.15)
            {
                corners[i].push_back(pointsApprox[i][(j+1)%pointsApprox[i].size()]);
                //std::cout << "figure " << i << ": " << pointsApprox[i][(j+1)%pointsApprox[i].size()].x << ' ' << pointsApprox[i][(j+1)%pointsApprox[i].size()].y << std::endl;
                cv::ellipse(imgCont, pointsApprox[i][(j+1)%pointsApprox[i].size()], cv::Size(10,10), 0, 0, 360, cv::Scalar(0,0,255), 2, CV_AA);
            }
        }
    }

    // achtung: wüüüüüüüst.
    // Konturen zerschneiden in jeweils vier Teilkonturen,
    // welche die vier Seitenwände des Puzzleteils darstellen.
    // Es wird wieder die ganze Kontur verwendet (nicht die approximierte):
    // jeder Punkt wird mit den vorher ermittelten Eckpunkten verglichen.
    // Bei einer Übereinstimmung wird der anschliessende Bereich bis zum
    // nächsten Eckpunkt als wand definiert.
    // Da die Eckpunkte in der gleichen Reihenfolge gesucht wurden wie die
    // Konturen, muss nicht jeder Punkt der Kontur mit jedem der vier Eckpunkte
    // verglichen werden, sondern ab dem auftreten des Eckpunktes 0 jeweils
    // einfach der Reihe nach.
    std::vector<std::vector<std::vector<cv::Point> > > sides;
    for(unsigned int i = 0; i < contours.size(); i++)
    {
        sides.push_back(std::vector<std::vector<cv::Point> >());
        int corner_counter = 0;

        // jeden Punkt der Kontur durchgehen und nach den Eckpunkten suchen
        bool corner_found = false;
        int j = 0;
        do
        {
            if(contours[i][j] == corners[i][corner_counter%4])
            {
                if(corner_found && corner_counter == 4)
                {
                    // Wenn schon einmal rund herum, abbrechen
                    break;
                }

                corner_found = true;
                corner_counter++;   // ab jetzt nach dem nächsten Eckpunkt suchen
                sides[i].push_back(std::vector<cv::Point>());
                sides[i][corner_counter - 1].push_back(contours[i][j]);
            }
            else
            {
                if(corner_found)
                {
                    sides[i][corner_counter - 1].push_back(contours[i][j]);
                }
            }

            j++;
            j %= contours[i].size();
        }
        while(corner_counter < 5);
    }

    // Vergleich von walls[0][0] mit den anderen Puzzleteilen.
    // Gleichheit-abhängiges Zeichnen der Teil-Konturen
    cv::Mat imgContSimilar = cv::Mat::zeros(img.size(), CV_8UC3);
    std::vector<std::vector<cv::Point> > pointTemp;
    pointTemp = sides[0];
    cv::drawContours(imgContSimilar, pointTemp, 0, cv::Scalar(0,0,255), 10, 8);
    std::cout << std::endl << "Puzzle piece 0, side 0 compared to:" << std::endl;
    for(unsigned int i = 1; i < sides.size(); i++)
    {
        for(unsigned int j = 0; j < 4; j++)
        {
            double result = cv::matchShapes(cv::Mat(sides[0][0]), cv::Mat(sides[i][j]), CV_CONTOURS_MATCH_I3, 0);
            std::cout << "Puzzle piece " << i << ", side " << j << ": " << result << std::endl;
            cv::Scalar color = cv::Scalar(255/2.5*result,255/2.5*result,255/2.5*result);
            pointTemp = sides[i];
            cv::drawContours(imgContSimilar, pointTemp, j, color, 10, 8);
        }
    }

    // Fenster erstellen
    cv::namedWindow("Kontur", 0);
    cv::namedWindow("Aehnlichkeit", 0);

    // Bild anzeigen
    cv::imshow("Kontur", imgCont);
    cv::imshow("Aehnlichkeit", imgContSimilar);

    // Warten auf einen Tastendruck
    cv::waitKey(0);
}
