cmake_minimum_required(VERSION 3.1)

project(avicon31_video)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4
                               gstreamer-sdp-1.0>=1.4
                               gstreamer-video-1.0>=1.4
                               gstreamer-app-1.0>=1.4)


add_executable(${PROJECT_NAME} "main.cpp" "mainwindow.cpp" "mainwindow.h" "mainwindow.ui" "gstreamerthreadworker.h" "gstreamerthreadworker.cpp" "audiovisualizer.h" "audiovisualizer.cpp" "videowidget.cpp" "videowidget.h" "commands.h" "commands.cpp" "waveanalyzer.hpp" "waveanalyzer.cxx" "audiosampleviewer.cxx" "audiosampleviewer.hpp" "waveanalyzerthread.hpp" "waveanalyzerthread.cxx" "gstreamervideoplayer.cxx" "gstreamervideoplayer.hpp" "controlserver.cxx" "controlserver.hpp" "videocontrolprotocol.hpp" "settings.cpp" "settings.h")
target_include_directories(${PROJECT_NAME} PUBLIC ${GST_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets Qt5::Network ${GST_LIBRARIES} pthread)
 
