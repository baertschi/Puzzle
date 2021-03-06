#include "mainwindow.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include "cvpath2d.h"

// Globale Variablen
int mouse_x, mouse_y;
bool mouse_flag = false;
bool close_flag = true;     // Als true initialisieren, f�r abfrage vor loadimage
cv::Mat img;

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
        return ;
    }
    mouse_x = x;
    mouse_y = y;
    std::cout << "Button Cklick   x: " << x << " y: " << y << std::endl;

    mouse_flag = true;
}

#define WEIGHT_MATCHSHAPE 0
#define WEIGHT_LENGTH 1
#define WEIGHT_ARC 1
#define WEIGHT_AREA 1

static double compare(std::vector<cv::Point>* basis_side, std::vector<cv::Point>* compare_side)
{
    // Hier werden beide Puzzleteile mit verschieden gewichteten Features verglichen

    // Hu-Momente (mit Routine matchShape)
    double result_matchShapes = cv::matchShapes(*basis_side, *compare_side, CV_CONTOURS_MATCH_I1, 0);

    // Umfang (Ua/|Ua-Ub|)
    double length_basis = cv::arcLength(*basis_side, false);
    double length_compare = cv::arcLength(*compare_side, false);
    double result_length = length_basis / std::fabs(length_basis - length_compare);

    // Tiefe der Ausbuchtung (Ta/|Ta-Tb|)
    double arc_basis = std::min(cv::minAreaRect(*basis_side).size.height, cv::minAreaRect(*basis_side).size.width);
    double arc_compare = std::min(cv::minAreaRect(*compare_side).size.height, cv::minAreaRect(*compare_side).size.width);
    double result_arc = arc_basis / std::fabs(arc_basis - arc_compare);

    // Fl�che der Ausbuchtung (angenommen, die geraden St�cke daneben haben keine "Fl�che") (Fa/|Fa-Fb|)
    double area_basis = cv::contourArea(*basis_side);
    double area_compare = cv::contourArea(*compare_side);
    double result_area = area_basis / std::fabs(area_basis - area_compare);

    std::cout << result_matchShapes << '\t' << result_length << '\t' << result_arc << '\t' << result_area << std::endl;

    // Gewichten der unterschiedlichen Features
    return result_matchShapes*WEIGHT_MATCHSHAPE + result_length*WEIGHT_LENGTH + result_arc*WEIGHT_ARC + result_area*WEIGHT_AREA;
}

/********************************************************************************************************/



void MainWindow::imageProcessing()
{

        // Fenster erstellen
        cv::namedWindow("Kontur", 0);
        cv::namedWindow("Aehnlichkeit", 0);
        cv::namedWindow("Originalbild", 0);

        // Originalbild anzeigen
        cv::imshow("Originalbild", img);

        // Grauwert Bild erzeugen
        cv::Mat imgGrey;
        cv::cvtColor(img, imgGrey, CV_BGR2GRAY);
        //cv::namedWindow("grey", 0);                 // F�r debugging
        //cv::imshow("grey", imgGrey);                // F�r debugging

        /*
        // Histogramm ausgleichen
        cv::Mat imgGreyHist;
        cv::equalizeHist(imgGrey,imgGreyHist);
        cv::namedWindow("hist", 0);
        cv::imshow("hist", imgGreyHist);
        */

        // Weichzeichnen (vor-entrauschen)
        cv::Mat imgGreyBlur;
        cv::blur(imgGrey, imgGreyBlur, cv::Size(3,3));
        //cv::namedWindow("blur", 0);                 // F�r debugging
        //cv::imshow("blur", imgGreyBlur);            // F�r debugging

        // Binarisieren
        cv::Mat imgBin;
        //cv::threshold(imgGreyBlur, imgBin, cv::mean(imgGreyBlur)[0]-10, 255, cv::THRESH_BINARY);

        cv::adaptiveThreshold(imgGreyBlur, imgBin,255,CV_ADAPTIVE_THRESH_MEAN_C,CV_THRESH_BINARY,2001,10);
        //cv::namedWindow("bin", 0);                  // F�r debugging
        //cv::imshow("bin", imgBin);                  // F�r debugging

        // Entrauschen: 2x �ffnen mit 3x3 Kernel
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3), cv::Point(1, 1));
        cv::morphologyEx(imgBin, imgBin, cv::MORPH_OPEN, kernel, cv::Point(-1,-1), 2);

        // Kantenerkennung:
        //cv::Mat imgEdge;
        //cv::Canny(imgBin, imgEdge, 50, 50);

        // Konturen suchen
        cv::Mat imgBinTemp = imgBin;
        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(imgBinTemp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        qDebug() << "gefundene konturen: " << contours.size();
        //std::cout << "gefundene konturen: " << contours.size() << std::endl;

        // Seitenwand Gr�sse absch�tzen
        double puzLength = 0, maxLength = 0;
        for(unsigned int i = 0; i < contours.size(); ++i)
        {
            puzLength = cv::arcLength(cv::Mat(contours[i]), true);

            // Maximum Finden
            if(puzLength > maxLength)
            {
                maxLength = puzLength;
            }
        }
        // Seitenl�nge absch�tzen
        double siteLength = maxLength / 4;
        qDebug() << "gesch�tzte Seitenl�nge: " << siteLength;

        // Gesamte Liste mit den Konturen durchgehen: zu kurze Konturen l�schen
        for(unsigned int i = 0; i < contours.size(); ++i)
        {
            double featArcLength = cv::arcLength(cv::Mat(contours[i]), true);

            // Kontur aus der Liste l�schen sofern sie zu kurz ist
            if(featArcLength < siteLength/10)
            {
                contours.erase(contours.begin()+i);  // Kontur l�schen
                i -= 1;  // Index auf die letzte Kontur zeigen lassen
            }
        }

        // Zeichnen des Bin�rbildes mit unterschiedlich gef�rbten Puzzleteilen
        cv::cvtColor(imgBin, imgBin, CV_GRAY2BGR);

        for(unsigned int i = 0; i< contours.size(); i++)
        {
            cv::Scalar color = CV_RGB(edge_distributed_rnd(), edge_distributed_rnd(), edge_distributed_rnd());
            cv::drawContours(imgBin, contours, i, color, -1, 8);
        }
        cv::namedWindow("Puzzleteile", 0);
        cv::imshow("Puzzleteile", imgBin);

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
            cv::approxPolyDP(cv::Mat(contours[i]), pointApproxTemp, (int)(siteLength/100 + 0.5), true);
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
                // Winkel berechnen zwischen diesem und den zwei n�chsten Vertices.
                double cosine = std::fabs(angle(pointsApprox[i][(j+2)%pointsApprox[i].size()], pointsApprox[i][j], pointsApprox[i][(j+1)%pointsApprox[i].size()]));

                // in datei abspeichern
                //datei << pointsApprox[number][j].x << '\t' << pointsApprox[number][j].y << '\t' << cosine << std::endl;

                // wenn Winkel > 81�, als Ecke abspeichern und visuell markieren
                if(cosine < 0.15)
                {
                    corners[i].push_back(pointsApprox[i][(j+1)%pointsApprox[i].size()]);
                }
            }
        }

        // Ecken zeichnen
        for(unsigned int i = 0; i < corners.size(); i++)
        {
            for(unsigned int j = 0; j < corners[i].size(); j++)
            {
                cv::circle(imgCont, corners[i][j], 8, CV_RGB(255,0,0), 2, CV_AA);
            }
        }

        // Wenn nicht �berall 4 Ecken gefunden wurden, die konturen entfernen, die nicht 4 Ecken haben.
        // Grund daf�r k�nnte sein:
        // - schlechte Binarisierung (nicht das ganze Puzzleteil erwischt)
        // - Ecke war abgerundet -> die Funktion approxPolyDP ha zwei Teil-Ecken daraus gemacht
        // - Die Ecke war nicht gen�gend rechtwinklig --> Threshold bei Zeile 173
        for(unsigned int i = 0; i < corners.size(); i++)
        {
            if(corners[i].size() != 4)
            {
                // Text hinzuf�gen
                int baseLine = 0;
                cv::Size textSize = cv::getTextSize("not 4 corners found", cv::FONT_HERSHEY_COMPLEX, 1, 2, &baseLine);
                cv::Point textPoint = cv::minAreaRect(contours[i]).center;
                cv::putText(imgCont, "not 4 corners found", textPoint - cv::Point(textSize.width/2, 0), cv::FONT_HERSHEY_COMPLEX, 1, CV_RGB(240,240,240), 2);

                // Gesamte Kontur entfernen
                corners.erase(corners.begin() + i);
                contours.erase(contours.begin() + i);
            }
        }

        // Wenn jetzt keine einzige Kontur mehr drin ist, Melden und abbrechen
        if(corners.size() == 0)
        {
            // Text hinzuf�gen
            int baseLine = 0;
            cv::Size textSize = cv::getTextSize("Error: no corners found at all", cv::FONT_HERSHEY_COMPLEX, 2, 2, &baseLine);
            cv::Point textPoint(imgCont.size().width/2, imgCont.size().height/2);
            cv::putText(imgCont, "Error: no corners found at all", textPoint - cv::Point(textSize.width/2, textSize.height/2), cv::FONT_HERSHEY_COMPLEX, 2, CV_RGB(255,255,255), 2);
            cv::putText(imgCont, "press any key to exit", textPoint + cv::Point(textSize.width/2, textSize.height/2), cv::FONT_HERSHEY_COMPLEX, 2, CV_RGB(255,255,255), 2);

            // Dannach auf Tastendruck warten und abbrechen.
            cv::waitKey(0);
            // Konsolenfenster schliessen
            cv::destroyAllWindows();
            // Gui-Anwendung schliessen
            close();
            return;
        }

        // Grundrechtecke zeichnen
        for(unsigned int i = 0; i < contours.size(); i++)
        {
            cv::Scalar color = CV_RGB(0,100,0);
            cv::drawContours(imgCont, corners, i, color, 2, CV_AA);
        }

        cv::imshow("Kontur", imgCont);



        // achtung: w�������st.
        // Konturen zerschneiden in jeweils vier Teilkonturen,
        // welche die vier Seitenw�nde des Puzzleteils darstellen.
        // Es wird wieder die ganze Kontur verwendet (nicht die approximierte):
        // jeder Punkt wird mit den vorher ermittelten Eckpunkten verglichen.
        // Bei einer �bereinstimmung wird der anschliessende Bereich bis zum
        // n�chsten Eckpunkt als wand definiert.
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
                    corner_counter++;   // ab jetzt nach dem n�chsten Eckpunkt suchen
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

        // Konturen, Grundrechtecke und Schwerpunkte anzeigen
        cv::imshow("Kontur", imgCont);


        // Gender finden: positiv = 1, negativ = -1, neutral = 0
        std::vector<std::vector<int> > genders;
        for(unsigned int i = 0; i < sideCentroids.size(); i++)
        {
            genders.push_back(std::vector<int>());
            for(unsigned int j = 0; j < 4; j++)
            {
                double dist = cv::pointPolygonTest(cv::Mat(corners[i]), sideCentroids[i][j], true);
                if(std::fabs(dist) < 5)
                {
                    // wenn praktisch auf der Linie -> wir als gerade Kante angenommen = Neutral
                    genders[i].push_back(0);
                }
                else
                {
                    // wenn innerhalb des Grundrechtecks, Gender = Negativ, ansonsten Gender = Positiv
                    genders[i].push_back(dist < 0? 1 : -1);
                }
            }
        }

        // Seitenw�nde noch gl�tten bevor sie der Funktion matchShape �bergeben werden
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
        cv::setMouseCallback( "Originalbild", onMouse, 0);    //, &contours

        while(1)
        {
            char key = 0;
            // warten, bis mit der Maus geklickt wird
            while(!mouse_flag && !close_flag && key != 27)
            {
                key = cv::waitKey(1);

            }
            mouse_flag = false;

            // wenn jedoch ESC gedr�ckt wurde, endlos-loop abbrechen.
            if(key == 27 || close_flag)
            {
                break;
            }

            double distance, dist_side, mindlist = -100;
            unsigned int piece, piece_side;
            cv::Point2i mouse_point;
            mouse_point.x = mouse_x;
            mouse_point.y = mouse_y;
            // Vergleich mit den Puzzleteilen
            for(unsigned int k = 0; k < contours.size(); k++)
            {
                distance = cv::pointPolygonTest(contours[k],mouse_point, true);
                std::cout << "Distanz " << distance << ", side " << k << std::endl;
                if(distance >= 0)
                {
                    piece = k;
                    break;
                }
                else if(distance > mindlist)        // Gr�sser, weil Distanz einen negativen Wert hat f�r Klicks ausserhalb des Teilchens
                {
                    mindlist = distance;
                    piece = k;
                }
            }
            // Seite erkennen
            mindlist = 100;
            for(unsigned int k = 0; k < 4; k++)
            {
                dist_side = cv::norm(mouse_point-sideCentroids[piece][k]);

                std::cout << "Distanz " << dist_side << ", side " << k << std::endl;
                if(dist_side < mindlist)
                {
                    piece_side = k;
                    mindlist = dist_side;
                }
            }
            //  std::cout << "Piece: "  << piece << " Side Selected " << piece_side <<  std::endl;


            // Vergleich der gew�hlten Seitenwand mit den anderen Puzzleteilen.
            double maxResult = 0, minResult = 999999;
            unsigned int maxElement[2] = {0, 0};
            std::vector<std::vector<double> > results;
            for(unsigned int i = 0; i < sidesFiltered.size(); i++)
            {
                results.push_back(std::vector<double>());
                if(i == piece)
                {
                    // wenn das basis-Puzzleteil dran ist, skippen.
                    continue;
                }

                for(unsigned int j = 0; j < 4; j++)
                {
                    if(genders[piece][piece_side] + genders[i][j] == 0)
                    {
                        // �berpr�fung der �hnlichkeit mit verschiedenen, gewichteten Features
                        std::vector<cv::Point> basis_side = cv::Mat(sidesFiltered[piece][piece_side]);
                        std::vector<cv::Point> compare_side = cv::Mat(sidesFiltered[i][j]);
                        results[i].push_back(compare(&basis_side, &compare_side));

                        // Zus�tzliche �berpr�fung mit Farbe

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
                        // wenn Gender nicht �bereinstimmt, �bereinstimmung = 0
                        results[i].push_back(0);
                    }
                }
            }



            // Zeichnen der �bereinstimmung: zuerst feine Konturen
            cv::Mat imgContSimilar = cv::Mat::zeros(img.size(), CV_8UC3);
            for(unsigned int i = 0; i < contours.size(); i++)
            {
                cv::drawContours(imgContSimilar, contours, i, CV_RGB(30,30,30), 2, CV_AA);
            }

            // Zeichnen der �bereinstimmung
            std::vector<cv::Point> side = sidesFiltered[piece][piece_side];
            const cv::Point *pts = (const cv::Point*) cv::Mat(side).data;
            int npts = cv::Mat(side).rows;
            cv::polylines(imgContSimilar, &pts,&npts, 1, false, CV_RGB(255,0,0), 10, CV_AA);
            std::cout << std::endl << "Puzzle piece " << piece << ", side " << piece_side << " compared to:" << std::endl;
            for(unsigned int i = 0; i < sidesFiltered.size(); i++)
            {
                if(i == piece)
                {
                    // wenn das basis-Puzzleteil dran ist, skippen.
                    continue;
                }

                for(unsigned int j = 0; j < 4; j++)
                {
                    if(results[i][j] != 0)
                    {
                        // wenn das Puzzleteil �berhaupt �bereinstimmt, farbsensitiv Seitenw�nde zeichnen
                        std::vector<cv::Point> side = sidesFiltered[i][j];
                        double value = (results[i][j] - minResult) / (maxResult - minResult);
                        value = (255-30)*value + 30;
                        cv::Scalar color = CV_RGB(30, value, 30);
                        const cv::Point *pts = (const cv::Point*) cv::Mat(side).data;
                        int npts = cv::Mat(side).rows;
                        cv::polylines(imgContSimilar, &pts,&npts, 1, false, color, 10, CV_AA);
                        std::cout << "-> Puzzle piece " << i << ", side " << j << ": " << results[i][j] << std::endl;
                    }
                }
            }


            // Zeichnen einer Bezierlinie vom einen Puzzleteil zum anderen.
            // Bezier Startkurven-Faktor - abh�ngig von der Gesamtdistanz:
            //cv::Point start = sideCentroids[puzzle][side];
            cv::Point start = sides[piece][piece_side][(unsigned int)(sides[piece][piece_side].size()/2)];
            cv::Point end = sides[maxElement[0]][maxElement[1]][(unsigned int)(sides[maxElement[0]][maxElement[1]].size()/2)];
            double curveFactor = norm(end - start)/2;

            // Vektor beim Basispuzzleteil:
            cv::Point startVector = start - sideCentroids[piece][(piece_side + 2)%4];
            startVector *= curveFactor/cv::norm(startVector);

            // Vektor beim Zeilpuzzleteil:
            cv::Point endVector = end - sideCentroids[maxElement[0]][(maxElement[1] + 2)%4];
            endVector *= curveFactor/cv::norm(endVector);

            cv::Path2D bezierLine;
            bezierLine.restart(start.x, start.y);
            bezierLine.curveTo(start + startVector, end + endVector, end);
            cv::drawPath2D(imgContSimilar, bezierLine, CV_RGB(20,20,255), -1, 10, CV_AA);

            // Pfeil zeichnen
            cv::Point orthogonal = cv::Point(-endVector.y, endVector.x)*30*(1/cv::norm(endVector));
            cv::line(imgContSimilar, end, end + endVector*70*(1/cv::norm(endVector)) + orthogonal, CV_RGB(20,20,255), 10, CV_AA);
            cv::line(imgContSimilar, end, end + endVector*70*(1/cv::norm(endVector)) - orthogonal, CV_RGB(20,20,255), 10, CV_AA);


            cv::imshow("Aehnlichkeit", imgContSimilar);
        }
        // Konsolenfenster schliessen
        cv::destroyAllWindows();
        // Gui-Anwendung schliessen
        close();
        cv::waitKey(1);
        return;
}

void MainWindow::closeProg()
{
    if(close_flag == true)
    {
        close();
    }
    // Flag setzen um Programm zu beenden
    else
    {
        close_flag = true;
    }
}

bool MainWindow::loadImage()
{

    // Close Flag l�schen
    close_flag = false;
    // get name of image to open passed by QFileDialog
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "data/images/", tr("Image Files (*.png *.jpg *.bmp)"));

    // only continue if a new file was selected
    if(!fileName.isEmpty())
    {

        // load OpenCV image
        img = cv::imread(fileName.toStdString());
        if(img.empty())
        {
            qDebug() << "Can not load image " << fileName;
            return false;
        }

        // do image processing
        imageProcessing();
    }

    return true;
}


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
  {
    setWindowTitle(tr("Puzzle"));

    quitButton = new QPushButton("Quit");
    loadButton = new QPushButton("Load");

    connect(quitButton, SIGNAL(clicked()), this, SLOT(closeProg()));
    connect(loadButton, SIGNAL(clicked()), this, SLOT(loadImage()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(loadButton);
    layout->addWidget(quitButton);

    QWidget *myCentralWidget = new QWidget;
    myCentralWidget->setLayout(layout);

    setCentralWidget(myCentralWidget);
  }

MainWindow::~MainWindow()
{
    delete loadButton;
    delete quitButton;
}

