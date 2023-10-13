//
// Created by swan on 2023/10/10.
//

#include <pthread.h>
#include "DZFFmpeg.h"
#include "DZConstDefine.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

DZFFmpeg::DZFFmpeg(DZJNICall *pJinCall, const char *url) {
    this->pJinCall = pJinCall;
    // 赋值一份 url，因为怕外面方法结束销毁 url
    this->url = static_cast<char *>(malloc(strlen(url) + 1));
    memcpy(this->url, url, strlen(url)+1);

    pPlayerStatus = new DZPlayerStatus();
}

DZFFmpeg::~DZFFmpeg() {
    release();
}

// 解码放数据线程
void* threadReadPacket(void* context){
    DZFFmpeg* pFFmpeg = static_cast<DZFFmpeg *>(context);
    while (pFFmpeg->pPlayerStatus != NULL && !pFFmpeg->pPlayerStatus->isExit){
        // 读取每一帧
        AVPacket *pPacket = av_packet_alloc();
        if (av_read_frame(pFFmpeg->pFormatContext, pPacket) >= 0){
            if (pPacket->stream_index == pFFmpeg->pAudio->streamIndex){ // 处理音频
                pFFmpeg->pAudio->pPacketQueue->push(pPacket);
            } else if (pFFmpeg->pVideo != NULL && pPacket->stream_index == pFFmpeg->pVideo->streamIndex){
                pFFmpeg->pVideo->pPacketQueue->push(pPacket);
            } else {
                // 1. 解引用数据 data 2. 销毁 pPacket 结构体内存 3. pPacket = NULL
                av_packet_free(&pPacket);
            }
        }else {
            // 1. 解引用数据 data 2. 销毁 pPacket 结构体内存 3. pPacket = NULL
            av_packet_free(&pPacket);
            // 睡眠一下，尽量不去消耗 cpu 的资源,也可以退出销毁这个线程
            //break;
        }
    }
    return 0;
}

void DZFFmpeg::play() {
    // 一个线程去读取 packet
    pthread_t decodePacketThreadT;
    pthread_create(&decodePacketThreadT, NULL, threadReadPacket, this);
    pthread_detach(decodePacketThreadT);

    if (pAudio != NULL){
        pAudio->play();
    }

    if (pVideo != NULL){
        pVideo->play();
    }
}

void DZFFmpeg::callPlayerJniError(ThreadMode threadMode,int code, char *msg) {
    // 释放资源
    release();

    // 回调给 java层
    pJinCall->callPlayerError(threadMode,code, msg);
}

void DZFFmpeg::release() {

    if (pFormatContext != NULL){
        avformat_close_input(&pFormatContext);
        avformat_free_context(pFormatContext);
        pFormatContext == NULL;
    }

    if (pPlayerStatus != NULL){
        delete(pPlayerStatus);
        pPlayerStatus == NULL;
    }

    if (pAudio != NULL){
        delete(pAudio);
        pAudio == NULL;
    }

    if (pVideo != NULL){
        delete(pVideo);
        pVideo == NULL;
    }

    avformat_network_deinit();
    if (url != NULL){
        free(url);
        url = NULL;
    }
}

void DZFFmpeg::prepare() {
    prepare(THREAD_MAIN);
}

void* threadPrepare(void* context){
    DZFFmpeg* pFFmpeg = static_cast<DZFFmpeg *>(context);
    pFFmpeg->prepare(THREAD_CHILD);
    return 0;
}

void DZFFmpeg::prepareAsync() {
    // 创建一个线程去播放，多线程编解码边播放
    pthread_t prepareThreadT;
    pthread_create(&prepareThreadT, NULL, threadPrepare, this);
    pthread_detach(prepareThreadT);
}



void DZFFmpeg::prepare(ThreadMode threadMode) {
    // 讲的理念
    // 初始化所有组件，只有调用了该函数，才能使用复用器和编解码器
    av_register_all();
    // 初始化网络
    avformat_network_init();

    int formatOpenInputRes = 0; // 0 成功，非0 不成功
    int formatFindStreamInfoRes = 0; // >= 0 成功

    // 函数会读文件头，对 mp4 文件而言，它会解析所有的 box。但它知识把读到的结果保存在对应的数据结构下
    formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if (formatOpenInputRes != 0){
        // 1. 需要回调给Java层
        // 2. 需要释放资源
        //return;
        LOGE("format open input error: %s", av_err2str(formatOpenInputRes));
        callPlayerJniError(threadMode,formatOpenInputRes, av_err2str(formatOpenInputRes));
        return;
    }

    // 读取一部分视音频数据并且获得一些相关的信息，会检测一些重要字段，如果是空白的，就设法填充它们
    formatFindStreamInfoRes = avformat_find_stream_info(pFormatContext, NULL);
    if (formatFindStreamInfoRes < 0){
        LOGE("format find stream info error: %s", av_err2str(formatFindStreamInfoRes));
        callPlayerJniError(threadMode,formatFindStreamInfoRes, av_err2str(formatFindStreamInfoRes));
        return;
    }

    // 查找音频流的 index , 以前没有这个函数时，我们一般都是写的 for 循环。
    int audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audioStreamIndex < 0){
        LOGE("format audio stream error：");
        callPlayerJniError(threadMode,FIND_STREAM_ERROR_CODE, "format audio stream error：");
        return;
    }

    // 不是我的事我不干，但是也不要想的过于复杂
    pAudio = new DZAudio(audioStreamIndex, pJinCall, pPlayerStatus);
    pAudio->analysisStream(threadMode, pFormatContext);

    // 查找视频流的 index , 以前没有这个函数时，我们一般都是写的 for 循环。
    int videoStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamIndex < 0){
        LOGE("format video stream error：");
        callPlayerJniError(threadMode,FIND_STREAM_ERROR_CODE, "format video stream error：");
        return;
    }
    // 如果没有视频，就直接播放音频
//    if (videoStreamIndex < 0){
//        return;
//    }

    pVideo = new DZVideo(videoStreamIndex, pJinCall, pPlayerStatus, pAudio);
    pVideo->analysisStream(threadMode, pFormatContext);

    // 回调到java层，告诉他准备好了
    pJinCall->callPlayerPrepared(threadMode);
}

/**
 * 创建 surface
 * @param surface
 */
void DZFFmpeg::setSurface(jobject surface) {
    if (pVideo != NULL){
        pVideo->setSurface(surface);
    }
}
