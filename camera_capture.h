#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H
#include <QImage>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <QMutex>
#include <time.h>
#include <sys/types.h>
#include "opencv2/opencv.hpp"
#include "opencv/highgui.h"
#include "opencv/cv.h"

//###
#include <QDebug>
#include <iostream>
#include <QTime>

class camera_capture : public QThread
{
        Q_OBJECT
public:
    camera_capture(int cameraID);
    void init(char *filename, bool recordImg);
    void initCC(char *filename);
    void initSG(char *filename);
    void startRecord(int record_type=0);
    void stopRecord();
    void stop();
    void clearImgIdx();

signals:
    void imageReady(const QImage &image);
    void fpsReady(const double fps);
    void frameNb(const int fnb, const int record_type);
    void finishOneFrame();
protected:
    void run() override;
private:
    const float frameRate = 30.0;
    cv::VideoCapture cap;
    cv::VideoWriter wrt;
    bool end, toRecord;
    cv::Mat img;
//    time_t cTime, pTime;
    double cTime, pTime;
    uint64_t cCnt, pCnt;
    //###
    bool recordImgFlag;
    bool firstLoop = true;
    int imgIdx;
    int recordType;
    std::string cameraImgPrefix, cameraImgDir;
    std::string ccPrefix, ccImgDir, sgPrefix, sgImgDir;

    //used for synchronizing thread
    QMutex mutex;
    QWaitCondition condition;

private slots:
    void processNextFrame();
};

#endif // CAMERA_CAPTURE_H
