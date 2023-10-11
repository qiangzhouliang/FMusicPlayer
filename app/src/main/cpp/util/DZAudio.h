//
// Created by swan on 2023/10/11.
//

#ifndef FMUSICPLAYER_DZAUDIO_H
#define FMUSICPLAYER_DZAUDIO_H


#include "DZJNICall.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

class DZAudio {
public:
    AVFormatContext *pFormatContext = NULL;
    AVCodecContext *pCodecContext = NULL;
    // 重采样
    SwrContext *swrContext = NULL;
    // 重采样缓存数据
    uint8_t *resampleOutBuffer = NULL;
    DZJNICall *pJinCall = NULL;
    int audioStreamIndex = -1;
public:
    DZAudio(int audioStreamIndex, DZJNICall *pJinCall, AVCodecContext *pCodecContext,AVFormatContext *pFormatContext,SwrContext *swrContext);

public:
    void play();

    void initCreateOpenSLES();

    int resampleAudio();
};


#endif //FMUSICPLAYER_DZAUDIO_H
