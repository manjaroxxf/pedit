cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(panda-note LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(KF5SyntaxHighlighting REQUIRED)

set(SOURCES
    resources.qrc
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)

add_executable(panda-note
    ${SOURCES}
)

target_link_libraries(panda-note Qt5::Widgets KF5::SyntaxHighlighting)
