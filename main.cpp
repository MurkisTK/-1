#include <limits.h>
#include <iostream>
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

using namespace std;

// глобальные переменные
char* pathToFile = "../../video.bin";
int width = 640;
int height = 512;

void myCalcHist(cv::Mat src, cv::Mat& dst, std::vector<float>& histValues){
    // гистограмма
    dst = cv::Mat::zeros(cv::Size(512, 512), CV_8UC3); // гистограмма
    histValues.resize(0);
    histValues.resize(256); // массив значений количества пикселей каждой интенсивности
    int binWidth = dst.cols / 256;
    int min = INT_MAX;
    int max = INT_MIN;
    int diff = 0;

    for(int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            histValues[src.at<uchar>(i, j)] += 1;
        }
    }

    //нахождение минимального и максимального значения яркости и их разности
    for(int i = 0; i < 256; i++){
        if(max < histValues[i]) max = histValues[i];
        if(min > histValues[i]) min = histValues[i];
    }
    diff = max - min;

    // минмаксная нормализация
    for(int i = 0; i < 256; i++){
        histValues[i] = (histValues[i] - min) / diff;
    }

    // построение гистограммы
    for(int i = 1; i < 256; i++){
        cv::line(dst,
                 cv::Point(binWidth * (i-1), dst.rows - dst.rows * histValues[i-1]),
                 cv::Point(binWidth * i, dst.rows - dst.rows * histValues[i]),
                 cv::Scalar(0, 0, 255), 2, 8, 0);
    }
}

int myCalcIntensityRangeCount(cv::Mat src, int i1, int i2){
    int sum = 0;

    for(int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            if(src.at<cv::Vec3b>(i, j)[0] < i2 && src.at<cv::Vec3b>(i, j)[0] > i1){
                sum += 1;
            }
        }
    }

    //std::cout << sum << std::endl;
    return sum;
}

int main()
{
    // изображение
    cv::Mat grayscaleImage = cv::Mat(cv::Size(width, height), CV_8UC3);
    cv::Mat histOriginal;
    cv::Mat histResult;
    std::vector<float> histValues;

    // открытие файла
    FILE* file = fopen(pathToFile, "r");
    if(!file){
        return 5;
    }

    // создание объекта для записи видео
    cv::VideoWriter video;
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    video.open("videoOutput1.mp4", fourcc, 10, cv::Size(width, height), true);
    if(!video.isOpened()){
        return 15;
    }

    // запись видео
    fseek(file, 32, SEEK_SET);
    int frame = 0;
    while(frame < 1000){
        // извлечение черно-белого кадра
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                char u2 = fgetc(file);
                char u1 = fgetc(file);
                unsigned short pixelValue16bit = (static_cast<unsigned short>(u2) << 8) + static_cast<unsigned short>(u1);
                uchar uChar = pixelValue16bit / 257;

                grayscaleImage.at<cv::Vec3b>(i, j)[0] = uChar;
                grayscaleImage.at<cv::Vec3b>(i, j)[1] = uChar;
                grayscaleImage.at<cv::Vec3b>(i, j)[2] = uChar;
            }
        }

        myCalcHist(grayscaleImage, histOriginal, histValues);


        //cv::medianBlur(grayscaleImage, grayscaleImage, 3);

        // получение таблицы для гамма-коррекции
        uchar lookUpTable[256]{0};

        int shift;
        int minShift = 20, maxShift = 80;
        int count = 0;
        count = myCalcIntensityRangeCount(grayscaleImage, 65, 135);
        float ratio = count / 4000.0 > 1? 1: count / 4000.0;
        shift = maxShift - (maxShift - minShift) * ratio;

        for(int i = 0; i < 256; i++){
            if(i < 65){
                lookUpTable[i] = cv::saturate_cast<uchar>(i*0.2 + shift);
            }
            else if(i > 135 &&  i < 170){
                lookUpTable[i] = cv::saturate_cast<uchar>(i*0.2 - 27 + shift);
            }
            else if(i >= 170){
                if(ratio > 0.3){
                    lookUpTable[i] = cv::saturate_cast<uchar>(i*0.2 - 34 + shift);
                }
                else{
                    lookUpTable[i] = cv::saturate_cast<uchar>(i - 170);
                }
            }
            else{
                lookUpTable[i] = i+80;
            }
        }
        // for(int i = 0; i < 256; i++){
        //     lookUpTable[i] = i;
        // }

        // гамма-коррекция
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                grayscaleImage.at<cv::Vec3b>(i, j)[0] = lookUpTable[grayscaleImage.at<cv::Vec3b>(i, j)[0]];
                grayscaleImage.at<cv::Vec3b>(i, j)[1] = lookUpTable[grayscaleImage.at<cv::Vec3b>(i, j)[1]];
                grayscaleImage.at<cv::Vec3b>(i, j)[2] = lookUpTable[grayscaleImage.at<cv::Vec3b>(i, j)[2]];
            }
        }

        myCalcHist(grayscaleImage, histResult, histValues);

        cv::imshow("histOriginal", histOriginal);
        cv::imshow("histResult", histResult);
        cv::imshow("grayscale", grayscaleImage);
        cv::waitKey(1);
        frame += 1;
        video.write(grayscaleImage);
    }
    video.release();
    fclose(file);
}
