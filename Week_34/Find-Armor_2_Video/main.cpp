#include <opencv2/opencv.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
struct LightBar
{
    cv::Point2f center;
    cv::Point2f points[4];
    float height;
    float area;
};

cv::Mat processFrame(const cv::Mat &frame)
{
    cv::Mat blurred, hsv, binary, result;
    cv::GaussianBlur(frame, blurred, cv::Size(7, 7), 0);
    cv::cvtColor(blurred, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(75, 50, 70), cv::Scalar(110, 255, 255), binary);
    cv::imshow("Binary Image", binary);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<LightBar> lightBars;
    for (const auto &contour : contours)
    {
        const cv::RotatedRect rect = cv::minAreaRect(contour);
        float width = rect.size.width;
        float height = rect.size.height;
        if (width == 0.0f || height == 0.0f)
            continue;

        bool swapped = false;
        if (width > height)
        {
            swapped = true;
            std::swap(width, height);
        }
        if ((!swapped && rect.angle > 30.0f) || (swapped && rect.angle < 60.0f))
            continue;

        const float ratio = height / width;
        const float area = width * height;
        if (ratio < 1.5f || ratio > 5.0f || area < 150.0f || area > 1500.0f)
            continue;

        LightBar bar;
        bar.center = rect.center;
        bar.height = height;
        bar.area = area;
        rect.points(bar.points);
        lightBars.push_back(bar);
    }

    std::vector<bool> hasNeighbor(lightBars.size(), false);
    for (size_t i = 0; i < lightBars.size(); ++i)
    {
        for (size_t j = 0; j < lightBars.size(); ++j)
        {
            if (i == j)
                continue;

            const float distance = static_cast<float>(cv::norm(lightBars[i].center - lightBars[j].center));
            const float verticalDistance = std::abs(lightBars[i].center.y - lightBars[j].center.y);
            const float averageHeight = (lightBars[i].height + lightBars[j].height) * 0.5f;
            // const float areaRatio = lightBars[j].area / lightBars[i].area;

            // A pair must be vertically aligned, similar in area, and within the existing distance range.
            if (verticalDistance < 0.7f * averageHeight /* &&
                areaRatio >= 0.35f && areaRatio <= 1.65f */ &&
                distance >= 0.6f * averageHeight && distance <= 5.0f * averageHeight)
            {
                hasNeighbor[i] = true;
                break;
            }
        }
    }

    result = frame.clone();
    for (size_t i = 0; i < lightBars.size(); ++i)
    {
        if (!hasNeighbor[i])
            continue;
        for (int point = 0; point < 4; ++point)
            cv::line(result, lightBars[i].points[point], lightBars[i].points[(point + 1) % 4],
                     cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
        cv::circle(result, lightBars[i].center, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
    }
    return result;
}
} // namespace

int main()
{
    const std::string videoDir = std::string(PROJECT_DIR) + "/video/";
    const std::vector<std::string> videoFiles = {"1.mp4", "2.mp4"};
    const auto frameDuration = std::chrono::duration<double>(1.0 / 2.0);
    bool quit = false;

    cv::namedWindow("Armor Detection", cv::WINDOW_NORMAL);
    cv::namedWindow("Binary Image", cv::WINDOW_NORMAL);
    for (const std::string &videoFile : videoFiles)
    {
        cv::VideoCapture video(videoDir + videoFile);
        if (!video.isOpened())
        {
            std::cerr << "Failed to open video: " << videoDir << videoFile << '\n';
            cv::destroyAllWindows();
            return 1;
        }

        cv::Mat frame;
        while (!quit)
        {
            const auto frameStart = std::chrono::steady_clock::now();
            if (!video.read(frame) || frame.empty())
                break;

            cv::Mat processed = processFrame(frame);
            cv::putText(processed, videoFile, cv::Point(20, 35), cv::FONT_HERSHEY_SIMPLEX,
                        1.0, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
            cv::imshow("Armor Detection", processed);

            const int key = cv::waitKey(1) & 0xFF;
            if (key == 'q' || key == 'Q' || key == 27)
            {
                quit = true;
                break;
            }

            std::this_thread::sleep_until(frameStart +
                                         std::chrono::duration_cast<std::chrono::steady_clock::duration>(frameDuration));
        }
    }

    cv::destroyAllWindows();
    return 0;
}
