#include <limits.h>
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

using namespace std;

// глобальные переменные
char* path = "../../dump_13122019_145433.bin";
int width = 640;
int height = 512;


int main()
{
    // изображения
    cv::Mat img = cv::Mat(cv::Size(width, height), CV_16UC1);
    cv::Mat grayscale = cv::Mat(cv::Size(width, height), CV_8UC3);


    // открытие файла
    FILE* f = fopen(path, "r");


    // построение гистограммы
    int histVal[256]{0};
    cv::Mat hist = cv::Mat(cv::Size(512, 512), CV_8UC3);
    int bin_w = hist.cols / 256;
    fseek(f, 32, SEEK_SET);
    int min = INT_MAX;
    int max = INT_MIN;
    int diff = 0;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            char u2 = fgetc(f);
            char u1 = fgetc(f);
            uchar val = ((static_cast<unsigned short>(u2) << 8) + static_cast<unsigned short>(u1)) / 257;
            histVal[val] += 1;
        }
    }
    for(int i = 0; i < 256; i++){
        if(max < histVal[i]) max = histVal[i];
        if(min > histVal[i]) min = histVal[i];
    }
    diff = max - min;
    for(int i = 1; i < 256; i++){
        cv::line(hist,
                 cv::Point(bin_w * (i-1), hist.rows - hist.rows * histVal[i-1] / diff),
                 cv::Point(bin_w * i, hist.rows - hist.rows * histVal[i] / diff),
                 cv::Scalar(0, 0, 255), 2, 8, 0);
    }
    cv::imshow("hist", hist);
    cv::waitKey(1);


    // таблица гамма-коррекции
    uchar p[256]{0};
    float gamma_ = 1.45;
    for( int i = 0; i < 256; ++i) p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma_) * 255.0);


    // цикл изображений
    fseek(f, 32, SEEK_SET);
    int frame = 0;
    cv::VideoWriter video;
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    video.open("videoOutput1.mp4", fourcc, 30, cv::Size(width, height), true);
    if(!video.isOpened()){
        return 15;
    }
    while(frame < 1000){
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                char u2 = fgetc(f);
                char u1 = fgetc(f);
                img.at<unsigned short>(i,j) = (static_cast<unsigned short>(u2) << 8) + static_cast<unsigned short>(u1);
            }
        }
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                unsigned u = p[img.at<unsigned short>(i,j) / 257];
                grayscale.at<cv::Vec3b>(i, j)[0] = u;
                grayscale.at<cv::Vec3b>(i, j)[1] = u;
                grayscale.at<cv::Vec3b>(i, j)[2] = u;
            }
        }
        cv::imshow("grayscale", grayscale);
        cv::waitKey(1);
        frame += 1;
        video.write(grayscale);
    }
    video.release();
}
