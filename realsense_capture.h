#ifndef REALSENSE_CAPTURE_H
#define REALSENSE_CAPTURE_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QWaitCondition>
#include <QMutex>
#include "pxcsensemanager.h"
#include "pxchanddata.h"
#include "pxchandconfiguration.h"
#include "pxchandmodule.h"
#include <sys/types.h>
#include "opencv2/opencv.hpp"
#include "opencv/highgui.h"
#include "opencv/cv.h"
#include "time.h"
#include "stdio.h"

//###
#include <QDebug>
#include <QTime>
#include <QFile>

#define JOINT_TYPE_NUM 22

const PXCHandData::JointType JOINTS[JOINT_TYPE_NUM] = {\
    PXCHandData::JointType::JOINT_WRIST, \
    PXCHandData::JointType::JOINT_CENTER, \
    PXCHandData::JointType::JOINT_THUMB_BASE, \
    PXCHandData::JointType::JOINT_THUMB_JT1, \
    PXCHandData::JointType::JOINT_THUMB_JT2, \
    PXCHandData::JointType::JOINT_THUMB_TIP, \
    PXCHandData::JointType::JOINT_INDEX_BASE, \
    PXCHandData::JointType::JOINT_INDEX_JT1, \
    PXCHandData::JointType::JOINT_INDEX_JT2, \
    PXCHandData::JointType::JOINT_INDEX_TIP, \
    PXCHandData::JointType::JOINT_MIDDLE_BASE, \
    PXCHandData::JointType::JOINT_MIDDLE_JT1, \
    PXCHandData::JointType::JOINT_MIDDLE_JT2, \
    PXCHandData::JointType::JOINT_MIDDLE_TIP, \
    PXCHandData::JointType::JOINT_RING_BASE, \
    PXCHandData::JointType::JOINT_RING_JT1, \
    PXCHandData::JointType::JOINT_RING_JT2, \
    PXCHandData::JointType::JOINT_RING_TIP, \
    PXCHandData::JointType::JOINT_PINKY_BASE, \
    PXCHandData::JointType::JOINT_PINKY_JT1, \
    PXCHandData::JointType::JOINT_PINKY_JT2, \
    PXCHandData::JointType::JOINT_PINKY_TIP
};

class realsense_capture : public QThread
{
    Q_OBJECT
public:
    realsense_capture();
    void stop();
    void stopRecord();
    void init(const char *filename);
    void init(const QString, const QString, const QString);
    void initCC(const QString);
    void initSG(const QString, const QString, const QString);
    void startRecord(int record_type=0);

    void clearImgIdx();
signals:
    void imageReady(const QImage &image);
    void fpsReady(const double fps);
    void frameNb(const int fnb, const int record_type);
    void finishOneFrame();
protected:
    void run() override;
private:
    bool end, toRecord;
    cv::VideoWriter writerColorRe, writerDepthRe, writerColorJoints, writerDepthJoints;
    cv::Mat frameColor;
    cv::Mat frameDepth;
    PXCSenseManager *pxcSenseManager;
    PXCHandModule *pxcHandModule;
    PXCHandData *pxcHandData;
    FILE *file;
    time_t cTime, pTime;
    uint64_t cCnt, pCnt;
    const cv::Size size = cv::Size(640, 480);
    const float frameRate = 30;

    QMutex mutex;
    QWaitCondition condition;

    //###
    std::string rgbPrefix, depthPrefix, coordPrefix;
    std::string rgbImgDir, depthImgDir, coordFileDir;
    std::string ccPrefix, ccrgbImgDir;
    std::string sgRGBPrefix, sgDepthPrefix, sgCoordPrefix;
    std::string sgRGBImgDir, sgDepthImgDir, sgCoordFileDir;
    QString txtLine;
    bool recordImgFlag;
    bool firstLoop = true;
    int imgIdx;
    int recordType;

private slots:
    void processNextFrame();
};

#endif // REALSENSE_CAPTURE_H
