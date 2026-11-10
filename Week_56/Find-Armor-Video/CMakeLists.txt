cmake_minimum_required(VERSION 3.10)
project(OpenCVtest CXX)

find_package(OpenCV REQUIRED)

add_executable(main main.cpp)

target_include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS})

target_compile_definitions(main PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(main ${OpenCV_LIBS})
