#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    bool loadImage();
    void closeProg();
    void imageProcessing();

private:
    QPushButton *loadButton;
    QPushButton *quitButton;
};

#endif // MAINWINDOW_H
