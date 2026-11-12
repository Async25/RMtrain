#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#ifndef CAMERA_CONFIG_FILE
#define CAMERA_CONFIG_FILE "/home/async25/Desktop/cvProject/capture/build/camera.yaml"
#endif

#ifndef CALIBRATION_FILE
#define CALIBRATION_FILE "/home/async25/Desktop/cvProject/capture/output/calibration.yml"
#endif

namespace
{
//constexpr double ArmorWidthCm = 14.0;
//constexpr double ArmorHeightCm = 12.5;
constexpr double ArmorWidthCm = 5.5;
constexpr double ArmorHeightCm = 5;
constexpr char WindowName[] = "Find Armor PnP";

struct CameraConfig
{
    std::string device = "/dev/video0";
    int width = 1280;
    int height = 720;
    int fps = 120;
    std::string ioMode = "mmap";
    int brightness = 0;
    int contrast = 32;
    int saturation = 64;
    int sharpness = 3;
    int hue = 0;
    int gain = 0;
    int exposureAuto = 0;
    int exposure = 600;
    int whiteBalanceAuto = 0;
    int whiteBalanceTemperature = 0;
    int powerLineFrequency = 1;
    int focusAuto = 1;
    int focusAbsolute = 0;
    bool dropFrames = true;
    int maxBuffers = 1;
    bool sync = false;
};

struct LightBar
{
    cv::RotatedRect rect;
    cv::Point2f center;
    cv::Point2f points[4];
    cv::Point2f direction;
    float width = 0.0f;
    float height = 0.0f;
    float ratio = 0.0f;
    float area = 0.0f;
};

struct Armor
{
    LightBar left;
    LightBar right;
};

struct Calibration
{
    cv::Mat cameraMatrix;
    cv::Mat distortionCoefficients;
    cv::Size imageSize;
};

bool loadCameraConfig(const std::string& path, CameraConfig& config);
std::string buildGstPipeline(const CameraConfig& config);
bool loadCalibration(const std::string& path, Calibration& calibration);
cv::Mat scaledCameraMatrix(const Calibration& calibration, const cv::Size& frameSize);
std::vector<std::vector<cv::Point>> imageToContours(const cv::Mat& image);
std::vector<LightBar> contoursToBars(const std::vector<std::vector<cv::Point>>& contours);
std::vector<Armor> barsToArmor(std::vector<LightBar> bars);
cv::Point2f getLongAxisDirection(const cv::RotatedRect& rect);
bool getArmorCorners(const Armor& armor, cv::Point2f corners[4]);
void drawArmorQuadrilateral(cv::Mat& image, const Armor& armor);
std::vector<cv::Point3f> armorObjectPoints();
bool solveArmorPnP(const Armor& armor, const cv::Mat& cameraMatrix,
                   const cv::Mat& distortionCoefficients, cv::Mat& rvec,
                   cv::Mat& tvec);
void drawPnPResult(cv::Mat& image, const cv::Mat& cameraMatrix,
                   const cv::Mat& distortionCoefficients, const cv::Mat& rvec,
                   const cv::Mat& tvec);
void rotationMatrixToEulerDegrees(const cv::Mat& rotationMatrix,
                                  double& pitch, double& yaw, double& roll);
void putOverlayText(cv::Mat& image, const std::string& text, int y,
                    const cv::Scalar& color = cv::Scalar(0, 255, 0));

} // namespace

int main(int argc, char** argv)
{
    const std::string cameraConfigPath =
        argc > 1 ? argv[1] : std::string(CAMERA_CONFIG_FILE);
    const std::string calibrationPath =
        argc > 2 ? argv[2] : std::string(CALIBRATION_FILE);

    CameraConfig cameraConfig;
    if (!loadCameraConfig(cameraConfigPath, cameraConfig))
        return EXIT_FAILURE;

    Calibration calibration;
    if (!loadCalibration(calibrationPath, calibration))
        return EXIT_FAILURE;

    const std::string pipeline = buildGstPipeline(cameraConfig);
    std::cout << "Opening camera with GStreamer pipeline:\n" << pipeline << '\n';
    cv::VideoCapture camera(pipeline, cv::CAP_GSTREAMER);
    if (!camera.isOpened())
    {
        std::cerr << "Cannot open USB camera pipeline.\n";
        return EXIT_FAILURE;
    }

    cv::namedWindow(WindowName, cv::WINDOW_NORMAL);
    cv::Mat frame;
    cv::Mat cameraMatrix;
    double previousTick = static_cast<double>(cv::getTickCount());

    while (camera.read(frame) && !frame.empty())
    {
        const double startTick = static_cast<double>(cv::getTickCount());
        if (cameraMatrix.empty())
            cameraMatrix = scaledCameraMatrix(calibration, frame.size());

        std::vector<std::vector<cv::Point>> contours = imageToContours(frame);
        std::vector<LightBar> bars = contoursToBars(contours);
        std::vector<Armor> armors = barsToArmor(std::move(bars));

        cv::Mat display = frame.clone();
        bool pnpSolved = false;
        cv::Mat rvec, tvec;
        if (!armors.empty())
        {
            const Armor& armor = armors.front();
            drawArmorQuadrilateral(display, armor);
            pnpSolved = solveArmorPnP(armor, cameraMatrix,
                                      calibration.distortionCoefficients,
                                      rvec, tvec);
            if (pnpSolved)
                drawPnPResult(display, cameraMatrix,
                              calibration.distortionCoefficients, rvec, tvec);
        }

        const double elapsedMs =
            (static_cast<double>(cv::getTickCount()) - startTick) *
            1000.0 / cv::getTickFrequency();
        const double frameIntervalMs =
            (static_cast<double>(cv::getTickCount()) - previousTick) *
            1000.0 / cv::getTickFrequency();
        previousTick = static_cast<double>(cv::getTickCount());
        const double fps = frameIntervalMs > 1e-6 ? 1000.0 / frameIntervalMs : 0.0;

        std::ostringstream status;
        status << std::fixed << std::setprecision(1)
               << "FPS: " << fps << "  processing: " << elapsedMs << " ms";
        putOverlayText(display, status.str(), 30, cv::Scalar(255, 255, 255));
        putOverlayText(display,
                       pnpSolved ? "Armor: detected / PnP: solved"
                                 : "Armor: searching",
                       60,
                       pnpSolved ? cv::Scalar(0, 255, 0)
                                 : cv::Scalar(0, 200, 255));

        cv::imshow(WindowName, display);
        const int key = cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 'Q' || key == 27)
            break;
        if (cv::getWindowProperty(WindowName, cv::WND_PROP_VISIBLE) == 0.0)
            break;
    }

    camera.release();
    cv::destroyAllWindows();
    return EXIT_SUCCESS;
}

namespace
{

bool loadCameraConfig(const std::string& path, CameraConfig& config)
{
    cv::FileStorage storage(path, cv::FileStorage::READ);
    if (!storage.isOpened())
    {
        std::cerr << "Cannot open camera configuration: " << path << '\n';
        return false;
    }
    const cv::FileNode camera = storage["camera"];
    if (camera.empty())
    {
        std::cerr << "Missing 'camera' section in: " << path << '\n';
        return false;
    }
    camera["device"] >> config.device;
    camera["width"] >> config.width;
    camera["height"] >> config.height;
    camera["fps"] >> config.fps;
    camera["io_mode"] >> config.ioMode;
    camera["brightness"] >> config.brightness;
    camera["contrast"] >> config.contrast;
    camera["saturation"] >> config.saturation;
    camera["sharpness"] >> config.sharpness;
    camera["hue"] >> config.hue;
    camera["gain"] >> config.gain;
    camera["exposure_auto"] >> config.exposureAuto;
    camera["exposure"] >> config.exposure;
    camera["white_balance_auto"] >> config.whiteBalanceAuto;
    camera["white_balance_temperature"] >> config.whiteBalanceTemperature;
    camera["power_line_frequency"] >> config.powerLineFrequency;
    camera["focus_auto"] >> config.focusAuto;
    camera["focus_absolute"] >> config.focusAbsolute;
    camera["drop_frames"] >> config.dropFrames;
    camera["max_buffers"] >> config.maxBuffers;
    camera["sync"] >> config.sync;
    return true;
}

std::string buildGstPipeline(const CameraConfig& config)
{
    std::ostringstream pipeline;
    pipeline << "v4l2src device=" << config.device
             << " io-mode=" << config.ioMode
             << " extra-controls=\"c,"
             << "brightness=" << config.brightness << ','
             << "contrast=" << config.contrast << ','
             << "saturation=" << config.saturation << ','
             << "sharpness=" << config.sharpness << ','
             << "hue=" << config.hue << ','
             << "gain=" << config.gain << ','
             << "exposure_auto=" << config.exposureAuto << ','
             << "exposure_time_absolute=" << config.exposure << ','
             << "white_balance_temperature_auto=" << config.whiteBalanceAuto << ','
             << "white_balance_temperature=" << config.whiteBalanceTemperature << ','
             << "power_line_frequency=" << config.powerLineFrequency << ','
             << "focus_auto=" << config.focusAuto << ','
             << "focus_absolute=" << config.focusAbsolute << "\" ! "
             << "image/jpeg,width=" << config.width
             << ",height=" << config.height
             << ",framerate=" << config.fps << "/1 ! "
             << "jpegdec ! videoconvert ! video/x-raw,format=BGR ! "
             << "appsink drop=" << (config.dropFrames ? "true" : "false")
             << " max-buffers=" << std::max(1, config.maxBuffers)
             << " sync=" << (config.sync ? "true" : "false");
    return pipeline.str();
}

bool loadCalibration(const std::string& path, Calibration& calibration)
{
    cv::FileStorage storage(path, cv::FileStorage::READ);
    if (!storage.isOpened())
    {
        std::cerr << "Cannot open calibration file: " << path << '\n';
        return false;
    }
    storage["camera_matrix"] >> calibration.cameraMatrix;
    storage["distortion_coefficients"] >> calibration.distortionCoefficients;
    int width = 0;
    int height = 0;
    storage["image_width"] >> width;
    storage["image_height"] >> height;
    calibration.imageSize = cv::Size(width, height);

    if (calibration.cameraMatrix.empty() ||
        calibration.distortionCoefficients.empty() ||
        calibration.cameraMatrix.rows != 3 ||
        calibration.cameraMatrix.cols != 3)
    {
        std::cerr << "Invalid camera calibration data in: " << path << '\n';
        return false;
    }
    calibration.cameraMatrix.convertTo(calibration.cameraMatrix, CV_64F);
    calibration.distortionCoefficients.convertTo(
        calibration.distortionCoefficients, CV_64F);
    std::cout << "Loaded calibration from " << path << '\n'
              << "Camera matrix:\n" << calibration.cameraMatrix << '\n'
              << "Distortion coefficients:\n"
              << calibration.distortionCoefficients << '\n';
    return true;
}

cv::Mat scaledCameraMatrix(const Calibration& calibration, const cv::Size& frameSize)
{
    cv::Mat result = calibration.cameraMatrix.clone();
    if (calibration.imageSize.width <= 0 || calibration.imageSize.height <= 0 ||
        calibration.imageSize == frameSize)
        return result;

    const double sx = static_cast<double>(frameSize.width) /
                      calibration.imageSize.width;
    const double sy = static_cast<double>(frameSize.height) /
                      calibration.imageSize.height;
    result.at<double>(0, 0) *= sx;
    result.at<double>(0, 2) *= sx;
    result.at<double>(1, 1) *= sy;
    result.at<double>(1, 2) *= sy;
    std::cerr << "Warning: scaled camera intrinsics from "
              << calibration.imageSize << " to " << frameSize << '\n';
    return result;
}

std::vector<std::vector<cv::Point>> imageToContours(const cv::Mat& image)
{
    cv::Mat gray, binary, hsv, hsvBlue, mask;
    std::vector<std::vector<cv::Point>> contours;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(75, 50, 70), cv::Scalar(110, 255, 255), hsvBlue);
    cv::threshold(gray, binary, 110, 255, cv::THRESH_BINARY);
    cv::bitwise_and(binary, hsvBlue, mask);
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    return contours;
}

std::vector<LightBar> contoursToBars(
    const std::vector<std::vector<cv::Point>>& contours)
{
    std::vector<LightBar> bars;
    bars.reserve(contours.size());
    for (const auto& contour : contours)
    {
        const cv::RotatedRect rect = cv::minAreaRect(contour);
        const float width = std::min(rect.size.width, rect.size.height);
        const float height = std::max(rect.size.width, rect.size.height);
        if (width <= 0.0f || height <= 0.0f)
            continue;
        if (rect.size.width < rect.size.height && rect.angle > 30.0f)
            continue;
        if (rect.size.width > rect.size.height && rect.angle < 60.0f)
            continue;
        const float area = width * height;
        if (area < 30.0f || area > 2400.0f)
            continue;
        const float ratio = height / width;
        if (ratio < 1.2f || ratio > 6.5f)
            continue;

        LightBar bar;
        bar.rect = rect;
        bar.center = rect.center;
        bar.width = width;
        bar.height = height;
        bar.ratio = ratio;
        bar.area = area;
        rect.points(bar.points);
        bar.direction = getLongAxisDirection(rect);
        if (bar.direction.dot(bar.direction) < 0.5f)
            continue;
        bars.push_back(bar);
    }
    return bars;
}

std::vector<Armor> barsToArmor(std::vector<LightBar> bars)
{
    std::sort(bars.begin(), bars.end(),
              [](const LightBar& a, const LightBar& b)
              { return a.center.x < b.center.x; });

    constexpr float maxHeightRatio = 1.2f;
    constexpr float minDistanceRatio = 0.6f;
    constexpr float maxDistanceRatio = 3.0f;
    constexpr float maxVerticalOffset = 0.4f;
    constexpr float maxParallelError = 0.1f;
    constexpr float maxPerpendicularError = 0.4f;
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
            if (minHeight <= 1e-6f || meanHeight <= 1e-6f)
                continue;
            const float leftLength = std::sqrt(left.direction.dot(left.direction));
            const float rightLength = std::sqrt(right.direction.dot(right.direction));
            if (leftLength <= 1e-6f || rightLength <= 1e-6f)
                continue;
            const cv::Point2f leftDirection = left.direction / leftLength;
            const cv::Point2f rightDirection = right.direction / rightLength;
            const float heightRatio =
                std::max(left.height, right.height) / minHeight;
            cv::Point2f connect = right.center - left.center;
            const float centerDistance = std::sqrt(connect.dot(connect));
            if (centerDistance <= 1e-6f)
                continue;
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
                continue;

            const float barAngle = std::acos(cosBarsAngle);
            const float barAngleScore = barAngle / maxBarAngle;
            const float leftVerticalAngle =
                std::abs(std::atan2(leftDirection.x, leftDirection.y));
            const float rightVerticalAngle =
                std::abs(std::atan2(rightDirection.x, rightDirection.y));
            const float verticalAngleScore =
                (leftVerticalAngle + rightVerticalAngle) / maxVerticalAngleSum;
            const float score = 0.4f * (1.0f / centerDistance) +
                                0.55f * barAngleScore +
                                0.05f * verticalAngleScore;
            if (score < bestScore)
            {
                bestScore = score;
                bestArmor = Armor{left, right};
                found = true;
            }
        }
    }
    return found ? std::vector<Armor>{bestArmor} : std::vector<Armor>{};
}

cv::Point2f getLongAxisDirection(const cv::RotatedRect& rect)
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
    if (length < 1e-6f)
        return cv::Point2f(0.0f, 0.0f);
    direction *= 1.0f / length;
    if (direction.y < 0.0f ||
        (std::abs(direction.y) < 1e-6f && direction.x < 0.0f))
        direction *= -1.0f;
    return direction;
}

bool getArmorCorners(const Armor& armor, cv::Point2f corners[4])
{
    cv::Point2f leftDirection = armor.left.direction;
    cv::Point2f rightDirection = armor.right.direction;
    const float leftLength = std::sqrt(leftDirection.dot(leftDirection));
    const float rightLength = std::sqrt(rightDirection.dot(rightDirection));
    if (leftLength <= 1e-6f || rightLength <= 1e-6f)
        return false;
    leftDirection *= 1.0f / leftLength;
    rightDirection *= 1.0f / rightLength;

    // RotatedRect 的长边是整根灯条长度，角点偏移使用半长。
    const cv::Point2f leftTop =
        armor.left.center - leftDirection * (armor.left.height * 0.5f);
    const cv::Point2f rightTop =
        armor.right.center - rightDirection * (armor.right.height * 0.5f);
    const cv::Point2f rightBottom =
        armor.right.center + rightDirection * (armor.right.height * 0.5f);
    const cv::Point2f leftBottom =
        armor.left.center + leftDirection * (armor.left.height * 0.5f);
    corners[0] = leftTop;
    corners[1] = rightTop;
    corners[2] = rightBottom;
    corners[3] = leftBottom;
    return true;
}

void drawArmorQuadrilateral(cv::Mat& image, const Armor& armor)
{
    cv::Point2f corners[4];
    if (!getArmorCorners(armor, corners))
        return;
    for (int i = 0; i < 4; ++i)
        cv::line(image, corners[i], corners[(i + 1) % 4],
                 cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
}

std::vector<cv::Point3f> armorObjectPoints()
{
    const float halfWidth = static_cast<float>(ArmorWidthCm * 0.5);
    const float halfHeight = static_cast<float>(ArmorHeightCm * 0.5);
    return {{-halfWidth, -halfHeight, 0.0f},
            {halfWidth, -halfHeight, 0.0f},
            {halfWidth, halfHeight, 0.0f},
            {-halfWidth, halfHeight, 0.0f}};
}

bool solveArmorPnP(const Armor& armor, const cv::Mat& cameraMatrix,
                   const cv::Mat& distortionCoefficients, cv::Mat& rvec,
                   cv::Mat& tvec)
{
    cv::Point2f corners[4];
    if (!getArmorCorners(armor, corners))
        return false;
    const std::vector<cv::Point2f> imagePoints(corners, corners + 4);
    const std::vector<cv::Point3f> objectPoints = armorObjectPoints();
    bool solved = cv::solvePnP(objectPoints, imagePoints, cameraMatrix,
                               distortionCoefficients, rvec, tvec, false,
                               cv::SOLVEPNP_IPPE);
    if (!solved)
        solved = cv::solvePnP(objectPoints, imagePoints, cameraMatrix,
                              distortionCoefficients, rvec, tvec, false,
                              cv::SOLVEPNP_ITERATIVE);
    return solved;
}

void drawPnPResult(cv::Mat& image, const cv::Mat& cameraMatrix,
                   const cv::Mat& distortionCoefficients, const cv::Mat& rvec,
                   const cv::Mat& tvec)
{
    const std::vector<cv::Point3f> axes = {
        {0.0f, 0.0f, 0.0f}, {5.0f, 0.0f, 0.0f},
        {0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 5.0f}};
    std::vector<cv::Point2f> projected;
    cv::projectPoints(axes, rvec, tvec, cameraMatrix,
                      distortionCoefficients, projected);
    if (projected.size() == 4)
    {
        cv::line(image, projected[0], projected[1], cv::Scalar(255, 0, 0), 3);
        cv::line(image, projected[0], projected[2], cv::Scalar(0, 255, 0), 3);
        cv::line(image, projected[0], projected[3], cv::Scalar(0, 0, 255), 3);
    }

    cv::Mat rotationMatrix;
    cv::Rodrigues(rvec, rotationMatrix);
    double pitch = 0.0;
    double yaw = 0.0;
    double roll = 0.0;
    rotationMatrixToEulerDegrees(rotationMatrix, pitch, yaw, roll);
    const double x = tvec.at<double>(0);
    const double y = tvec.at<double>(1);
    const double z = tvec.at<double>(2);
    const double distance = cv::norm(tvec);

    std::ostringstream distanceText;
    distanceText << std::fixed << std::setprecision(1)
                 << "Distance: " << distance << " cm  (X " << x
                 << ", Y " << y << ", Z " << z << ")";
    putOverlayText(image, distanceText.str(), 95, cv::Scalar(0, 255, 0));
    std::ostringstream angleText;
    angleText << std::fixed << std::setprecision(1)
              << "Rotation: pitch " << pitch << " deg  yaw " << yaw
              << " deg  roll " << roll << " deg";
    putOverlayText(image, angleText.str(), 130, cv::Scalar(0, 255, 0));
}

void rotationMatrixToEulerDegrees(const cv::Mat& rotationMatrix,
                                  double& pitch, double& yaw, double& roll)
{
    const double sy = std::hypot(rotationMatrix.at<double>(0, 0),
                                 rotationMatrix.at<double>(1, 0));
    const bool singular = sy < 1e-6;
    double x;
    double y;
    double z;
    if (!singular)
    {
        x = std::atan2(rotationMatrix.at<double>(2, 1),
                       rotationMatrix.at<double>(2, 2));
        y = std::atan2(-rotationMatrix.at<double>(2, 0), sy);
        z = std::atan2(rotationMatrix.at<double>(1, 0),
                       rotationMatrix.at<double>(0, 0));
    }
    else
    {
        x = std::atan2(-rotationMatrix.at<double>(1, 2),
                       rotationMatrix.at<double>(1, 1));
        y = std::atan2(-rotationMatrix.at<double>(2, 0), sy);
        z = 0.0;
    }
    constexpr double radToDeg = 180.0 / CV_PI;
    pitch = x * radToDeg;
    yaw = y * radToDeg;
    roll = z * radToDeg;
}

void putOverlayText(cv::Mat& image, const std::string& text, int y,
                    const cv::Scalar& color)
{
    const cv::Point origin(15, y);
    cv::putText(image, text, origin, cv::FONT_HERSHEY_SIMPLEX, 0.65,
                cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
    cv::putText(image, text, origin, cv::FONT_HERSHEY_SIMPLEX, 0.65,
                color, 1, cv::LINE_AA);
}

} // namespace
