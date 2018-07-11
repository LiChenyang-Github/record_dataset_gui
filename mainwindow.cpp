/*
 * Author: Phree Liu
 * Created Date: Mar.24.2018
 * Version: 0.1.0
 * used for managing camera, smart watch, myo and realsense
 */
#pragma comment(lib, "Ws2_32.lib") //add the dependency file for winsock2

#include "mainwindow.h"
#include "ui_mainwindow.h"

/*
 * used for acquiring a file name to save video stream
 */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->hintLabel->setText("Click start to start record progress.");
    image = new QImage();

    //setup winsock2
    WSADATA wsa_data = { 0 };
    WORD wsa_version = MAKEWORD(2, 2);
    int n_ret = WSAStartup(wsa_version, &wsa_data);
    if (n_ret != NO_ERROR) {
        printf("main: startup wsa failed!\n");
    }

//    int cameraID1 = 2;  // 4 is ID of the self-contained webcam
//    int cameraID2 = 5;
    int cameraID1 = 3;  // 4 is ID of the self-contained webcam
    int cameraID2 = 4;
    if(useCamera){
        camera1Thread = new camera_capture(cameraID1);
        camera2Thread = new camera_capture(cameraID2);
    }
    if(useMyo){
        myoThread = new myothread();
    }
    if(useRealsense){
        realsenseThread = new realsense_capture();
    }
    connect(ui->startRecordProgressButton, SIGNAL(clicked(bool)), \
            this, SLOT(startRecordProgress()));
    connect(ui->stopRecordProgressButton, SIGNAL(clicked(bool)), \
            this, SLOT(stopRecordProgress()));
//    if(useWear){
//        recieverThread.start();
//    }

    //###
    volNb = 10;
    restNb = 2;
    gestNb = 2;
    sGestNb = 2;    // static gesture number.
    sGestIdx = 0;
    gestDura = 3000; // 10000 ms per gesture
    ccDura = 3000;
    sgDura = 3000;
    dataRootDir = "./data";
    if(!dirManager.exists(dataRootDir)){
        dirManager.mkdir(dataRootDir);
    }
    checkTimer = new QTimer(this);
    connect(checkTimer, SIGNAL(timeout()), this, SLOT(timeHandler()));
}

MainWindow::~MainWindow(){
    delete ui;
}

//###
void MainWindow::qstring2char(QString src, char* dst){
    QByteArray tmp = src.toLocal8Bit();
    strcpy(dst, tmp.data());
}

void MainWindow::initAll(){
    char filename[512];

    if(useCamera) {
//        cameraLeftDir = dataGestDir + "/" + QString("camera_left_vol_%1_restState_%2_geture_%3.avi").arg(volIdx).arg(restIdx).arg(gestIdx);
//        qstring2char(cameraLeftDir, filename);
//        camera1Thread->init(filename, false);
//        cameraRightDir = dataGestDir + "/" + QString("camera_right_vol_%1_restState_%2_geture_%3.avi").arg(volIdx).arg(restIdx).arg(gestIdx);
//        qstring2char(cameraRightDir, filename);
//        camera2Thread->init(filename, false);

        cameraLeftDir = dataGestDir + "/camera_left";
        cameraRightDir = dataGestDir + "/camera_right";
        if(!dirManager.exists(cameraLeftDir)){
            dirManager.mkdir(cameraLeftDir);
        }
        if(!dirManager.exists(cameraRightDir)){
            dirManager.mkdir(cameraRightDir);
        }
        cameraLeftPrefix = cameraLeftDir + "/" + QString("camera_left_vol_%1_restState_%2_geture_%3_").arg(volIdx).arg(restIdx).arg(gestIdx);
        qstring2char(cameraLeftPrefix, filename);
        camera1Thread->init(filename, true);
        cameraRightPrefix = cameraRightDir + "/" + QString("camera_right_vol_%1_restState_%2_geture_%3_").arg(volIdx).arg(restIdx).arg(gestIdx);
        qstring2char(cameraRightPrefix, filename);
        camera2Thread->init(filename, true);
    }
    if(useMyo) {
        sprintf(filename, "myo_vol_%d_restState_%d_geture_%d.csv", \
            volIdx, restIdx, gestIdx);
        myoThread->init(filename);
    }
    if(useRealsense) {
//        realsenseDir = dataGestDir + "/" + QString("realsense_vol_%1_restState_%2_geture_%3").arg(volIdx).arg(restIdx).arg(gestIdx);
//        qstring2char(realsenseDir, filename);
//        realsenseThread->init(filename);

        realsenseRGBDir = dataGestDir + "/realsense_rgb";
        realsenseDepthDir = dataGestDir + "/realsense_depth";
        realsenseCoordDir = dataGestDir + "/realsense_coord";
        if(!dirManager.exists(realsenseRGBDir)){
            dirManager.mkdir(realsenseRGBDir);
        }
        if(!dirManager.exists(realsenseDepthDir)){
            dirManager.mkdir(realsenseDepthDir);
        }
        if(!dirManager.exists(realsenseCoordDir)){
            dirManager.mkdir(realsenseCoordDir);
        }
        realsenseRGBPrefix = realsenseRGBDir + "/" + QString("realsense_rgb_vol_%1_restState_%2_geture_%3_").arg(volIdx).arg(restIdx).arg(gestIdx);
        realsenseDepthPrefix = realsenseDepthDir + "/" + QString("realsense_depth_vol_%1_restState_%2_geture_%3_").arg(volIdx).arg(restIdx).arg(gestIdx);
        realsenseCoordPrefix = realsenseCoordDir + "/" + QString("realsense_coord_vol_%1_restState_%2_geture_%3_").arg(volIdx).arg(restIdx).arg(gestIdx);
        realsenseThread->init(realsenseRGBPrefix, realsenseDepthPrefix, realsenseCoordPrefix);
    }
}

bool MainWindow::initAllCC(){
    char filename[512];
    volName = ui->volNameLineE->text();
    volIdx = ui->volIDLineE->text().toInt();
    if (volName.isEmpty()){
        QMessageBox::information(NULL, "Attention", "Please enter your name.");
        return false;
    }
    if ((volIdx<0) || (volIdx>=volNb)){
        QMessageBox::information(NULL, "Attention", "Please enter correct volIdx.");
        return false;
    }
    ui->volIDLabel->setText(QString::number(volIdx)+"_"+volName);
    dataVolDir = dataRootDir + "/" + QString::number(volIdx) + "_" + volName;
    if(!dirManager.exists(dataVolDir)){
        dirManager.mkdir(dataVolDir);
    }
    ccDir = dataVolDir + "/camera_calibration";
    if(!dirManager.exists(ccDir)){
        dirManager.mkdir(ccDir);
    }

    if(useCamera) {

        ccCameraLeftDir = ccDir + "/camera_left";
        ccCameraRightDir = ccDir + "/camera_right";
        if(!dirManager.exists(ccCameraLeftDir)){
            dirManager.mkdir(ccCameraLeftDir);
        }
        if(!dirManager.exists(ccCameraRightDir)){
            dirManager.mkdir(ccCameraRightDir);
        }
        ccCameraLeftPrefix = ccCameraLeftDir + "/" + QString("calibration_camera_left_vol_%1_").arg(volIdx);
        qstring2char(ccCameraLeftPrefix, filename);
        camera1Thread->initCC(filename);
        ccCameraRightPrefix = ccCameraRightDir + "/" + QString("calibration_camera_right_vol_%1_").arg(volIdx);
        qstring2char(ccCameraRightPrefix, filename);
        camera2Thread->initCC(filename);
    }
    if(useRealsense) {

        ccRealsenseRGBDir = ccDir + "/realsense_rgb";

        if(!dirManager.exists(ccRealsenseRGBDir)){
            dirManager.mkdir(ccRealsenseRGBDir);
        }

        ccRealsenseRGBPrefix = ccRealsenseRGBDir + "/" + QString("calibration_realsense_vol_%1_").arg(volIdx);
        realsenseThread->initCC(ccRealsenseRGBPrefix);
    }
    return true;
}

void MainWindow::initAllSG(){
    char filename[512];
    volName = ui->volNameLineE->text();
    volIdx = ui->volIDLineE->text().toInt();
    if (volName.isEmpty()){
        QMessageBox::information(NULL, "Attention", "Please enter your name.");
        return;
    }
    if ((volIdx<0) || (volIdx>=volNb)){
        QMessageBox::information(NULL, "Attention", "Please enter correct volIdx.");
        return;
    }
    ui->volIDLabel->setText(QString::number(volIdx)+"_"+volName);
    dataVolDir = dataRootDir + "/" + QString::number(volIdx) + "_" + volName;
    ui->gestureIDLabel->setText(QString::number(sGestIdx));
    if(!dirManager.exists(dataVolDir)){
        dirManager.mkdir(dataVolDir);
    }
    sgDir = dataVolDir + "/static_gesture";
    if(!dirManager.exists(sgDir)){
        dirManager.mkdir(sgDir);
    }

    if(useCamera) {

        sGestDir = sgDir + "/gest_" + QString::number(sGestIdx);
        if(!dirManager.exists(sGestDir)){
            dirManager.mkdir(sGestDir);
        }
        sgCameraLeftDir = sGestDir + "/camera_left";
        sgCameraRightDir = sGestDir + "/camera_right";
        if(!dirManager.exists(sgCameraLeftDir)){
            dirManager.mkdir(sgCameraLeftDir);
        }
        if(!dirManager.exists(sgCameraRightDir)){
            dirManager.mkdir(sgCameraRightDir);
        }

        sgCameraLeftPrefix = sgCameraLeftDir + "/" + QString("camera_left_vol_%1_gest_%2_").arg(volIdx).arg(sGestIdx);
        qstring2char(sgCameraLeftPrefix, filename);
        camera1Thread->initSG(filename);
        sgCameraRightPrefix = sgCameraRightDir + "/" + QString("camera_right_vol_%1_gest_%2_").arg(volIdx).arg(sGestIdx);
        qstring2char(sgCameraRightPrefix, filename);
        camera2Thread->initSG(filename);
    }
    if(useRealsense) {

        sgRealsenseRGBDir = sGestDir + "/realsense_rgb";
        sgRealsenseDepthDir = sGestDir + "/realsense_depth";
        sgRealsenseCoordDir = sGestDir + "/realsense_coord";

        if(!dirManager.exists(sgRealsenseRGBDir)){
            dirManager.mkdir(sgRealsenseRGBDir);
        }
        if(!dirManager.exists(sgRealsenseDepthDir)){
            dirManager.mkdir(sgRealsenseDepthDir);
        }
        if(!dirManager.exists(sgRealsenseCoordDir)){
            dirManager.mkdir(sgRealsenseCoordDir);
        }
        sgRealsenseRGBPrefix = sgRealsenseRGBDir + "/" + QString("realsense_rgb_vol_%1_gest_%2_").arg(volIdx).arg(sGestIdx);
        sgRealsenseDepthPrefix = sgRealsenseDepthDir + "/" + QString("realsense_depth_vol_%1_gest_%2_").arg(volIdx).arg(sGestIdx);
        sgRealsenseCoordPrefix = sgRealsenseCoordDir + "/" + QString("realsense_coord_vol_%1_gest_%2_").arg(volIdx).arg(sGestIdx);
        realsenseThread->initSG(sgRealsenseRGBPrefix, sgRealsenseDepthPrefix, sgRealsenseCoordPrefix);
    }
}


void MainWindow::startAll(){
    if(useCamera) {
        startCamera(0);
    }
    if(useMyo) {
        startMyo();
    }
    if(useRealsense) {
        startRealsense(0);
    }
    if(useWear) {
        startWear();
    }
    checkTimer->start(30);
}

void MainWindow::startAllCC(){
    if(useCamera) {
        startCamera(1);
    }
    if(useRealsense) {
        startRealsense(1);
    }
    checkTimer->start(30);
}

void MainWindow::startAllSG(){
    if(useCamera) {
        startCamera(2);
    }
    if(useRealsense) {
        startRealsense(2);
    }
    checkTimer->start(30);
}

void MainWindow::stopAll(bool useButton){
    if (useButton){
        if(useCamera){
            camera1Thread->clearImgIdx();
            camera2Thread->clearImgIdx();
            stopCamera();
        }
        if(useRealsense){
            realsenseThread->clearImgIdx();
            stopRealsense();
        }
        if(useWear) stopWearandRecv();
        checkTimer->stop();
        frameFinishCamera1 = false;
        frameFinishCamera2 = false;
        frameFinishRealsense = false;
    }
    else{
        if(useCamera) stopCamera();
        if(useRealsense) stopRealsense();
        if(useMyo) stopMyo();
        if(useWear) stopWearandRecv();
        checkTimer->stop();
//        qDebug() << frameFinishCamera1 << frameFinishCamera2 << frameFinishRealsense;
        frameFinishCamera1 = false;
        frameFinishCamera2 = false;
        frameFinishRealsense = false;
    }
}

void MainWindow::stopAllCC(){
//    if(useCamera) stopCamera();
    if(useCamera){
        camera1Thread->stop();
        camera2Thread->stop();
    }
//    if(useRealsense) stopRealsense();
    if(useRealsense){
        realsenseThread->stop();
    }

    checkTimer->stop();
//        qDebug() << frameFinishCamera1 << frameFinishCamera2 << frameFinishRealsense;
    frameFinishCamera1 = false;
    frameFinishCamera2 = false;
    frameFinishRealsense = false;
}

void MainWindow::stopAllSG(){
    if(useCamera){
        camera1Thread->stop();
        camera2Thread->stop();
    }
    if(useRealsense){
        realsenseThread->stop();
    }

    checkTimer->stop();
    frameFinishCamera1 = false;
    frameFinishCamera2 = false;
    frameFinishRealsense = false;
}

void MainWindow::startRecordProgress(){
    buttonStop = false;
    volName = ui->volNameLineE->text();
    volIdx = ui->volIDLineE->text().toInt();
    restIdx = ui->restIDLineE->text().toInt();
    gestIdx = ui->gestureIDLineE->text().toInt();
    if (volName.isEmpty()){
//        throw "Please enter your name.";
        QMessageBox::information(NULL, "Attention", "Please enter your name.");
        return;
    }
    if ((volIdx<0) || (volIdx>=volNb)){
        QMessageBox::information(NULL, "Attention", "Please enter correct volIdx.");
        return;
    }
    if ((restIdx<0) || (restIdx>=restNb)){
        QMessageBox::information(NULL, "Attention", "Please enter correct restIdx.");
        return;
    }
    if ((gestIdx<0) || (gestIdx>=gestNb)){
        QMessageBox::information(NULL, "Attention", "Please enter correct gestIdx.");
        return;
    }

    ui->startRecordProgressButton->setDisabled(true);
    ui->stopRecordProgressButton->setEnabled(true);

    ui->volIDLabel->setText(QString::number(volIdx));
    dataVolDir = dataRootDir + "/" + QString::number(volIdx) + "_" + volName;
    if(!dirManager.exists(dataVolDir)){
        dirManager.mkdir(dataVolDir);
    }
    recordVol();
    if (!buttonStop){
        ui->hintLabel->setText(QString("Finish dynamic gesture! Thank you!"));
        ui->startRecordProgressButton->setEnabled(true);
        ui->stopRecordProgressButton->setDisabled(true);
    }
}

void MainWindow::stopRecordProgress(){
    stopAll(true);
    buttonStop = true;
    if(dirManager.exists(dataGestDir)){
        dirManager.rmdir(dataGestDir);
    }
    ui->hintLabel->setText("Click start to restart record progress.");
    ui->restIDLineE->setText(QString::number(restIdx));
    ui->gestureIDLineE->setText(QString::number(gestIdx));

    ui->stopRecordProgressButton->setDisabled(true);
}

void MainWindow::recordVol(){
    QTime time;
    time.start();
    for(; restIdx<restNb; restIdx++){
        if (buttonStop){
            return;
        }
        dataRestDir = dataVolDir + "/rest_" + QString::number(restIdx);
        if(!dirManager.exists(dataRestDir)){
            dirManager.mkdir(dataRestDir);
        }
        ui->restStateIDLabel->setText(QString::number(restIdx));
        ui->hintLabel->setText(QString("Rest State ")+QString::number(restIdx));
        time.restart();
        while(time.elapsed()<1000){
            QCoreApplication::processEvents();
        }
        for(; gestIdx<gestNb; gestIdx++){
            if (buttonStop){
                return;
            }
            dataGestDir = dataRestDir + "/gest_" + QString::number(gestIdx);
//            qDebug() << dataGestDir;
            if(!dirManager.exists(dataGestDir)){
                dirManager.mkdir(dataGestDir);
            }
            ui->gestureIDLabel->setText(QString::number(gestIdx));
            ui->hintLabel->setText("Ready");
            time.restart();
            // wait for 1s to record and stay responsible for evnets
            while(time.elapsed()<1000){
                QCoreApplication::processEvents();
            }
            ui->hintLabel->setText("Recording...");
//            qDebug() << "gestID" << gestIdx;
            recordUnit();
        }
        gestIdx = 0;
    }
    if(useCamera){  // stop the thread and rerun the thread when click the startRecordStaticGestButt button
        camera1Thread->stop();
        camera2Thread->stop();
    }
    if(useRealsense){
        realsenseThread->stop();
    }
}

// 一次最小记录单位
void MainWindow::recordUnit(){
    QTime time;
    initAll();
    startAll();
    time.restart();
    while(time.elapsed()<gestDura){
        QCoreApplication::processEvents();
    }
    if (!buttonStop){
        stopAll(false);
    }
    else{
        ui->startRecordProgressButton->setEnabled(true);
        qDebug() << "continue";
    }

//    qDebug() << "hahahahhahahahhahahahhahahahhahahhahahahhah";
//    qDebug() << frameFinishCamera1 << frameFinishCamera2 << frameFinishRealsense;
}

void MainWindow::cameraCalibrationUnit(){   // camera calibration (cc)
    QTime time;
    if (initAllCC()){
        startAllCC();
        time.restart();
        while(time.elapsed()<ccDura){
            QCoreApplication::processEvents();
        }
        stopAllCC();
    }

}

void MainWindow::recordStaticGestUnit(){
    QTime time;
    initAllSG();
    startAllSG();
    time.restart();
    while(time.elapsed()<sgDura){
        QCoreApplication::processEvents();
    }
    stopAllSG();
}

void MainWindow::startMyo(){
    myoThread->startRecord();
}

void MainWindow::stopMyo(){
    myoThread->stopRecord();
}

void MainWindow::startCamera(int record_type){    // 0 means dynamic gesture, 1 means cc, 2 means static gesture.

    camera1Thread->startRecord(record_type);
    camera2Thread->startRecord(record_type);


    connect(camera1Thread, SIGNAL(imageReady(QImage)), this, SLOT(updateCamera1(QImage)));
    connect(camera1Thread, SIGNAL(frameNb(int, int)), this, SLOT(updateFPSCamera1(int, int)));

    connect(camera2Thread, SIGNAL(imageReady(QImage)), this, SLOT(updateCamera2(QImage)));
    connect(camera2Thread, SIGNAL(frameNb(int, int)), this, SLOT(updateFPSCamera2(int, int)));

    connect(camera1Thread, SIGNAL(finishOneFrame()), this, SLOT(checkFrameFinishCamera1()));
    connect(camera2Thread, SIGNAL(finishOneFrame()), this, SLOT(checkFrameFinishCamera2()));
    connect(this, SIGNAL(allFrameFinish()), camera1Thread, SLOT(processNextFrame()));
    connect(this, SIGNAL(allFrameFinish()), camera2Thread, SLOT(processNextFrame()));
}

void MainWindow::stopCamera(){
    camera1Thread->stopRecord();
//    qDebug() << "stopRecord()";
    camera2Thread->stopRecord();

    disconnect(camera1Thread, SIGNAL(imageReady(QImage)), this, SLOT(updateCamera1(QImage)));
    disconnect(camera1Thread, SIGNAL(frameNb(int, int)), this, SLOT(updateFPSCamera1(int, int)));
    disconnect(camera2Thread, SIGNAL(imageReady(QImage)), this, SLOT(updateCamera2(QImage)));
    disconnect(camera2Thread, SIGNAL(frameNb(int, int)), this, SLOT(updateFPSCamera2(int, int)));

    disconnect(camera1Thread, SIGNAL(finishOneFrame()), this, SLOT(checkFrameFinishCamera1()));
    disconnect(camera2Thread, SIGNAL(finishOneFrame()), this, SLOT(checkFrameFinishCamera2()));
    disconnect(this, SIGNAL(allFrameFinish()), camera1Thread, SLOT(processNextFrame()));
    disconnect(this, SIGNAL(allFrameFinish()), camera2Thread, SLOT(processNextFrame()));

}

/*
 * start a new thread to record realsense stream
 */
void MainWindow::startRealsense(int record_type){
    realsenseThread->startRecord(record_type);
    connect(realsenseThread, SIGNAL(imageReady(QImage)), this, SLOT(updateRealsense(QImage)));
    connect(realsenseThread, SIGNAL(frameNb(int, int)), this, SLOT(updateFPSRealsense(int, int)));

    connect(realsenseThread, SIGNAL(finishOneFrame()), this, SLOT(checkFrameFinishRealsense()));
    connect(this, SIGNAL(allFrameFinish()), realsenseThread, SLOT(processNextFrame()));
}

void MainWindow::stopRealsense(){
    realsenseThread->stopRecord();

    disconnect(realsenseThread, SIGNAL(imageReady(QImage)), this, SLOT(updateRealsense(QImage)));
    disconnect(realsenseThread, SIGNAL(frameNb(int, int)), this, SLOT(updateFPSRealsense(int, int)));


    disconnect(realsenseThread, SIGNAL(finishOneFrame()), this, SLOT(checkFrameFinishRealsense()));
    disconnect(this, SIGNAL(allFrameFinish()), realsenseThread, SLOT(processNextFrame()));
}


/*
 * Display captured image on UI
 */
void MainWindow::updateCamera1(const QImage &image){
    ui->camera1Preview->setPixmap(QPixmap::fromImage(image.scaled(ui->camera1Preview->size())));
    ui->camera1Preview->show();
}
void MainWindow::updateCamera2(const QImage &image){
    ui->camera2Preview->setPixmap(QPixmap::fromImage(image.scaled(ui->camera2Preview->size())));
    ui->camera2Preview->show();
}
void MainWindow::updateRealsense(const QImage &image){
    ui->realsensePreview->setPixmap(QPixmap::fromImage(image.scaled(ui->camera1Preview->size())));
    ui->realsensePreview->show();
}

//void MainWindow::updateFPSCamera1(const double fps){
//    ui->camera1FPSValue->setText(QString::number(fps));
//}
//void MainWindow::updateFPSCamera2(const double fps){
//    ui->camera2FPSValue->setText(QString::number(fps));
//}
void MainWindow::updateFPSCamera1(const int fnb, const int record_type){
    if (record_type == 0){
        int fps = fnb * 1000 / gestDura;
//        qDebug() << fps;
        ui->camera1FPSValue->setText(QString::number(fps));
    }
    else if (record_type == 1){
        int fps = fnb * 1000 / ccDura;
        qDebug() << fps;
        ui->camera1FPSValue->setText(QString::number(fps));
    }

}
void MainWindow::updateFPSCamera2(const int fnb, const int record_type){
    if (record_type == 0){
        int fps = fnb * 1000 / gestDura;
        ui->camera2FPSValue->setText(QString::number(fps));
    }
    else if (record_type == 1){
        int fps = fnb * 1000 / ccDura;
        ui->camera2FPSValue->setText(QString::number(fps));
    }

}
void MainWindow::updateFPSRealsense(const int fnb, const int record_type){
    if (record_type == 0){
        int fps = fnb * 1000 / gestDura;
        ui->realsenseFPSValue->setText(QString::number(fps));
    }
    else if (record_type == 1){
        int fps = fnb * 1000 / ccDura;
        ui->realsenseFPSValue->setText(QString::number(fps));
    }

}

void MainWindow::checkFrameFinishCamera1(){
    frameFinishCamera1 = true;
}
void MainWindow::checkFrameFinishCamera2(){
    frameFinishCamera2 = true;
}
void MainWindow::checkFrameFinishRealsense(){
    frameFinishRealsense = true;
}

void MainWindow::timeHandler(){
    if ((frameFinishCamera1)&&(frameFinishCamera2)&&(frameFinishRealsense)){
        frameFinishCamera1 = false;
        frameFinishCamera2 = false;
        frameFinishRealsense = false;
        emit allFrameFinish();
    }
}

void MainWindow::closeEvent(QCloseEvent *event){
    if(useCamera){
        camera1Thread->stop();
        camera2Thread->stop();
    }
    if(useRealsense){
        realsenseThread->stop();
    }
    if(useMyo){
        myoThread->stop();
    }
//    if(useWear) disconnect2Wear();
}

void MainWindow::startWear(){
    if(!connected) connect2Wear();
    if(connected) send_command(serv_sock, start_c);
}

void MainWindow::stopWearandRecv(){
    //request smart watch to transfer file
    if(connected){
        send_command(serv_sock, stop_c);
        send_command(serv_sock, send_c);
    }
}
/*
 * setup a socket and try to connect to the server(smart watch)
 * connected will be set true if connect successfully
 * return:
 *      bool: true if connect successfully else false
 */
bool MainWindow::connect2Wear(){
    if(serv_sock==NULL || !send_command(serv_sock, hello_c)){
        memset(&send_buffer, 0, BUFFER_SIZE);
        memset(&recv_buffer, 0, BUFFER_SIZE);
        memset(&serv_addr, 0, sizeof(serv_addr));
        if(serv_sock==NULL){
            set_addr_s(&serv_addr, SERV_IP, SERV_PORT);
            serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (serv_sock == INVALID_SOCKET) {
                printf("main: get socket failed. %ld\n", WSAGetLastError());
                connected = false;
                return connected;
            }
        }
        if (::connect(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
            printf("main: unable to connect to remote server. %ld\n", WSAGetLastError());
            connected = false;
        }else{
            connected = true;
            send_command(serv_sock, hello_c);
            if(!recieveServOn) {
                recieverThread.start();
                recieveServOn = true;
            }
            if(!recieverThread.isRunning()) recieverThread.start();
        }
        return connected;
    }
}

void MainWindow::disconnect2Wear(){
    if(serv_sock && connected) send_command(serv_sock, bye_c);
    if(recieverThread.isRunning()) recieverThread.stop();
    connected = false;
}


void MainWindow::on_cameraCalibrationButton_clicked()
{
    for (int i=0; i < 3; i++){
        delaySecond(1);
        ui->hintLabel->setText(QString::number(3-i));
    }
    ui->hintLabel->setText(QString("Start record checkboard video!"));
    ui->stopRecordProgressButton->setDisabled(true);
    cameraCalibrationUnit();
    ui->hintLabel->setText(QString("Finish checkboard video! Thank you!"));
    ui->stopRecordProgressButton->setEnabled(true);
}

void MainWindow::on_startRecordStaticGestButt_clicked()
{
    for (int i=0; i < 3; i++){
        delaySecond(1);
        ui->hintLabel->setText(QString::number(3-i));
    }
    ui->stopRecordProgressButton->setDisabled(true);
    ui->restStateIDLabel->setText(QString("NULL"));
    ui->hintLabel->setText(QString("Start record static gesture!"));
    for(; sGestIdx<sGestNb; sGestIdx++){
        recordStaticGestUnit();
        delaySecond(1);
    }
    sGestIdx = 0;
    ui->hintLabel->setText(QString("Finish static gesture! Thank you!"));
    ui->stopRecordProgressButton->setEnabled(true);
}
