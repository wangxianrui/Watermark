//
// Created by wxrui on 2017/11/15.
//

#ifndef WATERMARK_WATERMARK_H
#define WATERMARK_WATERMARK_H

#include <iostream>
#include <opencv/cv.h>

using namespace std;
using namespace cv;

class Watermark {
public:
    ///初始化水印嵌入检测工具 fingerLibfile:指纹库  wkfile:指纹文件
    Watermark(string fingerLibfile = "fingerLib", string wkfile = "wk");

    ///指纹嵌入操作 imgMat:待嵌入图像 addLocate:是否添加定位点
    void imageInsert(Mat &imgMat, bool addLocate = false);

    ///指纹检测操作 imgMat:待检测图像
    float detectBlind(Mat imgMat, float prev_detect = 0);

    ///原图像和带水印图像PSNR
    float getPSNR(Mat srcMat, Mat dstMat);

private:
    float *wk;
    float **fingerLib;
    Rect RECT;
    int ROTATE;

    bool getWK(string wkfile = "wk");

    bool getFingerLib(string fingerLibfile = "fingerLib");

    float distance(float *ori, float *ext, int len);

    float similar(float *ori, float *ext, int len);

    float norm(float *temp, int len);

    void selectFinger(Mat imgMat, float *rsCode, float *finger);

    void juanji(float *Ypart, float *zsame, float *LocalMean, int width, int height);

    void getNVF(float *Ypart, float *nvf, int width, int height);

    void getTemplate(float *Ypart, float *finger, float *wkTemplate, int width, int height);

    void doInsert(float *Ypart, float *temeplate, int width, int height);

    void getRS(float *extFinger, float *rsCode);

    void zigZag(float *tarMat, float *srcMat, int width, int height);

    void izigZag(float *tarMat, float *srcMat, int width, int height);

    void creatLocate(Mat &img, int len);

    void getPoints(Mat imgMat, vector<Point> &points);

    void getRotate(vector<Point2f> points);

    void getRect(vector<Point2f> points);

    static Point Center_cal(vector<vector<Point> > contours, int i);

    static bool sortPoint(const Point point1, const Point point2);

    float detectMat(Mat img);

    void insertMat(Mat &img);
};


#endif //WATERMARK_WATERMARK_H
