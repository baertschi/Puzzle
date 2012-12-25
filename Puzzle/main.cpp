#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include "cvpath2d.h"

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

int edge_distributed_rnd()
{
    static cv::RNG rng(time(NULL));
    double value = float(rng);

    if(value > 0.5)
    {
        return 255*(std::sqrt((value-0.5)/2) + 0.5);
    }
    else
    {
        return 255*(-std::sqrt((0.5-value)/2) + 0.5);
    }
}

static void onMouse( int event, int x, int y, int, void* /* std::vector<cv::Point > cont */)
{
    // Nur linke Maustaste abfragen
    if( event != CV_EVENT_LBUTTONDOWN )
    {
        return;
    }
    std::cout << "Button Cklick" << x << y << std::endl;

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

    // Wenn kein Bild gelesen wurde, programm beenden
    if(img.data == NULL)
    {
        cv::namedWindow("Error", 0);
        img = cv::imread("Error.png");                  // Bild im Bin ordner hinzufügen
        cv::imshow("Error", img);
        std::cout << "Error: Can't read image" << std::endl;
        cv::waitKey(0);

        // wär gloub no suber weme d fänster würd destroye, het aber d funktion nid kennt
        // cv::destroyAllWindows();
        return 0;
    }

    // Originalbild anzeigen
    cv::namedWindow("original", 0);
    cv::imshow("original", img);

    // Grauwert Bild erzeugen
    cv::Mat imgGrey;
    cv::cvtColor(img, imgGrey, CV_BGR2GRAY);
    //cv::namedWindow("grey", 0);                 // Für debugging
    //cv::imshow("grey", imgGrey);                // Für debugging

    // Histogramm ausgleichen
    //cv::Mat imgGreyHist;
    //cv::equalizeHist(imgGrey,imgGreyHist);
    //cv::namedWindow("hist", 0);
    //cv::imshow("hist", imgGreyHist);

    // Weichzeichnen (vor-entrauschen)
    cv::Mat imgGreyBlur;
    cv::blur(imgGrey, imgGreyBlur, cv::Size(3,3));
    //cv::namedWindow("blur", 0);                 // Für debugging
    //cv::imshow("blur", imgGreyBlur);            // Für debugging


    // Binarisieren
    cv::Mat imgBin;
    cv::threshold(imgGreyBlur, imgBin, cv::mean(imgGreyBlur)[0]-10, 255, cv::THRESH_BINARY);

    //cv::adaptiveThreshold(imgGreyBlur, imgBin,255,CV_ADAPTIVE_THRESH_MEAN_C,CV_THRESH_BINARY,2001,10);
    //cv::namedWindow("bin", 0);                  // Für debugging
    //cv::imshow("bin", imgBin);                  // Für debugging

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

    // Zeichnen des Binärbildes mit unterschiedlich gefärbten Puzzleteilen
    cv::cvtColor(imgBin, imgBin, CV_GRAY2BGR);

    for(unsigned int i = 0; i< contours.size(); i++)
    {
        cv::Scalar color = CV_RGB(edge_distributed_rnd(), edge_distributed_rnd(), edge_distributed_rnd());
        cv::drawContours(imgBin, contours, i, color, -1, 8);
    }
    cv::namedWindow("Binär", 0);
    cv::imshow("Binär", imgBin);

    // Zeichnen jeder Kontur
    cv::Mat imgCont = img.clone();
    for(unsigned int i = 0; i< contours.size(); i++)
    {
        cv::Scalar color = CV_RGB(0,50,255);
        cv::drawContours(imgCont, contours, i, color, 4, 8);
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
        cv::polylines(imgCont, &p, &n, 1, true, CV_RGB(255,255,0), 2, CV_AA);
    }


    // Ecken erkennen
    std::vector<std::vector<cv::Point> > corners;
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
            }
        }
    }

    // Grundrechteck zeichnen
    for(unsigned int i = 0; i < contours.size(); i++)
    {
        cv::Scalar color = CV_RGB(0,100,0);
        cv::drawContours(imgCont, corners, i, color, 2, CV_AA);
    }

    // Ecken zeichnen
    for(unsigned int i = 0; i < corners.size(); i++)
    {
        for(unsigned int j = 0; j < corners[i].size(); j++)
        {
            cv::circle(imgCont, corners[i][j], 8, CV_RGB(255,0,0), 2, CV_AA);
        }
    }


    cv::imshow("Kontur", imgCont);
    cv::waitKey(0);
    return 0;


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

    // Seitenwandschwerpunkte finden
    // (eigentlich kein Schwerpunkt, sondern der Mittelpunkt des
    // minimalen umschliessenden Rechtecks einer Seitenwand)
    std::vector<std::vector<cv::Point> > sideCentroids;
    for(unsigned int i = 0; i < sides.size(); i++)
    {
        sideCentroids.push_back(std::vector<cv::Point>());
        for(unsigned int j = 0; j < 4; j++)
        {
            cv::RotatedRect minRect;
            minRect = cv::minAreaRect(sides[i][j]);
            sideCentroids[i].push_back(minRect.center);
        }
    }

    // Seitenwandschwerpunkte zeichnen
    for(unsigned int i = 0; i < sideCentroids.size(); i++)
    {
        for(unsigned int j = 0; j < sideCentroids[i].size(); j++)
        {
            cv::circle(imgCont, sideCentroids[i][j], 5, CV_RGB(255,255,0), CV_FILLED, CV_AA);
        }
    }

    // Gender finden: positiv = 1, negativ = -1, neutral = 0
    std::vector<std::vector<int> > genders;
    for(unsigned int i = 0; i < sideCentroids.size(); i++)
    {
        genders.push_back(std::vector<int>());
        for(unsigned int j = 0; j < 4; j++)
        {
            double dist = cv::pointPolygonTest(cv::Mat(corners[i]), sideCentroids[i][j], true);
            if(std::fabs(dist) < 3)
            {
                // wenn praktisch auf der Linie -> wir als gerade Kannte angenommen = Peutral
                genders[i].push_back(0);
            }
            else
            {
                // wenn innerhalb des Grundrechtecks, Gender = Negativ, ansonsten Gender = Positiv
                genders[i].push_back(dist < 0? 1 : -1);
            }
        }
    }

    // Seitenwände noch glätten bevor sie der Funktion matchShape übergeben werden
    std::vector<std::vector<std::vector<cv::Point> > > sidesFiltered;
    std::vector<cv::Point> sidesFilteredTemp;
    for(unsigned int i = 0; i < sides.size(); i++)
    {
        sidesFiltered.push_back(std::vector<std::vector<cv::Point> >());
        for(unsigned int j = 0; j < sides[i].size(); j++)
        {
            cv::approxPolyDP(cv::Mat(sides[i][j]), sidesFilteredTemp, 2, false);
            sidesFiltered[i].push_back(sidesFilteredTemp);
        }
    }


    // Mouse einlesen und geklickte Seitenwand bestimmen
    cv::setMouseCallback( "original", onMouse, 0);    //, &contours


    // Vergleich von sides[0][0] mit den anderen Puzzleteilen.
    const unsigned int puzzle = 0;
    const unsigned int side = 3;
    double maxResult = 0, minResult = 999999;
    unsigned int maxElement[2] = {0, 0};
    std::vector<std::vector<double> > results;
    for(unsigned int i = 0; i < sidesFiltered.size(); i++)
    {
        results.push_back(std::vector<double>());
        if(i == puzzle)
        {
            // wenn das basis-Puzzleteil dran ist, skippen.
            continue;
        }

        for(unsigned int j = 0; j < 4; j++)
        {
            if(genders[puzzle][side] + genders[i][j] == 0)
            {
                results[i].push_back(1/cv::matchShapes(cv::Mat(sidesFiltered[puzzle][side]), cv::Mat(sidesFiltered[i][j]), CV_CONTOURS_MATCH_I3, 0));
                maxResult = std::max(maxResult, results[i][j]);
                if(maxResult == results[i][j])
                {
                    maxElement[0] = i;
                    maxElement[1] = j;
                }
                minResult = std::min(minResult, results[i][j]);
            }
            else
            {
                // wenn Gender nicht übereinstimmt, Übereinstimmung = 0
                results[i].push_back(0);
            }
        }
    }



    // Zeichnen der Übereinstimmung: zuerst feine Konturen
    cv::Mat imgContSimilar = cv::Mat::zeros(img.size(), CV_8UC3);
    for(unsigned int i = 0; i < contours.size(); i++)
    {
        cv::drawContours(imgContSimilar, contours, i, CV_RGB(30,30,30), 2, CV_AA);
    }

    // Zeichnen der Übereinstimmung
    std::vector<std::vector<cv::Point> > temp;
    temp = sidesFiltered[puzzle];
    cv::drawContours(imgContSimilar, temp, side, CV_RGB(255,0,0), 10, 8);
    std::cout << std::endl << "Puzzle piece " << puzzle << ", side " << side << " compared to:" << std::endl;
    for(unsigned int i = 0; i < sidesFiltered.size(); i++)
    {
        if(i == puzzle)
        {
            // wenn das basis-Puzzleteil dran ist, skippen.
            continue;
        }

        for(unsigned int j = 0; j < 4; j++)
        {
            if(results[i][j] != 0)
            {
                // wenn das Puzzleteil überhaupt übereinstimmt, farbsensitiv Seitenwände zeichnen
                temp = sidesFiltered[i];
                double value = (results[i][j] - minResult) / (maxResult - minResult);
                value = (255-50)*value + 50;
                cv::Scalar color = CV_RGB(0, value, 0);
                cv::drawContours(imgContSimilar, temp, j, color, 10, CV_AA);
                std::cout << "-> Puzzle piece " << i << ", side " << j << ": " << results[i][j] << std::endl;
            }
        }
    }


    // Zeichnen einer Bezierlinie vom einen Puzzleteil zum anderen.
    // Bezier Startkurven-Faktor - abhängig von der Gesamtdistanz:
    //cv::Point start = sideCentroids[puzzle][side];
    cv::Point start = (sides[puzzle][side][sides[puzzle][side].size()-1] - sides[puzzle][side][0])*0.5;
    start += sides[puzzle][side][0];
    cv::Point end = (sides[maxElement[0]][maxElement[1]][sides[maxElement[0]][maxElement[1]].size()-1] - sides[maxElement[0]][maxElement[1]][0])*0.5;
    end += sides[maxElement[0]][maxElement[1]][0];
    //cv::Point end = sideCentroids[maxElement[0]][maxElement[1]];
    //cv::Point end = sideCentroids[maxElement[0]][maxElement[1]];
    double curveFactor = norm(end - start)/2;

    // Vektor beim Basispuzzleteil:
    cv::Point startVector = start - sideCentroids[puzzle][(side + 2)%4];
    startVector *= curveFactor/cv::norm(startVector);

    // Vektor beim Zeilpuzzleteil:
    cv::Point endVector = end - sideCentroids[maxElement[0]][(maxElement[1] + 2)%4];
    endVector *= curveFactor/cv::norm(endVector);

    cv::Path2D bezierLine;
    bezierLine.restart(start.x, start.y);
    bezierLine.curveTo(start + startVector, end + endVector, end);
    cv::drawPath2D(imgContSimilar, bezierLine, CV_RGB(0,0,240), -1, 8, CV_AA);

    // Fenster erstellen
    cv::namedWindow("Kontur", 0);
    cv::namedWindow("Aehnlichkeit", 0);

    // Bild anzeigen
    cv::imshow("Kontur", imgCont);
    cv::imshow("Aehnlichkeit", imgContSimilar);

    // Warten auf einen Tastendruck
    cv::waitKey(0);

    // wär gloub no suber weme d fänster würd destroye, het aber d funktion nid kennt
    // Fenster schliessen und Bilder "freigeben"
    //cv::destroyAllWindows();
    //cv::ReleaseImage(img);
    //cv::ReleaseImage(imgContSimilar);
    //cv::ReleaseImage(imgCont);
    //cv::ReleaseImage(imgBinTemp);
    //cv::ReleaseImage(imgBin);
    //cv::ReleaseImage(kernel);
    //cv::ReleaseImage(imgGrey);
    //cv::ReleaseImage(imgGreyBlure);
    //cv::ReleaseImage(imgGreyHist);


}
