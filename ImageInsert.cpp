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

void Watermark::imageInsert(Mat &imgMat, bool addLocate) {
    if (imgMat.channels() != 1)
        return;
    if (addLocate || RECT.area() == 0) {
        int modelSize = 4;
        qrReader reader;
        reader.addPattern(imgMat, modelSize);
        RECT.x = 0.1 * imgMat.cols + modelSize * 9 / 2;
        RECT.y = 0.1 * imgMat.rows + modelSize * 9 / 2;
        RECT.width = ceil(0.8 * imgMat.cols - modelSize * 9);
        RECT.height = ceil(0.8 * imgMat.rows - modelSize * 9);
    }
    Mat img;
    img = imgMat(RECT).clone();
    insertMat(img);
    img.copyTo(imgMat(RECT));
}

void Watermark::insertMat(Mat &img) {
    int width = img.cols;
    int height = img.rows;
    img.convertTo(img, CV_32F);
    float *Ypart = (float *) img.data;
    /****************wk --> rsCode****************************************/
    float *rsCode = new float[RSLENGTH];
    unsigned char inputarr[WKLENGTH];
    for (int i = 0; i < WKLENGTH; i++)
        inputarr[i] = wk[i] + '0';
    unsigned char code[RSLENGTH];
    initialize_ecc();
    encode_data(inputarr, code);
    for (int i = 0; i < RSLENGTH; i++)
        rsCode[i] = code[i] - '0';
    /****************wk --> rsCode****************************************/
    float *finger = new float[FINGERLENGTH];
    selectFinger(img, rsCode, finger);
    //wkTemplate h*w
    float *wkTemplate = new float[width * height];
    getTemplate(Ypart, finger, wkTemplate, width, height);
    doInsert(Ypart, wkTemplate, width, height);
    img.convertTo(img, CV_8U);

    delete[] rsCode;
    delete[] finger;
    delete[] wkTemplate;
}

void Watermark::selectFinger(Mat imgMat, float *rsCode, float *finger) {
    //dct zigzag
    Mat dctMat = imgMat.clone();
    resize(dctMat, dctMat, Size(STANDW, STANDH));
    dct(dctMat, dctMat);
    float *dctYpart = (float *) dctMat.data;
    float *zigDct = new float[STANDW * STANDH];
    zigZag(zigDct, dctYpart, STANDW, STANDH);
    //finger
    int group;
    float *sim = new float[32];
    float *zigPart = new float[FINGERLIB];
    int max_sim_index;
    for (int i = 0; i < RSLENGTH / 4; i++) {
        memcpy(zigPart, zigDct + INSPOSITION + i * FINGERLIB, sizeof(float) * FINGERLIB);
        //group 0--15
        group = 8 * rsCode[i * 4] + 4 * rsCode[i * 4 + 1] + 2 * rsCode[i * 4 + 2] + rsCode[i * 4 + 3];
        //fingerLib group*16--group*16+15
        for (int j = group * 32; j < (group + 1) * 32; j++) {
            sim[j % 32] = similar(fingerLib[j], zigPart, FINGERLIB);
            if (j % 32 == 0)
                max_sim_index = 0;
            else if (sim[j % 32] > sim[max_sim_index])
                max_sim_index = j % 32;
        }
        memcpy(finger + i * FINGERLIB, fingerLib[group * 32 + max_sim_index], sizeof(float) * FINGERLIB);
    }
    delete[] zigDct;
    delete[] sim;
    delete[] zigPart;
}

void Watermark::getTemplate(float *Ypart, float *finger, float *wkTemplate, int width, int height) {
    //在嵌入位置插入指纹
    float *wkfinger = new float[STANDW * STANDH];
    memset(wkfinger, 0, sizeof(float) * STANDW * STANDH);
    memcpy(wkfinger + INSPOSITION, finger, sizeof(float) * FINGERLENGTH);
    //izigzag
    float *fingerM = new float[STANDW * STANDH];
    izigZag(fingerM, wkfinger, STANDW, STANDH);
    Mat fingerMat(STANDH, STANDW, CV_32FC1);
    fingerMat.data = (uchar *) fingerM;
    idct(fingerMat, fingerMat);
    resize(fingerMat, fingerMat, Size(width, height));
    float *nvf = new float[width * height];
    getNVF(Ypart, nvf, width, height);
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            wkTemplate[i * width + j] =
                    ALPHA * (17 * (1 - nvf[i * width + j]) + 3 * nvf[i * width + j]) * fingerMat.at<float>(i, j);
    delete[] wkfinger;
    delete[] nvf;
    delete[] fingerM;
}

void Watermark::getNVF(float *Ypart, float *nvf, int width, int height) {
    int i = 0;
    int j = 0;
    //第一次求均值卷积时，保存结果
    float *LocalMean = new float[width * height];
    memset(LocalMean, 0, sizeof(float) * width * height);
    //第二次求平方卷积时，保存结果
    float *zsame = new float[width * height];
    memset(zsame, 0, sizeof(float) * width * height);
    float *LocalVar = new float[width * height];
    memset(LocalVar, 0, sizeof(float) * width * height);
    juanji(Ypart, zsame, LocalMean, width, height);
    float maxvar = -1;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            int index = i * width + j;
            LocalVar[index] = zsame[index] - LocalMean[index] * LocalMean[index];
            if (LocalVar[index] > maxvar) {
                maxvar = LocalVar[index];
            }
        }
    }
    const float Th0 = 150;
    const float Th = Th0 / maxvar;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            int index = i * width + j;
            nvf[index] = 1 / (1 + Th * LocalVar[index]);
        }
    }
    delete[] zsame;
    delete[] LocalMean;
    delete[] LocalVar;
}

void Watermark::juanji(float *Ypart, float *zsame, float *LocalMean, int width, int height) {
    int i, j;
    float *newX_2 = new float[(width + 4) * (height + 4)];
    memset(newX_2, 0, sizeof(float) * (width + 4) * (height + 4));
    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
            newX_2[(i + 2) * (width + 4) + j + 2] = Ypart[i * width + j] * Ypart[i * width + j];
    //先将x进行扩充,上下(左右)加2行(列)
    float *newX = new float[(width + 4) * (height + 4)];
    memset(newX, 0, sizeof(float) * (width + 4) * (height + 4));
    for (j = 0; j < height; j++)
        memcpy(newX + (j + 2) * (width + 4) + 2, Ypart + j * width, sizeof(float) * width);
    const int const_col = width + 4;
    for (i = 0; i < height; i++) {
        //每右移一格，只增加一列
        const int base_row_self = (i + 2) * const_col;
        const int base_row_up = (i + 1) * const_col;
        const int base_row_down = (i + 3) * const_col;
        float col_1 = newX[base_row_self + 1] + newX[base_row_up + 1] + newX[base_row_down + 1];
        float col_2 = newX[base_row_self + 2] + newX[base_row_up + 2] + newX[base_row_down + 2];
        float colZ_1 = newX_2[base_row_self + 1] + newX_2[base_row_up + 1] + newX_2[base_row_down + 1];
        float colZ_2 = newX_2[base_row_self + 2] + newX_2[base_row_up + 2] + newX_2[base_row_down + 2];
        int index1 = i * width;
        for (j = 0; j < width; j++, index1++) {
            float col_3 = newX[base_row_self + j + 3] + newX[base_row_up + j + 3] + newX[base_row_down + j + 3];
            float colZ_3 = newX_2[base_row_self + j + 3] + newX_2[base_row_up + j + 3] + newX_2[base_row_down + j + 3];
            LocalMean[index1] = col_1 + col_2 + col_3;
            LocalMean[index1] /= 9;
            zsame[index1] = colZ_1 + colZ_2 + colZ_3;
            zsame[index1] /= 9;
            col_1 = col_2;
            col_2 = col_3;
            colZ_1 = colZ_2;
            colZ_2 = colZ_3;
        }
    }
    delete[] newX_2;
    delete[] newX;
}

void Watermark::doInsert(float *Ypart, float *temeplate, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            Ypart[i * width + j] += temeplate[i * width + j];
//            Ypart[i * width + j] = 0;
        }
    }
}

void Watermark::creatLocate(Mat &img, int len) {
    img = Mat::zeros(7 * len, 7 * len, CV_8UC1);
    for (int i = 0 * len; i < 1 * len; i++)
        for (int j = 0 * len; j < 7 * len; j++)
            img.at<char>(i, j) = 255;
    for (int i = 6 * len; i < 7 * len; i++)
        for (int j = 0 * len; j < 7 * len; j++)
            img.at<char>(i, j) = 255;
    for (int i = 0 * len; i < 7 * len; i++)
        for (int j = 0 * len; j < 1 * len; j++)
            img.at<char>(i, j) = 255;
    for (int i = 0 * len; i < 7 * len; i++)
        for (int j = 6 * len; j < 7 * len; j++)
            img.at<char>(i, j) = 255;
    for (int i = 2 * len; i < 3 * len; i++)
        for (int j = 2 * len; j < 5 * len; j++)
            img.at<char>(i, j) = 255;
    for (int i = 4 * len; i < 5 * len; i++)
        for (int j = 2 * len; j < 5 * len; j++)
            img.at<char>(i, j) = 255;
    for (int i = 2 * len; i < 5 * len; i++)
        for (int j = 2 * len; j < 3 * len; j++)
            img.at<char>(i, j) = 255;
    for (int i = 2 * len; i < 5 * len; i++)
        for (int j = 4 * len; j < 5 * len; j++)
            img.at<char>(i, j) = 255;
}

