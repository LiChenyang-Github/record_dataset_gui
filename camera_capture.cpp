#include "camera_capture.h"

camera_capture::camera_capture(int cameraID){
    if(!cap.isOpened())
        if(!cap.open(cameraID)){
            printf("camera_capture: unable to open camera!\n");
        }

    pCnt = cCnt = 0;
//    time(&pTime);
    pTime = (double)cv::getTickFrequency();
    end = false;
    toRecord = false;
}

void camera_capture::init(char *filename, bool recordImg){
    recordImgFlag = recordImg;
    imgIdx = 0;
    if (recordImg){
//        strcpy(cameraImgPrefix, filename);
        cameraImgPrefix = filename;
    }
    else{
        mutex.lock();
        if(wrt.isOpened()) wrt.release();
        wrt.open(filename, CV_FOURCC('D','I','V','X'), frameRate, \
                 cv::Size(int(cap.get(CV_CAP_PROP_FRAME_WIDTH)), \
                          int(cap.get(CV_CAP_PROP_FRAME_HEIGHT))),\
                 true);
        mutex.unlock();
        if(wrt.isOpened()) printf("Video writer opened.\n");
        else printf("Unable to open video writer.\n");
    }
}

void camera_capture::initCC(char *filename){
    imgIdx = 0;
    ccPrefix = filename;
}

void camera_capture::initSG(char *filename){
    imgIdx = 0;
    sgPrefix = filename;
}

void camera_capture::startRecord(int record_type){

    recordType = record_type;
    imgIdx = 0;
    toRecord = true;
    end = false;
    firstLoop = true;
    if(!isRunning()){
        start();
    }else{
        mutex.lock();
        condition.wakeOne();
        mutex.unlock();
    }
}

void camera_capture::stopRecord(){
//    qDebug() << "emit frameNb(imgIdx+1)";
    emit frameNb(imgIdx+1, recordType);
//    qDebug() << "emit frameNb(imgIdx+1)";
    toRecord = false;
}


void camera_capture::stop(){
    mutex.lock();
    end = true;
    condition.wakeOne();
    mutex.unlock();
}

void camera_capture::clearImgIdx(){
    imgIdx = 0;
}

void camera_capture::run(){
    if (recordType == 0){
        for(;;){
            if(!end){
                while(!toRecord){
                    mutex.lock();
                    if(wrt.isOpened()) wrt.release();
                    printf("camera thread is sleeping now.\n");
                    condition.wait(&mutex);
                    mutex.unlock();
                }
                if (recordImgFlag){
                    if(cap.isOpened()){
                        cap >> img;
                        if (firstLoop){
                            firstLoop = false;
                            continue;
                        }
                        if(!img.empty()){
                            cameraImgDir = cameraImgPrefix + std::to_string(imgIdx) + ".jpg";
    //                        qDebug() << cameraImgDir.c_str();
                        }
                        cv::imwrite(cameraImgDir, img);
                        cv::cvtColor(img, img, CV_BGR2RGB);
    //                    cv::flip(img, img, 1);
                        QImage image((const uchar*) (img.data), img.cols, img.rows, QImage::Format_RGB888);
                        emit imageReady(image);
                        mutex.lock();
                        printf("camera thread is sleeping now: %d.\n", imgIdx);
                        imgIdx++;
                        emit finishOneFrame();
                        if (!end){
                            condition.wait(&mutex);
                        }
                        mutex.unlock();
                    }
                }
                else{
                    if(cap.isOpened()){
                        cap >> img;
                        if(!img.empty()){
                            mutex.lock();
                            wrt << img;
                            mutex.unlock();
                            mutex.lock();
                            cv::cvtColor(img, img, CV_BGR2RGB);
                            cv::flip(img, img, 1);
                            QImage image((const uchar*) (img.data), img.cols, img.rows, QImage::Format_RGB888);
                            emit imageReady(image);
                            mutex.unlock();
    //                        cCnt ++;
    //                        if((cCnt-pCnt)==200){
    ////                            time(&cTime);
    //                            cTime = (double)cv::getTickFrequency();
    //                            double fps = (cCnt-pCnt)/(cTime-pTime)*1000;
    //                            emit fpsReady(fps);
    //                            printf("Camera capture fps: %f\n", fps);
    //                            pCnt = cCnt;
    //                            pTime = cTime;
    //                        }
                        }
                    }
                }

            }else{ //terminate this thread
                if (!recordImgFlag){
                    if(cap.isOpened())cap.release();
                    if(wrt.isOpened())wrt.release();
                }
                imgIdx = 0;
                printf("Camera_thread terminated.\n");
                return;
            }
        }
    }
    else if (recordType == 1){
        for(;;){
            if(!end){
                while(!toRecord){
                    mutex.lock();
                    if(wrt.isOpened()) wrt.release();
                    printf("camera thread is sleeping now.\n");
                    condition.wait(&mutex);
                    mutex.unlock();
                }
                if(cap.isOpened()){
                    cap >> img;
                    if (firstLoop){
                        firstLoop = false;
                        continue;
                    }
                    if(!img.empty()){
                        ccImgDir = ccPrefix + std::to_string(imgIdx) + ".jpg";
                    }
                    cv::imwrite(ccImgDir, img);
                    cv::cvtColor(img, img, CV_BGR2RGB);
                    cv::flip(img, img, 1);
                    QImage image((const uchar*) (img.data), img.cols, img.rows, QImage::Format_RGB888);
                    emit imageReady(image);
                    mutex.lock();
                    printf("camera thread is sleeping now: %d.\n", imgIdx);
                    imgIdx++;
                    emit finishOneFrame();
                    if (!end){
//                        qDebug() << QTime::currentTime().msec();
                        condition.wait(&mutex);
                    }
                    mutex.unlock();
                }
            }
            else{ //terminate this thread
                imgIdx = 0;
                printf("Camera_thread terminated.\n");
                return;
            }
        }
    }
    else{
        for(;;){
            if(!end){
                while(!toRecord){
                    mutex.lock();
                    if(wrt.isOpened()) wrt.release();
                    printf("camera thread is sleeping now.\n");
                    condition.wait(&mutex);
                    mutex.unlock();
                }
                if(cap.isOpened()){
                    cap >> img;
                    if (firstLoop){
                        firstLoop = false;
                        continue;
                    }
                    if(!img.empty()){
                        sgImgDir = sgPrefix + std::to_string(imgIdx) + ".jpg";
                    }
                    cv::imwrite(sgImgDir, img);
                    cv::cvtColor(img, img, CV_BGR2RGB);
                    cv::flip(img, img, 1);
                    QImage image((const uchar*) (img.data), img.cols, img.rows, QImage::Format_RGB888);
                    emit imageReady(image);
                    mutex.lock();
                    printf("camera thread is sleeping now: %d.\n", imgIdx);
                    imgIdx++;
                    emit finishOneFrame();
                    if (!end){
                        condition.wait(&mutex);
                    }
                    mutex.unlock();
                }
            }else{ //terminate this thread
                imgIdx = 0;
                printf("Camera_thread terminated.\n");
                return;
            }
        }
    }
}

void camera_capture::processNextFrame(){
    mutex.lock();
    condition.wakeOne();
    mutex.unlock();
}
