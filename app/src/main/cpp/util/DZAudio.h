//
// Created by swan on 2023/10/11.
//

#ifndef FMUSICPLAYER_DZAUDIO_H
#define FMUSICPLAYER_DZAUDIO_H


#include "DZJNICall.h"
#include "DZPacketQueue.h"
#include "DZPlayerStatus.h"
#include "DZMedia.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

class DZAudio : public DZMedia{
public:
    // 重采样
    SwrContext *swrContext = NULL;
    // 重采样缓存数据
    uint8_t *resampleOutBuffer = NULL;

public:
    DZAudio(int audioStreamIndex, DZJNICall *pJinCall,DZPlayerStatus *pPlayerStatus);
    ~DZAudio();

public:
    void play();

    void initCreateOpenSLES();

    int resampleAudio();

    void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);


    void release();
};


#endif //FMUSICPLAYER_DZAUDIO_H
