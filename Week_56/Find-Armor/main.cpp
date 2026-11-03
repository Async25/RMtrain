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

cv::Mat Prepare(cv::Mat image);

int main(void)
{
    cv::Mat img, img_test, bandh;

    int i = FIRSTPHOTO;

    while (1)
    {
        if (i < FIRSTPHOTO || i > LASTPHOTO) break;

        img = cv::imread(std::string(PROJECT_DIR) + "/photo/" + std::to_string(i) + ".jpg");
        if (img.empty())
        {
            std::cerr << "Failed to load image: " << i << ".jpg" << std::endl;
            break;
        }
        
        std::vector<std::vector<cv::Point>> contours;

        bandh = Prepare(img);
        cv::findContours(bandh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        img.copyTo(img_test);                   //用于叠加绘制结果

        std::vector<cv::RotatedRect> Bars;      //存放经筛选的灯条

        for (int j = 0; j < contours.size(); j++)
        {
            cv::RotatedRect rect = cv::minAreaRect(contours[j]);

            float w = rect.size.width;
            float h = rect.size.height;
            if (w == 0 || h == 0 ) continue;                            //过滤异常矩形

            float area = w * h;
            if (area < 20 || area > 1800) continue;                     //面积过滤

            uint8_t flag = 0;                                           //修正长宽判断
            if (w > h)                                                 
            {
                flag = 1;
                float temp = w;
                w = h;
                h = temp;
            }

            if (flag == 0 && rect.angle > 40) continue;                 //分两类进行倾角过滤
            if (flag == 1 && rect.angle < 50) {flag = 0; continue;}

            float ratio = h / w;                                        //修正后再计算长宽比
            if (ratio < 1.1 || ratio > 20.0 ) continue;                 //长宽比过滤

            LightBar lightBar;
            lightBar.rect = rect;
            lightBar.center = rect.center;
            lightBar.width = w;
            lightBar.height = h;
            lightBar.ratio = ratio;
            lightBar.area = area;
            rect.points(lightBar.points);
            Bars.push_back(rect);

            // 绘制矩形
            for (int k = 0; k < 4; ++k)
            {
                cv::line(img_test, lightBar.points[k], lightBar.points[(k+1)%4], cv::Scalar(0, 255, 0), 2, 8, 0);
            }
            cv::circle(img_test, lightBar.center, 2, cv::Scalar(255, 0, 0), 2, 8, 0);
        }
	
        //cv::imshow("yuan", img);
        //cv::imshow("gauss", gauss);
        //cv::imshow("gray", gray);
        //cv::imshow("hsv", hsv);
        //cv::imshow("hsv_blue", hsv_blue);
        //cv::imshow("binary", binary);
        cv::imshow("bandh", bandh);
        cv::imshow("img_test", img_test);

        int num = cv::waitKey(30) & 0xFF;

        if (num == 'q' || num == 'Q' || num == 27) break;
        else if (num == '=') ++i;
        else if (num == '-') --i;
    }
    return 0;
}

cv::Mat Prepare(cv::Mat image)
{
    cv::Mat gauss, gray, binary, hsv, hsv_blue, binaryANDhsv;

    cv::GaussianBlur(image, gauss, cv::Size(3, 3), 0);
    cv::cvtColor(gauss, gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(gauss, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(75, 50, 70), cv::Scalar(110, 255, 255), hsv_blue);
    cv::threshold(gray, binary, 110, 255, cv::THRESH_BINARY);
    cv::bitwise_and(binary, hsv_blue, binaryANDhsv);

    cv::imshow("gauss", gauss);
    
    return binaryANDhsv;
}
