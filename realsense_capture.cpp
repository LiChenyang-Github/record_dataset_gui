#include "realsense_capture.h"

/*
 * utility function to transfer realsense format image to opencv format Mat
 * params:
 *  pxcImage: pointer to PXCImage instance
 *  format: can be NULL or predefined PIXEL_FORMAT_... format
 * return:
 *  a desired Mat
 */
cv::Mat PXCImage2CVMat(PXCImage *pxcImage, PXCImage::PixelFormat format){
    PXCImage::ImageData data;
    pxcImage->AcquireAccess(PXCImage::ACCESS_READ, format, &data);
    int width = pxcImage->QueryInfo().width;
    int height = pxcImage->QueryInfo().height;

    if(!format)
        format = pxcImage->QueryInfo().format;
    int type;
    switch(format){
    case PXCImage::PIXEL_FORMAT_Y8:
        type = CV_8UC1;
        break;
    case PXCImage::PIXEL_FORMAT_RGB24:
        type = CV_8UC3;
        break;
    case PXCImage::PIXEL_FORMAT_DEPTH_F32:
        type = CV_32FC1;
        break;
    }
    cv::Mat ocvImage = cv::Mat(cv::Size(width, height), type, data.planes[0]);
    pxcImage->ReleaseAccess(&data);
    return ocvImage;
}

int write3DFloat32(PXCPoint3DF32 point, FILE *file){
    return fprintf_s(file, "(%f, %f, %f)", double(point.x), double(point.y), double(point.z));
}

realsense_capture::realsense_capture(){
    mutex.lock();
    pxcStatus status;
    end = false;
    pCnt = cCnt = 0;
    time(&pTime);
    this->pxcSenseManager = PXCSenseManager::CreateInstance();

    // enable hand tracking
    status = this->pxcSenseManager->EnableHand();
    if(status==pxcStatus::PXC_STATUS_NO_ERROR) {
        printf("Enable realsense hand.\n");
    }else{
        printf("Unable realsense hand!\n");
    }

    status = pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_COLOR, \
                                            size.width, \
                                            size.height, \
                                            frameRate);
    // PXCCapture::Device::STREAM_OPTION_STRONG_STREAM_SYNC);
    // don't work, so commented out

    if(status==pxcStatus::PXC_STATUS_NO_ERROR) {
        printf("Enable realsense color stream successfully.\n");
    }else{
        printf("Unable realsense color stream!\n");
    }

    status = pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, \
                                  size.width, \
                                  size.height, \
                                  frameRate);
                                  //PXCCapture::Device::STREAM_OPTION_STRONG_STREAM_SYNC);

    if(status==pxcStatus::PXC_STATUS_NO_ERROR) {
        printf("Enable realsense depth stream successfully.\n");
    }else{
        printf("Unable realsense depth stream!\n");
    }

    PXCHandModule *pxcHandModule = pxcSenseManager->QueryHand();
    PXCHandConfiguration* pxcHandConfig = pxcHandModule->CreateActiveConfiguration();
    pxcHandConfig->EnableTrackedJoints(true);
    pxcHandConfig->SetTrackingMode(PXCHandData::TRACKING_MODE_FULL_HAND);
    pxcHandConfig->ApplyChanges();

    this->pxcHandData = pxcHandModule->CreateOutput();

    if(pxcSenseManager->Init() == pxcStatus::PXC_STATUS_NO_ERROR){
        printf("startRealsense: Init successfully.\n");
    }else{
        printf("startRealsense: Init failed!\n");
        end = true;
    }

    //initialize Mat to store image from realsense
    frameColor = cv::Mat::zeros(size, CV_8UC3);
    frameDepth = cv::Mat::zeros(size, CV_8UC1);
    mutex.unlock();
}

void realsense_capture::stop(){
    mutex.lock();
    end = true;
    condition.wakeOne();
    mutex.unlock();
}

void realsense_capture::stopRecord(){
    emit frameNb(imgIdx+1, recordType);
    toRecord = false;
}

void realsense_capture::init(const char *filename){
    imgIdx = 0;
    recordImgFlag = false;
    char filenames[5][256];
    strcpy_s(filenames[0], filename);
    strcat_s(filenames[0], "_color.avi");

    strcpy_s(filenames[1], filename);
    strcat_s(filenames[1], "_depth.avi");

    strcpy_s(filenames[2], filename);
    strcat_s(filenames[2], "_color_joints.avi");

    strcpy_s(filenames[3], filename);
    strcat_s(filenames[3], "_depth_joints.avi");

    strcpy_s(filenames[4], filename);
    strcat_s(filenames[4], "_joints.txt");
    mutex.lock();

//    //debug
//    for(int i=0; i<5; i++){
//        printf("Filename %d: %s\n", i+1, filenames[i]);
//    }

    file = fopen(filenames[2], "w");

    if(writerColorRe.isOpened()) writerColorRe.release();
    if(writerDepthRe.isOpened()) writerDepthRe.release();
    if(writerColorJoints.isOpened()) writerColorRe.release();
    if(writerDepthJoints.isOpened()) writerDepthRe.release();

    this->writerColorRe.open(filenames[0], CV_FOURCC('D','I','V','X'), frameRate, size, true);
    this->writerDepthRe.open(filenames[1], CV_FOURCC('D','I','V','X'), frameRate, size, false);
    this->writerColorJoints.open(filenames[2], CV_FOURCC('D','I','V','X'), frameRate, size, true);
    this->writerDepthJoints.open(filenames[3], CV_FOURCC('D','I','V','X'), frameRate, size, false);
    mutex.unlock();
}

void realsense_capture::init(const QString rgb_prefix, const QString depth_prefix, const QString coord_prefix){
    imgIdx = 0;
    recordImgFlag = true;
    rgbPrefix = rgb_prefix.toLocal8Bit().constData();
    depthPrefix = depth_prefix.toLocal8Bit().constData();
    coordPrefix = coord_prefix.toLocal8Bit().constData();
}

void realsense_capture::initCC(const QString rgb_prefix){
    imgIdx = 0;
    ccPrefix = rgb_prefix.toLocal8Bit().constData();
}

void realsense_capture::initSG(const QString rgb_prefix, const QString depth_prefix, const QString coord_prefix){
    imgIdx = 0;
    sgRGBPrefix = rgb_prefix.toLocal8Bit().constData();
    sgDepthPrefix = depth_prefix.toLocal8Bit().constData();
    sgCoordPrefix = coord_prefix.toLocal8Bit().constData();
}

void realsense_capture::startRecord(int record_type){

    recordType = record_type;
    imgIdx = 0;
    end = false;
    toRecord = true;
    firstLoop = true;
    if(!isRunning()){
        start();
    }else{
        mutex.lock();
        condition.wakeOne();
        mutex.unlock();
    }
}

void realsense_capture::clearImgIdx(){
    imgIdx = 0;
}

void realsense_capture::run(){
    // TODO: we should wait for realsense initialized completely
    // so that we can run this
    if (recordType == 0){
        for(;;){
            if(!end){
                while(!toRecord){
                    mutex.lock();
                    if(writerColorRe.isOpened()) writerColorRe.release();
                    if(writerDepthRe.isOpened()) writerDepthRe.release();
                    if(writerColorJoints.isOpened()) writerColorRe.release();
                    if(writerDepthJoints.isOpened()) writerDepthRe.release();
                    condition.wait(&mutex);
                    mutex.unlock();
                }

                if (recordImgFlag){

                    PXCHandData::IHand *ihand = 0;
                    PXCHandData::JointData jointData;
                    bool hasJointData;
                    //QuerySample function will NULL untill all frames are available
                    //unless you set its param ifall false
                    if(pxcSenseManager->AcquireFrame(true)<pxcStatus::PXC_STATUS_NO_ERROR) break;
                    PXCCapture::Sample *sample = pxcSenseManager->QuerySample();
                    pxcHandData->Update();
                    if(sample && !sample->IsEmpty()){
                        frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
                        PXCImage2CVMat(sample->depth, PXCImage::PIXEL_FORMAT_DEPTH_F32).convertTo(frameDepth, CV_8UC1);
                        if (firstLoop){
                            firstLoop = false;
                            continue;
                        }

                        rgbImgDir = rgbPrefix + std::to_string(imgIdx) + ".jpg";
                        depthImgDir = depthPrefix + std::to_string(imgIdx) + ".jpg";

                        cv::imwrite(rgbImgDir, frameColor);
                        cv::imwrite(depthImgDir, frameDepth);

                        // Now only one hand is supoorted
                        pxcHandData->QueryHandData(PXCHandData::ACCESS_ORDER_NEAR_TO_FAR, 0, ihand);
                        if(ihand){
                            if(ihand->HasTrackedJoints()) {
                                coordFileDir = coordPrefix + std::to_string(imgIdx) + ".txt";
                                QFile f_w(QString::fromStdString(coordFileDir));

                                for(int i=0;i<JOINT_TYPE_NUM-1;i++){
                                    ihand->QueryTrackedJoint(JOINTS[i], jointData);
                                    txtLine += QString::number(int(jointData.positionImage.x)) + " " + \
                                            QString::number(int(jointData.positionImage.y)) + " " + \
                                            QString::number(float(jointData.positionWorld.x)) + " " + \
                                            QString::number(float(jointData.positionWorld.y)) + " " + \
                                            QString::number(float(jointData.positionWorld.z)) + " " + \
                                            QString::number(int(jointData.confidence)) + "\n";
                                }
                                if (f_w.open(QFile::WriteOnly)) {
                                    QTextStream out(&f_w);
                                    out << txtLine;
                                }
                                f_w.close();
                                txtLine.clear();
                            }
                        }

                        cv::flip(frameDepth, frameDepth, 1);
                        QImage image((const uchar*)(frameDepth.data), \
                                     frameDepth.cols, \
                                     frameDepth.rows, \
                                     QImage::Format_Grayscale8);
                        emit imageReady(image);
                        pxcSenseManager->ReleaseFrame();
                        mutex.lock();
                        printf("realsense thread is sleeping now: %d.\n", imgIdx);
                        imgIdx++;
                        emit finishOneFrame();
                        if (!end){
                            condition.wait(&mutex);
                        }
                        mutex.unlock();
                    }
                }
                else{
                    mutex.lock();
                    PXCHandData::IHand *ihand = 0;
                    PXCHandData::JointData jointData;
                    bool hasJointData;
                    //QuerySample function will NULL untill all frames are available
                    //unless you set its param ifall false
                    if(pxcSenseManager->AcquireFrame(true)<pxcStatus::PXC_STATUS_NO_ERROR) break;
                    PXCCapture::Sample *sample = pxcSenseManager->QuerySample();
                    pxcHandData->Update();
                    if(sample && !sample->IsEmpty()){
                        frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
                        PXCImage2CVMat(sample->depth, PXCImage::PIXEL_FORMAT_DEPTH_F32).convertTo(frameDepth, CV_8UC1);

                        writerColorRe << frameColor;
                        writerDepthRe << frameDepth;

                        // Now only one hand is supoorted
                        pxcHandData->QueryHandData(PXCHandData::ACCESS_ORDER_NEAR_TO_FAR, 0, ihand);
                        if(ihand){
                            if(ihand->HasTrackedJoints()) {
                                hasJointData = true;
                                for(int i=0;i<JOINT_TYPE_NUM;i++){
                                    ihand->QueryTrackedJoint(JOINTS[i], jointData);
                                    cv::circle(frameColor, cv::Point(int(jointData.positionImage.x), \
                                                                     int(jointData.positionImage.y)), \
                                               5, cv::Scalar(int(2.54*jointData.confidence), 0, 0), cv::FILLED);
                                    cv::circle(frameDepth, cv::Point(int(jointData.positionImage.x), \
                                                                     int(jointData.positionImage.y)), \
                                               5, cv::Scalar(int(2.54*jointData.confidence)), cv::FILLED);
                                }
                            }
                        }

                        writerColorJoints << frameColor;
                        writerDepthJoints << frameDepth;

                        QImage image((const uchar*)(frameDepth.data), \
                                     frameDepth.cols, \
                                     frameDepth.rows, \
                                     QImage::Format_Grayscale8);
                        emit imageReady(image);
                        pxcSenseManager->ReleaseFrame();
                        cCnt ++;
                        if((cCnt-pCnt)==200){
                            time(&cTime);
                            double fps = (cCnt-pCnt)/(cTime-pTime);
                            printf("Realsense capture fps: %f\n", fps);
                            pCnt = cCnt;
                            pTime = cTime;
                        }
                        mutex.unlock();
                    }
                }

            }else{
                mutex.lock();
//                if(pxcSenseManager->IsConnected()) pxcSenseManager->Release();
                if(writerColorRe.isOpened()) writerColorRe.release();
                if(writerDepthRe.isOpened()) writerDepthRe.release();
                if(writerColorJoints.isOpened()) writerColorRe.release();
                if(writerDepthJoints.isOpened()) writerDepthRe.release();
                // fclose(file);
                mutex.unlock();
                printf("realsense_thread terminated.\n");
                return;
            }
        }
    }
    else if (recordType == 1){
        for(;;){
            if(!end){
                while(!toRecord){
                    mutex.lock();
                    if(writerColorRe.isOpened()) writerColorRe.release();
                    if(writerDepthRe.isOpened()) writerDepthRe.release();
                    if(writerColorJoints.isOpened()) writerColorRe.release();
                    if(writerDepthJoints.isOpened()) writerDepthRe.release();
                    condition.wait(&mutex);
                    mutex.unlock();
                }

                PXCHandData::IHand *ihand = 0;
                PXCHandData::JointData jointData;
                bool hasJointData;
                //QuerySample function will NULL untill all frames are available
                //unless you set its param ifall false
                if(pxcSenseManager->AcquireFrame(true)<pxcStatus::PXC_STATUS_NO_ERROR) break;
                PXCCapture::Sample *sample = pxcSenseManager->QuerySample();
                pxcHandData->Update();
                if(sample && !sample->IsEmpty()){
                    frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
                    if (firstLoop){
                        firstLoop = false;
                        continue;
                    }

                    ccrgbImgDir = ccPrefix + std::to_string(imgIdx) + ".jpg";

                    cv::imwrite(ccrgbImgDir, frameColor);
                    cv::cvtColor(frameColor, frameColor, CV_BGR2RGB);
                    cv::flip(frameColor, frameColor, 1);

                    QImage image((const uchar*)(frameColor.data), \
                                 frameColor.cols, \
                                 frameColor.rows, \
                                 QImage::Format_RGB888);
                    emit imageReady(image);
                    pxcSenseManager->ReleaseFrame();
                    mutex.lock();
                    printf("realsense thread is sleeping now: %d.\n", imgIdx);
                    imgIdx++;
                    emit finishOneFrame();
                    if (!end){
//                        qDebug() << QTime::currentTime().msec();
                        condition.wait(&mutex);
                    }
                    mutex.unlock();
                }
            }
            else{
                printf("realsense_thread terminated.\n");
                return;
            }
        }
    }
    else{
        for(;;){
            if(!end){
                while(!toRecord){
                    mutex.lock();
                    condition.wait(&mutex);
                    mutex.unlock();
                }

                PXCHandData::IHand *ihand = 0;
                PXCHandData::JointData jointData;
                //QuerySample function will NULL untill all frames are available
                //unless you set its param ifall false
                if(pxcSenseManager->AcquireFrame(true)<pxcStatus::PXC_STATUS_NO_ERROR) break;
                PXCCapture::Sample *sample = pxcSenseManager->QuerySample();
                pxcHandData->Update();
                if(sample && !sample->IsEmpty()){
                    frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
                    PXCImage2CVMat(sample->depth, PXCImage::PIXEL_FORMAT_DEPTH_F32).convertTo(frameDepth, CV_8UC1);
                    if (firstLoop){
                        firstLoop = false;
                        continue;
                    }

                    sgRGBImgDir = sgRGBPrefix + std::to_string(imgIdx) + ".jpg";
                    sgDepthImgDir = sgDepthPrefix + std::to_string(imgIdx) + ".jpg";

                    cv::imwrite(sgRGBImgDir, frameColor);
                    cv::imwrite(sgDepthImgDir, frameDepth);
                    cv::cvtColor(frameColor, frameColor, CV_BGR2RGB);

                    // Now only one hand is supoorted
                    pxcHandData->QueryHandData(PXCHandData::ACCESS_ORDER_NEAR_TO_FAR, 0, ihand);
                    if(ihand){
                        if(ihand->HasTrackedJoints()) {
                            sgCoordFileDir = sgCoordPrefix + std::to_string(imgIdx) + ".txt";
                            QFile f_w(QString::fromStdString(sgCoordFileDir));

                            for(int i=0;i<JOINT_TYPE_NUM-1;i++){
                                ihand->QueryTrackedJoint(JOINTS[i], jointData);
                                txtLine += QString::number(int(jointData.positionImage.x)) + " " + \
                                        QString::number(int(jointData.positionImage.y)) + " " + \
                                        QString::number(float(jointData.positionWorld.x)) + " " + \
                                        QString::number(float(jointData.positionWorld.y)) + " " + \
                                        QString::number(float(jointData.positionWorld.z)) + " " + \
                                        QString::number(int(jointData.confidence)) + "\n";
                                cv::circle(frameColor, cv::Point(int(jointData.positionImage.x), int(jointData.positionImage.y)), \
                                           5, cv::Scalar(255, 0, 0));
                            }
                            cv::flip(frameColor, frameColor, 1);
                            if (f_w.open(QFile::WriteOnly)) {
                                QTextStream out(&f_w);
                                out << txtLine;
                            }
                            f_w.close();
                            txtLine.clear();
                        }
                    }

                    QImage image((const uchar*)(frameColor.data), \
                                 frameColor.cols, \
                                 frameColor.rows, \
                                 QImage::Format_RGB888);
                    emit imageReady(image);
                    pxcSenseManager->ReleaseFrame();
                    mutex.lock();
                    printf("realsense thread is sleeping now: %d.\n", imgIdx);
                    imgIdx++;
                    emit finishOneFrame();
                    if (!end){
                        condition.wait(&mutex);
                    }
                    mutex.unlock();
                }
            }
            else{
                printf("realsense_thread terminated.\n");
                return;
            }
        }
    }
}

void realsense_capture::processNextFrame(){
    mutex.lock();
    condition.wakeOne();
    mutex.unlock();
}
