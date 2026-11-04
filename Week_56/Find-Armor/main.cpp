#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#define FIRSTPHOTO 1
#define LASTPHOTO 40

struct LightBar
{
    cv::RotatedRect rect;
    cv::Point2f center;
    cv::Point2f points[4];
    cv::Point2f direction; // 沿灯条长轴的单位方向向量
    float width = 0.0f;    // 短边
    float height = 0.0f;   // 长边
    float ratio = 0.0f;
    float area = 0.0f;
};

struct Armor
{
    LightBar left, right;
};

std::vector<std::vector<cv::Point>> Img2Contours(cv::Mat& image);
std::vector<LightBar> Contours2Bars(std::vector<std::vector<cv::Point>>& contours);
std::vector<Armor> Bars2Armor(std::vector<LightBar>& bars);
cv::Point2f GetLongAxisDirection(cv::RotatedRect& rect);

int main()
{
    cv::Mat img, img_test;

    for (int i = FIRSTPHOTO; i <= LASTPHOTO; )
    {
        img = cv::imread(std::string(PROJECT_DIR) + "/photo/" + std::to_string(i) + ".jpg");
        if (img.empty())
        {
            std::cerr << "Failed to load image: " << i << ".jpg\n";
            break;
        }

        std::vector<std::vector<cv::Point>> contours = Img2Contours(img);
        std::vector<LightBar> bars = Contours2Bars(contours);

        img.copyTo(img_test);

        for (int j = 0; j < bars.size(); ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                cv::line(img_test, bars[j].points[k], bars[j].points[(k + 1) % 4],
                         cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
            }
            cv::circle(img_test, bars[j].center, 2, cv::Scalar(255, 0, 0), -1, cv::LINE_AA);

            cv::Point2f end = bars[j].center + bars[j].direction * (bars[j].height * 0.5f);
            cv::arrowedLine(img_test, bars[j].center, end, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        }

        cv::imshow("img", img);
        cv::imshow("img_test", img_test);

        int key = cv::waitKey(0) & 0xFF;

        if (key == 'q' || key == 'Q' || key == 27) break;
        if (key == '=') ++i;
        else if (key == '-' && i > FIRSTPHOTO) --i;
    }
    return 0;
}

std::vector<std::vector<cv::Point>> Img2Contours(cv::Mat& image)
{
    cv::Mat gray, binary, hsv, hsv_blue, mask;
    std::vector<std::vector<cv::Point>> contours;

    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(75, 50, 70), cv::Scalar(110, 255, 255), hsv_blue);
    cv::threshold(gray, binary, 110, 255, cv::THRESH_BINARY);
    cv::bitwise_and(binary, hsv_blue, mask);
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::imshow("mask", mask);
    return contours;
}

std::vector<LightBar> Contours2Bars(std::vector<std::vector<cv::Point>>& contours)
{
    std::vector<LightBar> bars;
    bars.reserve(contours.size());

    for (auto& contour : contours)
    {
        cv::RotatedRect rect = cv::minAreaRect(contour);
        float w = std::min(rect.size.width, rect.size.height);
        float h = std::max(rect.size.width, rect.size.height);
        if (w <= 0.0f || h <= 0.0f) continue;

        float area = w * h;
        if (area < 30.0f || area > 2800.0f) continue;

        float ratio = h / w;
        if (ratio < 1.1f || ratio > 20.0f) continue;

        LightBar bar;
        bar.rect = rect;
        bar.center = rect.center;
        bar.width = w;
        bar.height = h;
        bar.ratio = ratio;
        bar.area = area;
        rect.points(bar.points);
        bar.direction = GetLongAxisDirection(rect);
        bars.push_back(bar);
    }
    return bars;
}

std::vector<Armor> Bars2Armor(std::vector<LightBar>& bars)
{
    std::sort(bars.begin(), bars.end(),
          [](const LightBar& a, const LightBar& b)
          {
              return a.center.x < b.center.x;
          });

    std::vector<Armor> armors;
    armors.reserve(bars.size());

    for (size_t i = 0; i+1 < bars.size(); ++i)
    {
        for (size_t j = i + 1; j < bars.size(); ++j)
        {
            LightBar& left = bars[i];
            LightBar& right = bars[j];

            float distance = right.center.x - left.center.x;                                // 间距
            if (distance > 1500.0f || distance < 100.0f) continue;

            float parallelError = 1.0f - std::abs(left.direction.dot(right.direction));     // 平行度 ->0
            if (parallelError > 0.3f) continue;

            cv::Point2f connect = right.center - left.center;                               // 中心连线垂直 ->0
            connect *= 1.0f / std::sqrt(connect.dot(connect));
            float perpendicularError = std::abs(connect.dot(left.direction));  
            if (perpendicularError > 0.5f) continue;

            armors.push_back({left, right});
        }
    }
    return armors;
}

cv::Point2f GetLongAxisDirection(cv::RotatedRect& rect)
{
    cv::Point2f points[4];
    rect.points(points);

    int longestEdge = 0;
    float longestLengthSquared = -1.0f;
    for (int i = 0; i < 4; ++i)
    {
        cv::Point2f edge = points[(i + 1) % 4] - points[i];
        float lengthSquared = edge.dot(edge);
        if (lengthSquared > longestLengthSquared)
        {
            longestLengthSquared = lengthSquared;
            longestEdge = i;
        }
    }

    cv::Point2f direction = points[(longestEdge + 1) % 4] - points[longestEdge];
    float length = std::sqrt(direction.dot(direction));
    if (length < 1e-6f) return cv::Point2f(0.0f, 0.0f);

    direction *= 1.0f / length;
    if (direction.y < 0.0f || (std::abs(direction.y) < 1e-6f && direction.x < 0.0f))
        direction *= -1.0f;

    return direction;
}
