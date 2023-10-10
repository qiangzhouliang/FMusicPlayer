//
// Created by swan on 2023/10/10.
//

#ifndef FMUSICPLAYER_DZFFMPEG_H
#define FMUSICPLAYER_DZFFMPEG_H

#include "DZJNICall.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}


class DZFFmpeg {
public:
    AVFormatContext *pFormatContext = NULL;
    AVCodecContext *pCodecContext = NULL;
    // 重采样
    SwrContext *swrContext = NULL;
    // 重采样缓存数据
    uint8_t *resampleOutBuffer = NULL;
    const char *url = NULL;
    DZJNICall *pJinCall = NULL;
public:
    DZFFmpeg(DZJNICall *pJinCall, const char *url);
    ~DZFFmpeg();
public:
    void play();

    void callPlayerJniError(int code, char *msg);

};


#endif //FMUSICPLAYER_DZFFMPEG_H
