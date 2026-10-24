#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace cv;

int main(void)
{
    cv::Mat img = cv::imread("example.jpg");

    cv::Mat gray, thres, roi, resize, hsv, hsv_inrange, gauss, gauss_thres, canny;
    cv::Rect rect(500, 20, 300, 300);

    cv::cvtColor(img, gray, COLOR_BGR2GRAY);
    cv::threshold(gray, thres, 110, 255, THRESH_BINARY);
    roi = img(rect);
    cv::resize(img, resize, cv::Size(600, 300));
    cv::cvtColor(img, hsv, COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(26, 43, 46), cv::Scalar(34, 255, 255), hsv_inrange);
    cv::GaussianBlur(gray, gauss, cv::Size(9, 9), 0, 0);
    cv::threshold(gauss, gauss_thres, 110, 255, THRESH_BINARY);
    cv::Canny(gray, canny, 100, 200, 3);

    std::cout<<"灰度图非0像素:"<<cv::countNonZero(gray)<<std::endl;
    std::cout<<"二值化非0像素:"<<cv::countNonZero(thres)<<std::endl;

    cv::imshow("原图", img);
    cv::imshow("灰度", gray);
    cv::imshow("二值化", thres);
    cv::imshow("ROI", roi);
    cv::imshow("缩放", resize);
    // cv::imshow("原始HSV", hsv);
    cv::imshow("HSV提取黄色", hsv_inrange);
    // cv::imshow("高斯模糊", gauss);
    cv::imshow("高斯模糊二值化", gauss_thres);
    cv::imshow("Canny", canny);

    cv::imwrite(std::string(PROJECT_DIR) + "/thres.jpg", thres);
    cv::imwrite(std::string(PROJECT_DIR) + "/roi.jpg", roi);
    cv::imwrite(std::string(PROJECT_DIR) + "/canny.jpg", canny);

    cv::waitKey(0);
    
    return 0;
}
