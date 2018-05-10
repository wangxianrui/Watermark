//
// Created by wxrui on 3/15/18.
//
#define NaN std::numeric_limits<float>::quiet_NaN()

#include "qrReader.h"

void qrReader::addPattern(Mat &img, int len, int delta) {
    Mat pattern;
    creatPattern(pattern, len);

    Mat leftTop, leftDown, rightTop;
    leftTop = img(Rect(0.1 * img.cols, 0.1 * img.rows, pattern.cols, pattern.rows));
    enRange(leftTop, pattern, delta);
    leftDown = img(Rect(0.1 * img.cols, ceil(0.9 * img.rows - pattern.rows), pattern.cols, pattern.rows));
    enRange(leftDown, pattern, delta);
    rightTop = img(Rect(ceil(0.9 * img.cols - pattern.cols), 0.1 * img.rows, pattern.cols, pattern.rows));
    enRange(rightTop, pattern, delta);
}

void qrReader::find(Mat img, int delta) {
    deRange(img, delta);
    possibleCenters.clear();
    estimatedModuleSize.clear();
    //隔skipRows扫描
    int skipRows = 1;

    int stateCount[5] = {0};
    int currentState = 0;
    for (int row = skipRows - 1; row < img.rows; row += skipRows) {
        stateCount[0] = 0;
        stateCount[1] = 0;
        stateCount[2] = 0;
        stateCount[3] = 0;
        stateCount[4] = 0;
        currentState = 0;
        const uchar *ptr = img.ptr<uchar>(row);

        for (int col = 0; col < img.cols; col++) {
            if (ptr[col] < 128) {
                // We're at a black pixel
                if ((currentState & 0x1) == 1) {            ///判断奇偶??
                    // We were counting white pixels
                    // So change the state now
                    // W->B transition
                    currentState++;
                }
                // Works for boths W->B and B->B
                stateCount[currentState]++;
            } else {
                // We got to a white pixel...
                if ((currentState & 0x1) == 1) {
                    // W->W change
                    stateCount[currentState]++;
                } else {
                    // ...but, we were counting black pixels
                    if (currentState == 4) {// We found the 'white' area AFTER the finder patter
                        // Do processing for it here
                        if (checkRatio(stateCount))   // This is where we do some more checks
                            handlePossibleCenter(img, stateCount, row, col);
                        currentState = 3;
                        stateCount[0] = stateCount[2];
                        stateCount[1] = stateCount[3];
                        stateCount[2] = stateCount[4];
                        stateCount[3] = 1;
                        stateCount[4] = 0;
                    } else {
                        // We still haven't go 'out' of the finder pattern yet
                        // So increment the state
                        // B->W transition
                        currentState++;
                        stateCount[currentState]++;
                    }
                }
            }
        }
    }
    sort(possibleCenters.begin(), possibleCenters.end(), sortPoint);
    getRightTriangle(possibleCenters);
}

bool qrReader::sortPoint(const Point point1, const Point point2) {
    if (abs(point1.x - point2.x) > 10)
        return point1.x < point2.x;
    else
        return point1.y < point2.y;
}

bool qrReader::checkRatio(int stateCount[]) {
    int totalFinderSize = 0;
    for (int i = 0; i < 5; i++) {
        int count = stateCount[i];
        totalFinderSize += count;
        if (count == 0)
            return false;
    }

    if (totalFinderSize < 7)
        return false;

    // Calculate the size of one module
    int moduleSize = ceil(totalFinderSize / 7.0);  ///向上取整
    int maxVariance = ceil(moduleSize / 2.0);      ///////////////////////////////////////////////////////////2

    bool retVal = ((abs(moduleSize - (stateCount[0])) < maxVariance) &&
                   (abs(moduleSize - (stateCount[1])) < maxVariance) &&
                   (abs(3 * moduleSize - (stateCount[2])) < 3 * maxVariance) &&
                   (abs(moduleSize - (stateCount[3])) < maxVariance) &&
                   (abs(moduleSize - (stateCount[4])) < maxVariance));

    return retVal;
}

bool qrReader::handlePossibleCenter(const Mat &img, int *stateCount, int row, int col) {
    int stateCountTotal = 0;
    for (int i = 0; i < 5; i++) {
        stateCountTotal += stateCount[i];
    }

    // Cross check along the vertical axis
    float centerCol = centerFromEnd(stateCount, col);
    float centerRow = crossCheckVertical(img, row, (int) centerCol, stateCount[2], stateCountTotal);
    if (isnan(centerRow)) {
        return false;
    }

    // Cross check along the horizontal axis with the new center-row
    centerCol = crossCheckHorizontal(img, centerRow, centerCol, stateCount[2], stateCountTotal);
    if (isnan(centerCol)) {
        return false;
    }

//    // Cross check along the diagonal with the new center row and col
//    bool validPattern = crossCheckDiagonal(img, centerRow, centerCol, stateCount[2], stateCountTotal);
//    if (!validPattern) {
//        return false;
//    }
    Point2f ptNew(centerCol, centerRow);
    float newEstimatedModuleSize = stateCountTotal / 7.0f;
    bool found = false;
    int idx = 0;
    // Definitely a finder pattern - but have we seen it before?
    for (Point2f pt : possibleCenters) {
        Point2f diff = pt - ptNew;
        float dist = (float) sqrt(diff.dot(diff));

        // If the distance between two centers is less than 10px, they're the same.
        if (dist < 10) {
            pt = pt + ptNew;
            pt.x /= 2.0f;
            pt.y /= 2.0f;
            estimatedModuleSize[idx] = (estimatedModuleSize[idx] + newEstimatedModuleSize) / 2.0f;
            found = true;
            break;
        }
        idx++;
    }
    if (!found) {
        possibleCenters.push_back(ptNew);
        estimatedModuleSize.push_back(newEstimatedModuleSize);
    }

    return false;
}

float qrReader::crossCheckVertical(const Mat &img, int startRow, int centerCol, int centralCount, int stateCountTotal) {
    int maxRows = img.rows;
    int crossCheckStateCount[5] = {0};

    ///up
    ///black
    int row = startRow;
    while (row >= 0 && img.at<uchar>(row, centerCol) < 128) {
        crossCheckStateCount[2]++;
        row--;
    }
    if (row < 0) {
        return NaN;
    }
    ///white
    while (row >= 0 && img.at<uchar>(row, centerCol) >= 128 && crossCheckStateCount[1] < centralCount) {
        crossCheckStateCount[1]++;
        row--;
    }
    if (row < 0 || crossCheckStateCount[1] >= centralCount) {
        return NaN;
    }
    ///black
    while (row >= 0 && img.at<uchar>(row, centerCol) < 128 && crossCheckStateCount[0] < centralCount) {
        crossCheckStateCount[0]++;
        row--;
    }
    if (row < 0 || crossCheckStateCount[0] >= centralCount) {
        return NaN;
    }

    ///down
    ///black
    row = startRow + 1;
    while (row < maxRows && img.at<uchar>(row, centerCol) < 128) {
        crossCheckStateCount[2]++;
        row++;
    }
    if (row == maxRows) {
        return NaN;
    }
    ///white
    while (row < maxRows && img.at<uchar>(row, centerCol) >= 128 && crossCheckStateCount[3] < centralCount) {
        crossCheckStateCount[3]++;
        row++;
    }
    if (row == maxRows || crossCheckStateCount[3] >= stateCountTotal) {
        return NaN;
    }
    ///black
    while (row < maxRows && img.at<uchar>(row, centerCol) < 128 && crossCheckStateCount[4] < centralCount) {
        crossCheckStateCount[4]++;
        row++;
    }
    if (row == maxRows || crossCheckStateCount[4] >= centralCount) {
        return NaN;
    }

    int crossCheckStateCountTotal = 0;
    for (int i = 0; i < 5; i++) {
        crossCheckStateCountTotal += crossCheckStateCount[i];
    }

    if (5 * abs(crossCheckStateCountTotal - stateCountTotal) >= 2 * stateCountTotal) {
        return NaN;
    }

    float center = centerFromEnd(crossCheckStateCount, row);
    return checkRatio(crossCheckStateCount) ? center : NaN;
}

float
qrReader::crossCheckHorizontal(const Mat &img, int centerRow, int startCol, int centerCount, int stateCountTotal) {
    int maxCols = img.cols;
    int stateCount[5] = {0};

    ///left
    ///black
    int col = startCol;
    const uchar *ptr = img.ptr<uchar>(centerRow);
    while (col >= 0 && ptr[col] < 128) {
        stateCount[2]++;
        col--;
    }
    if (col < 0) {
        return NaN;
    }
    ///white
    while (col >= 0 && ptr[col] >= 128 && stateCount[1] < centerCount) {
        stateCount[1]++;
        col--;
    }
    if (col < 0 || stateCount[1] == centerCount) {
        return NaN;
    }
    ///black
    while (col >= 0 && ptr[col] < 128 && stateCount[0] < centerCount) {
        stateCount[0]++;
        col--;
    }
    if (col < 0 || stateCount[0] == centerCount) {
        return NaN;
    }

    ///right
    ///black
    col = startCol + 1;
    while (col < maxCols && ptr[col] < 128) {
        stateCount[2]++;
        col++;
    }
    if (col == maxCols) {
        return NaN;
    }
    ///white
    while (col < maxCols && ptr[col] >= 128 && stateCount[3] < centerCount) {
        stateCount[3]++;
        col++;
    }
    if (col == maxCols || stateCount[3] == centerCount) {
        return NaN;
    }
    ///black
    while (col < maxCols && ptr[col] < 128 && stateCount[4] < centerCount) {
        stateCount[4]++;
        col++;
    }
    if (col == maxCols || stateCount[4] == centerCount) {
        return NaN;
    }


    int newStateCountTotal = 0;
    for (int i = 0; i < 5; i++) {
        newStateCountTotal += stateCount[i];
    }

    if (5 * abs(stateCountTotal - newStateCountTotal) >= stateCountTotal) {
        return NaN;
    }

    return checkRatio(stateCount) ? centerFromEnd(stateCount, col) : NaN;
}

void qrReader::getRightTriangle(vector<Point2f> &points) {
    float epsilon = 2.0;
    vector<Point2f> temp;
    if (points.size() < 3) {
        points.clear();
        return;
    }
    for (int i = 0; i < points.size() - 2; i++) {
        for (int j = i + 1; j < points.size() - 1; j++) {
            for (int k = j + 1; k < points.size(); k++) {
                if (((abs(points[i].x - points[j].x) < epsilon) &&
                     ((abs(points[i].y - points[k].y) < epsilon) || (abs(points[j].y - points[k].y) < epsilon))) ||
                    ((abs(points[j].x - points[k].x) < epsilon) &&
                     ((abs(points[i].y - points[j].y) < epsilon) || (abs(points[i].y - points[k].y) < epsilon)))) {
                    temp.push_back(points[i]);
                    temp.push_back(points[j]);
                    temp.push_back(points[k]);
                    break;
                }
            }
        }
    }
    if (temp.empty())
        points.clear();
    else
        points = temp;
}

void qrReader::creatPattern(Mat &pattern, int len) {
    pattern = Mat::zeros(9 * len, 9 * len, CV_8UC1);
    for (int i = 0 * len; i < 1 * len; i++)
        for (int j = 0 * len; j < 9 * len; j++)
            pattern.at<char>(i, j) = 255;
    for (int i = 8 * len; i < 9 * len; i++)
        for (int j = 0 * len; j < 9 * len; j++)
            pattern.at<char>(i, j) = 255;
    for (int i = 0 * len; i < 9 * len; i++)
        for (int j = 0 * len; j < 1 * len; j++)
            pattern.at<char>(i, j) = 255;
    for (int i = 0 * len; i < 9 * len; i++)
        for (int j = 8 * len; j < 9 * len; j++)
            pattern.at<char>(i, j) = 255;
    for (int i = 2 * len; i < 3 * len; i++)
        for (int j = 2 * len; j < 7 * len; j++)
            pattern.at<char>(i, j) = 255;
    for (int i = 6 * len; i < 7 * len; i++)
        for (int j = 2 * len; j < 7 * len; j++)
            pattern.at<char>(i, j) = 255;
    for (int i = 2 * len; i < 7 * len; i++)
        for (int j = 2 * len; j < 3 * len; j++)
            pattern.at<char>(i, j) = 255;
    for (int i = 2 * len; i < 7 * len; i++)
        for (int j = 6 * len; j < 7 * len; j++)
            pattern.at<char>(i, j) = 255;
}

void qrReader::enRange(Mat &img, Mat pattern, int delta) {
    int meanValue = mean(img)[0];
    img = (meanValue / delta * delta + meanValue % delta / 4) *
          ((meanValue / delta % 2 != (pattern > 0) / 255) / 255);
    img += ((meanValue / delta + 1) * delta - meanValue % delta / 4) *
           ((meanValue / delta % 2 == (pattern > 0) / 255) / 255);
}

void qrReader::deRange(Mat &img, int delta) {
    for (int r = 0; r < img.rows; r++)
        for (int c = 0; c < img.cols; c++)
            if (img.at<uchar>(r, c) % delta < delta / 2)
                img.at<uchar>(r, c) = (char) 255 * (img.at<uchar>(r, c) / delta + 1) % 2;
            else
                img.at<uchar>(r, c) = (char) 255 * (img.at<uchar>(r, c) / delta) % 2;
}

int qrReader::getRotate() {
    if (possibleCenters.size() != 3)
        return -1;
    if (abs(possibleCenters[0].x - possibleCenters[1].x) < 10) {
        if (abs(possibleCenters[0].y - possibleCenters[2].y) < 10)
            return 0;
        else
            return 90;
    } else {
        if (abs(possibleCenters[0].y - possibleCenters[2].y) < 10)
            return 180;
        else
            return 270;
    }
}

Rect qrReader::getRect() {
    Rect rect;
    if (possibleCenters.size() != 3)
        return rect;
    rect.x = min(min(possibleCenters[0].x, possibleCenters[1].x), possibleCenters[2].x);
    rect.y = min(min(possibleCenters[0].y, possibleCenters[1].y), possibleCenters[2].y);
    rect.width = abs(possibleCenters[0].x - possibleCenters[2].x);
    rect.height = max(abs(possibleCenters[0].y - possibleCenters[1].y),
                      abs(possibleCenters[0].y - possibleCenters[2].y));
    return rect;
}