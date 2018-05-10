//
// Created by wxrui on 3/15/18.
//

#ifndef SCANNINGQRCODE_QRREADER_H
#define SCANNINGQRCODE_QRREADER_H

#pragma once

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class qrReader {
public:
    void addPattern(Mat &img, int len, int delta = 64);

    void find(Mat img, int delta = 64);

    int getRotate();

    Rect getRect();

private:
    vector<Point2f> possibleCenters;
    vector<float> estimatedModuleSize;

    static bool sortPoint(const Point point1, const Point point2);

    bool checkRatio(int *stateCount);

    bool handlePossibleCenter(const Mat &img, int *stateCount, int row, int col);

    float crossCheckVertical(const Mat &img, int startRow, int centerCol, int stateCount, int stateCountTotal);

    float crossCheckHorizontal(const Mat &img, int centerRow, int startCol, int stateCount, int stateCountTotal);

    inline float centerFromEnd(int *stateCount, int end) {
        return (float) (end - stateCount[4] - stateCount[3]) - (float) stateCount[2] / 2.0f;
    }

    void getRightTriangle(vector<Point2f> &points);

    void creatPattern(Mat &pattern, int len);

    void enRange(Mat &img, Mat pattern, int delta);

    void deRange(Mat &img, int delta);

};


#endif //SCANNINGQRCODE_QRREADER_H
