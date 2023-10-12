//
// Created by swan on 2023/10/11.
//

#ifndef FMUSICPLAYER_DZAUDIO_H
#define FMUSICPLAYER_DZAUDIO_H


#include "DZJNICall.h"
#include "DZPacketQueue.h"
#include "DZPlayerStatus.h"

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
    DZPacketQueue *pPacketQueue = NULL;
    DZPlayerStatus *pPlayerStatus = NULL;

public:
    DZAudio(int audioStreamIndex, DZJNICall *pJinCall,AVFormatContext *pFormatContext);
    ~DZAudio();

public:
    void play();

    void initCreateOpenSLES();

    int resampleAudio();

    void analysisStream(ThreadMode threadMode, AVStream **streams);

    void callPlayerJniError(ThreadMode threadMode,int code, char *msg);

    void release();
};


#endif //FMUSICPLAYER_DZAUDIO_H
