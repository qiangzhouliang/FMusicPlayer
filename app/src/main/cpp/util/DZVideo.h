//
// Created by swan on 2023/10/12.
//

#ifndef FMUSICPLAYER_DZVIDEO_H
#define FMUSICPLAYER_DZVIDEO_H

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "DZConstDefine.h"
#include "DZMedia.h"
#include "DZAudio.h"

extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/time.h"
}

class DZVideo : public DZMedia{
public:
    SwsContext* pSwsContext = NULL;
    uint8_t *pFrameBuffer = NULL;
    int frameSize;
    AVFrame *pRgbaFrame = NULL;
    jobject surface = NULL;
    DZAudio *pAudio = NULL;
    /**
     * 视频的延时时间
     */
    double delayTime = 0;
    /**
     * 默认情况下最合适的一个延长时间，动态获取
     */
    double defaultDelayTime = 0.04;

public:
    DZVideo(int streamIndex, DZJNICall *pJinCall, DZPlayerStatus *pPlayerStatus,DZAudio *pAudio);
    ~DZVideo();
public:
    void play();


    void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);


    void release();


    void setSurface(jobject surface);
    /**
     * 视频同步音频-计算睡眠时间
     * @param pFrame 当前视频帧
     * @return 休眠时间（s）
     */
    double getFrameSleepTime(AVFrame *pFrame);
};


#endif //FMUSICPLAYER_DZVIDEO_H
