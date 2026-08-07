cmake_minimum_required(VERSION 3.16)
project(nusicVideoQml LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(KISSFFT_PKGCONFIG OFF CACHE BOOL "" FORCE)
set(KISSFFT_TEST OFF CACHE BOOL "" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "" FORCE)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Quick
        Multimedia
)
qt_standard_project_setup()

set(PROJECT_SOURCES
    main.cpp

    HeaderFile/LoadlocalMusic.h
    CPPFile/LoadlocalMusic.cpp

    HeaderFile/LoadlocalVideo.h
    CPPFile/LoadlocalVideo.cpp

    HeaderFile/PlayMusic.h
    CPPFile/PlayMusic.cpp

    HeaderFile/PlayDouble.h
    CPPFile/PlayDouble.cpp

    HeaderFile/AudioSeparator.h
    CPPFile/AudioSeparator.cpp

    HeaderFile/DoubleEar.h
    CPPFile/DoubleEar.cpp

    HeaderFile/LoadImage.h
    CPPFile/LoadImage.cpp

    HeaderFile/WebgetCover.h
    CPPFile/WebgetCover.cpp

    HeaderFile/AudioAnalyzer.h
    CPPFile/AudioAnalyzer.cpp

    HeaderFile/AudioAnalyzerProxy.h
    CPPFile/AudioAnalyzerProxy.cpp

    HeaderFile/InitManage.h
    CPPFile/InitManage.cpp
)

if(QT_VERSION VERSION_LESS 5.15)
    qt5_add_resources(PROJECT_SOURCES qml.qrc)
elseif(QT_VERSION VERSION_LESS 6.2)
    qt_add_resources(PROJECT_SOURCES qml.qrc)
endif()

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

if(QT_VERSION VERSION_GREATER_EQUAL 6.2)
    qt_add_qml_module(${PROJECT_NAME}
        URI nusicvideoqml
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml/"
        QML_FILES
            QmlFile/main.qml
            QmlFile/Home.qml
            QmlFile/Play.qml
            QmlFile/ButtonProgress.qml
            QmlFile/PlayDouble.qml
            QmlFile/Splash.qml
    )
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

add_subdirectory(kissFFT/kissfft-131.2.0)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Quick
        Qt::Multimedia
        kissfft::kissfft
)

if(Qt6_FOUND)
    qt_import_qml_plugins(${PROJECT_NAME})
endif()

# 设置 FFmpeg 路径
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/FFmpeg)
target_include_directories(nusicVideoQml PRIVATE 
    ${FFMPEG_DIR}/include
)

target_link_libraries(nusicVideoQml PRIVATE
    ${FFMPEG_DIR}/lib/avcodec.lib
    ${FFMPEG_DIR}/lib/avformat.lib
    ${FFMPEG_DIR}/lib/avutil.lib
    ${FFMPEG_DIR}/lib/swresample.lib
)

# 设置 ONNX Runtime 路径
set(ONNXRUNTIME_DIR ${CMAKE_SOURCE_DIR}/ONNXRuntime)
target_include_directories(nusicVideoQml PRIVATE ${ONNXRUNTIME_DIR}/include)
target_link_libraries(nusicVideoQml PRIVATE ${ONNXRUNTIME_DIR}/lib/onnxruntime.lib)