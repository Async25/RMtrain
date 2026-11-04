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

std::vector<std::vector<cv::Point>> Img2Contours(const cv::Mat& image);
std::vector<LightBar> Rect2Bars(const std::vector<std::vector<cv::Point>>& contours);
cv::Point2f GetLongAxisDirection(const cv::RotatedRect& rect);

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

        const auto contours = Img2Contours(img);
        const auto bars = Rect2Bars(contours);

        img.copyTo(img_test);
        for (const auto& bar : bars)
        {
            for (int k = 0; k < 4; ++k)
            {
                cv::line(img_test, bar.points[k], bar.points[(k + 1) % 4],
                         cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
            }
            cv::circle(img_test, bar.center, 2, cv::Scalar(255, 0, 0), -1, cv::LINE_AA);

            const cv::Point2f end = bar.center + bar.direction * (bar.height * 0.5f);
            cv::arrowedLine(img_test, bar.center, end, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        }

        cv::imshow("img", img);
        cv::imshow("img_test", img_test);
        const int key = cv::waitKey(0) & 0xFF;

        if (key == 'q' || key == 'Q' || key == 27) break;
        if (key == '=') ++i;
        else if (key == '-' && i > FIRSTPHOTO) --i;
    }
    return 0;
}

std::vector<std::vector<cv::Point>> Img2Contours(const cv::Mat& image)
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

std::vector<LightBar> Rect2Bars(const std::vector<std::vector<cv::Point>>& contours)
{
    std::vector<LightBar> bars;
    bars.reserve(contours.size());

    for (const auto& contour : contours)
    {
        const cv::RotatedRect rect = cv::minAreaRect(contour);
        const float w = std::min(rect.size.width, rect.size.height);
        const float h = std::max(rect.size.width, rect.size.height);
        if (w <= 0.0f || h <= 0.0f) continue;

        const float area = w * h;
        if (area < 30.0f || area > 2800.0f) continue;

        const float ratio = h / w;
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
        if (bar.direction.dot(bar.direction) < 0.5f) continue;

        bars.push_back(bar);
    }
    return bars;
}

cv::Point2f GetLongAxisDirection(const cv::RotatedRect& rect)
{
    cv::Point2f points[4];
    rect.points(points);

    int longestEdge = 0;
    float longestLengthSquared = -1.0f;
    for (int i = 0; i < 4; ++i)
    {
        const cv::Point2f edge = points[(i + 1) % 4] - points[i];
        const float lengthSquared = edge.dot(edge);
        if (lengthSquared > longestLengthSquared)
        {
            longestLengthSquared = lengthSquared;
            longestEdge = i;
        }
    }

    cv::Point2f direction = points[(longestEdge + 1) % 4] - points[longestEdge];
    const float length = std::sqrt(direction.dot(direction));
    if (length < 1e-6f) return cv::Point2f(0.0f, 0.0f);

    direction *= 1.0f / length;
    if (direction.y < 0.0f || (std::abs(direction.y) < 1e-6f && direction.x < 0.0f))
        direction *= -1.0f;

    return direction;
}
