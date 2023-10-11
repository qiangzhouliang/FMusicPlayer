//
// Created by swan on 2023/10/10.
//

#ifndef FMUSICPLAYER_DZFFMPEG_H
#define FMUSICPLAYER_DZFFMPEG_H

#include "DZJNICall.h"
#include "DZAudio.h"

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
    char *url = NULL;
    DZJNICall *pJinCall = NULL;
    DZAudio *pAudio = NULL;
public:
    DZFFmpeg(DZJNICall *pJinCall, const char *url);
    ~DZFFmpeg();
public:
    void play();

    void prepare();

    void prepareAsync();

    void prepare(ThreadMode threadMode);

    void callPlayerJniError(ThreadMode threadMode,int code, char *msg);

    void release();

};


#endif //FMUSICPLAYER_DZFFMPEG_H
