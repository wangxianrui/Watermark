//
// Created by wxrui on 2017/11/15.
//

#include <fstream>
#include <math.h>
#include <cstring>
#include <opencv/highgui.h>
#include "Watermark.h"
#include "ConstW.h"

Watermark::Watermark(string fingerLibfile, string wkfile) {
    wk = new float[WKLENGTH];
    fingerLib = new float *[2 * FINGERLIB];
    for (int i = 0; i < 2 * FINGERLIB; i++)
        fingerLib[i] = new float[FINGERLIB];
    getWK(wkfile);
    getFingerLib(fingerLibfile);
    ROTATE = 0;
    RECT = Rect(0, 0, 0, 0);
}

//提取原始水印 128
bool Watermark::getWK(string wkfile) {
    ifstream input(wkfile);
    if (!input) {
        return false;
    }
    for (int i = 0; i < WKLENGTH; i++) {
        input >> *(wk + i);
    }
    input.close();
    return true;
}

//读取指纹库
bool Watermark::getFingerLib(string fingerLibfile) {
    ifstream input(fingerLibfile);
    if (!input) {
        return false;
    }
    for (int i = 0; i < 2 * FINGERLIB; i++)
        for (int j = 0; j < FINGERLIB; j++)
            input >> *(fingerLib[i] + j);
    input.close();
    return true;
}

//计算相关值sim
float Watermark::similar(float *ori, float *ext, int len) {
    float sim = 0;
    float norm_ori, norm_ext;
    for (int i = 0; i < len; i++) {
        sim += (*(ori + i)) * (*(ext + i));
    }
    norm_ori = norm(ori, len);
    norm_ext = norm(ext, len);
    if (!norm_ext) {
        cout << "norm_ext===0" << endl;
        return 0;
    } else if (!norm_ori) {
        cout << "norm_ori===0" << endl;
        return 0;
    } else {
        sim /= (norm_ori * norm_ext);
        return sim;
    }
}

//norm
float Watermark::norm(float *temp, int len) {
    int i = 0;
    double sum = 0;
    for (; i < len; i++)
        sum += (*(temp + i)) * (*(temp + i));
    sum = sqrtf(sum);
    return sum;
}

void Watermark::zigZag(float *tarMat, float *srcMat, int width, int height) {
    int h = 1, v = 1;
    int vmin = 1, hmin = 1;
    int vmax = height, hmax = width;
    int i;

    memset(tarMat, 0, sizeof(float) * height * width);

    i = 1;
    while (v <= vmax && h <= hmax) {
        if ((h + v) % 2 == 0) {
            if (v == vmin) {
                tarMat[i - 1] = *(srcMat + width * (v - 1) + h - 1);

                if (h == hmax)
                    v = v + 1;
                else
                    h = h + 1;

                i = i + 1;
            } else if ((h == hmax) && (v < vmax)) {
                tarMat[i - 1] = *(srcMat + width * (v - 1) + h - 1);
                v = v + 1;
                i = i + 1;
            } else if ((v > vmin) && (h < hmax)) {
                tarMat[i - 1] = *(srcMat + width * (v - 1) + h - 1);
                v = v - 1;
                h = h + 1;
                i = i + 1;
            }
        } else {
            if ((v == vmax) && (h <= hmax)) {
                tarMat[i - 1] = *(srcMat + width * (v - 1) + h - 1);
                h = h + 1;
                i = i + 1;
            } else if (h == hmin) {
                tarMat[i - 1] = *(srcMat + width * (v - 1) + h - 1);
                if (v == vmax)
                    h = h + 1;
                else
                    v = v + 1;
                i = i + 1;
            } else if ((v < vmax) && (h > hmin)) {
                tarMat[i - 1] = *(srcMat + width * (v - 1) + h - 1);
                v = v + 1;
                h = h - 1;
                i = i + 1;
            }
        }

        if ((v == vmax) && (h == hmax)) {
            tarMat[i - 1] = *(srcMat + width * (v - 1) + h - 1);
            break;
        }
    }
}

void Watermark::izigZag(float *tarMat, float *srcMat, int width, int height) {
    int h = 1, v = 1;
    int vmin = 1, hmin = 1;
    int vmax = height, hmax = width;
    int i;
    memset(tarMat, 0, sizeof(float) * height * width);
    i = 1;
    while (v <= vmax && h <= hmax) {
        if ((h + v) % 2 == 0) {
            if (v == vmin) {
                *(tarMat + width * (v - 1) + h - 1) = srcMat[i - 1];

                if (h == hmax)
                    v = v + 1;
                else
                    h = h + 1;

                i = i + 1;
            } else if ((h == hmax) && (v < vmax)) {
                *(tarMat + width * (v - 1) + h - 1) = srcMat[i - 1];
                v = v + 1;
                i = i + 1;
            } else if ((v > vmin) && (h < hmax)) {
                *(tarMat + width * (v - 1) + h - 1) = srcMat[i - 1];
                v = v - 1;
                h = h + 1;
                i = i + 1;
            }
        } else {
            if ((v == vmax) && (h <= hmax)) {
                *(tarMat + width * (v - 1) + h - 1) = srcMat[i - 1];
                h = h + 1;
                i = i + 1;
            } else if (h == hmin) {
                *(tarMat + width * (v - 1) + h - 1) = srcMat[i - 1];
                if (v == vmax)
                    h = h + 1;
                else
                    v = v + 1;
                i = i + 1;
            } else if ((v < vmax) && (h > hmin)) {
                *(tarMat + width * (v - 1) + h - 1) = srcMat[i - 1];
                v = v + 1;
                h = h - 1;
                i = i + 1;
            }
        }

        if ((v == vmax) && (h == hmax)) {
            *(tarMat + width * (v - 1) + h - 1) = srcMat[i - 1];
            break;
        }
    }
}

float Watermark::distance(float *ori, float *ext, int len) {
    float res = 0;
    float sum = 0;
    for (int i = 0; i < len; i++)
        sum += ((*(ori + i)) - (*(ext + i))) * ((*(ori + i)) - (*(ext + i)));
    res = 1 - sqrtf(sum) / sqrtf(len);
    return res;
}

Point Watermark::Center_cal(vector<vector<Point> > contours, int i) {
    int centerx = 0, centery = 0, n = contours[i].size();
    for (int j = 0; j < n; j++) {
        centerx += contours[i][j].x;
        centery += contours[i][j].y;
    }
    return Point(centerx / n, centery / n);
}

bool Watermark::sortPoint(const Point point1, const Point point2) {
    if (abs(point1.x - point2.x) > 10)
        return point1.x < point2.x;
    else
        return point1.y < point2.y;
}

void Watermark::getPoints(Mat imgMat, vector<Point> &points) {
    points.clear();
    if (imgMat.channels() != 1)
        cvtColor(imgMat, imgMat, CV_BGR2GRAY);
    Mat gb;
    GaussianBlur(imgMat, gb, Size(3, 3), 0); ///gaussianblur
    Mat binary;
    adaptiveThreshold(gb, binary, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY, 41, 0);
    Mat edges;
    Canny(binary, edges, 100, 200);
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(edges, contours, hierarchy, RETR_TREE, CV_CHAIN_APPROX_NONE);
    vector<int> found;
    int k, c, k_temp;
    for (int i = contours.size() - 1; i >= 0; i--) {
        k = i;
        c = 0;
        //不包含其他轮廓
        if (hierarchy[k][2] == -1) {
            //父轮廓存在
            while (hierarchy[k][3] != -1) {
                k_temp = k;
                k = hierarchy[k][3];
                if (k != k_temp - 1) break;
                c++;
                if (c >= 5) {
                    found.push_back(k);
                    break;
                }
            }
        }
    }
    if (found.size() == 3) {
        for (int i = 0; i < found.size(); i++)
            points.push_back(Center_cal(contours, found[i]));
        sort(points.begin(), points.end(), sortPoint);
        ///3个直角定位点
        if ((abs(points[0].x - points[1].x) < 10 &&
             (abs(points[2].y - points[0].y) < 10 || abs(points[2].y - points[1].y) < 10)) ||
            (abs(points[1].x - points[2].x) < 10 &&
             (abs(points[0].y - points[1].y) < 10 || abs(points[0].y - points[2].y) < 10))) {
            return;
        } else
            points.clear();
    }

}

void Watermark::getRotate(vector<Point2f> points) {
    if (points.size() != 3)
        return;
    if (abs(points[0].x - points[1].x) < 10) {
        if (abs(points[0].y - points[2].y) < 10)
            ROTATE = 0;
        else
            ROTATE = 90;
    } else {
        if (abs(points[0].y - points[2].y) < 10)
            ROTATE = 180;
        else
            ROTATE = 270;
    }
    cout << ROTATE << endl;
}

void Watermark::getRect(vector<Point2f> points) {
    if (points.size() != 3)
        return;
    RECT.x = min(min(points[0].x, points[1].x), points[2].x);
    RECT.y = min(min(points[0].y, points[1].y), points[2].y);
    RECT.width = abs(points[0].x - points[2].x);
    RECT.height = max(abs(points[0].y - points[1].y), abs(points[0].y - points[2].y));
    cout << RECT << endl;
}


