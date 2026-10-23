cmake_minimum_required(VERSION 3.16)
project(CodeAlpha_CGPA_Calculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(CGPACalculator
    src/main.cpp
    src/Grading.h
    src/Grading.cpp
    src/Storage.h
    src/Storage.cpp
    src/CourseDialog.h
    src/CourseDialog.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/Style.h
)

target_link_libraries(CGPACalculator PRIVATE Qt6::Widgets)
