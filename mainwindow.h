#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QTimer>
#include "opencv2/opencv.hpp"
#include "opencv/highgui.h"
#include "opencv/cv.h"
#include <QPainter>
#include <QCloseEvent>
#include <QString>
//###
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <iostream>

#include "tcp_reciever.h"
#include "utils.h"
#include "pxcsensemanager.h"
#include "time.h"
#include "camera_capture.h"
#include "realsense_capture.h"
#include "myothread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void startCamera(int record_type=0);
    void startRealsense(int record_type=0);
    void startWear();
    void startMyo();

    void initAll();
    void startAll();
    void stopAll(bool useButton);

    void recordVol();
    void recordUnit();

    void stopCamera();
    void stopWearandRecv();
    void stopRealsense();
    void stopMyo();

    bool connect2Wear();
    void disconnect2Wear();

    //###
    void qstring2char(QString src, char* dst);
    void cameraCalibrationUnit();
    void recordStaticGestUnit();
    bool initAllCC();
    void startAllCC();
    void stopAllCC();
    void initAllSG();
    void startAllSG();
    void stopAllSG();

signals:
    void allFrameFinish();

private:
    bool connected = false;
    bool recieveServOn = false;

    bool useCamera = true;
    bool useWear = false;
    bool useMyo = false;
    bool useRealsense = true;


    Ui::MainWindow *ui;
    QImage *image;
    QImage *label2Image;
    QTimer timer;

    SOCKET serv_sock=NULL;
    PXCSenseManager *pxcSenseManager;
    struct sockaddr_in serv_addr;

    myo::Hub *hub;
    myo::Myo* a_myo;
    DataCollector collector;

    camera_capture *camera1Thread, *camera2Thread;
    realsense_capture *realsenseThread;
    myothread *myoThread;
    tcp_reciever recieverThread;

    //###
    QDir dirManager;
    QString volName, dataRootDir, dataVolDir, dataRestDir, dataGestDir;
    QString cameraLeftDir, cameraRightDir, realsenseRGBDir, realsenseDepthDir, realsenseCoordDir;
    QString cameraLeftPrefix, cameraRightPrefix, realsenseRGBPrefix, realsenseDepthPrefix, realsenseCoordPrefix;
    QString ccDir, ccCameraLeftDir, ccCameraRightDir, ccRealsenseRGBDir;
    QString ccCameraLeftPrefix, ccCameraRightPrefix, ccRealsenseRGBPrefix;
    QString sgDir, sGestDir, sgCameraLeftDir, sgCameraRightDir, sgRealsenseRGBDir, sgRealsenseDepthDir, sgRealsenseCoordDir;
    QString sgCameraLeftPrefix, sgCameraRightPrefix, sgRealsenseRGBPrefix, sgRealsenseDepthPrefix, sgRealsenseCoordPrefix;
    QTimer *checkTimer;
    int volNb, restNb, gestNb, sGestNb;
    int volIdx, restIdx, gestIdx, sGestIdx;
    int gestDura, ccDura, sgDura;
    bool frameFinishCamera1 = false;
    bool frameFinishCamera2 = false;
    bool frameFinishRealsense = false;
    bool buttonStop = false;


private slots:
    // used for updating videos
    void updateCamera1(const QImage &image);
    void updateCamera2(const QImage &image);
    void updateRealsense(const QImage &image);
//    void updateFPSCamera1(const double fps);
//    void updateFPSCamera2(const double fps);
    void updateFPSCamera1(const int fnb, const int record_type);
    void updateFPSCamera2(const int fnb, const int record_type);
    void updateFPSRealsense(const int fnb, const int record_type);

    // triggered by button
    void startRecordProgress();
    void stopRecordProgress();

    //###
    void checkFrameFinishCamera1();
    void checkFrameFinishCamera2();
    void checkFrameFinishRealsense();
    void timeHandler();

    void on_cameraCalibrationButton_clicked();

    void on_startRecordStaticGestButt_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
