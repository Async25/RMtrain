#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
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
bool GetArmorCorners(const Armor& armor, cv::Point2f corners[4]);
void DrawArmorQuadrilateral(cv::Mat& image, const Armor& armor);
bool ExtractArmorROI(const cv::Mat& image, const Armor& armor, cv::Mat& roi);

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
        std::vector<Armor> armors = Bars2Armor(bars);

        img.copyTo(img_test);

        cv::Mat armorRoi;
        if (!armors.empty())
        {
            const Armor& armor = armors.front();
            DrawArmorQuadrilateral(img_test, armor);

            if (ExtractArmorROI(img, armor, armorRoi))
            {
                cv::imshow("armor_roi", armorRoi);
            }
            else
            {
                cv::destroyWindow("armor_roi");
            }
        }
        else
        {
            cv::destroyWindow("armor_roi");
        }

        for (const auto& bar : bars)
        {
            for (int k = 0; k < 4; ++k)
            {
                cv::line(img_test, bar.points[k], bar.points[(k + 1) % 4],
                         cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
            }

            cv::Point2f end = bar.center + bar.direction * (bar.height * 0.5f);
            cv::arrowedLine(img_test, bar.center, end,
                            cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        }

        //cv::imshow("img", img);
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

    //cv::imshow("mask", mask);
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

        if (bar.direction.dot(bar.direction) < 0.5f) continue;

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

    const float maxHeightRatio = 1.4f;
    const float minDistanceRatio = 0.4f;
    const float maxDistanceRatio = 3.5f;
    const float maxVerticalOffset = 0.8f;
    const float maxParallelError = 0.15f;
    const float maxPerpendicularError = 0.35f;

    const float maxBarAngle = static_cast<float>(CV_PI * 0.5);
    const float maxVerticalAngleSum = static_cast<float>(CV_PI);

    float bestScore = std::numeric_limits<float>::max();
    Armor bestArmor;
    bool found = false;

    for (size_t i = 0; i + 1 < bars.size(); ++i)
    {
        for (size_t j = i + 1; j < bars.size(); ++j)
        {
            const LightBar& left = bars[i];
            const LightBar& right = bars[j];

            const float minHeight = std::min(left.height, right.height);
            const float meanHeight = 0.5f * (left.height + right.height);
            if (minHeight <= 1e-6f || meanHeight <= 1e-6f) continue;

            const float leftDirectionLength =
                std::sqrt(left.direction.dot(left.direction));
            const float rightDirectionLength =
                std::sqrt(right.direction.dot(right.direction));
            if (leftDirectionLength <= 1e-6f || rightDirectionLength <= 1e-6f)
                continue;

            cv::Point2f leftDirection = left.direction / leftDirectionLength;
            cv::Point2f rightDirection = right.direction / rightDirectionLength;

            const float heightRatio =
                std::max(left.height, right.height) / minHeight;

            cv::Point2f connect = right.center - left.center;
            const float centerDistance = std::sqrt(connect.dot(connect));
            if (centerDistance <= 1e-6f) continue;

            const float normalizedDistance = centerDistance / meanHeight;
            const float verticalOffset =
                std::abs(left.center.y - right.center.y) / meanHeight;

            float cosBarsAngle = std::abs(leftDirection.dot(rightDirection));
            cosBarsAngle = std::max(0.0f, std::min(1.0f, cosBarsAngle));
            const float parallelError = 1.0f - cosBarsAngle;

            connect *= 1.0f / centerDistance;
            const float perpendicularError = std::max(
                std::abs(connect.dot(leftDirection)),
                std::abs(connect.dot(rightDirection)));

            if (heightRatio > maxHeightRatio ||
                normalizedDistance < minDistanceRatio ||
                normalizedDistance > maxDistanceRatio ||
                verticalOffset > maxVerticalOffset ||
                parallelError > maxParallelError ||
                perpendicularError > maxPerpendicularError)
            {
                continue;
            }

            const float barAngle = std::acos(cosBarsAngle);
            const float barAngleScore = barAngle / maxBarAngle;

            const float leftVerticalAngle = std::abs(
                std::atan2(leftDirection.x, leftDirection.y));
            const float rightVerticalAngle = std::abs(
                std::atan2(rightDirection.x, rightDirection.y));
            const float verticalAngleSum =
                leftVerticalAngle + rightVerticalAngle;
            const float verticalAngleScore =
                verticalAngleSum / maxVerticalAngleSum;

            const float score =
                0.3f * barAngleScore + 0.7f * verticalAngleScore;

            if (score < bestScore)
            {
                bestScore = score;
                bestArmor = Armor{left, right};
                found = true;
            }
        }
    }

    if (!found)
        return {};

    return {bestArmor};
}

bool GetArmorCorners(const Armor& armor, cv::Point2f corners[4])
{
    cv::Point2f leftDirection = armor.left.direction;
    cv::Point2f rightDirection = armor.right.direction;

    const float leftLength = std::sqrt(leftDirection.dot(leftDirection));
    const float rightLength = std::sqrt(rightDirection.dot(rightDirection));
    // if (leftLength <= 1e-6f || rightLength <= 1e-6f)
    //    return false;

    leftDirection *= 1.0f / leftLength;
    rightDirection *= 1.0f / rightLength;

    const float leftOffset = armor.left.height;
    const float rightOffset = armor.right.height;

    const cv::Point2f leftTop = armor.left.center - leftDirection * leftOffset;
    const cv::Point2f rightTop = armor.right.center - rightDirection * rightOffset;
    const cv::Point2f rightBottom = armor.right.center + rightDirection * rightOffset;
    const cv::Point2f leftBottom = armor.left.center + leftDirection * leftOffset;

    corners[0] = leftTop;
    corners[1] = rightTop;
    corners[2] = rightBottom;
    corners[3] = leftBottom;
    
    return true;
}

void DrawArmorQuadrilateral(cv::Mat& image, const Armor& armor)
{
    cv::Point2f corners[4];
    if (!GetArmorCorners(armor, corners))
        return;

    const cv::Scalar armorColor(0, 0, 255);
    for (int i = 0; i < 4; ++i)
    {
        cv::line(image, corners[i], corners[(i + 1) % 4],
                 armorColor, 2, cv::LINE_AA);
    }
}

bool ExtractArmorROI(const cv::Mat& image, const Armor& armor, cv::Mat& roi)
{
    cv::Point2f corners[4];
    if (!GetArmorCorners(armor, corners))
        return false;

    const float topWidth = static_cast<float>(cv::norm(corners[1] - corners[0]));
    const float bottomWidth = static_cast<float>(cv::norm(corners[2] - corners[3]));
    const float leftHeight = static_cast<float>(cv::norm(corners[3] - corners[0]));
    const float rightHeight = static_cast<float>(cv::norm(corners[2] - corners[1]));

    const int roiWidth = std::max(1, cvRound(0.5f * (topWidth + bottomWidth)));
    const int roiHeight = std::max(1, cvRound(0.5f * (leftHeight + rightHeight)));

    std::vector<cv::Point2f> sourcePoints = {
        corners[0], corners[1], corners[2], corners[3]
    };
    std::vector<cv::Point2f> targetPoints = {
        cv::Point2f(0.0f, 0.0f),
        cv::Point2f(static_cast<float>(roiWidth - 1), 0.0f),
        cv::Point2f(static_cast<float>(roiWidth - 1), static_cast<float>(roiHeight - 1)),
        cv::Point2f(0.0f, static_cast<float>(roiHeight - 1))
    };

    cv::Mat perspective = cv::getPerspectiveTransform(sourcePoints, targetPoints);
    cv::warpPerspective(image, roi, perspective, cv::Size(roiWidth, roiHeight),
                        cv::INTER_LINEAR, cv::BORDER_CONSTANT);
                        
    return !roi.empty();
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
    if (direction.y < 0.0f ||
        (std::abs(direction.y) < 1e-6f && direction.x < 0.0f))
    {
        direction *= -1.0f;
    }

    return direction;
}
