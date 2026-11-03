#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace cv;

#define FIRSTPHOTO 1
#define LASTPHOTO 40

struct LightBar
{
    cv::RotatedRect rect;
    cv::Point2f center;
    cv::Point2f points[4];
    float width;
    float height;
    float ratio;
    float area;
};

int main(void)
{
    cv::Mat img, img_test, gauss, gray, binary, hsv, hsv_blue, bandh;

    int i = FIRSTPHOTO;

    // cv::namedWindow("Trackbars", cv::WINDOW_AUTOSIZE);
    // cv::createTrackbar("H Min", "Trackbars", nullptr, 255);
    // cv::setTrackbarPos("H Min", "Trackbars", 75);
    // cv::createTrackbar("H Max", "Trackbars", nullptr, 255);
    // cv::setTrackbarPos("H Max", "Trackbars", 110);
    // cv::createTrackbar("S Min", "Trackbars", nullptr, 255);
    // cv::setTrackbarPos("S Min", "Trackbars", 50);
    // cv::createTrackbar("S Max", "Trackbars", nullptr, 255);
    // cv::setTrackbarPos("S Max", "Trackbars", 255);
    // cv::createTrackbar("V Min", "Trackbars", nullptr, 255);
    // cv::setTrackbarPos("V Min", "Trackbars", 70);
    // cv::createTrackbar("V Max", "Trackbars", nullptr, 255);
    // cv::setTrackbarPos("V Max", "Trackbars", 255);

    while (1)
    {
        if (i < FIRSTPHOTO || i > LASTPHOTO) break;

        img = cv::imread(std::string(PROJECT_DIR) + "/photo/" + std::to_string(i) + ".jpg");
        if (img.empty())
        {
            std::cerr << "Failed to load image: " << i << ".jpg" << std::endl;
            break;
        }

        // int h_min = cv::getTrackbarPos("H Min", "Trackbars");
        // int h_max = cv::getTrackbarPos("H Max", "Trackbars");
        // int s_min = cv::getTrackbarPos("S Min", "Trackbars");
        // int s_max = cv::getTrackbarPos("S Max", "Trackbars");
        // int v_min = cv::getTrackbarPos("V Min", "Trackbars");
        // int v_max = cv::getTrackbarPos("V Max", "Trackbars");

	/*
	
        std::vector<std::vector<cv::Point>> contours;
        std::vector<Vec4i> hierarchy;
        
        */

        std::vector<std::vector<cv::Point>> contours;

        cv::GaussianBlur(img, gauss, cv::Size(3, 3), 0);
        cv::cvtColor(gauss, gray, cv::COLOR_BGR2GRAY);
        cv::cvtColor(gauss, hsv, cv::COLOR_BGR2HSV);
        cv::inRange(hsv, cv::Scalar(75, 50, 70), cv::Scalar(110, 255, 255), hsv_blue);
        cv::threshold(gray, binary, 110, 255, cv::THRESH_BINARY);
        cv::bitwise_and(binary, hsv_blue, bandh);
        cv::findContours(bandh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        img.copyTo(img_test);

        

        for (int j = 0; j < contours.size(); j++)
        {
            cv::RotatedRect rect = cv::minAreaRect(contours[j]);

            float w = rect.size.width;
            float h = rect.size.height;
            float ratio = h / w;
            float area = w * h;

            LightBar lightBar;
            lightBar.rect = rect;
            lightBar.center = rect.center;
            lightBar.width = w;
            lightBar.height = h;
            lightBar.ratio = ratio;
            lightBar.area = area;
            rect.points(lightBar.points);
            //lightBars.push_back(lightBar);

            for (int k = 0; k < 4; ++k)
            {
                cv::line(img_test, lightBar.points[k], lightBar.points[(k+1)%4], cv::Scalar(0, 255, 0), 2, 8, 0);
            }
            cv::circle(img_test, lightBar.center, 2, cv::Scalar(255, 0, 0), 2, 8, 0);
        }
	/*
	
        img.copyTo(img_test);
        std::vector<LightBar> lightBars;

        for (int j=0 ; j<contours.size() ; j++)
        {
            cv::RotatedRect rect = cv::minAreaRect(contours[j]);

            uint8_t flag = 0;

            //修正长宽判断
            float w = rect.size.width;
            float h = rect.size.height;
            if (w == 0 || h == 0 ) continue;
            if (w > h)
            {
                flag = 1;
                float temp = w;
                w = h;
                h = temp;
            }

            //倾角过滤
            if (flag == 0 && rect.angle > 30) continue;
            if (flag == 1 && rect.angle < 60) {flag = 0; continue;}

            //长宽比过滤
            float ratio = h / w;
            if (ratio < 1.2 || ratio > 4.0 ) continue;

            //面积过滤
            float area = w * h;
            if (area < 90 || area > 400) continue;
            
            LightBar lightBar;
            lightBar.rect = rect;
            lightBar.center = rect.center;
            lightBar.width = w;
            lightBar.height = h;
            lightBar.ratio = ratio;
            lightBar.area = area;
            rect.points(lightBar.points);
            lightBars.push_back(lightBar);

            std::vector<LightBar> pairedLightBars;

            for (size_t m = 0; m < lightBars.size(); ++m)
            {
                bool hasNeighbor = false;
                for (size_t k = 0; k < lightBars.size(); ++k)
                {
                    if (m == k) continue;
                    
                    //中心距离过滤
                    float centerDistance = cv::norm(lightBars[m].center - lightBars[k].center);
                    float averageHeight = (lightBars[m].height + lightBars[k].height) * 0.5f;
                    if (centerDistance <= 5.0f * averageHeight && centerDistance >= 0.8f * averageHeight)
                    {
                        hasNeighbor = true;
                        break;
                    }
                }

                if (hasNeighbor)
                {
                    pairedLightBars.push_back(lightBars[m]);
                }
            }
            lightBars = pairedLightBars;
        
                    for(int m = 0; m < 4; m++)
            {
                cv::line(
                    img_test, lightBar.points[m], lightBar.points[(m + 1) % 4], cv::Scalar(0, 255, 0), 2, 8, 0);
            }
            cv::circle(img_test, lightBar.center, 2, cv::Scalar(255, 0, 0), 2, 8, 0);
        }
        
        */

        /********************
            for (const LightBar &lightBar : lightBars)
            {
                for (int n=0 ; n<4 ; n++)
                {
                    if (n==3)
                    {
                        cv::line(img_test, lightBar.points[n], lightBar.points[0], cv::Scalar(0, 255, 0), 2, 8, 0);
                        break;
                    }
                    cv::line(img_test, lightBar.points[n], lightBar.points[n+1], cv::Scalar(0, 255, 0), 2, 8, 0);
                }
                cv::circle(img_test, lightBar.center, 2, cv::Scalar(255, 0, 0), 2, 8, 0);   
            }
        }
        ********************/
        
        

        //cv::imshow("yuan", img);
        cv::imshow("gauss", gauss);
        //cv::imshow("gray", gray);
        //cv::imshow("hsv", hsv);
        cv::imshow("hsv_blue", hsv_blue);
        cv::imshow("binary", binary);
        cv::imshow("bandh", bandh);
        cv::imshow("img_test", img_test);

        int num = cv::waitKey(30) & 0xFF;

        if (num == 'q' || num == 'Q' || num == 27) break;
        else if (num == '=') ++i;
        else if (num == '-') --i;
    }
    return 0;
}
