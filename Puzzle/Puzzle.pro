#-------------------------------------------------
#
# Project created by QtCreator 2012-12-22T16:26:39
#
#-------------------------------------------------

QT       += core  gui

TARGET = test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
           mainwindow.cpp \
           cvpath2d.cpp

DEFINES += PATH2D_HAVE_OPENCV=1

INCLUDEPATH += C:/OpenCV/OpenCV-2.3.0/include \
  C:/OpenCV/OpenCV-2.3.0/modules/calib3d/include \
  C:/OpenCV/OpenCV-2.3.0/modules/contrib/include \
  C:/OpenCV/OpenCV-2.3.0/modules/core/include \
  C:/OpenCV/OpenCV-2.3.0/modules/features2d/include \
  C:/OpenCV/OpenCV-2.3.0/modules/flann/include \
  C:/OpenCV/OpenCV-2.3.0/modules/gpu/include \
  C:/OpenCV/OpenCV-2.3.0/modules/haartraining \
  C:/OpenCV/OpenCV-2.3.0/modules/highgui/include \
  C:/OpenCV/OpenCV-2.3.0/modules/imgproc/include \
  C:/OpenCV/OpenCV-2.3.0/modules/legacy/include \
  C:/OpenCV/OpenCV-2.3.0/modules/ml/include \
  C:/OpenCV/OpenCV-2.3.0/modules/objdetect/include \
  C:/OpenCV/OpenCV-2.3.0/modules/ts/include \
  C:/OpenCV/OpenCV-2.3.0/modules/video/include
LIBS += -LC:/OpenCV/OpenCV-2.3.0-bin/lib \
  -lopencv_calib3d230 \
  -lopencv_contrib230 \
  -lopencv_core230 \
  -lopencv_features2d230 \
  -lopencv_flann230 \
  -lopencv_gpu230 \
  -lopencv_haartraining_engine \
  -lopencv_highgui230 \
  -lopencv_imgproc230 \
  -lopencv_legacy230 \
  -lopencv_ml230 \
  -lopencv_objdetect230 \
  -lopencv_ts230 \
  -lopencv_video230

HEADERS += cvpath2d.h  \
           mainwindow.h
