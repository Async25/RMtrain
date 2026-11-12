cmake_minimum_required(VERSION 3.16)
project(FindArmorPnP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OpenCV_DIR "/usr/local/opencv-4.10.0/lib/cmake/opencv4" CACHE PATH
    "OpenCV CMake package directory")
find_package(OpenCV 4.10 REQUIRED COMPONENTS core imgproc highgui videoio calib3d)

set(CAMERA_CONFIG_FILE
    "/home/async25/Desktop/cvProject/capture/build/camera.yaml"
    CACHE FILEPATH "Reference USB camera configuration")
set(CALIBRATION_FILE
    "/home/async25/Desktop/cvProject/capture/output/calibration.yml"
    CACHE FILEPATH "Reference camera calibration")

add_executable(main main.cpp)
target_link_libraries(main PRIVATE ${OpenCV_LIBS})
target_compile_definitions(main PRIVATE
    CAMERA_CONFIG_FILE="${CAMERA_CONFIG_FILE}"
    CALIBRATION_FILE="${CALIBRATION_FILE}")
target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic)
