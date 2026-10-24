#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::Mat img = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::imshow("test", img);
    while (true) {
        int key = cv::waitKey(0);
        std::cout << "key: " << key << " (" << (key & 0xFF) << ")" << std::endl;
        if ((key & 0xFF) == 27) break;
    }
    return 0;
}
