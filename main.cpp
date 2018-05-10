#include <iostream>
#include <vector>
#include <dirent.h>
#include <cstring>
#include <algorithm>
#include <opencv/highgui.h>
#include <fstream>
#include "Watermark.h"

using namespace std;


void getFiles(string cate_dir, vector<string> &files, string substring) {
    DIR *dir;
    struct dirent *ptr;

    if ((dir = opendir(cate_dir.c_str())) == NULL) {
        perror("Open dir error...");
        exit(1);
    }
    while ((ptr = readdir(dir)) != NULL) {
        if (ptr->d_type == 8 && (bool) strstr(ptr->d_name, substring.c_str()))    ///file
            files.push_back(ptr->d_name);
    }
    closedir(dir);
    sort(files.begin(), files.end());
}

void salt(cv::Mat &image, int n) {
    for (int k = 0; k < n; k++) {
        // 生成行列的随机数
        int iCol = rand() % image.cols;
        int jRow = rand() % image.rows;
        // 判断是否是灰度图
        if (1 == image.channels()) {
            image.at<uchar>(jRow, iCol) = 255;
        }
            // 判断是否是彩色图像
        else if (3 == image.channels()) {
            image.at<cv::Vec3b>(jRow, iCol)[0] = 255;
            image.at<cv::Vec3b>(jRow, iCol)[1] = 255;
            image.at<cv::Vec3b>(jRow, iCol)[2] = 255;
        }
    }
}

void attack() {
    vector<string> files;
    getFiles("../dstImg/", files, "");
    string srcfile;
    string path = "../attackImg/";
    string filename, filetype;
    Mat img;
    Mat temp;
    for (string name : files) {
        srcfile = "../dstImg/" + name;
        img = imread(srcfile, 0);
        cout << "attack" << name << img.size() << endl;
        filename = name.substr(0, name.length() - 4);
        filetype = name.substr(name.length() - 4);
        //flipx
        flip(img, temp, 0);
        imwrite(path + filename + "_rotate0+flipx" + filetype, temp);
        //rotate90
        transpose(img, temp);
        flip(temp, temp, 0);
        imwrite(path + filename + "_rotate90" + filetype, temp);
        //rotate90 + filpx
        flip(temp, temp, 0);
        imwrite(path + filename + "_rotate90+flipx" + filetype, temp);
        //rotate180
        flip(img, temp, 1);
        flip(temp, temp, 0);
        imwrite(path + filename + "_rotate180" + filetype, temp);
        //rotate180 + flipx
        flip(temp, temp, 0);
        imwrite(path + filename + "_rotate180+flipx" + filetype, temp);
        //rotate270
        transpose(img, temp);
        flip(temp, temp, 1);
        imwrite(path + filename + "_rotate270" + filetype, temp);
        //rotate270 + flipx
        flip(temp, temp, 0);
        imwrite(path + filename + "_rotate270+flipx" + filetype, temp);
        //crop
        temp = img(Rect(img.cols * 0.05, img.rows * 0.05, 0.9 * img.cols, 0.9 * img.rows));
        imwrite(path + filename + "_crop" + filetype, temp);
        //averageblurblur
        blur(img, temp, Size(3, 3));
        imwrite(path + filename + "_averageblur" + filetype, temp);
        //gaussianblur
        GaussianBlur(img, temp, Size(5, 5), 0);
        imwrite(path + filename + "_gaussianblur" + filetype, temp);
        //resize
        resize(img, temp, Size(img.cols * 2, img.rows * 2));
        imwrite(path + filename + "_size2" + filetype, temp);
        resize(img, temp, Size(img.cols * 0.5, img.rows * 0.5));
        imwrite(path + filename + "_size05" + filetype, temp);
        //salt
        temp = img.clone();
        salt(temp, temp.size().area() / 100);
        imwrite(path + filename + "salt" + filetype, temp);
    }
}

void insert(string srcpath, string dstpath) {
    Watermark wm;
    string srcfile, dstfile;
    Mat srcMat, dstMat;
    vector<string> files;
    getFiles(srcpath, files, ".jpg");
    for (int i = 0; i < 100; i++) {
        srcfile = srcpath + files[i];
        dstfile = dstpath + files[i];
        srcMat = imread(srcfile, 0);
        cout << "insert" << srcfile << endl;
        if (i % 20 == 0)
            wm.imageInsert(srcMat, true);
        else
            wm.imageInsert(srcMat, false);
        imwrite(dstfile, srcMat);
    }
}

void detect(string srcpath, string imgSize, string attackType) {
    Watermark wm;
    string srcfile;
    Mat srcMat;
    vector<string> files;
    getFiles(srcpath, files, attackType + ".jpg");
    ofstream output("result/" + imgSize + attackType);
    for (int i = 0; i < files.size(); i++) {
        srcfile = srcpath + files[i];
        srcMat = imread(srcfile, 0);
        cout << "detect" << imgSize << files[i] << endl;
        output << wm.detectBlind(srcMat) << endl;
    }
}

void psnr(string srcpath, string dstpath, string imgSize) {
    Watermark wm;
    string srcfile, dstfile;
    Mat srcMat, dstMat;
    vector<string> files;
    getFiles(dstpath, files, ".jpg");
    ofstream output("result/" + imgSize + "psnr");
    for (int i = 0; i < files.size(); i++) {
        srcfile = srcpath + files[i];
        dstfile = dstpath + files[i];
        srcMat = imread(srcfile, 0);
        dstMat = imread(dstfile, 0);
        cout << "psnr" << imgSize << files[i] << endl;
        output << wm.getPSNR(srcMat, dstMat) << endl;
    }
}

int main() {

//    "480_272", "640_360", "720_480", "1024_600", "1920_1080", "2560_1440", "3840_2160"
    string imageSize[] = {"480_272"};
//    "averageblur", "crop", "gaussianblur", "rotate0+flipx", "rotate90", "rotate90+flipx",
//            "rotate180", "rotate180+flipx", "rotate270", "rotate270+flipx", "salt", "size2", "size05"
    string attackType[] = {"rotate0+flipx", "rotate90", "rotate90+flipx",
                           "rotate180", "rotate180+flipx", "rotate270", "rotate270+flipx", "salt", "size2", "size05"};
    string srcpath;
    string dstpath = "../dstImg/";
    string attackpath = "../attackImg/";
    for (string imgSize : imageSize) {
        srcpath = "../oriImg/" + imgSize + "/";
        insert(srcpath, dstpath);
//        detect(dstpath, imgSize, "");
//        psnr(srcpath, dstpath, imgSize);
        attack();
        for (string attType : attackType)
            detect(attackpath, imgSize, attType);
    }


    return 0;
}
