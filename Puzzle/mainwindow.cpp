#include "mainwindow.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include "cvpath2d.h"

// Globale Variablen
cv::Mat img;
bool close_flag = true;

cv::RNG rng(12345);
int thresh = 14;
int max_thresh = 40;
cv::Mat imgCont;
std::vector<std::vector<cv::Point> > contours;

void approx(int, void* );

void MainWindow::imageProcessing()
{
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
    cv::Mat imgThreshTemp = imgBin;
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
    cv::namedWindow("Kontur", 0);
    cv::imshow("Kontur", imgCont);

    // slider bereitstellen
    cv::createTrackbar("epsilon", "Kontur", &thresh, max_thresh, approx);
    approx(0, 0);

    // Warten auf einen Tastendruck
    cv::waitKey(0);
    close();
    return;
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

    // Close Flag löschen
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

