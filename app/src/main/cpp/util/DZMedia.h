//
// Created by swan on 2023/10/12.
//

#ifndef FMUSICPLAYER_DZMEDIA_H
#define FMUSICPLAYER_DZMEDIA_H

#include "DZJNICall.h"
#include "DZPacketQueue.h"
#include "DZPlayerStatus.h"

extern "C" {
#include "libavformat/avformat.h"
}

class DZMedia {
public:
    int streamIndex = -1;
    AVCodecContext *pCodecContext = NULL;
    DZJNICall *pJinCall = NULL;
    DZPacketQueue *pPacketQueue = NULL;
    DZPlayerStatus *pPlayerStatus = NULL;
    /**
     * 视频总时长
     */
    int duration = 0;
    /**
     * 记录当前播放时间
     */
    double currentTime = 0;
    /**
     * 上次更新的时间（回调到java 层）
     */
    double lastUpdateTime = 0;

    /**
     * 时间基
     */
    AVRational timeBase;
public:
    DZMedia(int streamIndex, DZJNICall *pJinCall, DZPlayerStatus *pPlayerStatus);
    ~DZMedia();

public:
    /**
     * 纯虚函数，需要子类实现
     */
    virtual void play() = 0;

    void analysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);

    /**
     * 不同的部分，需要自己去实现
     * @param threadMode
     * @param pFormatContext
     */
    virtual void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) = 0;

    /**
     * 释放资源
     */
    virtual void release();
private:
    /**
     * 公共流解析
     * @param threadMode
     * @param pFormatContext
     */
    void publicAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);

protected:
    void callPlayerJniError(ThreadMode threadMode, int code, char *msg);
};


#endif //FMUSICPLAYER_DZMEDIA_H
