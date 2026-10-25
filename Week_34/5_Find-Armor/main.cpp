#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace cv;

#define FIRSTPHOTO 1
#define LASTPHOTO 30

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
    cv::Mat img, hsv, gauss, blue, open, dila;
    cv::Mat imt_test;

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
        std::vector<Vec4i> hierarchy;

        cv::cvtColor(img, hsv, COLOR_BGR2HSV);
        cv::GaussianBlur(hsv, gauss, cv::Size(7, 7), 0);
        // cv::inRange(hsv, cv::Scalar(75, 50, 70), cv::Scalar(110, 255, 255), blue);  //仅能识别低曝光图片的阈值
        cv::inRange(gauss, cv::Scalar(75, 10, 65), cv::Scalar(110, 255, 255), blue);
        cv::morphologyEx(blue, open, MORPH_OPEN, cv::getStructuringElement (cv::MORPH_RECT, cv:: Size (7, 7)));
        // cv::dilate(open, dila, cv::getStructuringElement(0, cv::Size(3, 3)));
        cv::findContours(open, contours, 0, 2, cv::Point());

        img.copyTo(imt_test);
        std::vector<LightBar> lightBars;

        for(int j=0; j<contours.size(); j++)
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
        }

/**************************************************************************/

        std::vector<LightBar> pairedLightBars;
        for (size_t j = 0; j < lightBars.size(); ++j)
        {
            bool hasNeighbor = false;
            for (size_t k = 0; k < lightBars.size(); ++k)
            {
                if (j == k) continue;
                
                //中心距离过滤
                float centerDistance = cv::norm(lightBars[j].center - lightBars[k].center);
                float averageHeight = (lightBars[j].height + lightBars[k].height) * 0.5f;
                if (centerDistance <= 5.0f * averageHeight && centerDistance >= 0.8f * averageHeight)
                {
                    hasNeighbor = true;
                    break;
                }
            }

            if (hasNeighbor)
            {
                pairedLightBars.push_back(lightBars[j]);
            }
        }
        lightBars = pairedLightBars;

/**************************************************************************/

        for (const LightBar& lightBar : lightBars)
        {
            for(int m = 0; m < 4; m++)
            {
                cv::line(
                    imt_test,
                    lightBar.points[m],
                    lightBar.points[(m + 1) % 4],
                    cv::Scalar(0, 255, 0),
                    2,
                    8,
                    0);
            }
            cv::circle(imt_test, lightBar.center, 2, cv::Scalar(255, 0, 0), 2, 8, 0);
        }

        // cv::imshow(std::to_string(i)+"开运算", open);
        cv::imshow(std::to_string(i)+"轮廓", imt_test);

        int num = cv::waitKey(0) & 0xFF;

        // cv::destroyWindow(std::to_string(i)+"开运算");
        cv::destroyWindow(std::to_string(i)+"轮廓");

        if (num == 'q' || num == 'Q' || num == 27) break;
        else if (num == '=') ++i;
        else if (num == '-') --i;
    }
    return 0;
}
