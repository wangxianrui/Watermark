//
// Created by wxrui on 2017/11/16.
//
#include <iostream>
#include <opencv/cv.h>
#include "Watermark.h"
#include "ConstW.h"
#include "RS/ecc.h"
#include "QR/qrReader.h"

using namespace std;
using namespace cv;

float Watermark::detectBlind(Mat imgMat, float prev_detect) {
    if (imgMat.channels() != 1)
        return 0;
    int doubles = ceil((float) 1024 / max(imgMat.rows, imgMat.cols));
    resize(imgMat, imgMat, Size(imgMat.cols * doubles, imgMat.rows * doubles));
    if (prev_detect < 0.75) {
        // Unsatisfied detect    results
        qrReader reader;
        reader.find(imgMat.clone());
        if (reader.getRect().area() > imgMat.size().area() / 3) {
            // correct centerPoints, Rotate must be correct
            RECT = reader.getRect();
            ROTATE = reader.getRotate();
        }
    }
    ///旋转矫正
    Mat imgMatR[2];  ///imgMatR[0].size == imgMat.size   imgMatR[1].size == imgMat.size'
    switch (ROTATE) {
        case 0:
            /// 0
            imgMatR[0] = imgMat.clone();
            ///transpose
            transpose(imgMat, imgMatR[1]);
            break;
        case 90:
            /// flip(0)
            flip(imgMat, imgMatR[0], 0);
            /// -90:  flip(0), transpose
            flip(imgMat, imgMatR[1], 0);
            transpose(imgMatR[1], imgMatR[1]);
            break;
        case 180:
            /// -180:  flip(0), flip(1)
            flip(imgMat, imgMatR[0], 0);
            flip(imgMatR[0], imgMatR[0], 1);
            ///flip(1),transpose,flip(1)
            flip(imgMat, imgMatR[1], 1);
            transpose(imgMatR[1], imgMatR[1]);
            flip(imgMatR[1], imgMatR[1], 1);
            break;
        case 270:
            ///flip(1)
            flip(imgMat, imgMatR[0], 1);
            /// -270:  transpose,flip(0)
            transpose(imgMat, imgMatR[1]);
            flip(imgMatR[1], imgMatR[1], 0);
            break;
    }
    ///检测
    Mat img[2];
    if (RECT.area() == 0) {
        img[0] = imgMatR[0].clone();
        img[1] = imgMatR[1].clone();
    } else {
        img[0] = imgMatR[0](RECT).clone();
        img[1] = imgMatR[1](Rect(RECT.y, RECT.x, RECT.height, RECT.width)).clone();
    }
    return max(detectMat(img[0]), detectMat(img[1]));
}

float Watermark::detectMat(Mat img) {
    img.convertTo(img, CV_32F);
    resize(img, img, Size(STANDW, STANDH));
    dct(img, img);
    float *dctYpart = (float *) img.data;
    float *zigYpart = new float[STANDW * STANDH];
    zigZag(zigYpart, dctYpart, STANDW, STANDH);
    float *extFinger = new float[FINGERLENGTH];
    memcpy(extFinger, zigYpart + INSPOSITION, sizeof(float) * FINGERLENGTH);
    float *rsCode = new float[RSLENGTH];
    getRS(extFinger, rsCode);
    float *extWK = new float[WKLENGTH];
    /****************rsCode --> extwk****************************************/
    unsigned char code[RSLENGTH];
    for (int i = 0; i < RSLENGTH; i++)
        code[i] = rsCode[i] + '0';
    unsigned char outputarr[WKLENGTH];
    initialize_ecc();
    decode_data(code, outputarr);
    for (int i = 0; i < WKLENGTH; i++)
        extWK[i] = outputarr[i] - '0';
    /****************rsCode -->extwk****************************************/
    float sim = distance(wk, extWK, WKLENGTH);
    delete[] zigYpart;
    delete[] extFinger;
    delete[] rsCode;
    delete[] extWK;
    return sim;
}

void Watermark::getRS(float *extFinger, float *rsCode) {
    float *fingerPart = new float[FINGERLIB];
    float *sim = new float[2 * FINGERLIB];
    int maxsim;
    for (int i = 0; i < RSLENGTH / 4; i++) {
        memcpy(fingerPart, extFinger + i * FINGERLIB, sizeof(float) * FINGERLIB);
        for (int j = 0; j < 2 * FINGERLIB; j++) {
            sim[j] = similar(fingerLib[j], fingerPart, FINGERLIB);
            if (j == 0)
                maxsim = 0;
            else if (sim[j] > sim[maxsim])
                maxsim = j;
        }
        maxsim /= 32;
        rsCode[i * 4] = maxsim / 8;
        rsCode[i * 4 + 1] = (maxsim % 8) / 4;
        rsCode[i * 4 + 2] = ((maxsim % 8) % 4) / 2;
        rsCode[i * 4 + 3] = (((maxsim % 8) % 4) % 2);
    }
    delete[] fingerPart;
    delete[] sim;
}

float Watermark::getPSNR(Mat srcMat, Mat dstMat) {
    Mat difmat;
    absdiff(srcMat, dstMat, difmat);
    difmat.convertTo(difmat, CV_32F);
    difmat = difmat.mul(difmat);

    Scalar s = sum(difmat);
    float sse = s.val[0] + s.val[1] + s.val[2];

    if (sse <= 1e-10)
        return 0;
    else {
        float mse = sse / (float) (srcMat.channels() * dstMat.total());
        float psnr = 10.0 * log10((255 * 255) / mse);
        return psnr;//返回PSNR
    }
}

