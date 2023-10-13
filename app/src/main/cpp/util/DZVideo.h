//
// Created by swan on 2023/10/12.
//

#ifndef FMUSICPLAYER_DZVIDEO_H
#define FMUSICPLAYER_DZVIDEO_H

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "DZConstDefine.h"
#include "DZMedia.h"
extern "C" {
#include "libswscale/swscale.h"
}

class DZVideo : public DZMedia{
public:
    SwsContext* pSwsContext = NULL;
    uint8_t *pFrameBuffer = NULL;
    int frameSize;
    AVFrame *pRgbaFrame = NULL;
    jobject surface = NULL;

public:
    DZVideo(int streamIndex, DZJNICall *pJinCall, DZPlayerStatus *pPlayerStatus);
    ~DZVideo();
public:
    void play();


    void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);


    void release();


    void setSurface(jobject surface);
};


#endif //FMUSICPLAYER_DZVIDEO_H
