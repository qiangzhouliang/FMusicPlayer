cmake_minimum_required(VERSION 3.22.1)

project("fmusicplayer")

# 指定引入头文件
# FFmpeg
include_directories(${CMAKE_SOURCE_DIR}/../jniLibs/include)
# 自己的
include_directories(${CMAKE_SOURCE_DIR}/util)

# 指定共享库目录
link_directories(${CMAKE_SOURCE_DIR}/../jniLibs/${CMAKE_ANDROID_ARCH_ABI})

# src/main/cpp 目录下所有的.cpp 文件都需要编译进来
#aux_source_directory(${CMAKE_SOURCE_DIR}/util/*.cpp SRC_LIST)
file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/util/*.cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native-lib.cpp
        video-player.cpp
        ${SOURCE})

target_link_libraries(
        # 链接额外的 ffmpeg 的编译
        ${CMAKE_PROJECT_NAME}
        log
        -Wl,--start-group # 忽略顺序引发的问题
        # 编解码(最重要的库)
        avcodec-57
        # 设备信息
        avdevice-57
        # 滤镜特效处理库
        avfilter-6
        # 封装格式处理库
        avformat-57
        # 工具库(大部分库都需要这个库的支持)
        avutil-55
        # 后期处理
        postproc-54
        # 音频采样数据格式转换库
        swresample-2
        # 视频像素数据格式转换
        swscale-4
        -Wl,--end-group
        # 链接 android ndk 自带的一些库
        android
        z
        # 链接 OpenSLES
        OpenSLES)