#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace cv;

#define FIRSTPHOTO 1
#define LASTPHOTO 30

// void callback(int, void*);
// int LOW_H, HIGH_H=255, LOW_S, HIGH_S=255, LOW_V, HIGH_V=255;

int i = FIRSTPHOTO;

cv::Mat img, hsv, gauss, blue, open, dila;
cv::Mat imt_test;

int main(void)
{
    while (true)
    {
        if (i < FIRSTPHOTO || i > LASTPHOTO) break;

        img = cv::imread(std::string(PROJECT_DIR) + "/photo/" + std::to_string(i) + ".jpg");
        if (img.empty())
        {
            std::cerr << "Failed to load image: " << i << ".jpg" << std::endl;
            break;
        }
        // cv::imshow(std::to_string(i), img);
        // cv::imshow(std::to_string(i), blue);       
        // cv::inRange(hsv, cv::Scalar(75, 10, 65), cv::Scalar(110, 255, 255), blue);   //所有图片最边界阈值
        
        // cv::namedWindow(std::to_string(i));

        // cv::createTrackbar("LOW_H", std::to_string(i), nullptr, 255, callback, 0);
        // cv::createTrackbar("HIGH_H", std::to_string(i), nullptr, 255, callback, 0);
        // cv::createTrackbar("LOW_S", std::to_string(i), nullptr, 255, callback, 0);
        // cv::createTrackbar("HIGH_S", std::to_string(i), nullptr, 255, callback, 0);
        // cv::createTrackbar("LOW_V", std::to_string(i), nullptr, 255, callback, 0);
        // cv::createTrackbar("HIGH_V", std::to_string(i), nullptr, 255, callback, 0);

        // cv::setTrackbarPos("LOW_H", std::to_string(i), LOW_H);
        // cv::setTrackbarPos("HIGH_H", std::to_string(i), HIGH_H);
        // cv::setTrackbarPos("LOW_S", std::to_string(i), LOW_S);
        // cv::setTrackbarPos("HIGH_S", std::to_string(i), HIGH_S);
        // cv::setTrackbarPos("LOW_V", std::to_string(i), LOW_V);
        // cv::setTrackbarPos("HIGH_V", std::to_string(i), HIGH_V);

        std::vector<std::vector<cv::Point>> contours;
        std::vector<Vec4i> hierarchy;

        cv::cvtColor(img, hsv, COLOR_BGR2HSV);
        cv::inRange(hsv, cv::Scalar(75, 50, 70), cv::Scalar(110, 255, 255), blue);  //仅能识别低曝光图片的阈值
        // cv::inRange(hsv, cv::Scalar(75, 10, 65), cv::Scalar(110, 255, 255), blue);
        cv::morphologyEx(blue, open, MORPH_OPEN, cv::Mat(5, 5, CV_8UC1));
        cv::dilate(open, dila, cv::getStructuringElement(0, cv::Size(5, 5)));
        cv::findContours(dila, contours, 0, 2, cv::Point());
        img.copyTo(imt_test);

        for(int j = 0; j < contours.size(); j++)
        {
            cv::RotatedRect rect = cv::minAreaRect(contours[j]);
            cv::Point2f points[4];
            rect.points(points);
            cv::Point2f cpt = rect.center;
        
            for(int m=0 ; m<4; m++)
            {
                if (m == 3)
                {
                    cv::line(imt_test, points[m], points[0], cv::Scalar(0, 255, 0), 2, 8, 0);
                    break;
                }
                cv::line(imt_test, points[m], points[m+1], cv::Scalar(0, 255, 0), 2, 8, 0);
            }
            cv::circle(imt_test, cpt, 2, cv::Scalar(255, 0, 0), 2, 8, 0);
        }

        cv::imshow(std::to_string(i)+"原图", img);
        // cv::imshow(std::to_string(i)+"HSV提取", blue);
        // cv::imshow(std::to_string(i)+"HSV提取开", open);
        cv::imshow(std::to_string(i)+"开运算膨胀", dila);
        cv::imshow(std::to_string(i)+"轮廓", imt_test);

        int num = cv::waitKey(0) & 0xFF;

        cv::destroyWindow(std::to_string(i)+"原图");
        // cv::destroyWindow(std::to_string(i)+"HSV提取");
        // cv::destroyWindow(std::to_string(i)+"HSV提取开");
        cv::destroyWindow(std::to_string(i)+"开运算膨胀");
        cv::destroyWindow(std::to_string(i)+"轮廓");

        if (num == 'q' || num == 'Q' || num == 27) break;
        else if (num == '=') ++i;
        else if (num == '-') --i;
    }

    return 0;
}

// void callback(int, void*)
// {
//     LOW_H = cv::getTrackbarPos("LOW_H", std::to_string(i));
//     HIGH_H = cv::getTrackbarPos("HIGH_H", std::to_string(i));
//     LOW_S = cv::getTrackbarPos("LOW_S", std::to_string(i));
//     HIGH_S = cv::getTrackbarPos("HIGH_S", std::to_string(i));
//     LOW_V = cv::getTrackbarPos("LOW_V", std::to_string(i));
//     HIGH_V = cv::getTrackbarPos("HIGH_V", std::to_string(i));

//     cv::inRange(hsv, cv::Scalar(LOW_H, LOW_S, LOW_V), cv::Scalar(HIGH_H, HIGH_S, HIGH_V), blue);
//     cv::imshow(std::to_string(i), blue);
// }
